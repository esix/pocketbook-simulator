#ifndef EVENT_HANDLER
#define EVENT_HANDLER

#include "menuHandler.h"
#include "basicView.h"

#include <memory>

class EventHandler {
public:
    EventHandler();

    int eventDistributor(int type, int par1, int par2);

    static std::unique_ptr<EventHandler> _eventHandlerStatic;

private:
    std::unique_ptr<BasicView> _basicView;
    MenuHandler _menu = MenuHandler("Hello World");

    static void mainMenuHandlerStatic(int index);
    void mainMenuHandler(int index);
    int pointerHandler(int type, int par1, int par2);
};

#endif
