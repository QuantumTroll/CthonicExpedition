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
#include <FL/glut.H>
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
    const char filename[200]="art/tileset1.png";

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
    
    mainScreen_x = tiles_on_screen_x;
    mainScreen_y = tiles_on_screen_y;
    
    // initialise the shade matrices
    int i, j,k;
    
    shade_main = (Float3 ***)malloc(mainScreen_x * sizeof(Float3 *));
    for(i=0; i<mainScreen_x; i++)
    {
        shade_main[i] = (Float3 **)malloc(mainScreen_y * sizeof(Float3*));
        for(j=0; j<mainScreen_y; j++)
        {
            shade_main[i][j] = (Float3 *)malloc(4 * sizeof(Float3));
            for(k=0; k<4; k++)
            {
                shade_main[i][j][k] = {0,0,0};
            }
        }
    }
    
    sidePanel_x = tiles_on_screen_x/4+1;
    sidePanel_y = tiles_on_screen_y;
    sidePanelX_y = tiles_on_screen_y/2;
    sidePanelY_y = tiles_on_screen_y/2;
    shade_side = (Float3 ***)malloc(sidePanel_x * sizeof(Float3 *));
    for(i=0; i < sidePanel_x; i++)
    {
        shade_side[i] = (Float3 **)malloc(sidePanel_y * sizeof(Float3*));
        for(j=0; j<sidePanel_y; j++)
        {
            shade_side[i][j] = (Float3 *)malloc(4 * sizeof(Float3));
            for(k=0; k<4; k++)
            {
                shade_side[i][j][k] = {0,0,0};
            }
        }
    }
    //printf("shade_side is of size %d %d %d\n",sidePanel_x ,sidePanel_y,4);
}


void DrawWindow::draw() {
    if (!valid()) {
        //  ... set up projection, viewport, etc ...
        //  ... window size is in w() and h().
        //  ... valid() is turned on by FLTK after draw() returns
        
        glViewport(0, 0, w(), h());
        glMatrixMode(GL_PROJECTION);
        
	    glClearColor(0,0,0,1);
        //glClearColor(1,1,1,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDepthFunc(GL_ALWAYS);
        glDisable(GL_DEPTH_TEST);
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
        
        
        // GLF Font rendering
        /* Initialise the library */
        glfInit();
        
        glfDisable(GLF_TEXTURING);
        glfDisable(GLF_CONTOURING);
        

        /* Install all the fonts */
        char f[100] = "fonts/arial1.glf"; // "fonts/penta1.glf";

        thefont = glfLoadFont(f);
        //printf("thefont %d\n", thefont);
    }

   // glDrawBuffer(GL_BACK_LEFT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  //  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    
    glPushMatrix();
    glTranslatef(-1, -1, 0);
    
    glPushMatrix();
    
    // main window offset
    glTranslatef(0,.5,0);
    
        //TODO: add e.g. "Z = +1" if showing z+1 from player.
    drawTiles();
    shadeMain();
    zeroShade();
    
    // draw visible entities in main window
    drawWorld();
    
    //TODO: this "double-shades" the ground underneath. what do?
    shadeMainEnts();
    
    
    glPopMatrix();
    
    //TODO: add e.g. "Y = +1" if showing y+1 from player, ditto for X.
    drawSidePanel();
    shadeSidePanel();
    
    zeroShade();
    
    drawBottomPanel();
    
    drawLabels();
    
    drawCurrentMenu();
    
    glPopMatrix();
   

    
    glFlush();
}


