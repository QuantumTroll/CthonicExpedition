//
//  DrawWindow.cpp
//  HPRL
//
//  Created by Marcus Holm on 04/10/17.
//  Copyright © 2017 Marcus Holm. All rights reserved.
//


#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/gl.h>
#include <FL/glut.h>
#include <math.h>

#include "DrawWindow.hpp"
#include "lodepng.h"
#include "MapCell.hpp"

//#define TILEMAP

DrawWindow::DrawWindow(int X, int Y, int W, int H, char *L, World* w, Game* g):Fl_Gl_Window(X,Y,W,H,L)
{
    world = w;
    game = g;
    mask = COMP_IS_VISIBLE;
    const char filename[200]="/Users/marcus/Dropbox/Shared Programming/HPRL/HPRL/HPRL/art/tileset1.png";
    //"HPRL/art/hyptosis_tile-art-batch-1";
    unsigned error = lodepng::decode(tileset,ts_width,ts_height,filename);
    
    if(error)
    {
        printf("error loading tileset: %ud: %s\n",error,lodepng_error_text(error));
        exit(1);
    }
    
    float panel_prop = 0.25;
    
    panel_width = W*panel_prop;
    drawPane_width = W - panel_width;
    
    tiles_on_screen_x = 24;
    tex_width = 32;
    tex_height = tex_width;
    tile_width = 2.0*(1-panel_prop)/tiles_on_screen_x;
    tile_height = 2.0*((float)drawPane_width/H)/tiles_on_screen_x;
    tiles_on_screen_y = tiles_on_screen_x * tile_width/tile_height;
    printf("tiles on screen x=%d, tiles on screen y=%d\n",tiles_on_screen_x,tiles_on_screen_y);
}


void DrawWindow::draw() {
    if (!valid()) {
        //  ... set up projection, viewport, etc ...
        //  ... window size is in w() and h().
        //  ... valid() is turned on by FLTK after draw() returns
        
        glViewport(0, 0, w(), h());
        glMatrixMode(GL_PROJECTION);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();
        
        //glOrtho(-1,1,-1,1,1,10);
        
        // transparency
        glEnable( GL_BLEND) ;
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Enable the texture for OpenGL.
        glEnable(GL_TEXTURE_2D);
        glEnable (GL_TEXTURE_RECTANGLE_ARB);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //GL_NEAREST = no smoothing
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 4, ts_width, ts_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &tileset[0]);
        
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    glPushMatrix();
    glTranslatef(-1, -1, 0);
    //glScalef(.8, .8, 1);
    
    drawTiles();
    
    drawSidePanel();
    
    drawBottomPanel();
    
    //TODO: drawTopRightCorner(); // what goes here?
    
    glPopMatrix();
}


Float3 DrawWindow::getFloat3AtTile(int i, int j)
{
    Float3 p;
    /* // Entity's position
     int x = (int)(world->position[ent].x+.5);
     int y = (int)(world->position[ent].y+.5);
     int z = (int)(world->position[ent].z+.5);
     
     // the tile on which to draw it
     int i = game->getLookAt().x-x+tiles_on_screen_x/2;
     int j = game->getLookAt().y-y+tiles_on_screen_y/2; */
    p.x = game->getLookAt().x+i-tiles_on_screen_x/2;
    p.y = game->getLookAt().y+j-tiles_on_screen_y/2;
    p.z = game->getLookAt().z;
    p = sumFloat3(p, {0.5,0.5,0.5});
    return p;
}

void DrawWindow::drawTile(int x, int y, int tx, int ty)
{
    ty = tiles_on_screen_y - ty;
    int tex = tx + ty*1024/tex_width;
    drawTile(x,y,tex);
}

