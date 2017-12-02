//
//  MapCell.cpp
//  HPRL
//
//  Created by Marcus Holm on 13/10/17.
//  Copyright © 2017 Marcus Holm. All rights reserved.
//

#include <stdlib.h>
#include "MapCell.hpp"


PosInt MapCell::getLocalPos(PosInt p)
{
    PosInt localP;
    localP.x = p.x - gx*sizexy;
    localP.y = p.y - gy*sizexy;
    localP.z = p.z - gz*sizez;
    if(localP.x < 0 || localP.x >= sizexy  || localP.y < 0 || localP.y >= sizexy || localP.z < 0 || localP.z >= sizez  )
        printf("ARG %p\n",this);
    return localP;
}

PosInt MapCell::getGlobalPos(PosInt p)
{
    return sumPosInt(p, {gx*sizexy,gy*sizexy,gz*sizez});
}

MapTile * MapCell::getGlobalTile(int x, int y, int z)
{
    return getGlobalTile({x,y,z});
}
MapTile * MapCell::getGlobalTile(PosInt p)
{
    PosInt localP = getLocalPos(p);
    return getTile(localP);
}
MapTile * MapCell::getTile(PosInt p)
{
    return &tiles[p.z][p.x][p.y];
}
MapTile* MapCell::getFloorTileAbove(PosInt p)
{
    PosInt floor = getFloorAbove(p);
    if(floor.x > 0)
        return getTile(floor);
    return NULL;
}
MapTile* MapCell::getCeilingTileBelow(PosInt p)
{
    PosInt ceiling = getCeilingBelow(p);
    if(ceiling.x > 0)
        return getTile(ceiling);
    return NULL;
}
PosInt MapCell::getFloorAbove(PosInt p)
{
    int i = p.z;
    while(i < sizez)
    {
        MapTile * t = getTile({p.x,p.y,i});
        if(t->propmask & TP_AIR)
            return {p.x,p.y,i};
        i++;
    }
    return {-1,-1,-1};
}
PosInt MapCell::getCeilingBelow(PosInt p)
{
    int i = p.z;
    while(i >= 0)
    {
        MapTile * t = getTile({p.x,p.y,i});
        if(t->propmask & TP_AIR)
            return {p.x,p.y,i};
        i--;
    }
    return {-1,-1,-1};
}
PosInt MapCell::getGlobalFloorAbove(PosInt p){return getFloorAbove(getLocalPos(p));}
PosInt MapCell::getGlobalCeilingBelow(PosInt p){return getCeilingBelow(getLocalPos(p));}
MapTile* MapCell::getGlobalFloorTileAbove(PosInt p){return getFloorTileAbove(getLocalPos(p));}
MapTile* MapCell::getGlobalCeilingTileBelow(PosInt p){return getCeilingTileBelow(getLocalPos(p));}



int MapCell::isInside(Float3 p)
{
    if(! (gx*sizexy <= p.x  && p.x < (gx+1)*sizexy))
        return false;
    if(!(gy*sizexy <= p.y && p.y < (gy+1)*sizexy))
        return false;
    if(!(gz*sizez <= p.z && p.z < (gz+1)*sizez))
        return false;
    return true;
}

void MapCell::unload(World* w)
{
    int i, j, k;
    
    // open file for writing.
    char filename[128];
    
    sprintf(filename,"mc<%d,%d,%d>.dat",gx,gy,gz);
    
    FILE* f = fopen(filename,"w");
    
    if(f == NULL)
    {
        printf("Error opening file %s for saving mapcell\n",filename);
        return;
    }
    
    //fwrite(&h,sizeof(struct hex),1,f);

    // go through tiles. save them all.
    for(i=0; i<sizez; i++)
    {
        for(j=0; j<sizexy; j++)
        {
            for(k=0; k<sizexy; k++)
            {
                fwrite(&tiles[i][j][k],sizeof(MapTile),1,f);
            }
        }
    }
    
    // then go through entities. save the ones in this mapcell. Then destroy them.
    for(i=0; i<maxEntities; i++)
    {
        if(w->mask[i] & COMP_POSITION)
        {
            if(isInside(w->position[i]))
            {
                // save this entity to file f
                w->saveEntity(i, f);
                w->destroyEntity(i);
            }
        }
    }
    
    
    fclose(f);
}


