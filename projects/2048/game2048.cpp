#include "inkview.h"
#include <emscripten.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <cmath>

// ── Layout ────────────────────────────────────────────────────────────────────
//  Portrait  600×800: header above board (y=165), board 560×560
//  Landscape 800×600: left control panel (w=220), board to the right at x=230
//
//  Board is always 560×560 with padding=12 gap=12 cell=125
// ─────────────────────────────────────────────────────────────────────────────
#define GRID_SIZE  4
// BOARD_X / BOARD_Y are dynamic — see bx() / by() below
#define BOARD_X    (bx())
#define BOARD_Y    (by())
#define BOARD_W    560
#define BOARD_H    560
#define PADDING    12
#define GAP        12
#define CELL_SIZE  125   // (560 - 2*12 - 3*12) / 4

static bool is_landscape = false;
static int bx() { return is_landscape ? 230 : 20; }
static int by() { return is_landscape ? 20  : 165; }

// ── Greyscale palette ─────────────────────────────────────────────────────────
//  All colours use R=G=B (pure grey).  Macro GS(v) builds 0xVVVVVV.
#define GS(v) (((v)<<16)|((v)<<8)|(v))

#define C_BG        GS(0xF4)   // page background  – near white
#define C_BOARD     GS(0x88)   // board surround   – mid grey
#define C_EMPTY     GS(0xC8)   // empty cell       – light grey
#define C_TEXT_DARK GS(0x22)   // text on light tiles
#define C_TEXT_LITE GS(0xF4)   // text on dark tiles
#define C_SCORE_BG  GS(0x66)   // score box fill
#define C_BTN_BG    GS(0x44)   // button fill
#define C_TITLE     GS(0x00)   // title text

// ── Tile style table ──────────────────────────────────────────────────────────
//  Each row: { bg, fg, pattern_colour, pattern_type, step_px }
//  pattern_type: 0=solid  1=h-lines  2=v-lines  3=crosshatch
struct TileStyle { int bg, fg, pc, pt, step; };
static const TileStyle TS[] = {
    {},                                          // [0] unused
    {GS(0xEE), GS(0x22), 0,         0,  0},     //  2   solid very-light
    {GS(0xDD), GS(0x22), 0,         0,  0},     //  4   solid light
    {GS(0xCC), GS(0x22), GS(0xB0),  1, 12},     //  8   h-lines
    {GS(0xBB), GS(0x22), GS(0x9E),  2, 12},     // 16   v-lines
    {GS(0xAA), GS(0x11), GS(0x8A),  3, 14},     // 32   crosshatch
    {GS(0x88), GS(0xF4), GS(0x68),  1, 10},     // 64   h-lines  (dark bg)
    {GS(0x66), GS(0xF4), GS(0x4A),  2, 10},     // 128  v-lines
    {GS(0x44), GS(0xF4), GS(0x2C),  3, 12},     // 256  crosshatch
    {GS(0x33), GS(0xF4), GS(0x48),  1,  8},     // 512  h-lines
    {GS(0x1A), GS(0xFF), GS(0x32),  2,  8},     // 1024 v-lines
    {GS(0x00), GS(0xFF), 0,         0,  0},     // 2048 solid black
    {GS(0x00), GS(0xFF), GS(0x28),  3,  6},     // super crosshatch on black
};

// ── Game state ────────────────────────────────────────────────────────────────
static int  grid[GRID_SIZE][GRID_SIZE];
static int  score      = 0;
static int  best_score = 0;
static bool won        = false;
static bool game_over  = false;
static bool keep_playing = false;

// ── Animation state ───────────────────────────────────────────────────────────
// Phases: SLIDE (tiles move) → POP (merged pop + new tile appear) → IDLE

struct MoveRec { int sr, sc, dr, dc; };   // source/dest cell indices

static int      old_grid[GRID_SIZE][GRID_SIZE];
static MoveRec  move_recs[GRID_SIZE * GRID_SIZE];
static int      move_recs_n = 0;
static bool     merge_cells[GRID_SIZE][GRID_SIZE];  // cells that received a merge
static int      new_tile_r = -1, new_tile_c = -1;