void DrawWindow::drawTile(int x, int y, int tex, float tl, float tr, float br, float bl)
{
    int tx, ty, tx2, ty2;
    tl *=2;    tr *=2;    bl *=2;    br *=2;
    // get the tile's texture coords
    tx = tex*tex_width % ts_width; // index*width per tile % tileset width
    ty = tex*tex_width/ts_width*tex_height;
    //printf("%d,%d,%d: %d,%d\n",x,y,tex,tx,ty);
    tx2 = tx+tex_width;
    ty2 = ty+tex_height;
    
    glPushMatrix();
    glTranslatef(tile_width*x, tile_height*y, 0);
    glBegin(GL_QUADS);
    glNormal3f(cosf(tl*M_2_PI),0,sinf(tl*M_2_PI));
    glTexCoord2d(tx,ty);   glVertex3f(0,tile_height,0);
    glNormal3f(cosf(tr*M_2_PI),0,sinf(tr*M_2_PI));
    glTexCoord2d(tx2,ty);  glVertex3f(tile_width,tile_height,0);
    glNormal3f(cosf(br*M_2_PI),0,sinf(br*M_2_PI));
    glTexCoord2d(tx2,ty2); glVertex3f(tile_width,0,0);
    glNormal3f(cosf(bl*M_2_PI),0,sinf(bl*M_2_PI));
    glTexCoord2d(tx,ty2);  glVertex3f(0,0,0);
    glEnd();
    glPopMatrix();
}

void DrawWindow::drawTile(int x, int y, int tex)
{
    drawTile(x,y,tex,1,1,1,1);
}


// draw landscape "tiles" in main window
void DrawWindow::drawTiles() {
    
    glPushMatrix();
    
    // main window offset
    glTranslatef(0,.5,0);
    
    int i,j,k;
#ifdef TILEMAP
    for(i=0; i<tiles_on_screen_x; i++)
    {
        for(j=0; j<tiles_on_screen_y; j++)
        {
            int ty = tiles_on_screen_y - j;
            int tex = i + ty*1024/tex_width;
            drawTile(i,j,tex,1,1,1,1);
            char s[128];
            sprintf(s,"  %d",tex);
            print_text2((const *)s,i,j);
        }
    }
#else
    
    //TODO: pull this out into a function that can be reused for side panel?
    
    PosInt p = Float32PosInt(world->position[game->getPlayerEntity()]);
  //  int cx = 0, cy = 0, cz = 0; // cell offset we're drawing from
    int z = game->getLookAt().z;
    int px = game->getLookAt().x-tiles_on_screen_x/2;
    int py = game->getLookAt().y-tiles_on_screen_y/2;
    
    int x, y;    
    
    for(i=0; i<tiles_on_screen_x; i++)
    {
        x = i+px;
        
        for(j=0; j<tiles_on_screen_y; j++)
        {
            y = j+py;
            
            int depth = 0;
            int depth_max = 8;
            for(depth = 0; depth < depth_max; depth ++)
            {
                int zm = z - depth;
            
                // check if tile is viewable and player can see it
                MapTile * tile = game->getTile(x,y,zm);
                if( tile->propmask & TP_ISVISIBLE && game->visibility(p,{x,y,zm})>0)
                {

                    //printf("depth was %d\n",depth);
                    Float3 tp = PosInt2Float3({x,y,zm}); //getFloat3AtTile(x, y);
                    
                    tp = sumFloat3(tp, {0.5f, 0.5f, 0.5f});
                    
                    float q = 0.49;
                    //  printf("tl");
                    float tl = fmin(1,game->lighting(sumFloat3(tp,{-q,q,0})));
                    // printf("tr");
                    float tr = fmin(1,game->lighting(sumFloat3(tp,{q,q,0})));
                    // printf("br");
                    float br = fmin(1,game->lighting(sumFloat3(tp,{q,-q,0})));
                    // printf("bl");
                    float bl = fmin(1,game->lighting(sumFloat3(tp,{-q,-q,0})));
                    
                    if (depth== 0){
                        drawTile(i,j,tile->tex,tl,tr,br,bl);
                    }else {
                        drawTile(i,j,tile->tex_surface,tl,tr,br,bl);
                    }
                    depth = depth_max;
                } //TODO: if outside LoS, check memory, draw from memory
            }
        }
    }

    #endif
    
    // draw visible entities in main window
    drawWorld();
    
    glPopMatrix();
}

