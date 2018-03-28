//
//  Overworld.hpp
//  HPRL
//
//  Created by Marcus Holm on 20/11/17.
//  Copyright © 2017 Marcus Holm. All rights reserved.
//

#ifndef Overworld_hpp
#define Overworld_hpp

#include <stdio.h>
#include "Common.h"

typedef enum
{
    ROCK_LIMESTONE,
    ROCK_SHALE,
    ROCK_GRANITE,
    ROCK_GNEISS,
    ROCK_ICE
} RockType;

typedef enum
{
    MCT_DEFAULT,
    MCT_ENTRANCE,
    MCT_ICESHAFT,
    MCT_GLACIER,
    MCT_GLACIERBOTTOM,
    MCT_LIMESTONE,
    MCT_SHALE,
    MCT_OLDONESCITY,
    MCT_MAGMASEA,
    MCT_CRYSTALCAVE,
    MCT_TREASURE
} MCType;

/*connections between mapcells:
 - left, right, north, south, up, or down.
 - location on interface
 - size
 - list of which connections must connect up*/
typedef struct
{
    Direction dir;
    float i, j; //(0,1)
    int size;
    //float waterLevel; //[0,1] // could be tricky for vertical connections
    //float waterSpeed;
    //Direction flowDir;
} MCConnection;

typedef struct
{
    PosInt pos;
    MCType type;
    int numConnections;
    MCConnection connection[32];
    short int connectMatrix[32][32];
    int numWaterSources;
    PosInt source[32]; //TODO: bug in the making...
}MCInfo;

typedef struct
{
    int age;
    RockType rock;
    //int fossils; // TODO: implement fossils
}TileInfo;


typedef struct CN
{
    PosInt p; // 3D coordinates of node
    int numChild;
    struct CN* parent; // where we came from
    struct CN* children[3]; // up to three children, why not?
}CaveNode;

// TODO: implement a geological model
class Overworld
{
private:
    // cave network. Is a graph. Starts with root at 0,0,0.
    CaveNode root;
    
    int OWdepth = 30;
    int OWwidth = 30;
    float length_per_cavelimb = 3; // TODO: relate number of branches per km to geology
    float verticality_bias = 0.3;  // TODO: relate direction of branches to geology
    float branches_per_node = 1.8;
    
    // cave cells. is a 3d array. Starts with root at ???
    MCInfo *** cells;
    
    void setMapCellType(PosInt p, MCType t);
    
    void initCaveNode(CaveNode* cn, PosInt p, CaveNode* parent);
    void buildCaverns(CaveNode* root, int depth);
    void buildSubglacialStream(PosInt start, PosInt end);
    
    void connectCells(MCInfo * a, MCInfo * b);
    
   // void connectCells(MCInfo * a, MCInfo * b, float flow);
public:
    Overworld();
    //MCInfo(int int int) returns a struct with general mapcell gen info
    MCInfo getMCInfo(int gx, int gy, int gz);
    
    //takes global x,y,z and produces pcg parameters for tile generation: age, fossil content, rock type
    TileInfo getTileInfo(int x, int y, int z);
};

#endif /* Overworld_hpp */
