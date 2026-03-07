#include "inkview.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

/* ── bool portability (C89 has no stdbool.h) ─────────────────────────────── */
#ifndef __bool_true_false_are_defined
typedef int bool;
#define true  1
#define false 0
#define __bool_true_false_are_defined 1
#endif

/* ── Emscripten / platform shims ─────────────────────────────────────────── */
#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#else
#  include <sys/time.h>
static double emscripten_get_now(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}
#  ifndef BEST_SCORE_FILE
#    define BEST_SCORE_FILE "/mnt/ext1/.game2048_best"
#  endif
#endif

/* ── Scale factor ────────────────────────────────────────────────────────── */
/* Reference design: 600x800 portrait / 800x600 landscape.                   */
/* g_scale is computed from actual screen size in update_scale().             */
static float g_scale = 1.0f;
#define S(v) ((int)((v) * g_scale))

/* ── Layout ──────────────────────────────────────────────────────────────── */
#define GRID_SIZE  4
#define BOARD_X    (bx())
#define BOARD_Y    (by())
#define BOARD_W    S(560)
#define BOARD_H    S(560)
#define PADDING    S(12)
#define GAP        S(12)
#define CELL_SIZE  S(125)

static bool is_landscape = false;
static int bx(void) { return is_landscape ? S(230) : S(20); }
static int by(void) { return is_landscape ? S(20)  : S(165); }

static void update_scale(void) {
    int sw = ScreenWidth(), sh = ScreenHeight();
    float sx, sy;
    if (is_landscape) { sx = sw / 800.0f; sy = sh / 600.0f; }
    else              { sx = sw / 600.0f; sy = sh / 800.0f; }
    g_scale = sx < sy ? sx : sy;
}

/* ── Greyscale palette ───────────────────────────────────────────────────── */
#define GS(v) (((v)<<16)|((v)<<8)|(v))

#define C_BG        GS(0xF4)
#define C_BOARD     GS(0x88)
#define C_EMPTY     GS(0xC8)
#define C_TEXT_DARK GS(0x22)
#define C_TEXT_LITE GS(0xF4)
#define C_SCORE_BG  GS(0x66)
#define C_BTN_BG    GS(0x44)
#define C_TITLE     GS(0x00)

/* ── Tile style table ────────────────────────────────────────────────────── */
typedef struct { int bg, fg, pc, pt, step; } TileStyle;
static const TileStyle TS[] = {
    {0, 0, 0, 0, 0},
    {GS(0xEE), GS(0x22), 0,         0,  0},     /*  2   */
    {GS(0xDD), GS(0x22), 0,         0,  0},     /*  4   */
    {GS(0xCC), GS(0x22), GS(0xB0),  1, 12},     /*  8   */
    {GS(0xBB), GS(0x22), GS(0x9E),  2, 12},     /* 16   */
    {GS(0xAA), GS(0x11), GS(0x8A),  3, 14},     /* 32   */
    {GS(0x88), GS(0xF4), GS(0x68),  1, 10},     /* 64   */
    {GS(0x66), GS(0xF4), GS(0x4A),  2, 10},     /* 128  */
    {GS(0x44), GS(0xF4), GS(0x2C),  3, 12},     /* 256  */
    {GS(0x33), GS(0xF4), GS(0x48),  1,  8},     /* 512  */
    {GS(0x1A), GS(0xFF), GS(0x32),  2,  8},     /* 1024 */
    {GS(0x00), GS(0xFF), 0,         0,  0},     /* 2048 */
    {GS(0x00), GS(0xFF), GS(0x28),  3,  6},     /* 4096+*/
};

/* ── Game state ──────────────────────────────────────────────────────────── */
static int  grid[GRID_SIZE][GRID_SIZE];
static int  score      = 0;
static int  best_score = 0;
static bool won        = false;
static bool game_over  = false;
static bool keep_playing = false;

/* ── Animation state ─────────────────────────────────────────────────────── */
typedef struct { int sr, sc, dr, dc; } MoveRec;