#define SLIDE_MS  110.0
#define POP_MS    180.0
static double   phase_start = 0.0;
static bool     in_slide = false;
static bool     in_pop   = false;

// ── Input ─────────────────────────────────────────────────────────────────────
static int  ptr_x0, ptr_y0;
static bool ptr_down = false;

// ── Fonts ─────────────────────────────────────────────────────────────────────
static ifont *f_title, *f_title_sm;   // 72pt portrait / 48pt landscape
static ifont *f_score_lbl, *f_score_val, *f_btn;
static ifont *f_tagline, *f_message, *f_msg_sub;
static ifont *f_tile[5];   // [1]=1-2 digit … [4]=5 digit

// ── Helpers ───────────────────────────────────────────────────────────────────
static int tile_idx(int v) { int i=0; while(v>1){v>>=1;i++;} return i>12?12:i; }
static const TileStyle &tile_style(int v) { return TS[tile_idx(v)]; }
static ifont *tile_font(int v) {
    if (v<100)   return f_tile[1];
    if (v<1000)  return f_tile[2];
    if (v<10000) return f_tile[3];
    return f_tile[4];
}

// ── Pattern helpers ───────────────────────────────────────────────────────────
static void pat_hlines(int x,int y,int w,int h,int c,int s){
    for(int dy=s/2; dy<h; dy+=s) DrawLine(x,y+dy,x+w-1,y+dy,c);
}
static void pat_vlines(int x,int y,int w,int h,int c,int s){
    for(int dx=s/2; dx<w; dx+=s) DrawLine(x+dx,y,x+dx,y+h-1,c);
}
static void pat_cross(int x,int y,int w,int h,int c,int s){
    pat_hlines(x,y,w,h,c,s); pat_vlines(x,y,w,h,c,s);
}
static int cell_x(int c) { return BOARD_X + PADDING + c*(CELL_SIZE+GAP); }
static int cell_y(int r) { return BOARD_Y + PADDING + r*(CELL_SIZE+GAP); }
static float smoothstep(float t) { return t*t*(3.0f-2.0f*t); }

// ── Persistence ───────────────────────────────────────────────────────────────
static void load_best() {
    best_score = EM_ASM_INT({ return parseInt(localStorage.getItem('pb2048_best')||'0'); });
}
static void save_best() {
    EM_ASM({ localStorage.setItem('pb2048_best', $0); }, best_score);
}

// ── Game logic ────────────────────────────────────────────────────────────────
static int empty_count() {
    int n=0;
    for(int r=0;r<GRID_SIZE;r++) for(int c=0;c<GRID_SIZE;c++) if(!grid[r][c]) n++;
    return n;
}

static void add_random_tile() {
    int cnt=empty_count(); if(!cnt){new_tile_r=new_tile_c=-1;return;}
    int tgt=rand()%cnt, k=0;
    for(int r=0;r<GRID_SIZE;r++) for(int c=0;c<GRID_SIZE;c++) if(!grid[r][c]){
        if(k++==tgt){ grid[r][c]=(rand()%10==0)?4:2; new_tile_r=r; new_tile_c=c; return; }
    }
}

static bool moves_available() {
    if(empty_count()>0) return true;
    for(int r=0;r<GRID_SIZE;r++) for(int c=0;c<GRID_SIZE;c++){
        int v=grid[r][c];
        if(c+1<GRID_SIZE&&grid[r][c+1]==v) return true;
        if(r+1<GRID_SIZE&&grid[r+1][c]==v) return true;
    }
    return false;
}

static void new_game() {
    memset(grid,0,sizeof(grid));
    memset(merge_cells,0,sizeof(merge_cells));
    score=0; won=false; game_over=false; keep_playing=false;
    in_slide=false; in_pop=false;
    add_random_tile(); new_tile_r=new_tile_c=-1;
    add_random_tile(); new_tile_r=new_tile_c=-1;
}

