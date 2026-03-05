#ifndef MENU_HANDLER
#define MENU_HANDLER

#include <string>
#include <memory>
#include <cstring>

class MenuHandler {
public:
    explicit MenuHandler(const std::string &name);
    ~MenuHandler();

    irect *getContentRect()    { return &_contentRect; }
    irect *getMenuButtonRect() { return &_menuButtonRect; }

    int createMenu(iv_menuhandler handler);

private:
    ifont *_menuFont;

    char *_menu       = strdup("Menu");
    char *_clearScreen = strdup("Clear screen");
    char *_closeApp   = strdup("Close App");

    int _panelMenuBeginX;
    int _panelMenuBeginY;
    int _panelMenuHeight;
    int _mainMenuWidth;

    irect _menuButtonRect;
    irect _contentRect;

    static void panelHandlerStatic();
};

#endif
