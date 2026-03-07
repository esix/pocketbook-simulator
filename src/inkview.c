#include "inkview.h"
#include <emscripten.h>
#include <unistd.h>

// helper function
EM_JS(char*, __pack_string, (char* jsstr), {
  if (jsstr === null || jsstr === undefined) {
    return 0; //  NULL
  }
  var length = lengthBytesUTF8(jsstr) + 1;
  var ptr = _malloc(length);
  stringToUTF8(jsstr, ptr, length);
  return ptr;
});

// EMSCRIPTEN_KEEPALIVE
// char* _pack_string(char* ptr) {
//     return strdup(ptr);
// }

EMSCRIPTEN_KEEPALIVE
ifont* _create_ifont(const char* name, const char* family, int size, unsigned char aa,
                    unsigned char isbold, unsigned char isitalic, unsigned short charset,
                    int color, int height, int linespacing, int baseline, void* fdata) {
    ifont* font = (ifont*)malloc(sizeof(ifont));
    if (!font) return NULL;

    font->name = name ? strdup(name) : NULL;
    font->family = family ? strdup(family) : NULL;
    font->size = size;
    font->aa = aa;
    font->isbold = isbold;
    font->isitalic = isitalic;
    font->_r1 = 0;
    font->charset = charset;
    font->_r2 = 0;
    font->color = color;
    font->height = height;
    font->linespacing = linespacing;
    font->baseline = baseline;
    font->fdata = fdata;

    return font;
}




// api
EM_JS(void, OpenScreen, (), { return Module.api.OpenScreen() });
EM_JS(void, OpenScreenExt, (), { return Module.api.OpenScreenExt() });
EM_JS(void, InkViewMain, (iv_handler handler), { return Module.api.InkViewMain(handler) });
EM_JS(void, CloseApp, (), { return Module.api.CloseApp() });

EM_JS(int, ScreenWidth, (), { return Module.api.ScreenWidth() });
EM_JS(int, ScreenHeight, (), { return Module.api.ScreenHeight() });

EM_JS(void, SetOrientation, (int n), { return Module.api.SetOrientation(n) });
EM_JS(int, GetOrientation, (), { return Module.api.GetOrientation() });
EM_JS(void, SetGlobalOrientation, (int n), { return Module.api.SetGlobalOrientation(n) });
EM_JS(int, GetGlobalOrientation, (), { return Module.api.GetGlobalOrientation() });
EM_JS(int, QueryGSensor, (), { return Module.api.QueryGSensor() });
// void SetGSensor(int mode);
// int ReadGSensor(int *x, int *y, int *z);
// void CalibrateGSensor();

EM_JS(void, ClearScreen, (), { Module.api.ClearScreen() });
EM_JS(void, SetClip, (int x, int y, int w, int h), { Module.api.SetClip(x, y, w, h) });
EM_JS(void, DrawPixel, (int x, int y, int color), { Module.api.DrawPixel(x, y, color) });
EM_JS(void, DrawLine, (int x1, int y1, int x2, int y2, int color), { Module.api.DrawLine(x1, y1, x2, y2, color); });
// void DrawRect(int x, int y, int w, int h, int color);
EM_JS(void, FillArea, (int x, int y, int w, int h, int color), { Module.api.FillArea(x, y, w, h, color) });
EM_JS(void, DrawRect, (int x, int y, int w, int h, int color), { Module.api.DrawRect(x, y, w, h, color) });
EM_JS(void, InvertAreaBW, (int x, int y, int w, int h), { Module.api.InvertAreaBW(x, y, w, h) });
EM_JS(void, DrawCircle, (int x0, int y0, int radius, int color), { Module.api.DrawCircle(x0, y0, radius, color) });
// void InvertArea(int x, int y, int w, int h);
// void InvertAreaBW(int x, int y, int w, int h);
// void DimArea(int x, int y, int w, int h, int color);
// void DrawSelection(int x, int y, int w, int h, int color);
// void DitherArea(int x, int y, int w, int h, int levels, int method);
// void Stretch(const unsigned char *src, int format, int sw, int sh, int scanline, int dx, int dy, int dw, int dh, int rotate);
// void SetCanvas(icanvas *c);
// icanvas *GetCanvas();
// void Repaint();