static int     old_grid[GRID_SIZE][GRID_SIZE];
static MoveRec move_recs[GRID_SIZE * GRID_SIZE];
static int     move_recs_n = 0;
static bool    merge_cells[GRID_SIZE][GRID_SIZE];
static int     new_tile_r = -1, new_tile_c = -1;

#define SLIDE_MS  110.0
#define POP_MS    180.0
static double phase_start = 0.0;
static bool   in_slide = false;
static bool   in_pop   = false;

/* ── Input ───────────────────────────────────────────────────────────────── */
static int  ptr_x0, ptr_y0;
static bool ptr_down = false;

/* ── Fonts ───────────────────────────────────────────────────────────────── */
static ifont *f_title, *f_title_sm;
static ifont *f_score_lbl, *f_score_val, *f_btn;
static ifont *f_tagline, *f_message, *f_msg_sub;
static ifont *f_tile[5];

/* ── Helpers ─────────────────────────────────────────────────────────────── */
static int tile_idx(int v) { int i=0; while(v>1){v>>=1;i++;} return i>12?12:i; }
static const TileStyle *tile_style(int v) { return &TS[tile_idx(v)]; }
static ifont *tile_font(int v) {
    if (v<100)   return f_tile[1];
    if (v<1000)  return f_tile[2];
    if (v<10000) return f_tile[3];
    return f_tile[4];
}

/* ── Pattern helpers ─────────────────────────────────────────────────────── */
static void pat_hlines(int x,int y,int w,int h,int c,int s){
    int dy;
    for(dy=s/2; dy<h; dy+=s) DrawLine(x,y+dy,x+w-1,y+dy,c);
}
static void pat_vlines(int x,int y,int w,int h,int c,int s){
    int dx;
    for(dx=s/2; dx<w; dx+=s) DrawLine(x+dx,y,x+dx,y+h-1,c);
}
static void pat_cross(int x,int y,int w,int h,int c,int s){
    pat_hlines(x,y,w,h,c,s); pat_vlines(x,y,w,h,c,s);
}
static int cell_x(int c) { return BOARD_X + PADDING + c*(CELL_SIZE+GAP); }
static int cell_y(int r) { return BOARD_Y + PADDING + r*(CELL_SIZE+GAP); }
static float smoothstep(float t) { return t*t*(3.0f-2.0f*t); }

/* ── Persistence ─────────────────────────────────────────────────────────── */
#ifdef __EMSCRIPTEN__
static void load_best(void) {
    best_score = EM_ASM_INT({ return parseInt(localStorage.getItem('pb2048_best')||'0'); });
}
static void save_best(void) {
    EM_ASM({ localStorage.setItem('pb2048_best', $0); }, best_score);
}
#else
static void load_best(void) {
    FILE *f = fopen(BEST_SCORE_FILE, "r");
    if(f) { fscanf(f, "%d", &best_score); fclose(f); }
}
static void save_best(void) {
    FILE *f = fopen(BEST_SCORE_FILE, "w");
    if(f) { fprintf(f, "%d", best_score); fclose(f); }
}
#endif

/* ── Game logic ──────────────────────────────────────────────────────────── */
static int empty_count(void) {
    int n=0, r, c;
    for(r=0;r<GRID_SIZE;r++) for(c=0;c<GRID_SIZE;c++) if(!grid[r][c]) n++;
    return n;
}

static void add_random_tile(void) {
    int cnt, tgt, k, r, c;
    cnt=empty_count(); if(!cnt){new_tile_r=new_tile_c=-1;return;}
    tgt=rand()%cnt; k=0;
    for(r=0;r<GRID_SIZE;r++) for(c=0;c<GRID_SIZE;c++) if(!grid[r][c]){
        if(k++==tgt){ grid[r][c]=(rand()%10==0)?4:2; new_tile_r=r; new_tile_c=c; return; }
    }
}

