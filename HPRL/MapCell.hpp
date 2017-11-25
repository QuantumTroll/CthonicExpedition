//
//  MapCell.hpp
//  HPRL
//
//  Created by Marcus Holm on 13/10/17.
//  Copyright © 2017 Marcus Holm. All rights reserved.
//

#ifndef MapCell_hpp
#define MapCell_hpp

#include <stdio.h>
#include "Common.h"

#include "Overworld.hpp"

typedef enum
{
    TT_ROCK1=0,
    TT_AIR=1<<0,
    TT_COLUMN=1<<1,
    TT_RAMP=1<<2,
}TileType;

typedef enum
{
    TP_NONE=0,
    TP_AIR=1<<0,
    TP_WATER=1<<1,
    TP_SOLID=1<<2,
    TP_GRIPPABLE=1<<3,
    TP_DESTRUCTIBLE=1<<4,
    TP_SLIPPERY=1<<5,
    TP_ISVISIBLE=1<<6,
    TP_ISTRANSPARENT=1<<7,
    TP_RAMP=1<<8,
}TileProp;

typedef struct
{
   // int x,y,z;
    TileProp propmask;
    float damage;
    float toughness;
    float temperature;
    int tex;
    int tex_surface;
    char * mat_name;
    char * mat_description;
}MapTile;

class MapCell
{
private:
    Overworld* overworld;
    
    MapTile* getTile(PosInt p);

    int gx, gy, gz; // location of map cell in world
    int sizexy, sizez; // dimensions of map cell
    
    int isInside(Float3 p);
    
    MapTile *** tiles;
    
    void genLocalDefault(MCInfo info);
    void initTile(int x, int y, int z);
    void generateLocalTopology();
    
public:
    MapTile* getGlobalTile(PosInt p);
    MapTile* getGlobalTile(int x, int y, int z);
    
    void setTileType(PosInt p, TileType tt);
    void setTileType(int x, int y, int z, TileType tt);

    MapCell(int sxy, int sz, int x, int y, int z, Overworld* ow);
    
    void unload(World* w);
    void load(World* w, int x, int y, int z);
};

#endif /* MapCell_hpp */