void DrawWindow::drawWorld()
{
    // find ents with the right components
    entity_t ent;
    
    int px = tiles_on_screen_x/2 - game->getLookAt().x ;
    int py = tiles_on_screen_y/2 - game->getLookAt().y ;
    
    for(ent = 0; ent<maxEntities; ent++)
    {
        if(world->mask[ent] & mask) 
        {
            // TODO: fix with regards to side-panel
            // check whether we're on the same z-level
            int zdiff = game->getLookAt().z - (int)(world->position[ent].z+.5);
            if( zdiff < 0 || zdiff > 6)
            {
             //   printf("entity %d not drawn: %d - %d = %d\n",ent,game->getLookAt().z,(int)(world->position[ent].z+.5),zdiff);
                continue;
            }
            
            // Entity's position
            PosInt pos = Float32PosInt(world->position[ent]);
            int x = pos.x;//(int)(world->position[ent].x+.5);
            int y = pos.y;//(int)(world->position[ent].y+.5);
            int z = pos.z;//(int)(world->position[ent].z+.5);
            
            // the tile on which to draw it // i = px-x;
            int i = px + x;
            int j = py + y;
            
            // check whether entity is on-screen.
            if(! (i < 0 || i >= tiles_on_screen_x || j < 0 || j >= tiles_on_screen_y))
            {
                // check whether entity is within LoS
                PosInt p = Float32PosInt(world->position[game->getPlayerEntity()]);
                if(game->visibility(p,{x,y,z})>0)
                {
                    //printf("depth was %d\n",depth);
                    Float3 tp = PosInt2Float3({x,y,z}); //getFloat3AtTile(x, y);
                    
                    tp = sumFloat3(tp, {0.5f, 0.5f, 0.5f});
                    
                    float q = 0.49;
                    //  printf("tl");
                    float tl = fmin(1,game->lighting(sumFloat3(tp,{-q,q,0})));
                    // printf("tr");
                    float tr = fmin(1,game->lighting(sumFloat3(tp,{q,q,0})));
                    // printf("br");
                    float br = fmin(1,game->lighting(sumFloat3(tp,{q,-q,0})));
                    // printf("bl");
                    float bl = fmin(1,game->lighting(sumFloat3(tp,{-q,-q,0})));

                    // draw the entity
                    int tex = world->is_visible[ent].tex;
                    drawTile(i,j,tex,tl,tr,br,bl);
                }
            }
            
            // TODO: draw it in side panel?
            
        }
    }

}

// draw side-panel. 3 parts: vertical cross section in x, in y, and "info" pane
void DrawWindow::drawSidePanel()
{
    
    
    // panel coordinates are:
    // [tiles_on_screen_x+1, tiles_on_screen_y-tilesWide*3+1] to
    // [tiles_on_screen_x+2+tilesWide, tiles_on_screen_y - 1 ]
    
    /*
    int i,j,depth, depth_max = 8;
    for(i = 0; i < tilesWide+1; i++)
    {
        // get world x coordinate
        // get the cell x offset
        for(j=0; j < tilesWide*3-1; j++)
        {
            // get world z coordinate
            // get the cell z offset
            
           // for(depth=0; depth<depth_max; depth++)
           // {
                // check if tile is viewable and player can see it
                drawTile(tiles_on_screen_x+1+i,tiles_on_screen_y-tilesWide*3+j+1,88,1,1,1,1,1,0);
           // }
        }
    }*/
    
    drawCrossX();
    drawWorldX();
    drawCrossY();
    drawWorldY();
}