static bool moves_available(void) {
    int r, c, v;
    if(empty_count()>0) return true;
    for(r=0;r<GRID_SIZE;r++) for(c=0;c<GRID_SIZE;c++){
        v=grid[r][c];
        if(c+1<GRID_SIZE&&grid[r][c+1]==v) return true;
        if(r+1<GRID_SIZE&&grid[r+1][c]==v) return true;
    }
    return false;
}

static void new_game(void) {
    memset(grid,0,sizeof(grid));
    memset(merge_cells,0,sizeof(merge_cells));
    score=0; won=false; game_over=false; keep_playing=false;
    in_slide=false; in_pop=false;
    add_random_tile(); new_tile_r=new_tile_c=-1;
    add_random_tile(); new_tile_r=new_tile_c=-1;
}

static bool do_move(int dir) {
    static const int DX[]={0,1,0,-1}, DY[]={-1,0,1,0};
    bool mflag[GRID_SIZE][GRID_SIZE];
    bool moved;
    int vx, vy, r0, r1, rs, c0, c1, cs, r, c;

    vx=DX[dir]; vy=DY[dir];
    memset(mflag,0,sizeof(mflag));
    memset(merge_cells,0,sizeof(merge_cells));
    move_recs_n=0;
    moved=false;

    r0=(vy>0)?GRID_SIZE-1:0; r1=(vy>0)?-1:GRID_SIZE; rs=(vy>0)?-1:1;
    c0=(vx>0)?GRID_SIZE-1:0; c1=(vx>0)?-1:GRID_SIZE; cs=(vx>0)?-1:1;

    for(r=r0;r!=r1;r+=rs) for(c=c0;c!=c1;c+=cs){
        MoveRec tmp;
        int val, pr, pc, nr, nc;
        if(!grid[r][c]) continue;
        val=grid[r][c];
        pr=r; pc=c; nr=r+vy; nc=c+vx;
        while(nr>=0&&nr<GRID_SIZE&&nc>=0&&nc<GRID_SIZE&&!grid[nr][nc]){pr=nr;pc=nc;nr+=vy;nc+=vx;}
        if(nr>=0&&nr<GRID_SIZE&&nc>=0&&nc<GRID_SIZE&&grid[nr][nc]==val&&!mflag[nr][nc]){
            tmp.sr=r; tmp.sc=c; tmp.dr=nr; tmp.dc=nc;
            move_recs[move_recs_n++]=tmp;
            grid[nr][nc]=val*2; mflag[nr][nc]=true; merge_cells[nr][nc]=true;
            grid[r][c]=0;
            score+=val*2;
            if(score>best_score){best_score=score;save_best();}
            if(val*2==2048&&!keep_playing) won=true;
            moved=true;
        } else if(pr!=r||pc!=c){
            tmp.sr=r; tmp.sc=c; tmp.dr=pr; tmp.dc=pc;
            move_recs[move_recs_n++]=tmp;
            grid[pr][pc]=val; grid[r][c]=0;
            moved=true;
        }
    }
    return moved;
}

/* ── Drawing helpers ─────────────────────────────────────────────────────── */
static void draw_tile_at(int value, int x, int y, int size) {
    const TileStyle *s;
    char buf[12];
    int ps;
    s=tile_style(value);
    FillArea(x, y, size, size, s->bg);
    if(s->pt && s->pc){
        ps = s->step * size / S(125);
        if(ps<3) ps=3;
        switch(s->pt){
        case 1: pat_hlines(x,y,size,size,s->pc,ps); break;
        case 2: pat_vlines(x,y,size,size,s->pc,ps); break;
        case 3: pat_cross (x,y,size,size,s->pc,ps); break;
        }
    }
    snprintf(buf,sizeof(buf),"%d",value);
    SetFont(tile_font(value), s->fg);
    DrawTextRect(x, y, size, size, buf, ALIGN_CENTER|VALIGN_MIDDLE);
}