// do_move: updates grid[], populates move_recs/merge_cells for animation
static bool do_move(int dir) {
    static const int DX[]={0,1,0,-1}, DY[]={-1,0,1,0};
    int vx=DX[dir], vy=DY[dir];

    bool mflag[GRID_SIZE][GRID_SIZE]={};
    memset(merge_cells,0,sizeof(merge_cells));
    move_recs_n=0;
    bool moved=false;

    int r0=(vy>0)?GRID_SIZE-1:0, r1=(vy>0)?-1:GRID_SIZE, rs=(vy>0)?-1:1;
    int c0=(vx>0)?GRID_SIZE-1:0, c1=(vx>0)?-1:GRID_SIZE, cs=(vx>0)?-1:1;

    for(int r=r0;r!=r1;r+=rs) for(int c=c0;c!=c1;c+=cs){
        if(!grid[r][c]) continue;
        int val=grid[r][c];
        int pr=r,pc=c, nr=r+vy,nc=c+vx;
        while(nr>=0&&nr<GRID_SIZE&&nc>=0&&nc<GRID_SIZE&&!grid[nr][nc]){pr=nr;pc=nc;nr+=vy;nc+=vx;}
        if(nr>=0&&nr<GRID_SIZE&&nc>=0&&nc<GRID_SIZE&&grid[nr][nc]==val&&!mflag[nr][nc]){
            move_recs[move_recs_n++]={r,c,nr,nc};
            grid[nr][nc]=val*2; mflag[nr][nc]=true; merge_cells[nr][nc]=true;
            grid[r][c]=0;
            score+=val*2;
            if(score>best_score){best_score=score;save_best();}
            if(val*2==2048&&!keep_playing) won=true;
            moved=true;
        } else if(pr!=r||pc!=c){
            move_recs[move_recs_n++]={r,c,pr,pc};
            grid[pr][pc]=val; grid[r][c]=0;
            moved=true;
        }
    }
    return moved;
}

// ── Drawing helpers ───────────────────────────────────────────────────────────
static void draw_tile_at(int value, int x, int y, int size) {
    const TileStyle &s = tile_style(value);
    FillArea(x, y, size, size, s.bg);
    if (s.pt && s.pc) {
        // Scale pattern step proportionally to tile size
        int ps = s.step * size / CELL_SIZE;
        if (ps < 5) ps = 5;
        switch (s.pt) {
        case 1: pat_hlines(x,y,size,size,s.pc,ps); break;
        case 2: pat_vlines(x,y,size,size,s.pc,ps); break;
        case 3: pat_cross (x,y,size,size,s.pc,ps); break;
        }
    }
    char buf[12]; snprintf(buf,sizeof(buf),"%d",value);
    SetFont(tile_font(value), s.fg);
    DrawTextRect(x, y, size, size, buf, ALIGN_CENTER|VALIGN_MIDDLE);
}

static void draw_board_bg() {
    FillArea(BOARD_X, BOARD_Y, BOARD_W, BOARD_H, C_BOARD);
    for(int r=0;r<GRID_SIZE;r++) for(int c=0;c<GRID_SIZE;c++)
        FillArea(cell_x(c), cell_y(r), CELL_SIZE, CELL_SIZE, C_EMPTY);
}