void DrawWindow::drawWorldX()
{
    int tilesWide = tiles_on_screen_x/4;
    // bottom half draws along X axis
    int xOffset = tiles_on_screen_x + 1;
    int yOffset = tiles_on_screen_y - tilesWide*3 + 2;
    PosInt p = Float32PosInt(world->position[game->getPlayerEntity()]);
    
    int pz = (int)tilesWide*2/3 - game->getLookAt().z;
    int px = tilesWide/2 - game->getLookAt().x;
    int py = game->getLookAt().y;
    
    
    // find ents with the right components
    entity_t ent;
    
    for(ent = 0; ent<maxEntities; ent++)
    {
        if(world->mask[ent] & mask)
        {
            // check whether we're on the same z-level
            // TODO: add depth
            if((int)(world->position[ent].y+.5) != py)
            {
                continue;
            }
            
            // Entity's position
            int x = (int)(world->position[ent].x+.5);
            int y = (int)(world->position[ent].y+.5);
            int z = (int)(world->position[ent].z+.5);
            
            // the tile on which to draw it
            int i = px+x;
            int j = pz+z;
            
            // check whether entity is on-screen.
            if(! (i < 0 || i > tilesWide || j < 0 || j > tilesWide*1.5))
            {
                // check whether entity is within LoS
                if(game->visibility(p,{x,y,z})>0)
                {
                    //printf("depth was %d\n",depth);
                    Float3 tp = PosInt2Float3({x,y,z}); //getFloat3AtTile(x, y);
                    
                    tp = sumFloat3(tp, {0.5f, 0.5f, 0.5f});
                    
                    float q = 0.49;
                    //  printf("tl");
                    float tl = fmin(1,game->lighting(sumFloat3(tp,{-q,0,q})));
                    // printf("tr");
                    float tr = fmin(1,game->lighting(sumFloat3(tp,{q,0,q})));
                    // printf("br");
                    float br = fmin(1,game->lighting(sumFloat3(tp,{q,0,-q})));
                    // printf("bl");
                    float bl = fmin(1,game->lighting(sumFloat3(tp,{-q,0,-q})));
                    
                    // draw the entity
                    int tex = world->is_visible[ent].tex_side;
                    drawTile(i+xOffset,j+yOffset,tex,tl,tr,br,bl);
                }
            }
        }
    }
    
}

void DrawWindow::drawCrossX()
{
    int tilesWide = tiles_on_screen_x/4;
    // bottom half draws along X axis
    int xOffset = tiles_on_screen_x + 1;
    int yOffset = tiles_on_screen_y - tilesWide*3 + 1;
    PosInt p = Float32PosInt(world->position[game->getPlayerEntity()]);
    int pz = game->getLookAt().z-(int)tilesWide*2/3-1;
    //printf("%d\n",pz);
    int px = game->getLookAt().x-tilesWide/2;
    int py = game->getLookAt().y;
    int x, y, z, i, j, depth, max_depth = 1;
    y = py;
    
    for(i = 0; i < tilesWide+1; i++)
    {
        x = i + px;
        
        for(j = 0; j < tilesWide*1.5; j++)
        {
            z = j + pz;
            
            // drawTile(i+xOffset,j+yOffset,88,1,1,1,1,0, 0);
            for(depth = 0; depth < max_depth; depth++)
            {
                int ym = y - depth;
                float size = 1;
                MapTile * tile = game->getTile(x,ym,z);
                // check if tile is viewable and player can see it
                if( tile->propmask & TP_ISVISIBLE && game->visibility(p,{x,ym,z})>0)
                {
                    Float3 tp = PosInt2Float3({x,ym,z}); //getFloat3AtTile(x, y);
                    
                    tp = sumFloat3(tp, {0.5f, 0.5f, 0.5f});
                    
                    float q = 0.49;
                    //  printf("tl");
                    float tl = fmin(1,game->lighting(sumFloat3(tp,{-q,0,q})));
                    // printf("tr");
                    float tr = fmin(1,game->lighting(sumFloat3(tp,{q,0,q})));
                    // printf("br");
                    float br = fmin(1,game->lighting(sumFloat3(tp,{q,0,-q})));
                    // printf("bl");
                    float bl = fmin(1,game->lighting(sumFloat3(tp,{-q,0,-q})));
                    
                    // tl = 1;
                    // br = 1;
                    if (depth== 0){
                        drawTile(i+xOffset,j+yOffset,tile->tex,tl,tr,br,bl);
                    }else {
                        drawTile(i+xOffset,j+yOffset,tile->tex_surface,tl,tr,br,bl);
                    }
                    
                    depth = max_depth;
                } //TODO: if outside LoS, check memory, draw from memory
                
            }
        }
    }
}