void MapCell::load(World* w, int x, int y, int z)
{
    world = w;
    
    int i,j,k;
    gx = x; gy = y; gz = z;
    
    // if there's a file, load it.
    char filename[128];
    
    sprintf(filename,"mc<%d,%d,%d>.dat",gx,gy,gz);
    
    FILE * f = fopen(filename,"r");
    //if(f == NULL)       // if a .dat isn't found for the current position, make a new mapcell
    if(1)
    {
        generateLocalTopology();
        return;
    }
    
    // load from the file
    for(i=0; i<sizez; i++)
    {
        for(j=0; j<sizexy; j++)
        {
            for(k=0; k<sizexy; k++)
            {
                fread(&tiles[i][j][k],sizeof(MapTile),1,f);
            }
        }
    }
    
}

//TODO: first generate an overworld with stratigraphy and some in/out info for each cell, then use that to generate cell
MapCell::MapCell(int sxy, int sz, int x, int y, int z, Overworld* ow)
{
    overworld = ow;
    int i,j,k;
    sizexy = sxy;
    sizez = sz;
    gx = x;
    gy = y;
    gz = z;
    
   // printf("%p cell %d %d %d goes from %d %d %d to %d %d %d\n",this,gx,gy,gz,gx*sxy,gy*sxy,gz*sz,(gx+1)*sizexy,(gy+1)*sizexy,(gz+1)*sizez);
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
                // load or generate the tile
                
                // if there's a save file for the map cell, load it
                if(useSave)
                {
                    // foo
                    fprintf(stderr,"why are you trying to use a save when that's not implemented?");
                    exit(1);
                }else {
                    // initialise tiles according to stratigraphy/overworld
                    initTile(i,j,k);
                }
            }
        }
    }
   
}

void MapCell::setTileType(PosInt p, TileType tt)
{
    setTileType(p.x,p.y,p.z,tt);
}

void MapCell::setTileType(int x, int y, int z, TileType tt)
{
    if(x < 0 || x >= sizexy || y < 0 || y >= sizexy || z < 0 || z >= sizez )
        return;
    
    MapTile * tile = &tiles[z][x][y];
    
    switch(tt)
    {
        case TT_AIR:
            tile->propmask = TP_NONE;
            tile->propmask = (TileProp)(TP_AIR | TP_ISTRANSPARENT);
            tile->temperature = 5;
            sprintf(tile->mat_name,"air");
            sprintf(tile->mat_description,"Oppressive");
            break;
        case TT_ROCK1:
            tile->propmask = TP_NONE;
            tile->propmask = (TileProp)(TP_SOLID | TP_GRIPPABLE | TP_ISVISIBLE);
            tile->temperature = 5;
            tile->tex = 0;
            tile->tex_surface = 1;
            sprintf(tile->mat_name,"stone");
            sprintf(tile->mat_description,"Cold");
            break;
        case TT_COLUMN:
            tile->propmask = TP_NONE;
            tile->propmask = (TileProp)(TP_SOLID | TP_GRIPPABLE | TP_ISVISIBLE);
            tile->temperature = 5;
            tile->tex = 2;
            tile->tex_surface = 2;
            sprintf(tile->mat_name,"rock");
            sprintf(tile->mat_description,"Pale");
            break;
        case TT_ICE:
            tile->propmask = TP_NONE;
            tile->propmask = (TileProp)(TP_SOLID | TP_SLIPPERY | TP_ISVISIBLE | TP_ISTRANSPARENT);
            tile->temperature = -5;
            tile->tex = 12;
            tile->tex_surface = 13;
            sprintf(tile->mat_name,"ice");
            sprintf(tile->mat_description,"Slick");
            break;
        case TT_CRYSTAL:
            tile->propmask = TP_NONE;
            tile->propmask = (TileProp)(TP_SOLID | TP_ISVISIBLE | TP_ISTRANSPARENT);
            tile->temperature = 5;
            tile->tex = 14;
            tile->tex_surface = 15;
            sprintf(tile->mat_name,"crystal");
            sprintf(tile->mat_description,"Smooth");
            break;
        case TT_SCREE:
            tile->propmask = TP_NONE;
            tile->propmask = (TileProp)(TP_SOLID | TP_GRIPPABLE | TP_ISVISIBLE | TP_SLIPPERY);
            tile->temperature = 5;
            tile->tex = 16;
            tile->tex_surface = 17;
            sprintf(tile->mat_name,"scree");
            sprintf(tile->mat_description,"Unstable");
            break;
    }
}