static void draw_board_bg(void) {
    int r, c;
    FillArea(BOARD_X, BOARD_Y, BOARD_W, BOARD_H, C_BOARD);
    for(r=0;r<GRID_SIZE;r++) for(c=0;c<GRID_SIZE;c++)
        FillArea(cell_x(c), cell_y(r), CELL_SIZE, CELL_SIZE, C_EMPTY);
}

static void draw_header(void) {
    int sw, sh;
    char buf[16];
    sw=ScreenWidth(); sh=ScreenHeight();
    FillArea(0, 0, sw, sh, C_BG);

    if(is_landscape){
        DrawLine(S(220), 0, S(220), sh, GS(0xBB));
        SetFont(f_title_sm, C_TITLE);
        DrawString(S(12), S(12), "2048");
        FillArea(S(12), S(78), S(94), S(52), C_SCORE_BG);
        FillArea(S(116),S(78), S(94), S(52), C_SCORE_BG);
        SetFont(f_score_lbl, C_TEXT_LITE);
        DrawTextRect(S(12), S(84), S(94), S(18), "SCORE", ALIGN_CENTER);
        DrawTextRect(S(116),S(84), S(94), S(18), "BEST",  ALIGN_CENTER);
        SetFont(f_score_val, C_TEXT_LITE);
        snprintf(buf,sizeof(buf),"%d",score);
        DrawTextRect(S(12), S(104),S(94), S(24), buf, ALIGN_CENTER|VALIGN_MIDDLE);
        snprintf(buf,sizeof(buf),"%d",best_score);
        DrawTextRect(S(116),S(104),S(94), S(24), buf, ALIGN_CENTER|VALIGN_MIDDLE);
        FillArea(S(12), S(142),S(198),S(38), C_BTN_BG);
        SetFont(f_btn, C_TEXT_LITE);
        DrawTextRect(S(12),S(142),S(198),S(38),"New Game",ALIGN_CENTER|VALIGN_MIDDLE);
        SetFont(f_tagline, C_TEXT_DARK);
        DrawTextRect(S(12),S(192),S(198),S(36),"Join the tiles,\nget to 2048!",ALIGN_LEFT);
    } else {
        SetFont(f_title, C_TITLE);
        DrawString(S(20), S(18), "2048");
        FillArea(S(375),S(15),S(100),S(58),C_SCORE_BG);
        FillArea(S(485),S(15),S(100),S(58),C_SCORE_BG);
        SetFont(f_score_lbl, C_TEXT_LITE);
        DrawTextRect(S(375),S(21),S(100),S(18),"SCORE",ALIGN_CENTER);
        DrawTextRect(S(485),S(21),S(100),S(18),"BEST", ALIGN_CENTER);
        SetFont(f_score_val, C_TEXT_LITE);
        snprintf(buf,sizeof(buf),"%d",score);
        DrawTextRect(S(375),S(40),S(100),S(28),buf,ALIGN_CENTER|VALIGN_MIDDLE);
        snprintf(buf,sizeof(buf),"%d",best_score);
        DrawTextRect(S(485),S(40),S(100),S(28),buf,ALIGN_CENTER|VALIGN_MIDDLE);
        FillArea(S(375),S(82),S(210),S(40),C_BTN_BG);
        SetFont(f_btn,C_TEXT_LITE);
        DrawTextRect(S(375),S(82),S(210),S(40),"New Game",ALIGN_CENTER|VALIGN_MIDDLE);
        SetFont(f_tagline,C_TEXT_DARK);
        DrawTextRect(S(20),S(138),S(340),S(22),"Join the tiles, get to 2048!",ALIGN_LEFT|VALIGN_MIDDLE);
    }
}

