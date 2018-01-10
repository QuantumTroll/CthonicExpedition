//
//  Overworld.cpp
//  HPRL
//
//  Created by Marcus Holm on 20/11/17.
//  Copyright © 2017 Marcus Holm. All rights reserved.
//

#include "Overworld.hpp"

Overworld::Overworld()
{
    int i,j,k;
    
    cells = (MCInfo ***) malloc(OWdepth*sizeof(MCInfo**));
    for(k=0;k<OWdepth;k++)
    {
        cells[k] = (MCInfo **) malloc(OWwidth*sizeof(MCInfo*));
        for(i=0; i < OWwidth; i++)
        {
            cells[k][i] = (MCInfo *) malloc(OWwidth*sizeof(MCInfo));

            for(j=0; j < OWwidth; j++)
            {
                cells[k][i][j].type = MCT_DEFAULT;
                cells[k][i][j].numConnections = 0;
                cells[k][i][j].pos = {i,j,k};
            }
        }
    }
    
    // build cave network (development stub)
    initCaveNode(&root,{1,1,OWdepth-2}, NULL);
    buildCaverns(&root, 10);
    
    //TODO: this is where we start building the world for real
    
    // begin with subglacial streams. These are watery round tunnels that hug the interface between glacier and surface rock.
    
    // one or more subglacial streams tunnel down into the rock. These form the first "biome" that the player traverses. Rounded, twisty tunnels form pair-wise — drier upper (with pools) and wetter lower (with fish?). Occasional cavernous spaces with limestone formations. Fossils from Cenozoic to Triassic. Footprints with strange chevron shape.
    
    // narrow, angular pressure cracks in the rock lead to intense downward climb. Water is carried down by a series of falls. Find older fossils — ancient seafood. Same chevron prints.
    
    // ...
}

void Overworld::initCaveNode(CaveNode *cn, PosInt p, CaveNode *parent)
{
    cn->p = p;
    cn->parent = parent;
    cn->numChild = 0;
}

// build a cavern starting here, going to depth steps
void Overworld::buildCaverns(CaveNode *current, int depth)
{
    if(depth == 0)
    {
     //   printf("cave end point at %d %d %d\n",current->p.x,current->p.y,current->p.z);
        return;
    }
    
    depth--;
    
    if(frand(0,1) < branches_per_node - 1)
        current->numChild = 2;
    else
        current->numChild = 1;
    
    int i;
    for(i = 0; i < current->numChild; i++)
    {
        
        PosInt from;
        if(current->parent)
            from = current->parent->p;
        else
            from = {0,0,0};
        Float3 prevDir = normaliseFloat3(PosInt2Float3_rounded(diffPosInt(current->p, from)));
        
        Float3 nextDir;
        
        // if we're child 0 then 50% chance to keep going roughly in same direction
        if(i == 0 && random() & 1)
        {
            nextDir = normaliseFloat3(sumFloat3(prevDir,{frand(-.2,.2),frand(-.2,.2),frand(-.2,.2)}));
        }else{ // 50% totally another direction
            nextDir = normaliseFloat3({frand(-1,1),frand(-1,1),frand(-1,1)});
            // sumPosInt(current->p,{(int)random()%5-2,(int)random()%5-2,(int)random()%5-2});
        }
        
        // but how far?
        float distance = length_per_cavelimb + frand(-0.2*length_per_cavelimb,0.2*length_per_cavelimb);
        
        PosInt next = Float32PosInt_rounded( mulFloat3(nextDir,distance)) ;
        next = sumPosInt(current->p,next);
        
        // check to see if next is valid coordinates. If not, try again.
        if(next.x < 0 || next.x >= OWwidth || next.y < 0 || next.y >= OWwidth || next.z < 0 || next.z >= OWdepth )
        {
            --i;
            continue;
        }
        
      //  printf("tunnel from %d %d %d to %d %d %d\n",from.x,from.y,from.z,next.x,next.y,next.z);
        
        //next is now the coordinates of the next cell. Create a node of it.
        current->children[i] = (CaveNode*) malloc(sizeof(CaveNode));
        initCaveNode(current->children[i],next,current);
        
        // the path from here to there
        std::vector<PosInt> path = getStraightPath(current->p, next);
        int j;
        
        // traverse the path and set the cells' connections.
        //cells[current->p.z][current->p.x][current->p.y].numConnections++;
        MCInfo* prev = &cells[current->p.z][current->p.x][current->p.y];
        MCInfo* cur;
        for(j=0;j<path.size(); j++)
        {
            cur = &cells[path[j].z][path[j].x][path[j].y];
            connectCells(prev, cur);
            prev = cur;
        }
        
        //recurse!
        if(depth > 0)
            buildCaverns(current->children[i],depth);
        else
            cells[next.z][next.x][next.y].type = MCT_CRYSTALCAVE;
        
        //TODO: connect the Crystal chambers together. choose the deepest end chamber and make it Treasure chamber
    }
}