Float3 DrawWindow::getFloat3AtTile(int i, int j)
{
    Float3 p;
    
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

void DrawWindow::drawTile(int x, int y, int tex)
{
//    tl = 1; tr = 1; bl = 1; br = 1;
    int tx, ty, tx2, ty2;
   // tl *=2;    tr *=2;    bl *=2;    br *=2;
    // get the tile's texture coords
    tx = tex*tex_width % ts_width; // index*width per tile % tileset width
    ty = tex*tex_width/ts_width*tex_height;
    //printf("%d,%d,%d: %d,%d\n",x,y,tex,tx,ty);
    tx2 = tx+tex_width;
    ty2 = ty+tex_height;
    glBlendEquation(GL_ADD);
    glPushMatrix();
    glTranslatef(tile_width*x, tile_height*y, 0);
    glEnable(GL_TEXTURE_2D);
    glEnable (GL_TEXTURE_RECTANGLE_ARB);
    glEnable(GL_BLEND);
    //glEnable(GL_LIGHTING);
    glBegin(GL_QUADS);
  //  glNormal3f(cosf(tl*M_2_PI),0,sinf(tl*M_2_PI));
    glTexCoord2d(tx,ty);   glVertex3f(0,tile_height,0);
  //  glNormal3f(cosf(tr*M_2_PI),0,sinf(tr*M_2_PI));
    glTexCoord2d(tx2,ty);  glVertex3f(tile_width,tile_height,0);
  //  glNormal3f(cosf(br*M_2_PI),0,sinf(br*M_2_PI));
    glTexCoord2d(tx2,ty2); glVertex3f(tile_width,0,0);
  //  glNormal3f(cosf(bl*M_2_PI),0,sinf(bl*M_2_PI));
    glTexCoord2d(tx,ty2);  glVertex3f(0,0,0);
    glEnd();
    
    glPopMatrix();
}

void DrawWindow::shadeTile(int x, int y, Float3 tl, Float3 tr, Float3 br, Float3 bl)
{
    glEnable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glDisable (GL_TEXTURE_RECTANGLE_ARB);
    glDisable(GL_LIGHTING);
   // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendEquation(GL_MIN);

    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    glBlendEquation(GL_ADD);
    glPushMatrix();
    glTranslatef(tile_width*x, tile_height*y, 0);
    glBegin(GL_QUADS);
    float f = 1;
    float g = .5;
   // glColor4f(g,g,g,1);
    //glColor4f(.1,.5,.5,.5);
    glColor4f(tl.x,tl.y,tl.z,f);
    glVertex3f(0,tile_height,1);
    
    glColor4f(tr.x,tr.y,tr.z,f);
    glVertex3f(tile_width,tile_height,1);

    glColor4f(br.x,br.y,br.z,f);
    glVertex3f(tile_width,0,1);

    glColor4f(bl.x,bl.y,bl.z,f);
    glVertex3f(0,0,1);
    glColor4f(1,1,1,1);
    glEnd();
    glPopMatrix();
    glBlendEquation(GL_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

}

void DrawWindow::drawLabels()
{
    std::vector<entity_t> labels;
    
    glPushMatrix();
    
    // main window offset
    glTranslatef(0,.5,0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    entity_t ent;
    
    int px = tiles_on_screen_x/2 - game->getLookAt().x ;
    int py = tiles_on_screen_y/2 - game->getLookAt().y ;
    
    for(ent = 0; ent<maxEntities; ent++)
    {
        if(world->mask[ent] & COMP_LABEL)
        {
            // Entity's position
            PosInt pos = Float32PosInt(world->position[ent]);
            int x = pos.x;
            int y = pos.y;
            
            // the tile on which to draw it // i = px-x;
            int i = px + x;
            int j = py + y;
            
            // draw label with offset if other labels already put in same spot
            int offset = 0;
            for(auto e = labels.begin(); e!= labels.end(); e++)
            {
                if( distFloat3({world->position[ent].x,world->position[ent].y,0},{world->position[*e].x,world->position[*e].y,0}) < .9 )
                    offset ++;
            }
            
            // check whether entity is on-screen.
            if(! (i < 0 || i >= tiles_on_screen_x || j < 0 || j >= tiles_on_screen_y))
            {
                // draw text
                Float3 color = world->label[ent].color;
                //color = mulFloat3(color,world->counter[ent].count/world->counter[ent].max);
                glColor4f(color.x,color.y,color.z,world->counter[ent].count/world->counter[ent].max);
                print_text2(world->label[ent].text,i,j+offset);
                labels.push_back(ent);
            }
        }
    }
    glPopMatrix();
}


// draw landscape "tiles" in main window
void DrawWindow::drawTiles()
{
    glEnable( GL_BLEND) ;
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Enable the texture for OpenGL.
    glEnable(GL_TEXTURE_2D);
    glEnable (GL_TEXTURE_RECTANGLE_ARB);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //GL_NEAREST = no smoothing
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 4, ts_width, ts_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &tileset[0]);

    
    
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
           // printf("%d %d %d\n",x,y,zm);
                // check if tile is viewable
                MapTile * tile = game->getTile(x,y,zm);

                if( tile->propmask & TP_ISVISIBLE)
                {
                    //  and player can see it
                    if(game->visibility(p,{x,y,zm})>0)
                    {
                        Float3 tp = PosInt2Float3({x,y,zm});
                        
                        tp = sumFloat3(tp, {0.5f, 0.5f, 0.5f});
                        
                        // if lookat.z > p.z and zm > p.z, then use the light coming from below. Trust me.
                        if(z > p.z && zm > p.z)
                            tp = sumFloat3(tp,{0,0,-0.5f});
                        
                        float q = 0.49;
                        
                        shade_main[i][j][0] = game->lighting3f(sumFloat3(tp,{-q,q,0}));
                        shade_main[i][j][1] = game->lighting3f(sumFloat3(tp,{q,q,0}));
                        shade_main[i][j][2] = game->lighting3f(sumFloat3(tp,{q,-q,0}));
                        shade_main[i][j][3] = game->lighting3f(sumFloat3(tp,{-q,-q,0}));
                        
                       // tl = 1; tr = 1; br = 1; bl = 1;
                        if (depth== 0){
                            drawTile(i,j,tile->tex); //,tl,tr,br,bl);
                        }else {
                            drawTile(i,j,tile->tex_surface); //,tl,tr,br,bl);
                        }
                        depth = depth_max;
                        float light; // = (tl+tr+br+bl)*0.25;
                        light = (normFloat3(shade_main[i][j][0])+normFloat3(shade_main[i][j][1])+normFloat3(shade_main[i][j][2])+normFloat3(shade_main[i][j][3]))*0.25;
                        game->setWasSeen(tile,light);
                    }else if(game->wasSeen(tile) > 0){ //if outside LoS, check memory, draw from memory
                        float l = game->wasSeen(tile);
                        if (depth== 0){
                            drawTile(i,j,tile->tex);
                            drawTile(i,j,memorytex);
                        }else if(depth == 1){
                            drawTile(i,j,tile->tex_surface);
                            drawTile(i,j,memorytex);
                        }
                        shade_main[i][j][0] = {l,l,l};
                        shade_main[i][j][1] = {l,l,l};
                        shade_main[i][j][2] = {l,l,l};
                        shade_main[i][j][3] = {l,l,l};
                    }
                }
                
            }
        }
    }

    #endif
    
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
            // check whether we're on the same z-level
            int zdiff = game->getLookAt().z - (int)(world->position[ent].z+.5);
            if( zdiff < 0 || zdiff > 6)
            {
                continue;
            }
            
            // Entity's position
            PosInt pos = Float32PosInt(world->position[ent]);
            int x = pos.x;
            int y = pos.y;
            int z = pos.z;
            
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
                    // check whether camera's view is blocked by something.
                    int k;
                    int isBlocked = 0;
                    for(k=z; k <= game->getLookAt().z; k++)
                    {
                        if(! (game->getTile(x,y,k)->propmask & TP_ISTRANSPARENT))
                        {
                            isBlocked = 1;
                            break;
                        }
                    }
                    if(isBlocked)
                        continue;
                    
                    //printf("depth was %d\n",depth);
                    Float3 tp = PosInt2Float3({x,y,z}); 
                    
                    tp = sumFloat3(tp, {0.5f, 0.5f, 0.5f});
                    
                    float q = 0.49;
                    shade_main[i][j][0] = maxFloat3(shade_main[i][j][0],game->lighting3f(sumFloat3(tp,{-q,q,0})));
                    shade_main[i][j][1] = maxFloat3(shade_main[i][j][1],game->lighting3f(sumFloat3(tp,{q,q,0})));
                    shade_main[i][j][2] = maxFloat3(shade_main[i][j][2],game->lighting3f(sumFloat3(tp,{q,-q,0})));
                    shade_main[i][j][3] = maxFloat3(shade_main[i][j][3],game->lighting3f(sumFloat3(tp,{-q,-q,0})));
                    
                    entShades.push_back({i,j,0});
                    
                    // draw the entity
                    int tex = world->is_visible[ent].tex;
                    drawTile(i,j,tex);//,tl,tr,br,bl);
                    //drawTileShade(i,j,tex,stl,str,sbr,sbl);
                    //TODO: don't draw lower-z ents over higher-z ents
                    
                }
            }
        }
    }
}

