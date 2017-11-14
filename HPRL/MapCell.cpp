//
//  MapCell.cpp
//  HPRL
//
//  Created by Marcus Holm on 13/10/17.
//  Copyright © 2017 Marcus Holm. All rights reserved.
//

#include <stdlib.h>
#include "MapCell.hpp"


int gx, gy, gz; // location of map cell in world
int sizexy, sizez; // dimensions of map cell

//TODO: make actually global
MapTile * MapCell::getGlobalTile(PosInt p)
{
    return getTile(p);
}
MapTile * MapCell::getTile(PosInt p)
{
    return &tiles[p.z][p.x][p.y];
}


//TODO: first generate an overworld with stratigraphy and some in/out info for each cell, then use that to generate cell
MapCell::MapCell(int sxy, int sz, int x, int y, int z)
{
    int i,j,k;
    sizexy = sxy;
    sizez = sz;
    gx = x;
    gy = y;
    gz = z;
    
    // if there's a save file for this map cell, open it
    int useSave = 0;
    
    tiles = (MapTile ***) malloc(sizez * sizeof(MapTile**));
    for(k=0; k<sizez; k++)
    {
        tiles[k] = (MapTile **) malloc(sizexy * sizeof(MapTile*));
        for(i=0; i<sizexy; i++)
        {
            tiles[k][i] = (MapTile *) malloc(sizexy * sizeof(MapTile));
            for(j=0; j<sizexy; j++)
            {
                // load or generate the fucking tile
                
                // if there's a save file for the map cell, load it
                if(useSave)
                {
                    // foo
                }else {
                    // initialise tiles according to stratigraphy/overworld
                    initTile(i,j,k);
                }
            }
        }
    }
    // generate caves and shit, also according to overworld
    generateLocalTopology();
}

void MapCell::setTileType(int x, int y, int z, TileType tt)
{
    MapTile * tile = &tiles[z][x][y];
    
    switch(tt)
    {
        case TT_AIR:
            tile->propmask = TP_NONE;
            tile->propmask = (TileProp)(TP_AIR | TP_ISTRANSPARENT);
            tile->temperature = 5;
            break;
        case TT_ROCK1:
            tile->propmask = TP_NONE;
            tile->propmask = (TileProp)(TP_SOLID | TP_GRIPPABLE | TP_ISVISIBLE);
            tile->temperature = 5;
            tile->tex = 0;
            tile->tex_surface = 1;
            break;
        case TT_COLUMN:
            tile->propmask = TP_NONE;
            tile->propmask = (TileProp)(TP_SOLID | TP_GRIPPABLE | TP_ISVISIBLE);
            tile->temperature = 5;
            tile->tex = 2;
            tile->tex_surface = 2;
            break;
        case TT_RAMP:
            tile->propmask = (TileProp)( tile->propmask | TP_ISTRANSPARENT | TP_RAMP);
            break;
    }
}

void MapCell::initTile(int x, int y, int z)
{
    setTileType(x,y,z,TT_ROCK1);
}

void MapCell::generateLocalTopology()
{
    // takes info from overworld and translates it into local topology
    int i, j, k;
        //placeholder
    if(gx == 0 && gy == 0 && gz == 0)
    {
        printf("air between %d %d %d, %d %d %d\n",sizexy/4, sizexy/4,sizez/4,3*sizexy/4, 3*sizexy/4,3*sizez/4);
        
        for(i=sizexy/4; i<3*sizexy/4; i++)
        {
            for(j=sizexy/4; j<3*sizexy/4; j++)
            {
                for(k=sizez/4; k<3*sizez/4; k++)
                {
                    if(i % 5 == 0 && j % 3 == 0)
                        setTileType(i,j,k,TT_COLUMN);
                    else
                        setTileType(i,j,k,TT_AIR);
                }
            }
        }
        for(i=sizexy/4; i<2*sizexy/4; i++)
        {
            for(j=2*sizexy/4; j<3*sizexy/4; j++)
            {
                for(k=sizez/4; k<i-sizexy/4+1; k++)
                {
                    setTileType(i,j,k,TT_ROCK1);
                }
               // printf("%d %d %d is not a ramp. visible=%d\n",i,j,k-1,tiles[k-1][i][j].propmask & TP_ISVISIBLE);
                //printf("propmask before: %p\n",tiles[k-1][i][j].propmask);
               // setTileType(i,j,i-sizexy/4,TT_RAMP);
                setTileType(i,j,i-sizexy/4+1,TT_AIR);
                setTileType(i-1,j,i-sizexy/4+1,TT_AIR);
                //printf("propmask after: %p\n",tiles[k-1][i][j].propmask);
              //  printf("%d %d %d is a ramp. visible=%d\n",i,j,k-1,tiles[k-1][i][j].propmask & TP_ISVISIBLE);
            }
        }
        
    }
}

int MapCell::visibility(Float3 o, Float3 t)
{
    return 1;
}