void DrawWindow::drawWorldY()
{
    int tilesWide = tiles_on_screen_x/4;
    // bottom half draws along X axis
    int xOffset = tiles_on_screen_x + 1;
    int yOffset = tiles_on_screen_y - tilesWide*1.5 + 3;
    PosInt p = Float32PosInt(world->position[game->getPlayerEntity()]);
    int pz = (int)tilesWide*2/3 - game->getLookAt().z;
    int px = game->getLookAt().x;
    int py = tilesWide/2 - game->getLookAt().y;
    
    // find ents with the right components
    entity_t ent;
    
    for(ent = 0; ent<maxEntities; ent++)
    {
        if(world->mask[ent] & mask)
        {
            // check whether we're on the same z-level
            // TODO: add depth
            if((int)(world->position[ent].x+.5) != px)
            {
                continue;
            }
            
            // Entity's position
            int x = (int)(world->position[ent].x+.5);
            int y = (int)(world->position[ent].y+.5);
            int z = (int)(world->position[ent].z+.5);
            
            // the tile on which to draw it
            int i = py+y;
            int j = pz+z;
            
            // check whether entity is on-screen.
            if(! (i < 0 || i > tilesWide || j < 0 || j > tilesWide*1.5))
            {
                // check whether entity is within LoS
                if(game->visibility(p,{x,y,z})>0)
                {
                    //printf("depth was %d\n",depth);
                    Float3 tp = PosInt2Float3({x,y,z}); //getFloat3AtTile(x, y);
                    
                    tp = sumFloat3(tp, {0.5f, 0.5f, 0.5f});
                    
                    float q = 0.49;
                    //  printf("tl");
                    float tl = fmin(1,game->lighting(sumFloat3(tp,{0,-q,q})));
                    // printf("tr");
                    float tr = fmin(1,game->lighting(sumFloat3(tp,{0,q,q})));
                    // printf("br");
                    float br = fmin(1,game->lighting(sumFloat3(tp,{0,q,-q})));
                    // printf("bl");
                    float bl = fmin(1,game->lighting(sumFloat3(tp,{0,-q,-q})));
                    
                    // draw the entity
                    int tex = world->is_visible[ent].tex_side;
                    drawTile(i+xOffset,j+yOffset,tex,tl,tr,br,bl);
                }
            }
        }
    }
    
}

void DrawWindow::drawCrossY()
{
    int tilesWide = tiles_on_screen_x/4;
    // top half draws along Y axis at fixed X
    int xOffset = tiles_on_screen_x + 1;
    int yOffset = tiles_on_screen_y - tilesWide*1.5 + 2;
    MapCell * cell;
    PosInt p = Float32PosInt(world->position[game->getPlayerEntity()]);
    int cx = 0, cy = 0, cz = 0; // cell offset we're drawing from
    int pz = game->getLookAt().z-(int)tilesWide*2/3-1;
    int px = game->getLookAt().x;
    int py = game->getLookAt().y-tilesWide/2;
    
    int x, y, z, i, j, depth, max_depth = 1;
    x = px;
    for(i = 0; i < tilesWide+1; i++)
    {
        y = i + py;
        
        for(j = 0; j < tilesWide*1.5; j++)
        {
            z = j + pz;
            // drawTile(i+xOffset,j+yOffset,88,1,1,1,1,0, 0);
            for(depth = 0; depth < max_depth; depth++)
            {
                int xm = x - depth;
                
                MapTile * tile = game->getTile(xm,y,z);
                //cell = game->getCellCoords(x,y,z);
                // check if tile is viewable and player can see it
                if( tile->propmask & TP_ISVISIBLE && game->visibility(p,{xm,y,z})>0)
                {
                    Float3 tp = PosInt2Float3({xm,y,z}); //getFloat3AtTile(x, y);
                    
                    tp = sumFloat3(tp, {0.5f, 0.5f, 0.5f});
                    
                    float q = 0.49;
                    //  printf("tl");
                    float tl = fmin(1,game->lighting(sumFloat3(tp,{0,-q,q})));
                    // printf("tr");
                    float tr = fmin(1,game->lighting(sumFloat3(tp,{0,q,q})));
                    // printf("br");
                    float br = fmin(1,game->lighting(sumFloat3(tp,{0,q,-q})));
                    // printf("bl");
                    float bl = fmin(1,game->lighting(sumFloat3(tp,{0,-q,-q})));
                    
                    // tl = 1;
                    // br = 1;
                    if (depth== 0){
                        drawTile(i+xOffset,j+yOffset,tile->tex,tl,tr,br,bl);
                    }else {
                        drawTile(i+xOffset,j+yOffset,tile->tex_surface,tl,tr,br,bl);
                    }
                    
                    depth = max_depth;
                } //TODO: if outside LoS, check memory, draw from memory
                
            }
        }
    }
}