void DrawWindow::shadeMain()
{
    
    int i,j;
    
    for(i=0; i<tiles_on_screen_x; i++)
    {
        for(j=0; j<tiles_on_screen_y; j++)
        {
         //   printf("%d %d: %f %f %f %f\n",i,j,shade_main[i][j][0].x,shade_main[i][j][1].x,shade_main[i][j][2].x,shade_main[i][j][3].x);
            shadeTile(i,j,shade_main[i][j][0],shade_main[i][j][1],shade_main[i][j][2],shade_main[i][j][3]);
            //int t = (int)(shade_main[i][j][0].x*100);
            //drawTile(i,j,t);
        }
    }
}

void DrawWindow::shadeMainEnts()
{
    int i, j;
    for(auto p=entShades.begin(); p!=entShades.end(); p++)
    {
        i=p->x;
        j=p->y;
        shadeTile(i,j,shade_main[i][j][0],shade_main[i][j][1],shade_main[i][j][2],shade_main[i][j][3]);
    }
    entShades.clear();
}

void DrawWindow::zeroShade()
{
    int i, j,k;
    
    for(i=0; i<mainScreen_x; i++)
    {
        for(j=0; j<mainScreen_y; j++)
        {
            for(k=0; k<4; k++)
            {
                shade_main[i][j][k] = {0,0,0};
            }
        }
    }
    for(i=0; i < sidePanel_x; i++)
    {
        for(j=0; j<sidePanel_y; j++)
        {
            for(k=0; k<4; k++)
            {
                shade_side[i][j][k] = {0,0,0};
            }
        }
    }
}

