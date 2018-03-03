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
#include "timer.h"

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
    
    timeType t0, t1;
    // start event loop
    while(true)
    {
        t0 = timer();
        Fl::wait(.1);
        t1 = timer();
        //theGame->addToLog("doing systems");
        // TODO: get delta t for this frame...
        theGame->doSystems(t1-t0);
        myWin->redraw();
    }
    
    return 0;
}

/*
 * Function: Timer
 * Usage: printf("Time = %f\n",Timer()-t0);
 * ----------------------------------------
 * Returns the time in seconds and uses the timer
 * defined in timer.h.
 *
 */
timeType timer(void) {
#ifdef CLOCK
    return (timeType)clock()/(timeType)CLOCKS_PER_SEC;
#endif
    
#ifdef TIMES
    return (timeType)times(&tms_time)/(timeType)sysconf(_SC_CLK_TCK);
#endif
    
#ifdef GETRUSAGE
    struct rusage rusage0;
    getrusage(RUSAGE_SELF,&rusage0);
    return evaltime(rusage0);
#endif
    
#ifdef GETTIMEOFDAY
    struct timeval timeval_time;
    gettimeofday(&timeval_time,NULL);
    return evaltime(timeval_time);
#endif
}