static void draw_header() {
    int sw = ScreenWidth(), sh = ScreenHeight();
    FillArea(0, 0, sw, sh, C_BG);
    char buf[16];

    if (is_landscape) {
        // ── Left control panel (x=0..220, full height) ──
        // Vertical divider
        DrawLine(220, 0, 220, sh, GS(0xBB));

        // Title
        SetFont(f_title_sm, C_TITLE);
        DrawString(12, 12, "2048");

        // Score boxes side by side
        FillArea(12, 78, 94, 52, C_SCORE_BG);
        FillArea(116, 78, 94, 52, C_SCORE_BG);
        SetFont(f_score_lbl, C_TEXT_LITE);
        DrawTextRect(12, 84,  94, 18, "SCORE", ALIGN_CENTER);
        DrawTextRect(116, 84, 94, 18, "BEST",  ALIGN_CENTER);
        SetFont(f_score_val, C_TEXT_LITE);
        snprintf(buf, sizeof(buf), "%d", score);
        DrawTextRect(12, 104,  94, 24, buf, ALIGN_CENTER|VALIGN_MIDDLE);
        snprintf(buf, sizeof(buf), "%d", best_score);
        DrawTextRect(116, 104, 94, 24, buf, ALIGN_CENTER|VALIGN_MIDDLE);

        // New Game button
        FillArea(12, 142, 198, 38, C_BTN_BG);
        SetFont(f_btn, C_TEXT_LITE);
        DrawTextRect(12, 142, 198, 38, "New Game", ALIGN_CENTER|VALIGN_MIDDLE);

        // Tagline
        SetFont(f_tagline, C_TEXT_DARK);
        DrawTextRect(12, 192, 198, 36, "Join the tiles,\nget to 2048!", ALIGN_LEFT);

    } else {
        // ── Portrait header (above board) ──
        SetFont(f_title, C_TITLE);
        DrawString(20, 18, "2048");

        FillArea(375,15,100,58,C_SCORE_BG); FillArea(485,15,100,58,C_SCORE_BG);
        SetFont(f_score_lbl, C_TEXT_LITE);
        DrawTextRect(375,21,100,18,"SCORE",ALIGN_CENTER);
        DrawTextRect(485,21,100,18,"BEST", ALIGN_CENTER);
        SetFont(f_score_val, C_TEXT_LITE);
        snprintf(buf, sizeof(buf), "%d", score);
        DrawTextRect(375,40,100,28,buf,ALIGN_CENTER|VALIGN_MIDDLE);
        snprintf(buf, sizeof(buf), "%d", best_score);
        DrawTextRect(485,40,100,28,buf,ALIGN_CENTER|VALIGN_MIDDLE);

        FillArea(375,82,210,40,C_BTN_BG);
        SetFont(f_btn,C_TEXT_LITE);
        DrawTextRect(375,82,210,40,"New Game",ALIGN_CENTER|VALIGN_MIDDLE);

        SetFont(f_tagline,C_TEXT_DARK);
        DrawTextRect(20,138,340,22,"Join the tiles, get to 2048!",ALIGN_LEFT|VALIGN_MIDDLE);
    }
}

static void draw_overlay(const char *title, bool show_keep, bool show_try) {
    int ox=BOARD_X+50, oy=BOARD_Y+170, ow=BOARD_W-100;
    FillArea(ox,oy,ow,220,GS(0xEE));
    DrawRect(ox,oy,ow,220,GS(0x00));
    SetFont(f_message,GS(0x00));
    DrawTextRect(ox,oy+10,ow,80,title,ALIGN_CENTER|VALIGN_MIDDLE);
    char sub[32]; snprintf(sub,sizeof(sub),"Score: %d",score);
    SetFont(f_msg_sub,GS(0x22));
    DrawTextRect(ox,oy+96,ow,28,sub,ALIGN_CENTER|VALIGN_MIDDLE);
    int by=oy+140;
    if(show_keep&&show_try){
        int bw=(ow-30)/2;
        FillArea(ox+10,    by,bw,40,GS(0x44)); FillArea(ox+20+bw,by,bw,40,GS(0x66));
        SetFont(f_btn,C_TEXT_LITE);
        DrawTextRect(ox+10,    by,bw,40,"Keep Going",ALIGN_CENTER|VALIGN_MIDDLE);
        DrawTextRect(ox+20+bw, by,bw,40,"Try Again", ALIGN_CENTER|VALIGN_MIDDLE);
    } else if(show_try){
        FillArea(ox+(ow-170)/2,by,170,40,C_BTN_BG);
        SetFont(f_btn,C_TEXT_LITE);
        DrawTextRect(ox+(ow-170)/2,by,170,40,"Try Again",ALIGN_CENTER|VALIGN_MIDDLE);
    }
}