void DrawWindow::drawBottomPanel()
{
    int tilesWide = tiles_on_screen_x/4;
    int i,j;
    
    // Paint "panel"
    for(i = 0; i < tilesWide*4-1; i++)
    {
        for(j=0; j < tilesWide; j++)
        {
            drawTile(1+i,j,11,.51,.51,.51,.51);
        }
    }
    // draw log
    // TODO: let player toggle longer log with a button press
    log = game->getLog();
    int numEntries = 5;
    for(i=log.size()-1; i >= 0 && i > (int)log.size()-numEntries; i--)
    {
        int pos = log.size() - i;
        const char* s = log[i].c_str();
        print_text2(s,1,pos);
    }
    
    // draw character info
    j = 4;
    i = tilesWide*3;
    
    Character* pc = game->getCharacter();
    print_text2(pc->name.c_str(),i,j);
    char s[64];
    j = 3;
    sprintf(s,"Energy: %d/%d Recover: %d/turn",(int)pc->energy,(int)(pc->maxEnergy-pc->bruise),(int)(pc->recover-pc->bleed));
    print_text2(s,i,j);
    j = 2;
    sprintf(s,"Mood: %d/10",pc->mood);
    print_text2(s,i,j);
    if(pc->bleed > 0)
    {
        sprintf(s,"Bleeding");
        print_text2(s,i+4,j);
    }
    
    // TODO: use tiles to creatively draw the character's orientation (walk/climb/jump).
}




int DrawWindow::handle(int event) {
    switch(event) {
        case FL_MOVE:
            //
            //printf("mouse move at %d %d\n",Fl::event_x(),Fl::event_y());
            //mySim->hoverAt((float)Fl::event_x()/this->w(),1-(float)Fl::event_y()/this->h());
            return 1;
        case FL_PUSH:
            //        ... mouse down event ...
            //        ... position in Fl::event_x() and Fl::event_y()
            //  printf("mouse down at %d %d\n",Fl::event_x(),Fl::event_y());
           // mySim->wasClicked((float)Fl::event_x()/this->w(),1-(float)Fl::event_y()/this->h());
            return 1;
        case FL_DRAG:
            //         ... mouse moved while down event ...
            return 1;
        case FL_RELEASE:
            //          ... mouse up event ...
            return 1;
        case FL_FOCUS :
        case FL_UNFOCUS :
            //         ... Return 1 if you want keyboard events, 0 otherwise
            return 1;
        case FL_KEYBOARD:
            //         ... keypress, key is in Fl::event_key(), ascii in Fl::event_text()
            //         ... Return 1 if you understand/use the keyboard event, 0 otherwise...
            
            if(Fl::event_key() == 'q' || Fl::event_key(FL_Escape))
                exit(0);
            
            game->addInput(Fl::event_key());
            
            redraw();            
            
            return 1;
        case FL_KEYUP:
            
            return 1;
        case FL_SHORTCUT:
            //         ... shortcut, key is in Fl::event_key(), ascii in Fl::event_text()
            //         ... Return 1 if you understand/use the shortcut event, 0 otherwise...
            return 1;
        default:
            // pass other events to the base class...
            return Fl_Gl_Window::handle(event);
    }
}

void DrawWindow::print_text(const char* str, float x, float y)
{
    glRasterPos2f(x, y);
    print_text(str);
}
void DrawWindow::print_text2(const char* str, int x, int y)
{
    print_text(str,x*tile_width,y*tile_height);
}

void DrawWindow::print_text(const char * str)
{
    char s = str[0];
    int i = 0;
    while(s != '\0' && i < 128)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, s);
        i++;
        s = str[i];
    }
}