//
//  main.cpp
//  HPRL
//
//  Created by Marcus Holm on 04/10/17.
//  Copyright © 2017 Marcus Holm. All rights reserved.
//


#include <iostream>
#include <FL/Fl.H>
//#include <FL/Fl_Window.H>


#include "DrawWindow.hpp"
#include "Common.h"
#include "Game.hpp"

int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << "Hello, World!\n";
    
    World w;
    
    Game* theGame = new Game(&w);
    theGame->init();
    
    DrawWindow* myWin = new DrawWindow(10,10,800,600,(char*)"Cthonic Expedition", &w, theGame);
    myWin->mode(FL_RGB | FL_ALPHA | FL_DEPTH | FL_ACCUM);
    myWin->show();
    
    // start event loop
    while(true)
    {
        Fl::wait();
        //theGame->addToLog("doing systems");
        theGame->doSystems();
        myWin->redraw();
    }
    
    return 0;
}