void MapCell::initTile(int x, int y, int z)
{
    setTileType(x,y,z,TT_ROCK1);
    tiles[z][x][y].wasSeen = -1;
}

// takes info from overworld and translates it into local topology
void MapCell::generateLocalTopology()
{
    MCInfo info = overworld->getMCInfo(gx, gy, gz);
    
    switch(info.type)
    {
        case(MCT_ENTRANCE): genLocalEntrance(info); break;
        case(MCT_GLACIER): genLocalGlacier(info); break;
        case(MCT_GLACIERBOTTOM): genLocalGlacierBottom(info); break;
        case(MCT_CRYSTALCAVE): genLocalCrystalCave(info); break;
        case(MCT_TREASURE): genLocalTreasure(info); break;
        
        default: genLocalDefault(info);
    }
}



void MapCell::genTunnel_round(PosInt start, PosInt end, int diam)
{
    //genTunnel_square(start, end, diam);
    setTileType(start,TT_AIR);
    
    std::vector<PosInt> path = getStraightPath(start, end);
    path.push_back(start); // don't forget to dig out the start...
    
    int j, k,l,m;
    
    float rad = diam*0.5;
    
    // traverse the path and set the tiles to air
    for(j=0;j<path.size(); j++)
    {
        PosInt pos = path[j];
        float nrad = rad*frand(0.8,1.2);
        pos = sumPosInt(pos,{irand(-1,1),irand(-1,1),irand(-1,1)});
        for(k=0;k<diam;k++)
        {
            for(l=0;l<diam;l++)
            {
                for(m=0;m<diam;m++)
                {
                    pos = sumPosInt(path[j],{k-diam/2,l-diam/2,m-diam/2});
                    float dist = distFloat3(PosInt2Float3(pos),PosInt2Float3(path[j]));
                    if(dist < nrad)
                        setTileType(pos,TT_AIR);
                }
            }
        }
    }
}

void MapCell::genTunnel_round_straight(PosInt start, PosInt end, int diam)
{
    //genTunnel_square(start, end, diam);
    setTileType(start,TT_AIR);
    
    std::vector<PosInt> path = getStraightPath(start, end);
    path.push_back(start); // don't forget to dig out the start...
    
    int j, k,l,m;
    
    float rad = diam*0.5;
    
    // traverse the path and set the tiles to air
    for(j=0;j<path.size(); j++)
    {
        PosInt pos = path[j];
        for(k=0;k<diam;k++)
        {
            for(l=0;l<diam;l++)
            {
                for(m=0;m<diam;m++)
                {
                    pos = sumPosInt(path[j],{k-diam/2,l-diam/2,m-diam/2});
                    float dist = distFloat3(PosInt2Float3(pos),PosInt2Float3(path[j]));
                    if(dist < rad)
                        setTileType(pos,TT_AIR);
                }
            }
        }
    }
}


void MapCell::genTunnel_square(PosInt start, PosInt end, int diam)
{
    setTileType(start,TT_AIR);
    
    std::vector<PosInt> path = getStraightPath(start, end);
    
    int j, k,l,m;
    
    // traverse the path and set the tiles to air
    for(j=0;j<path.size(); j++)
    {
        PosInt pos = path[j];
        for(k=0;k<diam;k++)
        {
            for(l=0;l<diam;l++)
            {
                for(m=0;m<diam;m++)
                {
                    pos = sumPosInt(path[j],{k-diam/2,l-diam/2,m-diam/2});
                    setTileType(pos,TT_AIR);
                }
            }
        }
    }
}

void MapCell::genLocalCrystalCave(MCInfo info)
{
    // everything starts out as rock
    
    PosInt c = {sizexy/2,sizexy/2,sizez/2};// {irand(-sizexy/6,sizexy/6),irand(-sizexy/6,sizexy/6),irand(-sizexy/6,sizexy/6)};
    
    // carve out a big center
    genVoid_sphere(c, sizexy/2-2);
    
    // connections
    genStandardConnections(info); // TODO: redo the connections to here so there is guaranteed access but won't break the crystals.
    
    int numCrystals = irand(6,18);
    
    int i,j;
    for(i=0; i<numCrystals; i++)
    {
        Float3 dir = normaliseFloat3({frand(-1,1),frand(-1,1),frand(-1,1)});
        int length = irand(3,10);
        
        PosInt start = sumPosInt(c,Float32PosInt_rounded( mulFloat3(dir,sizexy/2-2-length)));
        PosInt end = sumPosInt(c,Float32PosInt_rounded( mulFloat3(dir,sizexy/2)));
        std::vector<PosInt> beam = getStraightPath(start, end);
        
        for(j=0; j<beam.size(); j++)
        {
            setTileType(beam[j],TT_CRYSTAL);
        }
    }
    
}