// Stable full draw
static void draw_game() {
    draw_header();
    draw_board_bg();
    for(int r=0;r<GRID_SIZE;r++) for(int c=0;c<GRID_SIZE;c++)
        if(grid[r][c]) draw_tile_at(grid[r][c],cell_x(c),cell_y(r),CELL_SIZE);
    if(!keep_playing&&won)
        draw_overlay("You win!",true,true);
    else if(game_over)
        draw_overlay("Game over!",false,true);
    FullUpdate();
}

// SLIDE phase: tiles move from old positions toward new positions
static void draw_slide(float t) {
    draw_header();
    draw_board_bg();

    // Which cells are "sources" (their tile has left)
    bool is_src[GRID_SIZE][GRID_SIZE]={};
    for(int i=0;i<move_recs_n;i++) is_src[move_recs[i].sr][move_recs[i].sc]=true;

    // Stationary tiles (were not moved away)
    for(int r=0;r<GRID_SIZE;r++) for(int c=0;c<GRID_SIZE;c++)
        if(old_grid[r][c]&&!is_src[r][c])
            draw_tile_at(old_grid[r][c],cell_x(c),cell_y(r),CELL_SIZE);

    // Sliding tiles at interpolated position (drawn on top)
    for(int i=0;i<move_recs_n;i++){
        MoveRec &m=move_recs[i];
        int fx=cell_x(m.sc), fy=cell_y(m.sr);
        int tx=cell_x(m.dc), ty=cell_y(m.dr);
        int x=fx+(int)((tx-fx)*t);
        int y=fy+(int)((ty-fy)*t);
        draw_tile_at(old_grid[m.sr][m.sc],x,y,CELL_SIZE);
    }
    PartialUpdate(BOARD_X, BOARD_Y, BOARD_W, BOARD_H);
}

// POP phase: merged tiles pop (scale 1→1.1→1) + new tile appears (scale 0.5→1)
static void draw_pop(float t) {
    draw_header();
    draw_board_bg();

    // Normal tiles (not merged, not new)
    for(int r=0;r<GRID_SIZE;r++) for(int c=0;c<GRID_SIZE;c++){
        if(!grid[r][c]) continue;
        if(merge_cells[r][c]) continue;
        if(r==new_tile_r&&c==new_tile_c) continue;
        draw_tile_at(grid[r][c],cell_x(c),cell_y(r),CELL_SIZE);
    }

    // Merged tiles: scale 1.0 → 1.12 → 1.0  (sin over [0,π])
    for(int r=0;r<GRID_SIZE;r++) for(int c=0;c<GRID_SIZE;c++){
        if(!merge_cells[r][c]||!grid[r][c]) continue;
        float sc=1.0f+0.12f*(float)sinf((float)M_PI*t);
        int sz=(int)(CELL_SIZE*sc);
        int ox=cell_x(c)-(sz-CELL_SIZE)/2;
        int oy=cell_y(r)-(sz-CELL_SIZE)/2;
        draw_tile_at(grid[r][c],ox,oy,sz);
    }

    // New tile: scale 0.5 → 1.0
    if(new_tile_r>=0&&grid[new_tile_r][new_tile_c]){
        float sc=0.5f+0.5f*t;
        int sz=(int)(CELL_SIZE*sc);
        int ox=cell_x(new_tile_c)+(CELL_SIZE-sz)/2;
        int oy=cell_y(new_tile_r)+(CELL_SIZE-sz)/2;
        draw_tile_at(grid[new_tile_r][new_tile_c],ox,oy,sz);
    }
    PartialUpdate(BOARD_X, BOARD_Y, BOARD_W, BOARD_H);
}

// ── Timer callbacks ───────────────────────────────────────────────────────────
static void anim_slide_tick();
static void anim_pop_tick();

