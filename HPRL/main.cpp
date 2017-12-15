//
//  main.cpp
//  HPRL
//
//  Created by Marcus Holm on 04/10/17.
//  Copyright © 2017 Marcus Holm. All rights reserved.
//


#include <iostream>
#include <FL/Fl.H>


#include "DrawWindow.hpp"
#include "Common.h"
#include "Game.hpp"

int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << "Hello, World!\n";
    
    int windowHeight = 768, windowWidth = 1024;
    
    if(argc > 1)
    {
        windowWidth = atoi(argv[1]);
        windowHeight = (int)windowWidth*0.75;
    }
    if(argc > 2)
    {
        windowHeight = atoi(argv[2]);
    }
    
    World* w = new World();
    
    Game* theGame = new Game(w);
    theGame->init();
    
    DrawWindow* myWin = new DrawWindow(10,10,windowWidth,windowHeight,(char*)"Cthonic Expedition", w, theGame);
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
