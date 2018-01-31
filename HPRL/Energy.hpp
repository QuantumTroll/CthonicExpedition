//
//  Energy.hpp
//  HPRL
//
//  Created by Marcus Holm on 11/11/17.
//  Copyright © 2017 Marcus Holm. All rights reserved.
//

#ifndef Energy_hpp
#define Energy_hpp

#include <stdio.h>
#include "Common.h"
#include "Game.hpp"

class Game;


class EnergySystem
{
    entity_t player;
    World* world;
    Character* pc;
    Game* game;
    
    float getMoodMods();
    
public:
    EnergySystem(World* w, Character* c, Game* g){world=w;pc = c;game=g;}
    void exec(float timestep);
};

#endif /* Energy_hpp */