// draw side-panel. 3 parts: vertical cross section in x, in y, and "info" pane
void DrawWindow::drawSidePanel()
{
    
    // panel coordinates are:
    // [tiles_on_screen_x+1, tiles_on_screen_y-tilesWide*3+1] to
    // [tiles_on_screen_x+2+tilesWide, tiles_on_screen_y - 1 ]
  
    drawCrossX();
    drawWorldX();
    drawCrossY();
    drawWorldY();
}

// shade side-panel.
void DrawWindow::shadeSidePanel()
{
    glPushMatrix();
    
    int i,j;
   // int tilesWide = tiles_on_screen_x/4;
    int xOffset = tiles_on_screen_x + 1;
    int yOffset = 2; //tiles_on_screen_y - tilesWide*3 + 1;
    for(i=0; i<sidePanel_x; i++)
    {
        for(j=0; j<sidePanelX_y; j++)
        {
           // printf("%d %d: %f %f %f %f\n",i,j,shade_main[i][j][0].x,shade_main[i][j][1].x,shade_main[i][j][2].x,shade_main[i][j][3].x);
            shadeTile(i+xOffset,j+yOffset,shade_side[i][j][0],shade_side[i][j][1],shade_side[i][j][2],shade_side[i][j][3]);
            //int t = (int)(shade_main[i][j][0].x*100);
            //drawTile(i,j,t);
        }
    }
    yOffset = sidePanelX_y+3;
    for(i=0; i<sidePanel_x; i++)
    {
        for(j=0; j<sidePanelY_y; j++)
        {
            // printf("%d %d: %f %f %f %f\n",i,j,shade_main[i][j][0].x,shade_main[i][j][1].x,shade_main[i][j][2].x,shade_main[i][j][3].x);
            shadeTile(i+xOffset,j+yOffset,shade_side[i][j+sidePanelX_y][0],shade_side[i][j+sidePanelX_y][1],shade_side[i][j+sidePanelX_y][2],shade_side[i][j+sidePanelX_y][3]);
            //int t = (int)(shade_main[i][j][0].x*100);
            //drawTile(i,j,t);
        }
    }
    glPopMatrix();
    

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
                    shade_side[i][j][0] = maxFloat3(shade_side[i][j][0], game->lighting3f(sumFloat3(tp,{-q,0,q})));
                    shade_side[i][j][1] = maxFloat3(shade_side[i][j][1],game->lighting3f(sumFloat3(tp,{q,0,q})));
                    shade_side[i][j][2] = maxFloat3(shade_side[i][j][2],game->lighting3f(sumFloat3(tp,{q,0,-q})));
                    shade_side[i][j][3] = maxFloat3(shade_side[i][j][3],game->lighting3f(sumFloat3(tp,{-q,0,-q})));
                    
                    // draw the entity
                    int tex = world->is_visible[ent].tex_side;
                    drawTile(i+xOffset,j+yOffset,tex);//,tl,tr,br,bl);
                }
            }
        }
    }
    
}