static void draw_overlay(const char *title, bool show_keep, bool show_try) {
    int ox, oy, ow, bw, btnY;
    char sub[32];
    ox=BOARD_X+S(50); oy=BOARD_Y+S(170); ow=BOARD_W-S(100);
    FillArea(ox,oy,ow,S(220),GS(0xEE));
    DrawRect(ox,oy,ow,S(220),GS(0x00));
    SetFont(f_message,GS(0x00));
    DrawTextRect(ox,oy+S(10),ow,S(80),title,ALIGN_CENTER|VALIGN_MIDDLE);
    snprintf(sub,sizeof(sub),"Score: %d",score);
    SetFont(f_msg_sub,GS(0x22));
    DrawTextRect(ox,oy+S(96),ow,S(28),sub,ALIGN_CENTER|VALIGN_MIDDLE);
    btnY=oy+S(140);
    if(show_keep&&show_try){
        bw=(ow-S(30))/2;
        FillArea(ox+S(10),    btnY,bw,S(40),GS(0x44));
        FillArea(ox+S(20)+bw, btnY,bw,S(40),GS(0x66));
        SetFont(f_btn,C_TEXT_LITE);
        DrawTextRect(ox+S(10),    btnY,bw,S(40),"Keep Going",ALIGN_CENTER|VALIGN_MIDDLE);
        DrawTextRect(ox+S(20)+bw, btnY,bw,S(40),"Try Again", ALIGN_CENTER|VALIGN_MIDDLE);
    } else if(show_try){
        FillArea(ox+(ow-S(170))/2,btnY,S(170),S(40),C_BTN_BG);
        SetFont(f_btn,C_TEXT_LITE);
        DrawTextRect(ox+(ow-S(170))/2,btnY,S(170),S(40),"Try Again",ALIGN_CENTER|VALIGN_MIDDLE);
    }
}

static void draw_game_buf(void) {
    int r, c;
    draw_header();
    draw_board_bg();
    for(r=0;r<GRID_SIZE;r++) for(c=0;c<GRID_SIZE;c++)
        if(grid[r][c]) draw_tile_at(grid[r][c],cell_x(c),cell_y(r),CELL_SIZE);
    if(!keep_playing&&won)
        draw_overlay("You win!",true,true);
    else if(game_over)
        draw_overlay("Game over!",false,true);
}

/* Full quality refresh — use only on first show to avoid e-ink blink */
static void draw_game_full(void) { draw_game_buf(); FullUpdate(); }

/* Fast no-blink refresh — use after every move */
static void draw_game(void)      { draw_game_buf(); SoftUpdate(); }

/* SLIDE phase */
static void draw_slide(float t) {
    bool is_src[GRID_SIZE][GRID_SIZE];
    int i, r, c;
    draw_header();
    draw_board_bg();

    memset(is_src,0,sizeof(is_src));
    for(i=0;i<move_recs_n;i++) is_src[move_recs[i].sr][move_recs[i].sc]=true;

    for(r=0;r<GRID_SIZE;r++) for(c=0;c<GRID_SIZE;c++)
        if(old_grid[r][c]&&!is_src[r][c])
            draw_tile_at(old_grid[r][c],cell_x(c),cell_y(r),CELL_SIZE);

    for(i=0;i<move_recs_n;i++){
        MoveRec *m;
        int fx, fy, tx, ty, x, y;
        m=&move_recs[i];
        fx=cell_x(m->sc); fy=cell_y(m->sr);
        tx=cell_x(m->dc); ty=cell_y(m->dr);
        x=fx+(int)((tx-fx)*t);
        y=fy+(int)((ty-fy)*t);
        draw_tile_at(old_grid[m->sr][m->sc],x,y,CELL_SIZE);
    }
    PartialUpdate(BOARD_X, BOARD_Y, BOARD_W, BOARD_H);
}

