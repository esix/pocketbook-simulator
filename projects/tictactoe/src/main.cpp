//------------------------------------------------------------------
// main.cpp
//
// Author:           JuanJakobo
// Date:             19.4.2020
// Description:      sets the inkview main handler
//-------------------------------------------------------------------

#include "inkview.h"
#include "eventHandler.h"

EventHandler* events = nullptr;

int Inkview_handler(int type, int par1, int par2){

    if(type==EVT_INIT)
    {
        events = new EventHandler();
        return 1;
    }else if(type==EVT_EXIT || type==EVT_HIDE)
    {
        delete events;
        events = nullptr;
        return 1;
    }else
    {
        return events->eventDistributor(type,par1,par2);
    }

    return 0;
}


int main(){

    OpenScreen();
    SetOrientation(0);
    InkViewMain(Inkview_handler);
    return 0;
}
