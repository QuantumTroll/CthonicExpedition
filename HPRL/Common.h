//
//  Common.h
//  HPRL
//
//  Created by Marcus Holm on 10/10/17.
//  Copyright © 2017 Marcus Holm. All rights reserved.
//

#ifndef Common_h
#define Common_h

#include <iostream>
#include <vector>
#include <cmath>
#include <stdlib.h>

typedef unsigned int entity_t;

const int maxEntities = 1024;

const float sqrt2 = sqrtf(2.0);
const float isqrt2 = 1.0/sqrt2;


typedef struct {
    float x;
    float y;
    float z;
} Float3;

typedef struct {
    int x;
    int y;
    int z;
} PosInt;

typedef struct {
    int x, y, z;
} int3;


// Components
typedef enum
{
    COMP_NONE = 0,
    COMP_POSITION = 1 << 0,
    COMP_OWNED = 1 << 1,
    COMP_IS_PLAYER_CONTROLLED = 1 << 2,
    COMP_CAN_MOVE = 1 << 3,
    COMP_IS_VISIBLE = 1 << 4,
    COMP_OMNILIGHT = 1 << 5,
    COMP_ANCHOR = 1 << 6,
    COMP_BANDAGE = 1 << 7,
    COMP_ROPE = 1 << 8,
    COMP_COUNTER = 1 << 9,
    COMP_IS_EDIBLE = 1 << 10,
    COMP_PICKABLE = 1 << 11,
    COMP_DIRLIGHT = 1 << 12,
    COMP_MUTATOR = 1 << 13
} Component;

typedef enum
{
    KEY_NONE = 0,
    KEY_NORTH = 1 << 0,
    KEY_SOUTH = 1 << 1,
    KEY_EAST = 1 << 2,
    KEY_WEST = 1 << 3,
    KEY_UP = 1 << 4,
    KEY_DOWN = 1 << 5,
    KEY_FLARE = 1 << 6,
    KEY_LOOK = 1 << 7,
    KEY_JUMP = 1 << 8,
    KEY_WAIT = 1 << 9,
    KEY_CLIMB = 1 << 10,
    KEY_INVENTORY = 1 << 11,
    KEY_EXAMINE = 1 << 12,
    KEY_ORIENTEER = 1 << 13,
    KEY_CLOSEEYES = 1 << 14,
    KEY_DROP = 1 << 15,
    KEY_PICKUP = 1 << 16
} Key;

typedef enum
{
    MOV_NONE = 0,
    MOV_FREE = 1 << 0,
    MOV_WALK = 1 << 1,
    MOV_CLIMB = 1 << 2,
    MOV_FLOAT = 1 << 3,
    MOV_SWIM = 1 << 4,
    MOV_WIELDED = 1 << 5
} MoveType;

typedef enum
{
    ITM_BANDAGE,
    ITM_FLARE,
    ITM_CHOCOLATE,
    ITM_ANCHOR,
    ITM_INSTRUMENT,
    ITM_LIGHT,
    ITM_BATTERY,
    ITM_SLIME
}ItemType;

typedef enum
{
    SLM_NIGHTSIGHT,
    SLM_LUMO,
    SLM_LITHOVORE,
    SLM_METABO,
    SLM_STAMINA,
    SLM_STRENGTH,
    SLM_CLAWS,
    SLM_GECKO,
    SLM_ROPE,
    SLM_WATERBREATH,
    SLM_SWIM
} SlimeType;

typedef enum
{
    CNT_FLARE,   // destroyed when 0
    CNT_ELECTRIC // switched off when 0
} CounterType;

typedef struct
{
    int type;
    float time;
} Event;

typedef struct
{
    int max;
    float count;
    CounterType type;
    int on;
} Counter;

typedef struct
{
    float calories;
    float moodMod;
    float quench;
} Edible;

typedef struct
{
    char name [16];
    int stack;
    int maxStack;
    ItemType type;
} Pickable;