/* POP phase */
static void draw_pop(float t) {
    int r, c;
    draw_header();
    draw_board_bg();

    for(r=0;r<GRID_SIZE;r++) for(c=0;c<GRID_SIZE;c++){
        if(!grid[r][c]) continue;
        if(merge_cells[r][c]) continue;
        if(r==new_tile_r&&c==new_tile_c) continue;
        draw_tile_at(grid[r][c],cell_x(c),cell_y(r),CELL_SIZE);
    }

    for(r=0;r<GRID_SIZE;r++) for(c=0;c<GRID_SIZE;c++){
        float sc; int sz, ox, oy;
        if(!merge_cells[r][c]||!grid[r][c]) continue;
        sc=1.0f+0.12f*(float)sinf((float)M_PI*t);
        sz=(int)(CELL_SIZE*sc);
        ox=cell_x(c)-(sz-CELL_SIZE)/2;
        oy=cell_y(r)-(sz-CELL_SIZE)/2;
        draw_tile_at(grid[r][c],ox,oy,sz);
    }

    if(new_tile_r>=0&&grid[new_tile_r][new_tile_c]){
        float sc; int sz, ox, oy;
        sc=0.5f+0.5f*t;
        sz=(int)(CELL_SIZE*sc);
        ox=cell_x(new_tile_c)+(CELL_SIZE-sz)/2;
        oy=cell_y(new_tile_r)+(CELL_SIZE-sz)/2;
        draw_tile_at(grid[new_tile_r][new_tile_c],ox,oy,sz);
    }
    PartialUpdate(BOARD_X, BOARD_Y, BOARD_W, BOARD_H);
}

/* ── Timer callbacks ─────────────────────────────────────────────────────── */
static void anim_slide_tick(void);
static void anim_pop_tick(void);

static void anim_pop_tick(void) {
    double now=emscripten_get_now();
    float p=(float)((now-phase_start)/POP_MS);
    if(p>=1.0f){
        in_pop=false;
        draw_game();
    } else {
        draw_pop(smoothstep(p));
        SetHardTimer("anim_pop", anim_pop_tick, 16);
    }
}

static void anim_slide_tick(void) {
    double now=emscripten_get_now();
    float p=(float)((now-phase_start)/SLIDE_MS);
    if(p>=1.0f){
        in_slide=false;
        new_tile_r=new_tile_c=-1;
        add_random_tile();
        if(!moves_available()) game_over=true;
        phase_start=emscripten_get_now();
        in_pop=true;
        SetHardTimer("anim_pop", anim_pop_tick, 16);
    } else {
        draw_slide(smoothstep(p));
        SetHardTimer("anim_slide", anim_slide_tick, 16);
    }
}

/* ── Input ───────────────────────────────────────────────────────────────── */
static bool is_animating(void) { return in_slide||in_pop; }

static void start_move(int dir) {
    if(is_animating()) return;
    if(game_over){ new_game(); draw_game(); return; }
    if(won&&!keep_playing) return;
    memcpy(old_grid,grid,sizeof(grid));
    if(do_move(dir)){
        phase_start=emscripten_get_now();
        in_slide=true;
        SetHardTimer("anim_slide", anim_slide_tick, 16);
    }
}

static void process_swipe(int dx, int dy){
    int adx=dx<0?-dx:dx, ady=dy<0?-dy:dy;
    if(adx<S(40)&&ady<S(40)) return;
    start_move(adx>ady?(dx>0?1:3):(dy>0?2:0));
}

static bool hit_new_game(int x,int y){
    if(is_landscape) return x>=S(12)&&x<S(210)&&y>=S(142)&&y<S(180);
    return x>=S(375)&&x<S(585)&&y>=S(82)&&y<S(122);
}

static bool hit_keep_going(int x,int y){
    int ox, oy, ow, bw, btnY;
    if(!won||keep_playing) return false;
    ox=BOARD_X+S(50); oy=BOARD_Y+S(170); ow=BOARD_W-S(100);
    bw=(ow-S(30))/2; btnY=oy+S(140);
    return x>=ox+S(10)&&x<ox+S(10)+bw&&y>=btnY&&y<btnY+S(40);
}

static bool hit_try_again(int x,int y){
    int ox, oy, ow, btnY, btnX, bw;
    if(!won&&!game_over) return false;
    ox=BOARD_X+S(50); oy=BOARD_Y+S(170); ow=BOARD_W-S(100); btnY=oy+S(140);
    if(won&&!keep_playing){
        bw=(ow-S(30))/2; btnX=ox+S(20)+bw;
        return x>=btnX&&x<btnX+bw&&y>=btnY&&y<btnY+S(40);
    }
    btnX=ox+(ow-S(170))/2;
    return x>=btnX&&x<btnX+S(170)&&y>=btnY&&y<btnY+S(40);
}