// ibitmap *LoadBitmap(const char *filename);
// int SaveBitmap(const char *filename, ibitmap *bm);
// ibitmap *BitmapFromScreen(int x, int y, int w, int h);
// ibitmap *BitmapFromScreenR(int x, int y, int w, int h, int rotate);
// ibitmap *NewBitmap(int w, int h);
// ibitmap *LoadJPEG(const char *path, int w, int h, int br, int co, int proportional);
// int SaveJPEG(const char *path, ibitmap *bmp, int quality);
// ibitmap *LoadPNG(const char *path, int dither);
// int SavePNG(const char *path, ibitmap *bmp);
// void SetTransparentColor(ibitmap **bmp, int color);
// void DrawBitmap(int x, int y, const ibitmap *b);
// void DrawBitmapArea(int x, int y, const ibitmap *b, int bx, int by, int bw, int bh);
// void DrawBitmapRect(int x, int y, int w, int h, ibitmap *b, int flags);
// void DrawBitmapRect2(irect *rect, ibitmap *b);
// void StretchBitmap(int x, int y, int w, int h, const ibitmap *src, int flags);
// void TileBitmap(int x, int y, int w, int h, const ibitmap *src);
// ibitmap *CopyBitmap(const ibitmap *bm);
// void MirrorBitmap(ibitmap *bm, int m);

// char **EnumFonts();
EM_JS(ifont*, jsOpenFont, (const char *name, int size, int aa), {
    var fontData = Module.api.OpenFont(UTF8ToString(name), size, aa);
    if (!fontData) return 0;

    var namePtr = __pack_string(fontData.name);
    var familyPtr = __pack_string(fontData.family);

    var ptr = Module.__create_ifont(
            namePtr,
            familyPtr,
            fontData.size,
            fontData.aa,
            fontData.isbold,
            fontData.isitalic,
            fontData.charset,
            fontData.color,
            fontData.height,
            fontData.linespacing,
            fontData.baseline,
            0
    );
    // Store pointer → JS font data mapping so SetFont can look it up
    Module.api._fontsByPtr.set(ptr, fontData);
    return ptr;
});
ifont *OpenFont(const char *name, int size, int aa) {return jsOpenFont(name, size, aa);}



EM_JS(void, CloseFont, (ifont *f), { return Module.api.CloseFont(f) });
EM_JS(void, SetFont, (ifont *font, int color), { return Module.api.SetFont(font, color) });
// ifont *GetFont();
// void DrawString(int x, int y, const char *s);
// void DrawStringR(int x, int y, const char *s);
// int TextRectHeight(int width, const char *s, int flags);
EM_JS(void, DrawString, (int x, int y, const char *s), { Module.api.DrawString(x, y, UTF8ToString(s)) });
EM_JS(void, DrawStringR, (int x, int y, const char *s), { Module.api.DrawString(x, y, UTF8ToString(s)) });
EM_JS(int, StringWidth, (const char *s), { return Module.api.StringWidth(UTF8ToString(s)) });
EM_JS(int, CharWidth, (unsigned short c), { return Module.api.CharWidth(c) });
EM_JS(char*, DrawTextRect, (int x, int y, int w, int h, const char *s, int flags), { return __pack_string(Module.api.DrawTextRect(x, y, w, h, UTF8ToString(s), flags)) });
// char *DrawTextRect2(irect *rect, const char *s);
// int CharWidth(unsigned  short c);
// int StringWidthExt(const char *s, int l);
// int StringWidth(const char *s);
// int DrawSymbol(int x, int y, int symbol);
// void RegisterFontList(ifont **fontlist, int count);
// void SetTextStrength(int n);