static void anim_pop_tick() {
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

static void anim_slide_tick() {
    double now=emscripten_get_now();
    float p=(float)((now-phase_start)/SLIDE_MS);
    if(p>=1.0f){
        in_slide=false;
        // Finalise logical state
        new_tile_r=new_tile_c=-1;
        add_random_tile();
        if(!moves_available()) game_over=true;
        // Transition to POP phase
        phase_start=emscripten_get_now();
        in_pop=true;
        SetHardTimer("anim_pop", anim_pop_tick, 16);
    } else {
        draw_slide(smoothstep(p));
        SetHardTimer("anim_slide", anim_slide_tick, 16);
    }
}

// ── Input ─────────────────────────────────────────────────────────────────────
static bool is_animating() { return in_slide||in_pop; }

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
    if(adx<40&&ady<40) return;
    start_move(adx>ady?(dx>0?1:3):(dy>0?2:0));
}

static bool hit_new_game(int x,int y){
    if(is_landscape) return x>=12&&x<210&&y>=142&&y<180;
    return x>=375&&x<585&&y>=82&&y<122;
}

static bool hit_keep_going(int x,int y){
    if(!won||keep_playing) return false;
    int ox=BOARD_X+50,oy=BOARD_Y+170,ow=BOARD_W-100;
    int bw=(ow-30)/2,by=oy+140;
    return x>=ox+10&&x<ox+10+bw&&y>=by&&y<by+40;
}
static bool hit_try_again(int x,int y){
    if(!won&&!game_over) return false;
    int ox=BOARD_X+50,oy=BOARD_Y+170,ow=BOARD_W-100,by=oy+140;
    if(won&&!keep_playing){int bw=(ow-30)/2,bx=ox+20+bw;return x>=bx&&x<bx+bw&&y>=by&&y<by+40;}
    int bx=ox+(ow-170)/2;
    return x>=bx&&x<bx+170&&y>=by&&y<by+40;
}

// ── Main handler ──────────────────────────────────────────────────────────────
static int handler(int type, int par1, int par2) {
    switch(type){
    case EVT_INIT:
        f_title     = OpenFont("LiberationSans-Bold",72,1);
        f_title_sm  = OpenFont("LiberationSans-Bold",48,1);
        f_score_lbl = OpenFont("LiberationSans-Bold",13,1);
        f_score_val = OpenFont("LiberationSans-Bold",22,1);
        f_btn       = OpenFont("LiberationSans-Bold",18,1);
        f_tagline   = OpenFont("LiberationSans",     17,1);
        f_message   = OpenFont("LiberationSans-Bold",50,1);
        f_msg_sub   = OpenFont("LiberationSans",     20,1);
        f_tile[1]   = OpenFont("LiberationSans-Bold",65,1);
        f_tile[2]   = OpenFont("LiberationSans-Bold",50,1);
        f_tile[3]   = OpenFont("LiberationSans-Bold",38,1);
        f_tile[4]   = OpenFont("LiberationSans-Bold",30,1);
        srand((unsigned)time(NULL));
        load_best();
        new_game();
        break;
    case EVT_SHOW:
        draw_game();
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
            if(adx<20&&ady<20){
                if(!is_animating()){
                    if(hit_new_game(ptr_x0,ptr_y0))    { new_game(); draw_game(); }
                    else if(hit_keep_going(ptr_x0,ptr_y0)){ keep_playing=true;won=false;draw_game(); }
                    else if(hit_try_again(ptr_x0,ptr_y0)) { new_game(); draw_game(); }
                }
            } else {
                process_swipe(dx,dy);
            }
            ptr_down=false;
        }
        break;
    case EVT_ORIENTATION:
        SetOrientation(par1);
        is_landscape = (par1 == 1 || par1 == 2);
        draw_game();
        break;
    case EVT_EXIT:
        CloseFont(f_title);    CloseFont(f_title_sm);
        CloseFont(f_score_lbl); CloseFont(f_score_val);
        CloseFont(f_btn);      CloseFont(f_tagline);   CloseFont(f_message);
        CloseFont(f_msg_sub);
        for(int i=1;i<=4;i++) CloseFont(f_tile[i]);
        break;
    }
    return 0;
}

int main(){
    InkViewMain(handler);
    return 0;
}
