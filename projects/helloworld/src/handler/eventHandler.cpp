#include "inkview.h"
#include "eventHandler.h"
#include "menuHandler.h"
#include "basicView.h"

std::unique_ptr<EventHandler> EventHandler::_eventHandlerStatic;

EventHandler::EventHandler() {
    _eventHandlerStatic = std::unique_ptr<EventHandler>(this);
    _basicView = std::unique_ptr<BasicView>(new BasicView(_menu.getContentRect()));
    FullUpdate();
}

int EventHandler::eventDistributor(int type, int par1, int par2) {
    if (ISPOINTEREVENT(type))
        return EventHandler::pointerHandler(type, par1, par2);
    return 0;
}

void EventHandler::mainMenuHandlerStatic(int index) {
    _eventHandlerStatic->mainMenuHandler(index);
}

void EventHandler::mainMenuHandler(int index) {
    switch (index) {
    case 101: {
        int dialogResult = DialogSynchro(ICON_QUESTION, "Question",
            "Are you sure that you want to clear the screen?", "Yes", "No", "Cancel");
        if (dialogResult == 1) {
            _basicView->clearContent();
            FullUpdate();
        }
        break;
    }
    case 102:
        CloseApp();
        break;
    default:
        break;
    }
}

int EventHandler::pointerHandler(int type, int par1, int par2) {
    if (type == EVT_POINTERDOWN) {
        if (IsInRect(par1, par2, _menu.getMenuButtonRect()) == 1) {
            return _menu.createMenu(EventHandler::mainMenuHandlerStatic);
        } else {
            _basicView->clicked(par1, par2);
            PartialUpdate(par1, par2, 100, 100);
            return 1;
        }
    }
    return 0;
}