void DrawWindow::drawCrossX()
{
    //int tilesWide = tiles_on_screen_x/4;
    // bottom half draws along X axis
    int xOffset = tiles_on_screen_x+1;
    int yOffset = 2;
    
    PosInt p = Float32PosInt(world->position[game->getPlayerEntity()]);
    int pz = game->getLookAt().z-sidePanelX_y/2;
    //printf("%d\n",pz);
    int px = game->getLookAt().x-sidePanel_x/2;
    int py = game->getLookAt().y;
    int x, y, z, i, j, depth, max_depth = 1;
    y = py;
    
    for(i = 0; i < sidePanel_x; i++)
    {
        x = i + px;
        
        for(j = 0; j < sidePanelX_y; j++)
        {
            z = j + pz;
            
            for(depth = 0; depth < max_depth; depth++)
            {
                int ym = y - depth;
                
                MapTile * tile = game->getTile(x,ym,z);
                
                // check if tile is viewable and player can see it
                
             //   printf("%d %d, %d %d\n",i+xOffset,j+yOffset,i+xOffset,j+tiles_on_screen_y - sidePanelX_y);
                //drawTile(i+xOffset,j+yOffset,88);
                if( tile->propmask & TP_ISVISIBLE)
                {
                    //  and player can see it
                    if(game->visibility(p,{x,ym,z})>0)
                    {
                        Float3 tp = PosInt2Float3({x,ym,z}); //getFloat3AtTile(x, y);
                        
                        tp = sumFloat3(tp, {0.5f, 0.5f, 0.5f});
                        
                        float q = 0.49;
                        
                        shade_side[i][j][0] = game->lighting3f(sumFloat3(tp,{-q,0,q}));
                        shade_side[i][j][1] = game->lighting3f(sumFloat3(tp,{q,0,q}));
                        shade_side[i][j][2] = game->lighting3f(sumFloat3(tp,{q,0,-q}));
                        shade_side[i][j][3] = game->lighting3f(sumFloat3(tp,{-q,0,-q}));

                        
                        // tl = 1;
                        // br = 1;
                        if (depth== 0){
                            drawTile(i+xOffset,j+yOffset,tile->tex);//,tl,tr,br,bl);
                        }else {
                            drawTile(i+xOffset,j+yOffset,tile->tex_surface);//,tl,tr,br,bl);
                        }
                        float light = (normFloat3(shade_side[i][j][0])+normFloat3(shade_side[i][j][1])+normFloat3(shade_side[i][j][2])+normFloat3(shade_side[i][j][3]))*0.25;
                        game->setWasSeen(tile,light);
                        depth = max_depth;
                    } else if(game->wasSeen(tile) > 0){ //if outside LoS, check memory, draw from memory
                        float l = game->wasSeen(tile);
                        if (depth== 0){
                            drawTile(i+xOffset,j+yOffset,tile->tex);//,l,l,l,l);
                            drawTile(i+xOffset,j+yOffset,memorytex);//,l,l,l,l);
                        }else if(depth == 1){
                            drawTile(i+xOffset,j+yOffset,tile->tex_surface);//,l,l,l,l);
                            drawTile(i+xOffset,j+yOffset,memorytex);//,l,l,l,l);
                        }
                        shade_side[i][j][0] = {l,l,l};
                        shade_side[i][j][1] = {l,l,l};
                        shade_side[i][j][2] = {l,l,l};
                        shade_side[i][j][3] = {l,l,l};
                    }
                }
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
    int shadeOffset = sidePanelX_y;
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
                   /* float tl = fmin(1,game->lighting(sumFloat3(tp,{0,-q,q})));
                    // printf("tr");
                    float tr = fmin(1,game->lighting(sumFloat3(tp,{0,q,q})));
                    // printf("br");
                    float br = fmin(1,game->lighting(sumFloat3(tp,{0,q,-q})));
                    // printf("bl");
                    float bl = fmin(1,game->lighting(sumFloat3(tp,{0,-q,-q})));
                    */
                    
                    shade_side[i][j+shadeOffset][0] = maxFloat3(shade_side[i][j+shadeOffset][0], game->lighting3f(sumFloat3(tp,{0,-q,q})));
                    shade_side[i][j+shadeOffset][1] = maxFloat3(shade_side[i][j+shadeOffset][1],game->lighting3f(sumFloat3(tp,{0,q,q})));
                    shade_side[i][j+shadeOffset][2] = maxFloat3(shade_side[i][j+shadeOffset][2],game->lighting3f(sumFloat3(tp,{0,q,-q})));
                    shade_side[i][j+shadeOffset][3] = maxFloat3(shade_side[i][j+shadeOffset][3],game->lighting3f(sumFloat3(tp,{0,-q,-q})));
                    // draw the entity
                    int tex = world->is_visible[ent].tex_side;
                    drawTile(i+xOffset,j+yOffset,tex);//,tl,tr,br,bl);
                }
            }
        }
    }
    
}

