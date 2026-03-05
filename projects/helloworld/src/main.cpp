#include <inkview.h>
#include <eventHandler.h>

EventHandler* events = nullptr;

int Inkview_handler(int type, int par1, int par2) {
    if (type == EVT_INIT) {
        events = new EventHandler();
        return 1;
    } else if (type == EVT_EXIT || type == EVT_HIDE) {
        // Note: EventHandler stores itself in a static unique_ptr, so we just
        // null the raw pointer here. Resources are cleaned up on process exit.
        events = nullptr;
        return 1;
    } else {
        if (events) return events->eventDistributor(type, par1, par2);
    }
    return 0;
}

int main() {
    OpenScreen();
    SetOrientation(0);
    InkViewMain(Inkview_handler);
    return 0;
}
