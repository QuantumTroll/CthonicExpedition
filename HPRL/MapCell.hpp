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
    TT_SCREE=1<<2,
    TT_ICE=1<<3,
    TT_CRYSTAL=1<<4,
    TT_WATER=1<<5
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
    TP_FLOW=1<<9
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
    char mat_name[32];
    char mat_description[32];
    float wasSeen;
    float flowSpeed;
    Direction flowDir;
}MapTile;

class MapCell
{
private:
    Overworld* overworld;
    World* world;
    
    MapTile* getTile(PosInt p);

    int gx, gy, gz; // location of map cell in world
    int sizexy, sizez; // dimensions of map cell
    
    int isInside(Float3 p);
    int isValidTile(PosInt p);
    
    MapTile *** tiles;
    
    std::vector<PosInt> waterSources; // TODO: probably want a vector of structs to communicate water temperature and similar
    
    PosInt getLocalPos(PosInt gPos);
    PosInt getGlobalPos(PosInt lPos);
    
    void genLocalCrystalCave(MCInfo info);
    void genLocalTreasure(MCInfo info);
    void genLocalGlacierBottom(MCInfo info);
    void genLocalGlacier(MCInfo info);
    void genStandardConnections(MCInfo info);
    void genStandardConnections(MCInfo info, PosInt center);
    void genSubglacialStreamConnections(MCInfo info);
    void genLocalEntrance(MCInfo info);
    void genLocalDefault(MCInfo info);
    void genTunnel_square(PosInt start, PosInt end, int diam);
    void genTunnel_round(PosInt start, PosInt end, int diam);
   // void genTunnel_round(PosInt start, PosInt end, int diam, Direction flowDir, float waterSpeed, float waterLevel);
    //void genTunnel_round(PosInt start, PosInt end, int diam, Direction flowDir, float waterSpeed, int waterLevel, Direction waterDir);
    void genTunnel_round_straight(PosInt start, PosInt end, int diam);
    void genVoid_sphere(PosInt center, int radius);
    void genVoid_sphere(PosInt center, int radius, int lumps);
    void initTile(int x, int y, int z);
    void generateLocalTopology();
    
    PosInt getFloorAbove(PosInt p);
    PosInt getCeilingBelow(PosInt p);
    MapTile* getFloorTileAbove(PosInt p);
    MapTile* getCeilingTileBelow(PosInt p);
    
    void setFlow(MapTile* t, float speed, Direction dir);
    void setFlow(PosInt lPos, float speed, Direction dir);
    void setGlobalFlow(PosInt gPos, float speed, Direction dir);
    
public:
    MapTile* getGlobalTile(PosInt p);
    MapTile* getGlobalTile(int x, int y, int z);
    
    PosInt getGlobalFloorAbove(PosInt p);
    PosInt getGlobalCeilingBelow(PosInt p);
    MapTile* getGlobalFloorTileAbove(PosInt p);
    MapTile* getGlobalCeilingTileBelow(PosInt p);
    
    void propagateWater();
    
    // these are found in Game
   // float wasSeen(MapTile* tile);
   // void setWasSeen(MapTile* tile, float light);
    
    entity_t makeLight(Float3 pos, float brightness, Float3 color);
    entity_t makeLight(Float3 pos, float brightness, Float3 color, entity_t ent);
    entity_t makeSlime(Float3 pos, SlimeType type);
    
    void setTileType(PosInt p, TileType tt);
    void setTileType(int x, int y, int z, TileType tt);

    MapCell(int sxy, int sz, int x, int y, int z, Overworld* ow);
    
    void unload(World* w);
    void load(World* w, int x, int y, int z);
};

#endif /* MapCell_hpp */