EM_JS(iv_mtinfo*, GetTouchInfo, (), {
    if (!Module._mtbuf) { Module._mtbuf = Module._malloc(16); }
    HEAP32[(Module._mtbuf)      >> 2] = (Module._touchX | 0);
    HEAP32[(Module._mtbuf + 4)  >> 2] = (Module._touchY | 0);
    HEAP32[(Module._mtbuf + 8)  >> 2] = 255;
    HEAP32[(Module._mtbuf + 12) >> 2] = 0;
    return Module._mtbuf;
});

EM_JS(void, FullUpdate, (), { return Module.api.FullUpdate() });
EM_JS(void, SoftUpdate, (), { Module.api.SoftUpdate() });
EM_JS(void, PartialUpdate, (int x, int y, int w, int h), { Module.api.PartialUpdate(x, y, w, h) });
EM_JS(void, PartialUpdateBW, (int x, int y, int w, int h), { Module.api.PartialUpdateBW(x, y, w, h) });
// void FullUpdateHQ();
// void PartialUpdate(int x, int y, int w, int h);
// void PartialUpdateBlack(int x, int y, int w, int h);
// void PartialUpdateBW(int x, int y, int w, int h);
// void DynamicUpdate(int x, int y, int w, int h);
// void DynamicUpdateBW(int x, int y, int w, int h);
// void FineUpdate();
// //void DynamicUpdate();
// int FineUpdateSupported();
// int HQUpdateSupported();
// void ScheduleUpdate(int x, int y, int w, int h, int bw);

// iv_handler SetEventHandler(iv_handler hproc);
// iv_handler SetEventHandlerEx(iv_handler hproc);
// iv_handler GetEventHandler();
// void SendEvent(iv_handler hproc, int type, int par1, int par2);
// void FlushEvents();
// char *iv_evttype(int type);
// char IsAnyEvents();

// void SetHardTimer(const char *name, iv_timerproc tproc, int ms);
// void SetWeakTimer(const char *name, iv_timerproc tproc, int ms);
// long long QueryTimer(iv_timerproc tp);
// void ClearTimer(iv_timerproc tproc);

// void OpenMenu(imenu *menu, int pos, int x, int y, iv_menuhandler hproc);
// void OpenMenuEx(imenuex *menu, int pos, int x, int y, iv_menuhandler hproc);
// void UpdateMenuEx(imenuex *menu);
// void OpenMenu3x3(const ibitmap *mbitmap, const char *strings[9], iv_menuhandler hproc);
// void OpenList(const char *title, const ibitmap *background, int itemw, int itemh, int itemcount, int cpos, iv_listhandler hproc);
// void OpenDummyList(const char *title, const ibitmap *background, char *text, iv_listhandler hproc);
// char **EnumKeyboards();
// void LoadKeyboard(const char *kbdlang);
// void OpenKeyboard(const char *title, char *buffer, int maxlen, int flags, iv_keyboardhandler hproc);
// void OpenCustomKeyboard(const char *filename, const char *title, char *buffer, int maxlen, int flags, iv_keyboardhandler hproc);
// void CloseKeyboard();
// void GetKeyboardRect(irect *rect);
// int IsKeyboardOpened();
// void OpenPageSelector(iv_pageselecthandler hproc);
// void OpenTimeEdit(const char *title, int x, int y, long intime, iv_timeedithandler hproc);
// void OpenDirectorySelector(const char *title, char *buf, int len, iv_dirselecthandler hproc);
// void OpenFontSelector(const char *title, const char *font, int with_size, iv_fontselecthandler hproc);
// void OpenBookmarks(int page, long long position, int *bmklist, long long *poslist,
// 		int *bmkcount, int maxbmks, iv_bmkhandler hproc);
// void SwitchBookmark(int page, long long position, int *bmklist, long long *poslist,
// 		int *bmkcount, int maxbmks, iv_bmkhandler hproc);
// void OpenContents(tocentry *toc, int count, long long position, iv_tochandler hproc);
// void OpenRotateBox(iv_rotatehandler hproc);
// void Message(int icon, const char *title, const char *text, int timeout);
// void Dialog(int icon, const char *title, const char *text, const char *button1, const char *button2, iv_dialoghandler hproc);
// void Dialog3(int icon, const char *title, const char *text, const char *button1, const char *button2, const char *button3, iv_dialoghandler hproc);
// void CloseDialog();
// void OpenProgressbar(int icon, const char *title, const char *text, int percent, iv_dialoghandler hproc);
// int  UpdateProgressbar(const char *text, int percent);
// void CloseProgressbar();
// void ShowHourglass();
// void ShowHourglassAt(int x, int y);
// void HideHourglass();
// void DisableExitHourglass();
// void LockDevice();
// void SetPanelType(int type);
// int  DrawPanel(const ibitmap *icon, const char *text, const char *title, int percent);
// int  DrawTabs(const ibitmap *icon, int current, int total);
// int  PanelHeight();
// void SetKeyboardRate(int t1, int t2);
// int QuickNavigatorSupported(int flags);
// void QuickNavigator(int x, int y, int w, int h, int cx, int cy, int flags);
// void SetQuickNavigatorXY(int x, int y);

