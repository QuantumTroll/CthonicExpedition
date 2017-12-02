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
#include "Overworld.hpp"

class Movement;
class EnergySystem;

class Game
{
private:
    float time;
    World* world;

    
    Character pc;
    entity_t player;
    entity_t look;
    int eyesOpen;
    
    // voxel world
    // a 3d array of cell pointers. We're always in the middle one (1,1,1).
    MapCell* cell[3][3][3];
    Overworld* overworld; 
    
    int cellSizeXY = 30;
    int cellSizeZ = 20;
    
    PosInt cellCoords;
    
    Movement*  movement;
    
    EnergySystem* energy;

    std::queue<Key> inputs; // queue of inputs
    
    std::queue<Event> events; // queue of game events
    
    int maxLogEntries = 50;
    std::deque<std::string> log;
    
    PosInt lookAt;
    
    void lookReport();
    
    int pointVisibility(Float3 f, Float3 t);
    
    float dropFlare();
    float throwFlare();
    float jump();
    void toggleLook();
    void toggleCloseEyes();
    float toggleClimb();
    void heal(float timeStep);    
    void updateCounters(float timeStep);
    
    float useBandage();

    int isOn;

    void unloadCells();
    void loadCells();
    
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
    
    int getCellSizeXY(){return cellSizeXY;}
    int getCellSizeZ(){return cellSizeZ;}
    
    // let DrawWindow and others ask for any cell.
    MapCell* getCellatCoords(int x, int y, int z);
    MapCell* getCell(int i, int j, int k) { return cell[i][j][k]; }
// returns the coordinates of the map cell for given global coords    
    PosInt getCellCoords(int x, int y, int z);
    PosInt getCellCoords(PosInt p) {return getCellCoords(p.x,p.y,p.z);}
    
    float wasSeen(MapTile* tile);
    void setWasSeen(MapTile* tile, float light);

    
    
    MapTile* getTile(int x, int y, int z);
    MapTile* getTile(float x, float y, float z);
    MapTile* getTile(PosInt p);
    MapTile* getTile(Float3 p);
    
    
    
    std::deque<std::string> getLog() { return log; }
    void addToLog(std::string str);
    void displayInventory();
    

    
    entity_t getPlayerEntity() { return player; }
    PosInt getLookAt() { return lookAt; }
    Character* getCharacter() { return &pc; }
    int visibility(PosInt from, PosInt to);
    float lighting(Float3 p);
};

#endif /* Game_hpp */