void MapCell::genLocalGlacier(MCInfo info)
{
    int i,j,k;
    // everything is ice
    for(i=0; i<sizexy; i++)
    {
        for(j=0; j<sizexy; j++)
        {
            for(k=0; k<sizez; k++)
            {
                setTileType(i,j,k,TT_ICE);
            }
        }
    }
    
    // should be no tunnels in the glacier, but:
    genStandardConnections(info);
}


void MapCell::genLocalGlacierBottom(MCInfo info)
{
    // everything starts out as rock
    int i,j,k;
    // the top half of the map is ice
    for(i=0; i<sizexy; i++)
    {
        for(j=0; j<sizexy; j++)
        {
            for(k=3*sizez/5; k<sizez; k++)
            {
                setTileType(i,j,k,TT_ICE);
            }
        }
    }
    // carve out a small center
    
    for(i=2*sizexy/5; i<3*sizexy/5; i++)
    {
        for(j=2*sizexy/5; j<3*sizexy/5; j++)
        {
            for(k=2*sizez/5; k<3*sizez/5; k++)
            {
                if(i % 5 == 0 && j % 3 == 0)
                    setTileType(i,j,k,TT_COLUMN);
                else
                    setTileType(i,j,k,TT_AIR);
            }
        }
    }
    
    genStandardConnections(info);
}

void MapCell::genLocalEntrance(MCInfo info)
{
    // everything starts out as rock
    int i,j,k;
    // the top half of the map is ice
    for(i=0; i<sizexy; i++)
    {
        for(j=0; j<sizexy; j++)
        {
            for(k=3*sizez/5; k<sizez; k++)
            {
                setTileType(i,j,k,TT_ICE);
            }
        }
    }
    // carve out a small center
    for(i=2*sizexy/5; i<3*sizexy/5; i++)
    {
        for(j=2*sizexy/5; j<3*sizexy/5; j++)
        {
            for(k=2*sizez/5; k<3*sizez/5; k++)
            {
                if(i % 5 == 0 && j % 3 == 0)
                    setTileType(i,j,k,TT_COLUMN);
                else
                    setTileType(i,j,k,TT_AIR);
            }
        }
    }
    // carve out a tunnel up
    genTunnel_round({sizexy/2,sizexy/2,sizez/2}, {sizexy/2,sizexy/2,sizez}, 2);
    
    
    genStandardConnections(info);
    
    // elevator crash site
    PosInt crashPos = getFloorAbove({sizexy/2,sizexy/2,0});
    crashPos = getGlobalPos(crashPos);//
    
    entity_t ent1 = world->createEntity();
    world->mask[ent1] = COMP_POSITION | COMP_IS_VISIBLE | COMP_OMNILIGHT;
    world->position[ent1] =  PosInt2Float3(crashPos);
    world->is_visible[ent1].tex = 8;
    world->is_visible[ent1].tex_side = 9;
    sprintf(world->is_visible[ent1].lookString,"a broken elevator");
    world->light_source[ent1].brightness = 2;
    
    // some bits of ice lying around
    for(i=0; i < 5; i++)
    {
        setTileType(getFloorAbove({irand(2*sizexy/5,3*sizexy/5),irand(2*sizexy/5,3*sizexy/5),0}), TT_ICE);
    }
}

void MapCell::genLocalTreasure(MCInfo info)
{
    // everything starts out as rock
    
    PosInt c = {0,0,0};// {irand(-sizexy/6,sizexy/6),irand(-sizexy/6,sizexy/6),irand(-sizexy/6,sizexy/6)};
    
    // carve out a big center
    int i,j,k;
    for(i=1*sizexy/5; i<4*sizexy/5; i++)
    {
        for(j=1*sizexy/5; j<4*sizexy/5; j++)
        {
            for(k=1*sizez/5; k<4*sizez/5; k++)
            {
                if(i % 5 == 0 && j % 3 == 0)
                    setTileType(i+c.x,j+c.y,k+c.z,TT_ICE);
                else
                    setTileType(i+c.x,j+c.y,k+c.z,TT_AIR);
            }
            setTileType(i+c.x,j+c.y,sizez/5,TT_ICE); //ice floor
        }
    }
    
    genStandardConnections(info);
    
}