// iconfig * GetGlobalConfig();
// iconfig * OpenConfig(const char *path, iconfigedit *ce);
// int SaveConfig(iconfig *cfg);
// void CloseConfig(iconfig *cfg);
// int ReadInt(iconfig *cfg, const char *name, int deflt);
// char *ReadString(iconfig *cfg, const char *name, const char *deflt);
// char *ReadSecret(iconfig *cfg, const char *name, const char *deflt);
// void WriteInt(iconfig *cfg, const char *name, int value);
// void WriteString(iconfig *cfg, const char *name, const char *value);
// void WriteSecret(iconfig *cfg, const char *name, const char *value);
// void WriteIntVolatile(iconfig *cfg, const char *name, int value);
// void WriteStringVolatile(iconfig *cfg, const char *name, const char *value);
// void DeleteInt(iconfig *cfg, const char *name);
// void DeleteString(iconfig *cfg, const char *name);
// void OpenConfigEditor(const char *header, iconfig *cfg, iconfigedit *ce, iv_confighandler hproc, iv_itemchangehandler cproc);
// void OpenConfigSubmenuExt(const char *title, iconfigedit *ice, int pos);
// void OpenConfigSubmenu(const char *title, iconfigedit *ice);
// void UpdateCurrentConfigPage();
// void UpdateConfigPage(const char *title, iconfigedit *ice);
// void CloseConfigLevel();
// void NotifyConfigChanged();
// void GetKeyMapping(char *act0[], char *act1[]);
// void GetKeyMappingEx(int what, char *act0[], char *act1[], int count);
// unsigned long QueryDeviceButtons();

// int MultitaskingSupported();
// int NewTask(char *path, char *args[], char *appname, char *name, ibitmap *icon, unsigned int flags);
// int NewSubtask(char *name);
// int SwitchSubtask(int subtask);
// void SubtaskFinished(int subtask);
// int GetCurrentTask();
// void GetActiveTask(int *task, int *subtask);
// int IsTaskActive();
// void GetPreviousTask(int *task, int *subtask);
// int GetTaskList(int *list, int size);
// taskinfo *GetTaskInfo(int task);
// int FindTaskByBook(char *name, int *task, int *subtask);
// int FindTaskByAppName(char *name);
// int SetTaskParameters(int task, char *appname, char *name, ibitmap *icon, unsigned int flags);
// int SetSubtaskInfo(int task, int subtask, char *name, char *book);
// int SetActiveTask(int task, int subtask);
// void GoToBackground();
// int CloseTask(int task, int subtask, int force);
// int SendEventTo(int task, int type, int par1, int par2);
// int SendEventSyncTo(int task, int type, int par1, int par2);
// int SendMessageTo(int task, int request, void *message, int len);
// int SetRequestListener(int request, int flags, iv_requestlistener hproc);
// int SendRequest(int request, void *data, int inlen, int outlen, int timeout);
// int SendRequestTo(int task, int request, void *data, int inlen, int outlen, int timeout);
// int SendGlobalRequest(int param);
// void SetMessageHandler(iv_msghandler hproc);
// void OpenTaskList();
// icanvas *GetTaskFramebuffer(int task);
// void ReleaseTaskFramebuffer(icanvas *fb);

