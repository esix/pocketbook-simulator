//------------------------------------------------------------------
// eventHandler.h
//
// Author:           JuanJakobo
// Date:             19.4.2020
// Description:      Contains the tik-tak-toe game logic
//-------------------------------------------------------------------

#ifndef EVENT_HANDLER
#define EVENT_HANDLER

#include "game.h"
#include "menuHandler.h"
#include "string"

using namespace std;

class EventHandler {
    public:

        EventHandler();
        ~EventHandler();
        int eventDistributor(int type, int par1, int par2);

    private:

        static EventHandler *eventHandlerStatic;
        Game *game;
        MenuHandler *menu;
        string result;

        void startNewGame();
        int pointerHandler(int type, int par1, int par2);
        static void DialogHandlerStatic(int Button);
};

#endif
