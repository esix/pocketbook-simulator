//------------------------------------------------------------------
// menuHandler.h
//
// Author:           JuanJakobo
// Date:             19.4.2020
// Description:      Handles the menubar and the menu
//-------------------------------------------------------------------

#ifndef MENU_HANDLER
#define MENU_HANDLER

#include <string>

class MenuHandler {
    public:
        MenuHandler(std::string name);
        void drawPanel();

        int getContentHeight();
        int getContentWidth();
        int getContentBeginX();
        int getContentBeginY();

    private:
        int menuHeight;
        int contentHeight;
        int contentWidth;
        int contentBeginX;
        int contentBeginY;

        static void panelHandlerStatic();
};

#endif