// ihash * hash_new(int prime);
// void hash_add(ihash *h, const char *name, const char *value);
// void hash_delete(ihash *h, const char *name);
// char *hash_find(ihash *h, const char *name);

// ihash * vhash_new(int prime, iv_hashaddproc addproc, iv_hashdelproc delproc);
// void vhash_add(ihash *h, const char *name, const void *value);
// void vhash_delete(ihash *h, const char *name);
// void *vhash_find(ihash *h, const char *name);

// void hash_clear(ihash *h);
// void hash_destroy(ihash *h);
// int  hash_count(ihash *h);
// void hash_enumerate(ihash *h, iv_hashcmpproc cmpproc, iv_hashenumproc enumproc, void *userdata);

// int iv_stat(const char *name, struct stat *st);
// int iv_access(const char *pathname, int mode);
// FILE *iv_fopen(const char *name, const char *mode);
// int iv_fread(void *buffer, int size, int count, FILE *f);
// int iv_fwrite(const void *buffer, int size, int count, FILE *f);
// int iv_fseek(FILE *f, long offset, int whence);
// long iv_ftell(FILE *f);
// int iv_fclose(FILE *f);
// int iv_fgetc(FILE *f);
// char *iv_fgets(char *string, int n, FILE *f);
// int iv_mkdir(const char *pathname, mode_t mode);
// void iv_buildpath(const char *filename);
// DIR *iv_opendir(const char *dirname);
// struct dirent *iv_readdir(DIR *dir);
// int iv_closedir(DIR *dir);
int iv_unlink(const char *name) { return unlink(name); }
int iv_rmdir(const char *name)  { return rmdir(name); }
// int iv_truncate(const char *name, int length);
// int iv_rename(const char *oldname, const char *newname);
// void iv_preload(const char *name, int count);
// void iv_sync();
// int iv_validate_name(const char *s, int flags);
// void iv_setbgresponse(int t);

// long iv_ipc_request(long type, long attr, unsigned char *data, int inlen, int outlen);
// long iv_ipc_request_secure(long type, long param, unsigned char *data, int inlen, int outlen);
// long iv_ipc_cmd(long type, long param);

// char ** EnumLanguages();
// void LoadLanguage(const char *lang);
// void AddTranslation(const char *label, const char *trans);
// char *GetLangText(const char *s);
// char *GetLangTextF(const char *s, ...);
// void SetRTLBook(int rtl);
// int IsRTL();  // depends only on the system language
// int IsBookRTL();	// can be overwritten by application

// char **EnumProfiles();
// int GetProfileType(const char *name);
// ibitmap **EnumProfileAvatars();
// ibitmap *GetProfileAvatar(const char *name);
// int SetProfileAvatar(const char *name, ibitmap *ava);
// int CreateProfile(const char *name, int type);
// int RenameProfile(const char *oldname, const char *newname);
// int DeleteProfile(const char *name);
// char *GetCurrentProfile();
// void SetCurrentProfile(const char *name, int flags);
// void OpenProfileSelector();

// char ** EnumThemes();
// void OpenTheme(const char *path);
// ibitmap *GetResource(const char *name, const ibitmap *deflt);
// int GetThemeInt(const char *name, int deflt);
// char *GetThemeString(const char *name, const char *deflt);
// ifont *GetThemeFont(const char *name, const char *deflt);
// void GetThemeRect(const char *name, irect *rect, int x, int y, int w, int h, int flags);

// iv_filetype *GetSupportedFileTypes();
// bookinfo *GetBookInfo(const char *name);
// ibitmap *GetBookCover(const char *name, int width, int height);
// char *GetAssociatedFile(const char *name, int index);
// char *CheckAssociatedFile(const char *name, int index);
// void SetReadMarker(const char *name, int isread);
// iv_filetype *FileType(const char *path);
// iv_filetype *FileTypeExt(const char *path, struct stat *f_stat);
// void SetFileHandler(const char *path, const char *handler);
// char *GetFileHandler(const char *path);
// int OpenBook(const char *path, const char *parameters, int flags);
// void BookReady(const char *path);
// char **GetLastOpen();
// void AddLastOpen(const char *path);
// void OpenLastBooks();
// void FlushLastOpen();