typedef struct {
    int tex;
    int tex_side;
    char lookString[32];
} IsVisible;

typedef struct {
    float brightness;
    float width; // beam width, used only for dirlights. In degrees.
    Float3 color;
} LightSource;

typedef struct {
    std::string name;
    int hasClimbed;
    Float3 facing;
    int injuries;
    float mood;
    float strength;
    float energy;
    float maxEnergy;
    float recover;
    float bleed;
    float bruise;
    float armour;
    float healing;
    float hunger;
    float thirst;
} Character;

typedef struct {
    char string[32]; //123456789012345678901234567890
    char key;
    entity_t ent;
}MenuOption;

typedef struct {
    char name[16];
    int x,y;
    int numOptions;
    MenuOption options[32]; // how many options is a reasonable limit?
    void (*menuHandler)(void*,char);
}Menu;


static PosInt sumPosInt(PosInt a, PosInt b)
{
    return PosInt{a.x+b.x,a.y+b.y,a.z+b.z};
}

static PosInt diffPosInt(PosInt a, PosInt b)
{
    return PosInt{a.x-b.x,a.y-b.y,a.z-b.z};
}

static bool isEqPosInt(PosInt a, PosInt b)
{
    return a.x==b.x && a.y==b.y && a.z==b.z;
}

static Float3 sumFloat3(Float3 a, Float3 b)
{
    return Float3{a.x+b.x,a.y+b.y,a.z+b.z};
}

static Float3 diffFloat3(Float3 a, Float3 b)
{
    return Float3{a.x-b.x,a.y-b.y,a.z-b.z};
}

static float distFloat3(Float3 a, Float3 b)
{
    return sqrtf((b.x-a.x)*(b.x-a.x)+(b.y-a.y)*(b.y-a.y)+(b.z-a.z)*(b.z-a.z));
}

static float normFloat3(Float3 a)
{
    return sqrtf(a.x*a.x+a.y*a.y+a.z*a.z);
}

static Float3 normaliseFloat3(Float3 a)
{
    float n = normFloat3(a);
    return {a.x/n, a.y/n, a.z/n};
}

static PosInt Float32PosInt(Float3 p)
{
    int x = (int)(p.x );
    int y = (int)(p.y );
    int z = (int)(p.z );
    return {x,y,z};
}
static PosInt Float32PosInt_rounded(Float3 p)
{
    int x = (int)(p.x + 0.5f);
    int y = (int)(p.y + 0.5f);
    int z = (int)(p.z + 0.5f);
    return {x,y,z};
}

static Float3 PosInt2Float3(PosInt p)
{
    return Float3{(float)p.x,(float)p.y,(float)p.z};
}
static Float3 PosInt2Float3_rounded(PosInt p)
{
    return Float3{(float)p.x+0.5f,(float)p.y+0.5f,(float)p.z+0.5f};
}


static int distPosInt(PosInt a, PosInt b)
{
    return (int)sqrtf((b.x-a.x)*(b.x-a.x)+(b.y-a.y)*(b.y-a.y)+(b.z-a.z)*(b.z-a.z));
}

static int manhattanPosInt(PosInt a, PosInt b)
{
    return abs(b.x-a.x)+abs(b.y-a.y)+abs(b.z-a.z);
}

static Float3 mulFloat3(Float3 a, float c)
{
    return Float3{c*a.x,c*a.y,c*a.z};
}