/* ── Font helpers ────────────────────────────────────────────────────────── */
static void open_fonts(void) {
    f_title     = OpenFont("LiberationSans-Bold", S(72), 1);
    f_title_sm  = OpenFont("LiberationSans-Bold", S(48), 1);
    f_score_lbl = OpenFont("LiberationSans-Bold", S(13), 1);
    f_score_val = OpenFont("LiberationSans-Bold", S(22), 1);
    f_btn       = OpenFont("LiberationSans-Bold", S(18), 1);
    f_tagline   = OpenFont("LiberationSans",      S(17), 1);
    f_message   = OpenFont("LiberationSans-Bold", S(50), 1);
    f_msg_sub   = OpenFont("LiberationSans",      S(20), 1);
    f_tile[1]   = OpenFont("LiberationSans-Bold", S(65), 1);
    f_tile[2]   = OpenFont("LiberationSans-Bold", S(50), 1);
    f_tile[3]   = OpenFont("LiberationSans-Bold", S(38), 1);
    f_tile[4]   = OpenFont("LiberationSans-Bold", S(30), 1);
}

static void close_fonts(void) {
    int i;
    CloseFont(f_title);    CloseFont(f_title_sm);
    CloseFont(f_score_lbl); CloseFont(f_score_val);
    CloseFont(f_btn);      CloseFont(f_tagline);
    CloseFont(f_message);  CloseFont(f_msg_sub);
    for(i=1;i<=4;i++) CloseFont(f_tile[i]);
}

/* ── Main handler ────────────────────────────────────────────────────────── */
static int handler(int type, int par1, int par2) {
    switch(type){
    case EVT_INIT:
        is_landscape = (GetOrientation() == 1 || GetOrientation() == 2);
        update_scale();
        srand((unsigned)time(NULL));
        open_fonts();
        load_best();
        new_game();
        break;
    case EVT_SHOW:
        draw_game_full();
        break;
    case EVT_KEYPRESS:
        switch(par1){
        case KEY_UP:    start_move(0); break;
        case KEY_RIGHT: start_move(1); break;
        case KEY_DOWN:  start_move(2); break;
        case KEY_LEFT:  start_move(3); break;
        case KEY_NEXT:  start_move(1); break;
        case KEY_PREV:  start_move(3); break;
        case KEY_OK:
            if(is_animating()) break;
            if(won&&!keep_playing){ keep_playing=true; won=false; draw_game(); }
            else if(game_over)    { new_game(); draw_game(); }
            break;
        }
        break;
    case EVT_POINTERDOWN:
        ptr_x0=par1; ptr_y0=par2; ptr_down=true;
        break;
    case EVT_POINTERUP:
        if(ptr_down){
            int dx=par1-ptr_x0, dy=par2-ptr_y0;
            int adx=dx<0?-dx:dx, ady=dy<0?-dy:dy;
            if(adx<S(20)&&ady<S(20)){
                if(!is_animating()){
                    if(hit_new_game(ptr_x0,ptr_y0))        { new_game(); draw_game(); }
                    else if(hit_keep_going(ptr_x0,ptr_y0)) { keep_playing=true;won=false;draw_game(); }
                    else if(hit_try_again(ptr_x0,ptr_y0))  { new_game(); draw_game(); }
                }
            } else {
                process_swipe(dx,dy);
            }
            ptr_down=false;
        }
        break;
    case EVT_ORIENTATION:
        SetOrientation(par1);
        is_landscape = (par1==1||par1==2);
        update_scale();
        close_fonts();
        open_fonts();
        draw_game_full();
        break;
    case EVT_EXIT:
        close_fonts();
        break;
    }
    return 0;
}

int main(void){
    InkViewMain(handler);
    return 0;
}