// void OpenPlayer();
// void ClosePlayer();
// void PlayFile(const char *filename);
// void LoadPlaylist(char **pl);
// char **GetPlaylist();
// void PlayTrack(int n);
// void PreviousTrack();
// void NextTrack();
// int GetCurrentTrack();
// int GetTrackSize();
// void SetTrackPosition(int pos);
// int GetTrackPosition();
// void SetPlayerState(int state);
// int GetPlayerState();
// void SetPlayerMode(int mode);
// int GetPlayerMode();
// void TogglePlaying();
// void SetVolume(int n);
// int GetVolume();
// void SetEqualizer(int *eq);
// void GetEqualizer(int *eq);

// char **EnumNotepads();
// void OpenNotepad(const char *name);
// void CreateNote(const char *filename, const char *title, long long position);
// void CreateNoteFromImages(const char *filename, const char *title, long long position, ibitmap *img1, ibitmap *img2);
// void CreateNoteFromPage(const char *filename, const char *title, long long position);
// void CreateEmptyNote(const char *text);
// void OpenNotesMenu(const char *filename, const char *title, long long position);

// char **EnumDictionaries();
// int OpenDictionary(const char *name);
// void CloseDictionary();
// int LookupWord(const char *what, char **word, char **trans);
// int LookupWordExact(const char *what, char **word, char **trans);
// int LookupPrevious(char **word, char **trans);
// int LookupNext(char **word, char **trans);
// void OpenDictionaryView(iv_wlist *wordlist, const char *dicname);
// void OpenControlledDictionaryView(pointer_to_word_hand_t pointer_handler, iv_wlist *wordlist, const char *dicname);

// void iv_reflow_start(int x, int y, int w, int h, int scale);
// void iv_reflow_bt();
// void iv_reflow_et();
// void iv_reflow_div();
// void iv_reflow_addchar(int code, int x, int y, int w, int h);
// void iv_reflow_addimage(int x, int y, int w, int h, int flags);
// int iv_reflow_subpages();
// void iv_reflow_render(int spnum);
// int iv_reflow_getchar(int *x, int *y);
// int iv_reflow_getimage(int *x, int *y, int *scale);
// int iv_reflow_words();
// char *iv_reflow_getword(int n, int *spnum, int *x, int *y, int *w, int *h);
// void iv_reflow_clear();

// void iv_fullscreen();
// void iv_sleepmode(int on);
// int GetSleepmode();
// int GetBatteryPower();
// int GetTemperature();
// int IsCharging();
// int IsUSBconnected();
// int IsSDinserted();
// int IsPlayingMP3();
// int IsKeyPressed(int key);
// char *GetDeviceModel();
// char *GetHardwareType();
// char *GetSoftwareVersion();
// int GetHardwareDepth();
// char *GetSerialNumber();
// char *GetWaveformFilename();
// char *GetDeviceKey();
// unsigned char *GetDeviceFingerprint();
// char *CurrentDateStr();
// char *DateStr(time_t t);
// int GoSleep(int ms, int deep);
// void SetAutoPowerOff(int en);
// void PowerOff();
// int SafeMode();
// void OpenMainMenu();
// void CloseAllTasks();
// int WriteStartupLogo(const ibitmap *bm);
// int PageSnapshot();
// int RestoreStartupLogo();
// int QueryTouchpanel();
// void CalibrateTouchpanel();
// void OpenCalendar();
// int StartSoftwareUpdate();
// int HavePowerForSoftwareUpdate();