void DrawWindow::drawCrossY()
{
   // int tilesWide = tiles_on_screen_x/4;
    // top half draws along Y axis at fixed X
    //int xOffset = tiles_on_screen_x + 1;
    //int yOffset = tiles_on_screen_y - tilesWide*1.5 + 2;
    int xOffset = tiles_on_screen_x+1;
    int yOffset = sidePanelY_y+3;
    
    
    PosInt p = Float32PosInt(world->position[game->getPlayerEntity()]);
    
//    int pz = game->getLookAt().z-(int)tilesWide*2/3-1;
    int pz = game->getLookAt().z-sidePanelY_y/2;
    int px = game->getLookAt().x;
    int py = game->getLookAt().y-sidePanel_x/2;
    
    int x, y, z, i, j, depth, max_depth = 1;
    x = px;
    for(i = 0; i < sidePanel_x; i++)
    {
        y = i + py;
        
        for(j = 0; j < sidePanelY_y; j++)
        {
            z = j + pz;
            int shadeOffset = sidePanelX_y;
            // drawTile(i+xOffset,j+yOffset,88,1,1,1,1,0, 0);
            for(depth = 0; depth < max_depth; depth++)
            {
                int xm = x - depth;
                
                MapTile * tile = game->getTile(xm,y,z);
                //cell = game->getCellCoords(x,y,z);
                if( tile->propmask & TP_ISVISIBLE)
                {
                    //  and player can see it
                    if(game->visibility(p,{xm,y,z})>0)
                    {
                    Float3 tp = PosInt2Float3({xm,y,z}); //getFloat3AtTile(x, y);
                    
                    tp = sumFloat3(tp, {0.5f, 0.5f, 0.5f});
                    
                    float q = 0.49;
                    //  printf("tl");
                    /*float tl = fmin(1,game->lighting(sumFloat3(tp,{0,-q,q})));
                    // printf("tr");
                    float tr = fmin(1,game->lighting(sumFloat3(tp,{0,q,q})));
                    // printf("br");
                    float br = fmin(1,game->lighting(sumFloat3(tp,{0,q,-q})));
                    // printf("bl");
                    float bl = fmin(1,game->lighting(sumFloat3(tp,{0,-q,-q})));*/
                        
                        shade_side[i][j+shadeOffset][0] = game->lighting3f(sumFloat3(tp,{0,-q,q}));
                        shade_side[i][j+shadeOffset][1] = game->lighting3f(sumFloat3(tp,{0,q,q}));
                        shade_side[i][j+shadeOffset][2] = game->lighting3f(sumFloat3(tp,{0,q,-q}));
                        shade_side[i][j+shadeOffset][3] = game->lighting3f(sumFloat3(tp,{0,-q,-q}));
                    
                    // tl = 1;
                    // br = 1;
                    if (depth== 0){
                        drawTile(i+xOffset,j+yOffset,tile->tex);//,tl,tr,br,bl);
                    }else {
                        drawTile(i+xOffset,j+yOffset,tile->tex_surface);//,tl,tr,br,bl);
                    }
                    float light = (normFloat3(shade_side[i][j+shadeOffset][0])+normFloat3(shade_side[i][j+shadeOffset][1])+normFloat3(shade_side[i][j+shadeOffset][2])+normFloat3(shade_side[i][j+shadeOffset][3]))*0.25;
                    game->setWasSeen(tile,light);
                    depth = max_depth;
                    } else if(game->wasSeen(tile) > 0){ //if outside LoS, check memory, draw from memory
                        float l = game->wasSeen(tile);
                        if (depth== 0){
                            drawTile(i+xOffset,j+yOffset,tile->tex);//,l,l,l,l);
                            drawTile(i+xOffset,j+yOffset,memorytex);//,l,l,l,l);
                        }else if(depth == 1){
                            drawTile(i+xOffset,j+yOffset,tile->tex_surface);//,l,l,l,l);
                            drawTile(i+xOffset,j+yOffset,memorytex);//,l,l,l,l);
                        }
                        // TODO: shade from memory pls.
                        shade_side[i][j+shadeOffset][0] = {l,l,l};
                        shade_side[i][j+shadeOffset][1] = {l,l,l};
                        shade_side[i][j+shadeOffset][2] = {l,l,l};
                        shade_side[i][j+shadeOffset][3] = {l,l,l};
                    }
                }
            }
        }
    }
}


