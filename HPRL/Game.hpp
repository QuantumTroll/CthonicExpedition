//
//  Game.hpp
//  HPRL
//
//  Created by Marcus Holm on 11/10/17.
//  Copyright © 2017 Marcus Holm. All rights reserved.
//

#ifndef Game_hpp
#define Game_hpp


/* sets up and maintains world
	- record and execute inputs
	- iterate through systems and execute them
	- provide call-backs for special conditions (achievements, deaths, menus, etc) */

#include <stdio.h>
#include <iostream>
#include <queue>
#include <vector>
#include <string>

#include "Common.h"
#include "Movement.hpp"
#include "MapCell.hpp"
#include "Energy.hpp"

class Movement;
class EnergySystem;

class Game
{
    float time;
    World* world;
    
    Character pc;
    entity_t player;
    entity_t look;
    
    // voxel world
    // a 3d array of cell pointers. We're always in the middle one (1,1,1).
    MapCell* cell[3][3][3];
    
    Movement*  movement;
    
    EnergySystem* energy;

    std::queue<Key> inputs; // queue of inputs
    
    std::queue<Event> events; // queue of game events
    
    int maxLogEntries = 50;
    std::deque<std::string> log;
    
    PosInt lookAt;
    
    int pointVisibility(Float3 f, Float3 t);
    
    void dropFlare();
    void throwFlare();
    void jump();
    void toggleLook();
    void toggleClimb();

    int isOn;
    
public:
    Game(World* w);
    void init();
    void addInput(char inchar);
    void onDeath();
    void onEscape();
    void doSystems();
    void addEvent(Event e);
    void collision(entity_t ent, float dV);
    // handle a menu somehow
    
    void turnOn(){isOn = 1;}
    void turnOff(){isOn = 0;}
    
    // let DrawWindow ask for any cell
    MapCell* getCellCoords(int x, int y, int z);
    MapCell* getCell(int i, int j, int k) { return cell[i][j][k]; }
    
    std::deque<std::string> getLog() { return log; }
    void addToLog(std::string str);
    

    
    entity_t getPlayerEntity() { return player; }
    PosInt getLookAt() { return lookAt; }
    Character* getCharacter() { return &pc; }
    int visibility(PosInt from, PosInt to);
    float lighting(Float3 p);
};

#endif /* Game_hpp */