// int QueryNetwork();
// char *GetHwAddress();
// char *GetHwBTAddress();
// char *GetHw3GIMEI();
// int GetBluetoothMode();
// int SetBluetoothMode(int mode, int flags);
// char **EnumBTdevices();
// void OpenBTdevicesMenu(char *title, int x, int y, iv_itemchangehandler hproc);
// int BtSendFiles(char *mac, char **files);
// char **EnumWirelessNetworks();
// char **EnumConnections();
// int GetBTservice(const char *mac, const char *service);
// int NetConnect(const char *name);
// int NetDisconnect();
// iv_netinfo *NetInfo();
// void OpenNetworkInfo();
// char *GetUserAgent();
// char *GetDefaultUserAgent();
// char *GetProxyUrl();
// void *QuickDownloadExt(const char *url, int *retsize, int timeout, char *cookie, char *post);
// void *QuickDownload(const char *url, int *retsize, int timeout);
// int NewSession();
// void CloseSession(int id);
// void SetUserAgent(int id, const char *ua);
// void SetProxy(int id, const char *host, int port, const char *user, const char *pass);
// int Download(int id, const char *url, const char *postdata, FILE **fp, int timeout);
// int DownloadTo(int id, const char *url, const char *postdata, const char *filename, int timeout);
// int SetSessionFlag(int _id, int _flag, void *_value);
// int GetSessionStatus(int id);
// char * GetHeader(int id, const char *name);
// iv_sessioninfo *GetSessionInfo(int id);
// void PauseTransfer(int id);
// void ResumeTransfer(int id);
// void AbortTransfer(int id);
// char *NetError(int e);
// void NetErrorMessage(int e);
// int GetA2dpStatus();

// int iv_strcmp(const char *s1, const char *s2);
// int iv_strncmp(const char *s1, const char *s2, size_t n);
// int iv_strcasecmp(const char *s1, const char *s2);
// int iv_strncasecmp(const char *s1, const char *s2, size_t n);
// int escape(const char *val, char *buf, int size);
// int unescape(const char *val, char *buf, int size);
// void trim_right(char *s);
// unsigned short *get_encoding_table(const char *enc);
// int convert_to_utf(const char *src, char *dest, int destsize, const char *enc);
// int utf2ucs(const char *s, unsigned short *us, int maxlen);
// int utf2ucs4(const char *s, unsigned int *us, int maxlen);
// int ucs2utf(const unsigned short *us, char *s, int maxlen);
// int utfcasecmp(const char *sa, const char *sb);
// int utfncasecmp(const char *sa, const char *sb, int n);
// char *utfcasestr(const char *sa, const char *sb);
// void utf_toupper(char *s);
// void utf_tolower(char *s);
// void md5sum(const unsigned char *data, int len, unsigned char *digest);
// int base64_encode(const unsigned char *in, int len, char *out);
// int base64_decode(const char *in, unsigned char *out, int len);
// int copy_file(const char *src, const char *dst);
// int move_file(const char *src, const char *dst);
// int copy_file_with_af(const char *src, const char *dst);
// int move_file_with_af(const char *src, const char *dst);
// int unlink_file_with_af(const char *name);
// int recurse_action(const char *path, iv_recurser proc, void *data, int creative, int this_too);
// void LeaveInkViewMain();

// int GetDialogShow(); // 1 - dialog showing, 0 - dialog hidden.
// void SetMenuFont(ifont *font); // font for menu (one time using), need to set every times when open menu.

EM_JS(char*, GetLangText, (const char *s), { return __pack_string(Module.api.GetLangText(UTF8ToString(s))) });
EM_JS(char*, GetDeviceModel, (), { return __pack_string(Module.api.GetDeviceModel()) });

// --- irect helpers (pure C, no JS needed) ---

irect iRect(int x, int y, int w, int h, int flags) {
    irect r;
    r.x = (short)x;
    r.y = (short)y;
    r.w = (short)w;
    r.h = (short)h;
    r.flags = flags;
    return r;
}

int IsInRect(int x, int y, const irect *r) {
    return (x >= r->x && x < r->x + r->w &&
            y >= r->y && y < r->y + r->h) ? 1 : 0;
}

void FillAreaRect(const irect *r, int color) {
    FillArea(r->x, r->y, r->w, r->h, color);
}

char *DrawTextRect2(irect *rect, const char *s) {
    return DrawTextRect(rect->x, rect->y, rect->w, rect->h, s,
                        (rect->flags ? rect->flags : ALIGN_CENTER) | VALIGN_MIDDLE);
}