void Overworld::connectCells(MCInfo *a, MCInfo *b, float flow)
{
    if(a == b) // got the same pointers for some reason :P
        return;
    
    // get cells orientation
    PosInt pa = a->pos;
    PosInt pb = b->pos;
    
    
   // printf("%d %d %d leads to %d %d %d\n",pa.x,pa.y,pa.z,pb.x,pb.y,pb.z);
    
    //TODO: add logic that detects duplicate connections and offsets them
 
    if(pa.x < pb.x) // a left of b
    {
        a->connection[a->numConnections].dir = DIR_EAST;
        b->connection[b->numConnections].dir = DIR_WEST;
    }else if(pa.x > pb.x) // a right of b
    {
        a->connection[a->numConnections].dir = DIR_WEST;
        b->connection[b->numConnections].dir = DIR_EAST;
    }else if(pa.y < pb.y) // a south of b
    {
        a->connection[a->numConnections].dir = DIR_NORTH;
        b->connection[b->numConnections].dir = DIR_SOUTH;
    }else if(pa.y > pb.y) // a north of b
    {
        a->connection[a->numConnections].dir = DIR_SOUTH;
        b->connection[b->numConnections].dir = DIR_NORTH;
    }else if(pa.z < pb.z) // a down of b
    {
        a->connection[a->numConnections].dir = DIR_UP;
        b->connection[b->numConnections].dir = DIR_DOWN;
    }else if(pa.z > pb.z) // a up of b
    {
        a->connection[a->numConnections].dir = DIR_DOWN;
        b->connection[b->numConnections].dir = DIR_UP;
    }else
        printf("PROBLEM creating mapcell connection: %d %d %d = %d %d %d?\n",pa.x,pa.y,pa.z,pb.x,pb.y,pb.z);
    
    float offsetX = frand(0.1,0.9);
    float offsetY = frand(0.1,0.9);
    
    int size = irand(1,4);
    
    a->connection[a->numConnections].i = offsetX;
    a->connection[a->numConnections].j = offsetY;
    a->connection[a->numConnections].size = size;
    
    b->connection[b->numConnections].i = offsetX;
    b->connection[b->numConnections].j = offsetY;
    b->connection[b->numConnections].size = size;
    
    // waterlevel is related to flow and size and speed.
    // doesn't have to be entirely physical.
    // assume water flows from a to b
    if(flow == 0)
    {
        a->connection[a->numConnections].waterLevel = 0;
        a->connection[a->numConnections].flowDir = DIR_NONE;
        
        b->connection[b->numConnections].waterLevel = 0;
        b->connection[b->numConnections].flowDir = DIR_NONE;
    }else {
        // liters/second (flow) is proportional to waterlevel*size*size * speed.
        
        //determine speed after the fact...
        float speed = 1.0;
        float waterLevel = flow/(size*size);
        if(waterLevel > 1) // lots of water flowing through narrow pipe means fast flow
        {
            speed = waterLevel;
            waterLevel = 1;
        }else if(waterLevel < 1.0/size) // can't have water level less than 1 tile
        {
            speed = flow/(waterLevel * size); //TODO: this is probably wrong
            waterLevel = 1.0/size;
        }
    }

    
    a->numConnections ++;
    b->numConnections ++;
}

void Overworld::connectCells(MCInfo *a, MCInfo *b)
{
    connectCells(a,b,0);
}

//MCInfo(int int int) returns a struct with general mapcell gen info
MCInfo Overworld::getMCInfo(int gx, int gy, int gz)
{
    if(gx < 0 || gx >= OWwidth || gy < 0 || gy >= OWwidth || gz < 0 || gz >= OWdepth )
        return cells[0][0][0]; // hey, why not?
    
    MCInfo info = cells[gz][gx][gy];
    
    if(info.type != MCT_DEFAULT) // if not default, preserve whatever is there
    {
        return info;
    }
    
    // if default, change type according to something
    // entrance
    if(gx == 2 && gy == 2 && gz == 28)
    {
        info.type = MCT_ENTRANCE;
        return info;
    }
    if(gz == 28)
    {
        info.type = MCT_GLACIERBOTTOM;
        return info;
    }
    if(gz > 28)
    {
        info.type = MCT_GLACIER;
        return info;
    }
    
    return info;
}

//takes global x,y,z and produces pcg parameters for tile generation: age, fossil content, rock type
// TODO: think about how to structure this in a good way. don't duplicate logic about e.g. rock type in getMCInfo
TileInfo Overworld::getTileInfo(int x, int y, int z)
{
    TileInfo info;
    info.age = 100000-z; // older further down
    info.rock = ROCK_GRANITE;
    return info;
}
