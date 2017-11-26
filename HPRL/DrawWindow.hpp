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

class DrawWindow : public Fl_Gl_Window {
    int panel_width;
    int drawPane_width;
    World* world;
    Game* game;
    int mask;
    
    int memorytex = 7;

    std::deque<std::string> log;
    
    void draw();
    int handle(int);
    void drawTiles();
    void drawTile(int x, int y, int tex);
    void drawTile(int x, int y, int tex, float size, int ramp, int depth);
    void drawTile(int x, int y, int tx, int ty);
    void drawTile(int x, int y, int tex, float tl, float tr, float br, float bl);
    void drawTile(int x, int y, int tex, float tl, float tr, float br, float bl, float size, int ramp);
    std::vector<unsigned char> tileset;
    unsigned ts_width, ts_height;
    int tex_width, tex_height;
    float tile_width, tile_height;
    int tiles_on_screen_x, tiles_on_screen_y;
    void drawWorld();
    void drawSidePanel();
    void drawBottomPanel();
    void drawCrossX();
    void drawCrossY();
    void drawWorldX();
    void drawWorldY();
    Float3 getFloat3AtTile(int i, int j);
    
    void drawLog();
public:
    DrawWindow(int X, int Y, int W, int H, char *L, World* w, Game* g);
    void print_text(const char* str);
    void print_text(const char* str, float x, float y);
    void print_text2(const char* str, int x, int y);
    
};


#endif /* DrawWindow_hpp */