int PanelHeight(void) { return 0; }

EM_JS(int, DrawPanel, (const ibitmap *icon, const char *text, const char *title, int percent), {
    return 0; // no-op: simulator has no system panel
});

// SetHardTimer: one-shot timer (fires once after ms milliseconds)
EM_JS(void, SetHardTimer, (const char *name, iv_timerproc tproc, int ms), {
    if (!Module._timers) Module._timers = {};
    var timerName = UTF8ToString(name);
    if (Module._timers[timerName]) {
        clearTimeout(Module._timers[timerName]);
        delete Module._timers[timerName];
    }
    if (tproc && ms > 0) {
        Module._timers[timerName] = setTimeout(function() {
            delete Module._timers[timerName];
            Module.wasmTable.get(tproc)();
        }, ms);
    }
});

// OpenMenu: reads imenu[] from WASM memory and shows an HTML popup
// imenu struct: short type (2B) + short index (2B) + char* text (4B) + imenu* submenu (4B) = 12 bytes
EM_JS(void, OpenMenu, (imenu *menu_ptr, int pos, int x, int y, iv_menuhandler hproc), {
    var items = [];
    var ptr = menu_ptr;
    while (true) {
        var type  = HEAP16[ptr >> 1];
        var index = HEAP16[(ptr + 2) >> 1];
        var textP = HEAP32[(ptr + 4) >> 2];
        if (type === 0) break;
        items.push({ type: type, index: index, text: textP ? UTF8ToString(textP) : "" });
        ptr += 12;
    }
    Module.api.OpenMenu(items, x, y, hproc);
});

EM_JS(void, SetWeakTimer, (const char *name, iv_timerproc tproc, int ms), {
    if (!Module._timers) Module._timers = {};
    var timerName = UTF8ToString(name);
    if (Module._timers[timerName]) {
        clearTimeout(Module._timers[timerName]);
        delete Module._timers[timerName];
    }
    if (tproc && ms > 0) {
        Module._timers[timerName] = setTimeout(function() {
            delete Module._timers[timerName];
            Module.wasmTable.get(tproc)();
        }, ms);
    }
});

EM_JS(void, Message, (int icon, const char *title, const char *text, int timeout), {
    Module.api.Message(icon, UTF8ToString(title), UTF8ToString(text), timeout);
});

EM_JS(void, Dialog, (int icon, const char *title, const char *text,
                      const char *btn1, const char *btn2, iv_dialoghandler hproc), {
    Module.api.Dialog(icon, UTF8ToString(title), UTF8ToString(text),
        btn1 ? UTF8ToString(btn1) : null,
        btn2 ? UTF8ToString(btn2) : null,
        hproc);
});

EM_JS(void, OpenProgressbar, (int icon, const char *title, const char *text,
                               int percent, iv_dialoghandler hproc), {
    Module.api.OpenProgressbar(icon, UTF8ToString(title), UTF8ToString(text), percent, hproc);
});

EM_JS(int, UpdateProgressbar, (const char *text, int percent), {
    Module.api.UpdateProgressbar(UTF8ToString(text), percent);
    return 0;
});

EM_JS(void, CloseProgressbar, (), {
    Module.api.CloseProgressbar();
});

// DialogSynchro: synchronous modal dialog using window.prompt
EM_JS(int, DialogSynchro, (int icon, const char *title, const char *text,
                            const char *btn1, const char *btn2, const char *btn3), {
    var msg = UTF8ToString(title) + '\n\n' + UTF8ToString(text) + '\n\n';
    var b1 = btn1 ? UTF8ToString(btn1) : "";
    var b2 = btn2 ? UTF8ToString(btn2) : "";
    var b3 = btn3 ? UTF8ToString(btn3) : "";
    msg += '[1] ' + b1;
    if (b2) msg += '  [2] ' + b2;
    if (b3) msg += '  [3] ' + b3;
    var result = window.prompt(msg, '1');
    if (result === null) return 2;
    var n = parseInt(result);
    return (n >= 1 && n <= 3) ? n : 2;
});