static float dotFloat3(Float3 a, Float3 b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

static Float3 minFloat3(Float3 a, Float3 b)
{
    return {(float)fmin(a.x,b.x),(float)fmin(a.y,b.y),(float)fmin(a.z,b.z)};
}


typedef enum
{
    DIR_NONE = 0,
    DIR_WEST = 1 << 0,
    DIR_EAST = 1 << 1,
    DIR_NORTH = 1 << 2,
    DIR_SOUTH = 1 << 3,
    DIR_UP = 1 << 4,
    DIR_DOWN = 1 << 5
}Direction;

static Direction reverse(Direction dir)
{
    switch(dir){
        case DIR_NONE: return DIR_NONE;
        case DIR_WEST: return DIR_EAST;
        case DIR_EAST: return DIR_WEST;
        case DIR_NORTH: return DIR_SOUTH;
        case DIR_SOUTH: return DIR_NORTH;
        case DIR_UP: return DIR_DOWN;
        case DIR_DOWN: return DIR_UP;
        default: return DIR_NONE;
    }
}


class World
{
public:
    int mask[maxEntities];
    
    Float3 position[maxEntities];
    Float3 velocity[maxEntities];
    
    IsVisible is_visible[maxEntities];
    LightSource light_source[maxEntities];
    MoveType move_type[maxEntities];
    Counter counter[maxEntities];
    Edible edible[maxEntities];
    Pickable pickable[maxEntities];
    SlimeType slime[maxEntities];
    
    World();
    entity_t createEntity();
    void destroyEntity(entity_t e);
    
    void saveEntity(entity_t ent, FILE * f);
};


// doesn't include 0th step. good/bad? Good, I think.
static std::vector<PosInt> getStraightPath(PosInt from, PosInt to)
{
    // write fp line equation. step along 1 unit at a time, check if x,y,z have changed.
    // or "Bresenham's line algorithm"
    
    // or write fp line equation. step along the dominant direction. figure the error for other directions. when error > 0.5, take a step along that direction instead.
    
    std::vector<PosInt> p;
    
    int x, y, z;     x=0; y=0; z=0;
    Float3 delta={(float)to.x-from.x,(float)to.y-from.y,(float)to.z-from.z};
    Float3 error = {0,0,0};
    //printf("%d %d %d to %d %d %d\n",from.x,from.y,from.z,to.x,to.y,to.z);
    
    if(fabs(delta.x) >= fabs(delta.y) && fabs(delta.x) >= fabs(delta.z))    // if x is dominant
    {
        
        float my = (float)delta.y/delta.x;
        float mz = (float)delta.z/delta.x;
        int xstep=1, ystep=1, zstep=1;
        if(delta.x < 0)
            xstep = -1;
        
        if(delta.y < 0)
            ystep = -1;
        
        if(delta.z < 0)
            zstep = -1;
        
        
        int counter = 0;
        //TODO: evaluate this counter limit
        while(! isEqPosInt(sumPosInt(from,{x,y,z}), to) && counter<40)
        {
            // printf("foo\n");
            float yerr =(float)(my*x-y);
            float zerr = (float)(mz*x-z);
            error = {0.0,yerr,zerr};
            
            if(error.y*ystep >= 0.5)
            {
                y += ystep;
            }else if(error.z*zstep >= 0.5)
            {
                z += zstep;
            }else
            {
                x += xstep;
            }
            counter++;
            p.push_back(sumPosInt(from,{x,y,z}));
        }
    }else if(fabs(delta.y) >= fabs(delta.x) && fabs(delta.y) >= fabs(delta.z))    // if y is dominant
    {
        // flip vectors so x is dominant and call getStraightPath
        p = getStraightPath({from.y,from.x,from.z}, {to.y,to.x,to.z});
        // then unflip
        for(int i=0; i<p.size(); i++)
        {
            p[i]={p[i].y,p[i].x,p[i].z};
        }
        
    }else{ // if z is dominant
        // flip vectors so x is dominant and call getStraightPath
        p = getStraightPath({from.z,from.y,from.x}, {to.z,to.y,to.x});
        // then unflip
        for(int i=0; i<p.size(); i++)
        {
            p[i]={p[i].z,p[i].y,p[i].x};
        }
    }
    
    return p;
}

static float frand(float xmin, float xmax) { return (xmin+(xmax-xmin)*(float)random()/(float)RAND_MAX); }
static int irand(int imin, int imax) { return random()%(imax-imin+1)+imin; }

static int isnan(float x) { return x != x; };
#endif /* Common_h */