void MapCell::genStandardConnections(MCInfo info, PosInt center)
{
    int i;
    
    for(i=0; i<info.numConnections; i++)
    {
        //printf("Cell %d %d %d connection %d goes %d\n",gx,gy,gz,i,info.connection[i].dir);
        PosInt start;
        switch(info.connection[i].dir)
        {
            case DIR_DOWN: start.z = 0; start.x = info.connection[i].i*sizexy; start.y = info.connection[i].j*sizexy; break;
            case DIR_UP: start.z = sizez-1; start.x = info.connection[i].i*sizexy; start.y = info.connection[i].j*sizexy; break;
            case DIR_NORTH: start.y = sizexy-1; start.x = info.connection[i].i*sizexy; start.z = info.connection[i].j*sizez; break;
            case DIR_SOUTH: start.y = 0; start.x = info.connection[i].i*sizexy; start.z = info.connection[i].j*sizez; break;
            case DIR_EAST: start.x = sizexy-1; start.y = info.connection[i].i*sizexy; start.z = info.connection[i].j*sizez; break;
            case DIR_WEST: start.x = 0; start.y = info.connection[i].i*sizexy; start.z = info.connection[i].j*sizez; break;
            default: start = {0,0,0};
        }
        
        int size = info.connection[i].size;
        
        genTunnel_round(start, center, size);
    }
}
void MapCell::genStandardConnections(MCInfo info)
{
    // for every connection, carve a path to the center
    PosInt end = {(int)sizexy/2,(int)sizexy/2,(int)sizez/2};
    genStandardConnections(info, end);
}

void MapCell::genVoid_sphere(PosInt center, int radius)
{
    int orad = radius;
    PosInt t = sumPosInt(center,{radius,radius,radius});
    while(t.x >= sizexy || t.y >= sizexy || t.z >= sizez)
    {
        radius--;
        t = sumPosInt(center,{radius,radius,radius});
    }
    t = sumPosInt(center,{-radius,-radius,-radius});
    while(t.x < 0 || t.y < 0 || t.z < 0)
    {
        radius--;
        t = sumPosInt(center,{-radius,-radius,-radius});
    }
    printf("sphere radius %d of %d\n",radius, orad);
    int i,j,k;
    for(i=-radius; i<radius; i++)
    {
        for(j=-radius; j<radius; j++)
        {
            for(k=-radius; k<radius; k++)
            {
                if(distFloat3(PosInt2Float3({i,j,k}), {0,0,0}) <= radius)
                {
                    PosInt p = sumPosInt({i,j,k},center);
                    setTileType(p, TT_AIR);
                }
            }
        }
    }
}
void MapCell::genVoid_sphere(PosInt center, int radius, int lumps)
{
    genVoid_sphere(center,radius);
}

void MapCell::genLocalDefault(MCInfo info)
{
    // everything starts out as rock
    
    PosInt c = {irand(-sizexy/6,sizexy/6),irand(-sizexy/6,sizexy/6),irand(-sizexy/6,sizexy/6)};
    c = sumPosInt(c,{sizexy/2,sizexy/2,sizez/2});
    
    // carve out center
    int r = irand(2,9);
    genVoid_sphere(c,r);
    
    // connections out
    genStandardConnections(info);
    
    // add some random bits and bobs.
    
    // stalagmites/tites/columns
    int numCols = irand(0,5);
    int i,j;
    for(i=0; i<numCols; i++)
    {
        PosInt p = sumPosInt(c,{irand(-r,r),irand(-r,r),0});
        PosInt ceil = getCeilingBelow({p.x,p.y,sizez-1});
        PosInt floor = getFloorAbove(p);
        int len = irand(1,6);
        for(j=0; j<len; j++)
        {
            if(j%2)
            {
                setTileType(ceil,TT_COLUMN);
                ceil = sumPosInt(ceil,{0,0,-1});
            } else {
                setTileType(floor,TT_COLUMN);
                floor = sumPosInt(floor,{0,0,1});
            }
            if(floor.z >= ceil.z)
                break;
        }
    }
    
}