void DrawWindow::drawBottomPanel()
{
    int tilesWide = tiles_on_screen_x/4;
    int i,j;
    
    // Paint "panel"
  /*  for(i = 0; i < tilesWide*4-1; i++)
    {
        for(j=0; j < tilesWide; j++)
        {
            drawTile(1+i,j,11);//,.51,.51,.51,.51);
        }
    }*/
    // draw log
    // TODO: let player toggle longer log with a button press
    log = game->getLog();
    int numEntries = 5;
    glColor3f(.575,.79,.68);
    for(i=log.size()-1; i >= 0 && i > (int)log.size()-numEntries; i--)
    {
        int pos = log.size() - i;
        const char* s = log[i].c_str();
        print_text2(s,1,pos);
    }
    
    // draw character info
    j = 4;
    i = tilesWide*3+2;
    
    Character* pc = game->getCharacter();
    print_text2(pc->name.c_str(),i,j);
    char s[64];
    j = 3;
    sprintf(s,"Energy: %d/%d",(int)pc->energy,(int)(pc->maxEnergy-pc->bruise));
    print_text2(s,i,j);
    j = 2;
    sprintf(s,"Recover: %d/turn",(int)(pc->recover-pc->bleed));
    print_text2(s,i,j);
    
    j = 1;
    sprintf(s,"Mood: %s",game->getMoodDescription(pc->mood));
    print_text2(s,i,j);
    
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

void DrawWindow::drawMenuBox(int x1, int y1, int x2, int y2)
{
    int i, j;
    for(i=x1; i<x2; i++)
    {
        for(j=y1; j<y2; j++)
        {
            drawTile(i,j,11);//,.45,.45,.45,.45);
        }
    }
}
void DrawWindow::drawMenu(int x1, int y1, int x2, int y2, int numOptions, MenuOption* options)
{
    drawMenuBox(x1,y1,x2,y2);
    int i;
    int x = x1 + 1;
    int y= y2;
    print_text2(game->getCurrentMenu()->name, x,y);
    y--;
    char s [50];
    for(i=0; i<numOptions; i++)
    {
        sprintf(s,"%c. %s",options[i].key,options[i].string);
        print_text2(s, x,y);
        y--;
    }
}
void DrawWindow::drawCurrentMenu()
{
    glColor3f(.575,.79,.68);
    Menu* menu = game->getCurrentMenu();
    if(menu)
    {
        int menuX = 2*tiles_on_screen_x/3;
        int menuY = 3*tiles_on_screen_y/4;
        drawMenu(menuX,menuY,menuX+menu->x,menuY+menu->y,menu->numOptions,menu->options);
    }
}


//#define GLUTBMP

void DrawWindow::print_text(const char* str, float x, float y)
{
    glPushMatrix();
    glDisable(GL_TEXTURE_2D);
    glDisable (GL_TEXTURE_RECTANGLE_ARB);
    glDisable(GL_LIGHTING);
   // glDisable(GL_BLEND);
    //glColor3f(.575,.79,.68);
#ifdef GLUTBMP
    glRasterPos3f(x, y,1);
    print_text(str);
#else
    glTranslatef(x,y,0);
    glScalef(.018,.018,1);
    // this shit works!
    glfDrawSolidString((char*)str);

#endif
   // print_text(str);
    glEnable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glEnable (GL_TEXTURE_RECTANGLE_ARB);
    glPopMatrix();
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
