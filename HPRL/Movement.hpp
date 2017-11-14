//
//  Movement.hpp
//  HPRL
//
//  Created by Marcus Holm on 10/10/17.
//  Copyright © 2017 Marcus Holm. All rights reserved.
//

#ifndef Movement_hpp
#define Movement_hpp

#include <stdio.h>
//#include "System.h"
#include "Common.h"
#include "Game.hpp"

class Game;

class Movement
{
    Game * game;
    int mask = COMP_CAN_MOVE;
    
    
    float walk(World* w, Key input, entity_t ent);
    float move(World* w, Key input, entity_t ent);
    float climb(World* w, Key input, entity_t ent);
    
public:
    Movement(){}
    int isWalkable(PosInt p);
    int isClimbable(PosInt p);
    
    void setGame(Game * g){game = g;}
    void exec(World* w, float timestep);
    float input(World* w, Key input); // returns time until resolution
};

#endif /* Movement_hpp */
