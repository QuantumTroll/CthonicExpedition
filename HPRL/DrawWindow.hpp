//
//  DrawWindow.hpp
//  HPRL
//
//  Created by Marcus Holm on 04/10/17.
//  Copyright © 2017 Marcus Holm. All rights reserved.
//

#ifndef DrawWindow_hpp
#define DrawWindow_hpp

#include <iostream>
#include <vector>
#include <stdio.h>
#include <FL/Fl_Gl_Window.H>
#include "Common.h"
#include "Game.hpp"
#include "glf.h"


class DrawWindow : public Fl_Gl_Window {
    int panel_width;
    int drawPane_width;
    World* world;
    Game* game;
    int mask;
    
    int memorytex = 7;
    
    int thefont;

    std::deque<std::string> log;
    
    void draw();
    int handle(int);
    void drawTiles();
    void drawTile(int x, int y, int tex);
    void drawTile(int x, int y, int tex, float size, int ramp, int depth);
    void drawTile(int x, int y, int tx, int ty);
   // void drawTile(int x, int y, int tex, float tl, float tr, float br, float bl);
   // void drawTile(int x, int y, int tex, float tl, float tr, float br, float bl, float size, int ramp);
    void shadeTile(int x, int y, Float3 tl, Float3 tr, Float3 br, Float3 bl);
    std::vector<unsigned char> tileset;
    unsigned ts_width, ts_height;
    int tex_width, tex_height;
    float tile_width, tile_height;
    int tiles_on_screen_x, tiles_on_screen_y;
    
    int mainScreen_x, mainScreen_y;
    int sidePanel_x, sidePanel_y;
    int sidePanelX_y, sidePanelY_y;
    
    Float3 *** shade_main;
    Float3 *** shade_side;
    
    void zeroShade();
    
    void drawWorld();
    void shadeMain();
    void drawSidePanel();
    void shadeSidePanel();
    void drawBottomPanel();
    // do I need a shadeBottomPanel()? Not rly.
    void drawCrossX();
    void drawCrossY();
    void drawWorldX();
    void drawWorldY();
    Float3 getFloat3AtTile(int i, int j);
    
    void drawMenuBox(int x1, int y1, int x2, int y2);
    void drawMenu(int x1, int y1, int x2, int y2, int numOptions, MenuOption* options);
    void drawCurrentMenu();
    
    void drawLog();
public:
    DrawWindow(int X, int Y, int W, int H, char *L, World* w, Game* g);
    void print_text(const char* str);
    void print_text(const char* str, float x, float y);
    void print_text2(const char* str, int x, int y);
    
};


#endif /* DrawWindow_hpp */
