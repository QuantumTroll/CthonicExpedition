//
//  Game.cpp
//  HPRL
//
//  Created by Marcus Holm on 11/10/17.
//  Copyright © 2017 Marcus Holm. All rights reserved.
//


/* sets up and maintains world
	- record and execute inputs
	- iterate through systems and execute them
	- provide call-backs for special conditions (achievements, deaths, menus, etc) */

#include "Game.hpp"

Game::Game(World* w){
    world=w; movement = new Movement(); movement->setGame(this); movement->setWorld(world);
    energy = new EnergySystem(world, &pc, this);
}

World::World(){
    entity_t entity;
    for(entity = 0; entity<maxEntities; entity++)
    {
        mask[entity] = COMP_NONE;
    }
}

entity_t World::createEntity()
{
    entity_t entity;
    for(entity = 0; entity<maxEntities; entity++)
    {
        if(mask[entity] == COMP_NONE)
            return entity;
    }
    fprintf(stderr, "Error! No more entities.\n");
    return -1;
}

void World::destroyEntity(entity_t e)
{
    mask[e] = COMP_NONE;
    // also get rid of other values?
}

void World::saveEntity(entity_t ent, FILE *f)
{
    printf("TODO: save entity %d to file\n", ent);
}

void Game::init()
{
    time = 0;
    srandom(3);
    
    // create a player character
    
    pc.name="Mary-Sue";
    pc.mood=85;
    pc.hasClimbed = 0;
    pc.facing = {0,0,-1};
    pc.strength = 10;
    pc.injuries=0;
    pc.maxEnergy = 100;
    pc.energy = 90;
    pc.recover = 10;
    pc.bleed = 0;
    pc.bruise = 0;
    pc.armour = 3;
    pc.healing = 0.01;
    pc.hunger = .10;
    pc.thirst = .10;
    pc.hungerLevel = getHungerLevel(pc.hunger);
    pc.thirstLevel = getThirstLevel(pc.thirst);
    pc.oxygen = 1;
    pc.oxygenLevel = getOxygenLevel(pc.oxygen);

// create a player entity
    player = world->createEntity();
    world->mask[player] = COMP_IS_PLAYER_CONTROLLED | COMP_IS_VISIBLE | COMP_CAN_MOVE | COMP_POSITION;
  //  world->is_player_controlled[player].is_player_controlled = 1;
  //  world->is_visible[player].is_visible = 1;
    world->position[player] = {(float)(cellSizeXY*2.5),(float)(cellSizeXY*2.5-1),(float)(cellSizeZ*28.5+3)};
    world->velocity[player] = {0,0,0};
    world->is_visible[player].tex = 266; // some thing
    world->is_visible[player].tex_side = 266; // some thing
    sprintf(world->is_visible[player].lookString,"yourself, %s",pc.name.c_str());
    world->move_type[player] = MOV_FREE; // default movement mode
    world->light_source[player].brightness = 0; // reduce when not debugging
    world->light_source[player].color = {1,1,1};
    lookAt = Float32PosInt( world->position[player]);
    
    // add starting gear to inventory
    getBandages(3);
    getFlares(5);
    getStartingFoods();
    getAnchors();
    getFlute();
    getFlashlight();
    
    eyesOpen = 1;
    
    look = 0;
    
    currentMenu = NULL;
    
    overworld = new Overworld();
    
    cellCoords = getCellCoords(Float32PosInt_rounded( world->position[player]));
    
    // generate map cells
    int i, j, k;
    for(i=0; i<3; i++)
        for(j=0; j<3; j++)
            for(k=0; k<3; k++){
                cell[i][j][k] = new MapCell(cellSizeXY,cellSizeZ,cellCoords.x+i-1,cellCoords.y+j-1,cellCoords.z+k-1,overworld);
                cell[i][j][k]->load(world,cellCoords.x+i-1,cellCoords.y+j-1,cellCoords.z+k-1);
              //  printf("cell %d %d %d has coords %d %d %d\n",i,j,k,i,j,k);
            }
    
    // propagate water in new cells
    propagateWater();
    
    addToLog("You wake up in the test cave. There is no escape.");
    turnOn();
}

// TODO: set controls in config file instead of hard-coding here
void Game::addInput(char inchar)
{
    Key input;
    // add input to inputs queue
    if(! currentMenu)
    {
        switch(inchar)
        {
         /*   case 'w': input = KEY_NORTH; break;
            case 's': input = KEY_SOUTH; break;
            case 'a': input = KEY_WEST; break;
            case 'd': input = KEY_EAST; break;*/
            case 'R': input = KEY_NORTH; break;
            case 'T': input = KEY_SOUTH; break;
            case 'Q': input = KEY_WEST; break;
            case 'S': input = KEY_EAST; break;
            case -31: input = KEY_DOWN; break;
            case '>': input = KEY_DOWN; break;
            case '<': input = KEY_UP; break;
            case 'f': input = KEY_FLARE; break;
            case 'l': input = KEY_LOOK; break;
            case 'j': input = KEY_JUMP; break;
            case '.': input = KEY_WAIT; break;
            case 'c': input = KEY_CLIMB; break;
            case 'i': input = KEY_INVENTORY; break;
            case 'o': input = KEY_ORIENTEER; break;
            case 'x': input = KEY_CLOSEEYES; break;
            case 'e': input = KEY_EXAMINE; break;
            case 'g': input = KEY_PICKUP; break;
            case 'd': input = KEY_DROP; break;
            case 'y': input = KEY_GHOST; break;
            default: input = KEY_NONE; printf("char %c (%d) pressed\n",inchar, (int) inchar);
        }
    }else {
        // a menu is open. send key directly to menu handler
        currentMenu->menuHandler(this,inchar);
    }
    
    inputs.push(input);
    turnOn();
}

void Game::dropMenuHandler_s(void* t, char c)
{
    ((Game*)t)->dropMenuHandler(c);
}
void Game::dropMenuHandler(char c)
{
    if(c == 'x')
    {
        exitMenu();
        return;
    }
    int opt = c-'a';
    if(opt >= currentMenu->numOptions)
        return;
    
    entity_t ent = currentMenu->options[opt].ent;
    
    dropItem(ent);
    exitMenu();
}

float Game::pickUpItems()
{
    // for each pickable, non-owned items in this position
    entity_t ent;
    for(ent = 0; ent < maxEntities; ent++)
    {
        if(world->mask[ent] & COMP_PICKABLE && ! (world->mask[ent] & COMP_OWNED) && distFloat3(world->position[ent],world->position[player]) < .9)
        {
            // set owned. make non-visible. set move_type to MOV_WIELDED?
            char s [32];
            sprintf(s,"Picking up %s\n",world->pickable[ent].name);
            addToLog(s);
            world->mask[ent] = (world->mask[ent] | COMP_OWNED) & ~COMP_IS_VISIBLE;
            world->move_type[ent] = MOV_WIELDED;
        }
    }
    return 1;
}

void Game::dropItem(entity_t ent)
{
    char s [32];
    sprintf(s,"Dropping %s\n",world->pickable[ent].name);
    
    addToLog(s);
    
        // make visible and give it a location etc
    world->mask[ent] = (world->mask[ent] | COMP_CAN_MOVE | COMP_POSITION | COMP_IS_VISIBLE) & ~ COMP_OWNED ; // no longer owned
    world->move_type[ent] = MOV_FREE;
    world->position[ent] = {world->position[player].x,world->position[player].y,world->position[player].z};
    world->velocity[ent] = {0,0,0};

    if(world->pickable[ent].type == ITM_BANDAGE)
    {
        sprintf(world->is_visible[ent].lookString,"Bandages");
        // placeholder
        world->is_visible[ent].tex = 32;
        world->is_visible[ent].tex_side = 32;
    }else if(world->pickable[ent].type == ITM_FLARE)
    {
        sprintf(world->is_visible[ent].lookString,"%s",world->pickable[ent].name);
        // placeholder
        world->is_visible[ent].tex = 4;
        world->is_visible[ent].tex_side = 4;
    }else if(world->pickable[ent].type == ITM_CHOCOLATE)
    {
        sprintf(world->is_visible[ent].lookString,"%s",world->pickable[ent].name);
        // placeholder
        // Defined when creating the food item.
        //world->is_visible[ent].tex = 33;
        //world->is_visible[ent].tex_side = 33;
    }if(world->pickable[ent].type == ITM_ANCHOR)
    {
        sprintf(world->is_visible[ent].lookString,"%s",world->pickable[ent].name);
        // placeholder
        world->is_visible[ent].tex = 267;
        world->is_visible[ent].tex_side = 267;
    }else if(world->pickable[ent].type == ITM_INSTRUMENT)
    {
        sprintf(world->is_visible[ent].lookString,"%s",world->pickable[ent].name);
        // placeholder
        world->is_visible[ent].tex = 36;
        world->is_visible[ent].tex_side = 36;
    }else if(world->pickable[ent].type == ITM_LIGHT)
    {
        sprintf(world->is_visible[ent].lookString,"%s",world->pickable[ent].name);
        // placeholder
        world->is_visible[ent].tex = 37;
        world->is_visible[ent].tex_side = 37;
    }else
    {
        printf("dropping default item\n");
    }
    
    return;
}

void Game::inventoryMenuHandler_s(void * t,char c)
{
    ((Game*)t)->inventoryMenuHandler(c);
}
void Game::inventoryMenuHandler(char c)
{
    if(c == 'x')
    {
        exitMenu();
        return;
    }
    int opt = c-'a';
    if(opt >= currentMenu->numOptions)
        return;
    
    entity_t ent = currentMenu->options[opt].ent;
    
    switch (world->pickable[ent].type)
    {
        case ITM_BANDAGE: useBandage(ent); break;
        case ITM_FLARE: wieldFlare(ent); break;
        case ITM_CHOCOLATE: eat(ent); break;
        case ITM_ANCHOR: useAnchor(ent); break;
        case ITM_INSTRUMENT: playInstrument(ent); break;
        case ITM_LIGHT: toggleLight(ent); break;
        case ITM_SLIME: eatSlime(ent); break;
        case ITM_BOTTLE: useBottle(ent); break;
        default: printf("pushed %c, got item %s\n",c,world->pickable[ent].name);
    }
    exitMenu();
}

void Game::displayInventory(int action) // 0 for use, 1 for drop, ...
{
    currentMenu = (Menu*) malloc(sizeof(Menu)); // freed in exitMenu()
    
    switch(action)
    {
        case 0: currentMenu->menuHandler = &inventoryMenuHandler_s;
                sprintf(currentMenu->name,"Inventory");
                break;
        case 1: currentMenu->menuHandler = &dropMenuHandler_s;
                sprintf(currentMenu->name,"Drop Item");
                break;
        default: printf("Unknown inventory menu action: %d\n",action);
    }
    
    MenuOption* options = currentMenu->options;

    int numItems = 0;
    
    int ent;
    for(ent = 0; ent < maxEntities; ent++)
    {
        if(world->mask[ent] & COMP_OWNED)
        {
            sprintf(options[numItems].string,"%d %s",world->pickable[ent].stack, world->pickable[ent].name);
            options[numItems].key = (char)('a'+numItems);
            options[numItems].ent = ent;
            numItems++;
        }
    }
    
    currentMenu->x = 7;
    currentMenu->y = numItems+3;
    currentMenu->numOptions = numItems;
}

void Game::addLabel(const char* str, float time, Float3 color)
{
    addLabel(str,world->position[player].x,world->position[player].y,time,color);
}

void Game::addLabel(const char* str, int x, int y, float time, Float3 color)
{
    entity_t label = world->createEntity();
    world->mask[label] = COMP_LABEL | COMP_POSITION | COMP_COUNTER;
    world->label[label].color = color;
    sprintf(world->label[label].text, str);
    world->position[label] = {(float)x,(float)y,0};
    world->counter[label].on = 1;
    world->counter[label].count = time;
    world->counter[label].max = time;
    world->counter[label].type = CNT_FLARE;
}

void Game::addToLog(std::string str)
{
    int numEntries = log.size();
    
    int t = (int) time;
    std::string entry = "T " + std::to_string(t) + ": " + str;
    
    if(numEntries > maxLogEntries)
    {
        log.pop_front();
    }
    
    log.push_back(entry);
}
void Game::doSystems(float dt)
{
    updateLabels(dt);
    if(! isOn)
        return;
    Key input;
    float timeStep = 0;
    //printf("%f %f %f\n",world->position[player].x,world->position[player].y,world->position[player].z);
    // process inputs
    while(! inputs.empty())
    {
        input = inputs.front();
        inputs.pop();
        
        if(input & (KEY_UP|KEY_DOWN|KEY_WEST|KEY_EAST|KEY_NORTH|KEY_SOUTH))
        {
         
            timeStep = movement->input(input);
        }else if(input & KEY_FLARE)
        {
            if(look == 0)
                timeStep = dropFlare();
            else
                timeStep = throwFlare();
            
        }else if(input & KEY_LOOK)
        {
            toggleLook();
        }else if(input & KEY_JUMP)
        {
            timeStep = jump();
        }else if(input & KEY_WAIT)
        {
            //TODO: if walking, center player.
            timeStep = movement->input(input);
        }else if(input & KEY_CLIMB)
        {
            timeStep = toggleClimb();
        }else if(input & KEY_INVENTORY)
        {
            displayInventory(0);
        }else if(input & KEY_ORIENTEER)
        {
            char s[32];
            sprintf(s,"You are in cell %d %d %d",cellCoords.x,cellCoords.y,cellCoords.z);
            addToLog(s);
        }else if(input & KEY_GHOST)
        {
            toggleGhost();
        }else if(input & KEY_CLOSEEYES)
        {
            toggleCloseEyes();
        }else if(input & KEY_EXAMINE)
        {
            lookReport();
        }else if(input & KEY_PICKUP)
        {
            // pickup items from this tile
            pickUpItems();
        }else if(input & KEY_DROP)
        {
            // open inventory menu for dropping
            displayInventory(1);
        }
    }
    // step simulation
    // grab an event. take timestep. //TODO: fix this so it works as intended. Maybe Looking/Floating should take a teensy bit of time? Or maybe control input shouldn't be stupid and trigger spurious doSystems() calls. //update: maybe fixed now?
    
    // update facing
    pc.facing = normaliseFloat3(world->velocity[player]);
    
    energy->exec(timeStep);
    movement->exec(timeStep);
    heal(timeStep);
    updateCounters(timeStep);
    time += timeStep;    
    
    //TODO: check if movement brought player in range of cell edge. if so, offload old and load new cells.
//    cellCoords
    PosInt currentCellCoords = getCellCoords(Float32PosInt_rounded( world->position[player]));
    if(manhattanPosInt(cellCoords, currentCellCoords) > 0)
    {
        // move cells
        moveCells(currentCellCoords);
        
        // propagate water in new cells
        propagateWater();
                
        // TODO: going back upstream *requires* loading saved cells.
        
        //TEST:
        //for(i=-40;i<40;i++)
        //    printf("%d gets cell %d\n",i,getCellCoords(i,0,0).x);
    }
    
    // update lookAt
    if(look == 0)
    {
        lookAt.x = (int)(world->position[player].x + 0.5);
        lookAt.y = (int)(world->position[player].y + 0.5);
        lookAt.z = (int)(world->position[player].z + 0.5);
        if(world->position[player].x < 0)
            lookAt.x --;
        if(world->position[player].y < 0)
            lookAt.y --;
        if(world->position[player].z < 0)
            lookAt.z --;
    }else
    {
        lookAt.x = (int)(world->position[look].x + 0.5);
        lookAt.y = (int)(world->position[look].y + 0.5);
        lookAt.z = (int)(world->position[look].z + 0.5);
        if(world->position[look].x < 0)
            lookAt.x --;
        if(world->position[look].y < 0)
            lookAt.y --;
        if(world->position[look].z < 0)
            lookAt.z --;                
    }
    printf("look at %d %d %d\n",lookAt.x,lookAt.y,lookAt.z);
    turnOff();
}

void Game::addWaterSource(PosInt p, MCInfo* mci)
{
    int i;
    if(mci->numWaterSources < maxWaterSourcesPerCell) {
        int alreadyThere = 0;
        for(i=0;i<mci->numWaterSources; i++)
        {
            if( distPosInt(mci->source[i],p) < 1) // < 2 would also be fine i think
            {
                alreadyThere = 1;
            }
        }
        if(! alreadyThere) {
            mci->source[mci->numWaterSources] = p;
            mci->numWaterSources++;
            printf("water source at %d %d %d\n",p.x,p.y,p.z);
        }
        printf("water source at %d %d %d\n",p.x,p.y,p.z);
    }else {
        printf("Too many watersources at %d %d %d\n",p.x,p.y,p.z);
    }
}

void Game::propagateWater()
{
    // propagate water here. Need to know sources... from mcinfo?
    // so how does water propagation work?
    // go through cells' mcinfo:s for sources. push onto stack of waterfront tiles
    // have stack of waterfront tiles, sorted by z-level
    //   pop a lowest waterfront tile, check directions where it can spread
    //   direction of flow of waterfront set in direction of new tiles
    //   spread water in those directions, add those tiles as new waterfronts
    //   if new waterfront is across a mapcell boundary, save to mcinfo as new source
    //   repeat until no more waterfront tiles
    
    int i,j, k;
    int s;
    std::queue<PosInt> waterFront;
    
    for(i=0; i<3; i++)
    {
        for(j=0; j<3; j++)
        {
            for(k=0; k<3; k++)
            {
                // go through watersources of cell ijk and add to waterfront
                for(s=0; s < cell[i][j][k]->waterSources.size(); s++)
                {
                    waterFront.push(cell[i][j][k]->waterSources[s]);
                   // printf("waterfront added at %d %d %d\n",cell[i][j][k]->waterSources[s].x,cell[i][j][k]->waterSources[s].y,cell[i][j][k]->waterSources[s].z);
                }
            }
        }
    }
    
    while(! waterFront.empty())
    {
        PosInt p = waterFront.front();
        waterFront.pop();
        MapTile* t = getTile(p);
        if(t->propmask & TP_WATER)
            continue;
        
        setTileType(p, TT_WATER);
        t->flowDir = DIR_NONE;
        t->flowSpeed = 1;
       // printf("made tile in %d %d %d into water\n",p.x,p.y,p.z);
        // look around for where water can flow, add to waterfront queue
        // IMPORTANT: if water runs down, don't run to the sides, otherwise spread out
        
        // check for map cell boundaries and make into sources. If we're at an outside boundary, don't propagate across bdy.
        MapCell * cell = getCellatCoords(p);
        if(cell->isGlobalEdge(p))
        {
            //printf("%d\n",__LINE__);
            // tell overworld to add water source to this cell's info
            
            PosInt gp = getCellCoords(p);
            MCInfo * mci = overworld->getMCInfo(gp.x,gp.y,gp.z);
            addWaterSource(p,mci);
        }
        
        // TODO: make it so small z-level bumps don't stop all water. Esp. 1 z-level should be ok.
        // but how?
        //
        // if water is absorbed/lost on a tile, go to max z and that record tile.
        //   For each such tile, travel through water body within a given radius  searching for a higher point
        //   if higher point is closer than this threshold distance, then make air around higher point into new waterfronts
        int hasOutflow = 0;
        
        PosInt p2 = sumPosInt(p,{0,0,-1});
        if(isInBounds(p2) && getTile(p2)->propmask & TP_AIR)
        {
            t->flowDir = DIR_DOWN;
            waterFront.push(p2);
            hasOutflow++;
            continue; //comment this out to make water spread over bumps
        }else if(! isInBounds(p2))
        {
            // tell overworld to add water source to this cell's info
            PosInt gp = getCellCoords(p2);
            MCInfo * mci = overworld->getMCInfo(gp.x,gp.y,gp.z);
            addWaterSource(p2,mci);
            hasOutflow++;
        }
        p2 = sumPosInt(p,{-1,0,0});
        if(isInBounds(p2) && getTile(p2)->propmask & TP_AIR)
        {
            t->flowDir = DIR_WEST;
            waterFront.push(p2);
            hasOutflow++;
        }else if(! isInBounds(p2))
        {
            // tell overworld to add water source to this cell's info
            PosInt gp = getCellCoords(p2);
            MCInfo * mci = overworld->getMCInfo(gp.x,gp.y,gp.z);
            addWaterSource(p2,mci);
            hasOutflow++;
        }
        p2 = sumPosInt(p,{1,0,0});
        if(isInBounds(p2) && getTile(p2)->propmask & TP_AIR)
        {
            t->flowDir = DIR_EAST;
            waterFront.push(p2);
            hasOutflow++;
        }else if(! isInBounds(p2))
        {
            // tell overworld to add water source to this cell's info
            PosInt gp = getCellCoords(p2);
            MCInfo * mci = overworld->getMCInfo(gp.x,gp.y,gp.z);
            addWaterSource(p2,mci);
            hasOutflow++;
        }
        p2 = sumPosInt(p,{0,-1,0});
        if(isInBounds(p2) && getTile(p2)->propmask & TP_AIR)
        {
            t->flowDir = DIR_SOUTH;
            waterFront.push(p2);
            hasOutflow++;
        }else if(! isInBounds(p2))
        {
            // tell overworld to add water source to this cell's info
            PosInt gp = getCellCoords(p2);
            MCInfo * mci = overworld->getMCInfo(gp.x,gp.y,gp.z);
            addWaterSource(p2,mci);
            hasOutflow++;
        }
        p2 = sumPosInt(p,{0,1,0});
        if(isInBounds(p2) && getTile(p2)->propmask & TP_AIR)
        {
            t->flowDir = DIR_NORTH;
            waterFront.push(p2);
            hasOutflow++;
        }else if(! isInBounds(p2))
        {
            // tell overworld to add water source to this cell's info
            PosInt gp = getCellCoords(p2);
            MCInfo * mci = overworld->getMCInfo(gp.x,gp.y,gp.z);
            addWaterSource(p2,mci);
            hasOutflow++;
        }
        
        if(! hasOutflow)
        {
            // if water is absorbed/lost on a tile, go to max z and that record tile.
            p2 = p;
            PosInt highest = p2;
            while(getTile(p2)->propmask & TP_WATER)
            {
                highest = p2;
                p2.z ++;
            }
            printf("no outflow from %d %d %d, high z %d\n",p.x,p.y,p.z,highest.z);
            //For each such tile, travel through water body within a given radius searching for a point of water with surrounding air
            int overflowRad = 5;
            for( i=-overflowRad; i<=overflowRad; i++ )
            {
               //for ( j=-overflowRad; j<=overflowRad; j++ )
              for(j=0; j < 1; j++)
               {
                   p2 = sumPosInt(highest, {i,j,0});
                   if(isInBounds(p2) && getTile(p2)->propmask & TP_WATER)
                   {
                       printf("found water at %d %d %d\n",p2.x,p2.y,p2.z);
                       PosInt high = p2;
                       while(getTile(p2)->propmask & TP_WATER)
                       {
                           high = p2;
                           p2.z ++;
                       }
                       p2 = high;
                       // make air around this point into new waterfronts
                       PosInt p3 = sumPosInt(p2,{-1,0,0});
                       if(isInBounds(p3) && getTile(p3)->propmask & TP_AIR)
                       {
                           t->flowDir = DIR_WEST;
                           waterFront.push(p3);
                       }else if(! isInBounds(p3))
                       {
                           // tell overworld to add water source to this cell's info
                           PosInt gp = getCellCoords(p3);
                           MCInfo * mci = overworld->getMCInfo(gp.x,gp.y,gp.z);
                           addWaterSource(p3,mci);
                       }
                       p3 = sumPosInt(p2,{1,0,0});
                       if(isInBounds(p3) && getTile(p3)->propmask & TP_AIR)
                       {
                           t->flowDir = DIR_EAST;
                           waterFront.push(p3);
                       }else if(! isInBounds(p3))
                       {
                           // tell overworld to add water source to this cell's info
                           PosInt gp = getCellCoords(p3);
                           MCInfo * mci = overworld->getMCInfo(gp.x,gp.y,gp.z);
                           addWaterSource(p3,mci);
                       }
                       p3 = sumPosInt(p2,{0,-1,0});
                       if(isInBounds(p3) && getTile(p3)->propmask & TP_AIR)
                       {
                           t->flowDir = DIR_SOUTH;
                           waterFront.push(p3);
                       }else if(! isInBounds(p3))
                       {
                           // tell overworld to add water source to this cell's info
                           PosInt gp = getCellCoords(p3);
                           MCInfo * mci = overworld->getMCInfo(gp.x,gp.y,gp.z);
                           addWaterSource(p3,mci);
                       }
                       p3 = sumPosInt(p2,{0,1,0});
                       if(isInBounds(p3) && getTile(p3)->propmask & TP_AIR)
                       {
                           t->flowDir = DIR_NORTH;
                           waterFront.push(p3);
                       }else if(! isInBounds(p3))
                       {
                           // tell overworld to add water source to this cell's info
                           PosInt gp = getCellCoords(p3);
                           MCInfo * mci = overworld->getMCInfo(gp.x,gp.y,gp.z);
                           addWaterSource(p3,mci);
                       }
                   }
               }
            }
        }
    }
    
}

void Game::moveCells(PosInt currentCellCoords)
{
    // offload old cells, load new cells
    printf("player at cell %d %d %d, UN/LOAD CELLS\n",currentCellCoords.x,currentCellCoords.y,currentCellCoords.z);
    
    int i, j;
    
    // move cells in local matrix
    if(cellCoords.x < currentCellCoords.x) // moved right
    {
        // unload cell[0][*][*]
        for(i=0;i<3;i++)
            for(j=0;j<3;j++)
            {
                cell[0][i][j]->unload(world);
                delete cell[0][i][j];
            }
        // cell[0][*][*] = cell[1][*][*]
        for(i=0;i<3;i++)
            for(j=0;j<3;j++)
                cell[0][i][j] = cell[1][i][j];
        // cell[1][*][*] = cell[2][*][*]
        for(i=0;i<3;i++)
            for(j=0;j<3;j++)
                cell[1][i][j] = cell[2][i][j];
        // cell[2][*][*] = load 9 cells
        for(i=0;i<3;i++)
            for(j=0;j<3;j++)
            {
                cell[2][i][j] = new MapCell(cellSizeXY,cellSizeZ,currentCellCoords.x+1,currentCellCoords.y+i-1,currentCellCoords.z+j-1, overworld);
                cell[2][i][j]->load(world,currentCellCoords.x+1,currentCellCoords.y+i-1,currentCellCoords.z+j-1);
            }
    }else if(cellCoords.x > currentCellCoords.x) // moved left
    {
        // unload cell[2][*][*]
        for(i=0;i<3;i++)
            for(j=0;j<3;j++){
                cell[2][i][j]->unload(world);
                delete cell[2][i][j];
            }
        // cell[2][*][*] = cell[1][*][*]
        for(i=0;i<3;i++)
            for(j=0;j<3;j++)
                cell[2][i][j] = cell[1][i][j];
        // cell[1][*][*] = cell[0][*][*]
        for(i=0;i<3;i++)
            for(j=0;j<3;j++)
                cell[1][i][j] = cell[0][i][j];
        // cell[0][*][*] = load 9 cells
        for(i=0;i<3;i++)
            for(j=0;j<3;j++)
            {
                cell[0][i][j] = new MapCell(cellSizeXY,cellSizeZ,currentCellCoords.x-1,currentCellCoords.y+i-1,currentCellCoords.z+j-1, overworld);
                cell[0][i][j]->load(world,currentCellCoords.x-1,currentCellCoords.y+i-1,currentCellCoords.z+j-1);
            }
    }else if(cellCoords.y < currentCellCoords.y) // moved north
    {
        // unload cell[*][0][*]
        for(i=0;i<3;i++)
            for(j=0;j<3;j++)
            {
                cell[i][0][j]->unload(world);
                delete cell[i][0][j];
            }
        // cell[*][0][*] = cell[*][1][*]
        for(i=0;i<3;i++)
            for(j=0;j<3;j++)
                cell[i][0][j] = cell[i][1][j];
        // cell[*][1][*] = cell[*][2][*]
        for(i=0;i<3;i++)
            for(j=0;j<3;j++)
                cell[i][1][j] = cell[i][2][j];
        // cell[*][2][*] = load 9 cells
        for(i=0;i<3;i++)
            for(j=0;j<3;j++)
            {
                cell[i][2][j] = new MapCell(cellSizeXY,cellSizeZ,currentCellCoords.x+i-1,currentCellCoords.y+1,currentCellCoords.z+j-1, overworld);
                cell[i][2][j]->load(world,currentCellCoords.x+i-1,currentCellCoords.y+1,currentCellCoords.z+j-1);
            }
    }else if(cellCoords.y > currentCellCoords.y) // moved south
    {
        // unload cell[*][2][*]
        for(i=0;i<3;i++)
            for(j=0;j<3;j++)
            {
                cell[i][2][j]->unload(world);
                delete cell[i][2][j];
            }
        // cell[*][2][*] = cell[*][1][*]
        for(i=0;i<3;i++)
            for(j=0;j<3;j++)
                cell[i][2][j] = cell[i][1][j];
        // cell[*][1][*] = cell[*][0][*]
        for(i=0;i<3;i++)
            for(j=0;j<3;j++)
                cell[i][1][j] = cell[i][0][j];
        // cell[*][0][*] = load 9 cells
        for(i=0;i<3;i++)
            for(j=0;j<3;j++)
            {
                cell[i][0][j] = new MapCell(cellSizeXY,cellSizeZ,currentCellCoords.x+i-1,currentCellCoords.y-1,currentCellCoords.z+j-1, overworld);
                cell[i][0][j]->load(world,currentCellCoords.x+i-1,currentCellCoords.y-1,currentCellCoords.z+j-1);
            }
    }else if(cellCoords.z < currentCellCoords.z) // moved up
    {
        // unload cell[*][*][0]
        for(i=0;i<3;i++)
            for(j=0;j<3;j++)
            {
                cell[i][j][0]->unload(world);
                delete cell[i][j][0];
            }
        // cell[*][*][0] = cell[*][*][1]
        for(i=0;i<3;i++)
            for(j=0;j<3;j++)
                cell[i][j][0] = cell[i][j][1];
        // cell[*][*][1] = cell[*][*][2]
        for(i=0;i<3;i++)
            for(j=0;j<3;j++)
                cell[i][j][1] = cell[i][j][2];
        // cell[*][*][2] = load 9 cells
        for(i=0;i<3;i++)
            for(j=0;j<3;j++)
            {
                cell[i][j][2] = new MapCell(cellSizeXY,cellSizeZ,currentCellCoords.x+i-1,currentCellCoords.y+j-1,currentCellCoords.z+1, overworld);
                cell[i][j][2]->load(world,currentCellCoords.x+i-1,currentCellCoords.y+j-1,currentCellCoords.z+1);
            }
    }else if(cellCoords.z > currentCellCoords.z) // moved down
    {
        // unload cell[*][*][2]
        for(i=0;i<3;i++)
            for(j=0;j<3;j++)
            {
                cell[i][j][2]->unload(world);
                delete cell[i][j][2];
            }
        // cell[*][*][2] = cell[*][*][1]
        for(i=0;i<3;i++)
            for(j=0;j<3;j++)
                cell[i][j][2] = cell[i][j][1];
        // cell[*][*][1] = cell[*][*][0]
        for(i=0;i<3;i++)
            for(j=0;j<3;j++)
                cell[i][j][1] = cell[i][j][0];
        // cell[*][*][0] = load 9 cells
        for(i=0;i<3;i++)
            for(j=0;j<3;j++)
            {
                cell[i][j][0] = new MapCell(cellSizeXY,cellSizeZ,currentCellCoords.x+i-1,currentCellCoords.y+j-1,currentCellCoords.z-1, overworld);
                cell[i][j][0]->load(world,currentCellCoords.x+i-1,currentCellCoords.y+j-1,currentCellCoords.z-1);
            }
    }
    
    // shift the current cell coords
    cellCoords = currentCellCoords;
}

int Game::getHungerLevel(float hunger)
{
    if(hunger < 0.125)
        return 3;
    else if(hunger < 0.5)
        return 2;
    else if(hunger < 1)
        return 1;
    else
        return 0;
    
}
int Game::getThirstLevel(float thirst)
{
   // printf("get thirst level %f\n",thirst);
    if(thirst < 0.125)
        return 3;
    else if(thirst < 0.5)
        return 2;
    else if(thirst < 0.75)
        return 1;
    else
        return 0;
}

int Game::getOxygenLevel(float oxygen)
{
    if(oxygen < 0.125)
        return 0;
    else if(oxygen < 0.5)
        return 1;
    else if(oxygen < 0.75)
        return 2;
    else
        return 3;
}

void Game::lookReport()
{
    char s[100];
    if(look == 0) // detailed character report
    {
        sprintf(s,"You examine yourself. You see a shell of a human being.");
        addToLog(s);
        if(pc.armour < 3)
        {
            sprintf(s,"Your clothes are torn.");
            addToLog(s);
        }
        if(pc.bruise > 0)
        {
            sprintf(s,"Your body is bruised and sore.");
            addToLog(s);
        }
        if(pc.bleed > 0)
        {
            sprintf(s,"You are bleeding.");
            if(pc.bleed > 5)
                sprintf(s,"You are bleeding a lot.");
            if(pc.bleed > 10)
                sprintf(s,"You are bleeding to death.");
            addToLog(s);
        }
        
        switch(pc.hungerLevel)
        {
            case 3: sprintf(s,"Your stomach is content. %f",pc.hunger); break;
            case 2: sprintf(s,"You feel like you could eat. %f",pc.hunger); break;
            case 1: sprintf(s,"You are hungry. %f",pc.hunger); break;
            case 0: sprintf(s,"You are starving. %f",pc.hunger); break;
            default: sprintf(s,"Your stomach feels wrong.");
        }
        /*
        if(pc.hunger < 0.125)
            sprintf(s,"Your stomach is content. %f",pc.hunger);
        else if(pc.hunger < 0.5)
            sprintf(s,"You feel like you could eat. %f",pc.hunger);
        else if(pc.hunger < 1)
            sprintf(s,"You are hungry. %f",pc.hunger);
        else
            sprintf(s,"You are starving. %f",pc.hunger);*/
        addToLog(s);
        
        switch(pc.thirstLevel)
        {
            case 3: sprintf(s,"You feel hydrated. %f",pc.thirst); break;
            case 2: sprintf(s,"You feel a little thirsty. %f",pc.thirst); break;
            case 1: sprintf(s,"You are thirsty. %f",pc.thirst); break;
            case 0: sprintf(s,"You are parched. %f",pc.thirst); break;
            default: sprintf(s,"Your throat feels wrong.");
        }/*
        if(pc.thirst < 0.125)
            sprintf(s,"You feel hydrated. %f",pc.thirst);
        else if(pc.thirst < 0.5)
            sprintf(s,"You feel a little thirsty. %f",pc.thirst);
        else if(pc.thirst < 0.75)
            sprintf(s,"You are thirsty. %f",pc.thirst);
        else
            sprintf(s,"You are parched. %f",pc.thirst);*/
        addToLog(s);
        
    }else     // check what's in lookAt, do addToLog with look strings for tile, entities.
    {
        int ent;
        for(ent = 0; ent < maxEntities; ent++)
        {
            if(ent == look)
                continue;
            if(world->mask[ent] & COMP_POSITION)
            {
                if(isEqPosInt(Float32PosInt(world->position[ent]),lookAt))
                {
                    sprintf(s,"You see %s.", world->is_visible[ent].lookString);
                    addToLog(s);
                }
            }
        }
        MapTile * tile = getTile(lookAt);
        sprintf(s,"You see %s. %s.",tile->mat_name,tile->mat_description);
        addToLog(s);
        //sprintf(s,"%s");
    }
}

void Game::toggleCloseEyes()
{
    if(eyesOpen)
    {
        addToLog("You close your eyes");
        eyesOpen = 0;
    }else {
        addToLog("You open your eyes");
        eyesOpen = 1;
    }
}

void Game::toggleGhost()
{
    if(world->move_type[player] == MOV_FLOAT)
    {
        printf("ghost mode off");
        world->move_type[player] = MOV_FREE;
    }else{
        printf("ghost mode on");
        world->move_type[player] = MOV_FLOAT;
    }
}

void Game::toggleLook()
{
    if(look == 0) // turn on look mode
    {
        printf("look mode on\n");
        look = world->createEntity();
        world->mask[look] = COMP_IS_PLAYER_CONTROLLED | COMP_CAN_MOVE | COMP_POSITION | COMP_IS_VISIBLE;
        world->is_visible[look].tex = 3;
        world->is_visible[look].tex_side = 3;
        world->position[look] = world->position[player];
        world->velocity[look] = world->velocity[player];
       // printf("%d %d %d\n",(int)(world->position[look].x+0.5),(int)(world->position[look].y+0.5),(int)(world->position[look].z+0.5));
        // remove player control from player
        world->mask[player] = world->mask[player] ^ COMP_IS_PLAYER_CONTROLLED;
        world->move_type[look] = MOV_FLOAT;
    }else {
        printf("look mode off\n");
        world->destroyEntity(look);
        look = 0;        
        world->mask[player] = world->mask[player] | COMP_IS_PLAYER_CONTROLLED;
    }
}

float Game::toggleClimb()
{
    float time = 0;
    if(!(world->move_type[player] & MOV_CLIMB))
    {
        if(pc.energy < 0)
        {
            addToLog("You don't have the energy to grab a hold");
            return 0;
        }
        // check surrounding tiles. Is there a grippable one? If so, grip it.
        PosInt p = Float32PosInt(world->position[player]);
        if( movement->isClimbable(p))
        {
            if( normFloat3(world->velocity[player]) > 2)
            {
                float v = normFloat3(world->velocity[player]);
                addToLog("You're falling too fast, but manage to slow down.");
                // v = v^ * 0.5*|v|
                world->velocity[player] = mulFloat3(normaliseFloat3(world->velocity[player]),v*0.5);
                pc.energy -= 20*v;
                time += 1;
            }else {
                world->move_type[player] = MOV_CLIMB;
                world->position[player] = PosInt2Float3(p);
                world->velocity[player] = {0,0,0};
                addToLog("You start to climb");
                pc.hasClimbed = 0;
                time += 1;
            }
        }else
        {
            addToLog("You can't find anything to grip");
            time += 1;
        }
    }else
    {
        addToLog("You let go");
        world->move_type[player] = MOV_FREE;
        time += 1;
    }
    return time;
}

entity_t Game::createLitFlare(Float3 pos)
{
    entity_t flare = world->createEntity();
    world->mask[flare] = COMP_POSITION | COMP_IS_VISIBLE | COMP_OMNILIGHT | COMP_CAN_MOVE | COMP_COUNTER | COMP_PICKABLE;
    world->light_source[flare].brightness = 5.0;
    world->light_source[flare].color = {1,1,1};
    world->is_visible[flare].tex = 4;
    world->is_visible[flare].tex_side = 4;
    sprintf(world->is_visible[flare].lookString,"a lit flare");
    world->position[flare] = pos;
    world->velocity[flare] = {0,0,0};
    world->move_type[flare] = MOV_FREE;
    world->counter[flare].type = CNT_FLARE;
    world->counter[flare].max = 25; //lasts for 25 steps
    world->counter[flare].count = world->counter[flare].max;
    world->counter[flare].on = 1;
    sprintf(world->pickable[flare].name,"a lit flare");
    world->pickable[flare].maxStack = 1;
    world->pickable[flare].stack = 1;
    world->pickable[flare].type = ITM_FLARE;
    return flare;
}

float Game::wieldFlare(entity_t ent)
{
    if(world->mask[ent] & COMP_PICKABLE && world->pickable[ent].type == ITM_FLARE && world->pickable[ent].stack > 0)
    {
        world->pickable[ent].stack --;
        if(world->pickable[ent].stack < 1)
            world->destroyEntity(ent);
        // create a wielded omnilight
        entity_t flare = createLitFlare(world->position[player]);
        world->mask[flare] = (world->mask[flare] | COMP_OWNED) & ~ COMP_IS_VISIBLE;
        world->move_type[flare] = MOV_WIELDED;
        addToLog("You wield a flare");
        return 1;
    }
    return 0;
}
float Game::dropFlare()
{
    // find a flare
    int ent;
    for(ent=0; ent < maxEntities; ent++)
    {
        if(world->mask[ent] & COMP_PICKABLE && world->pickable[ent].type == ITM_FLARE && world->pickable[ent].stack > 0)
        {
            world->pickable[ent].stack --;
            if(world->pickable[ent].stack < 1)
                world->destroyEntity(ent);
            // create a dropped omnilight
            createLitFlare(world->position[player]);
            //world->move_type[flare] = MOV_WIELDED;
            addToLog("You drop a flare");
            return 1;
        }
    }
    return 0;
}
float Game::throwFlare()
{
    // find a flare
    int ent;
    for(ent=0; ent < maxEntities; ent++)
    {
        if(world->mask[ent] & COMP_PICKABLE && world->pickable[ent].type == ITM_FLARE && world->pickable[ent].stack > 0)
        {
            world->pickable[ent].stack --;
            if(world->pickable[ent].stack < 1)
                world->destroyEntity(ent);
            entity_t flare = createLitFlare(world->position[player]);
            Float3 delta = diffFloat3(PosInt2Float3(lookAt), world->position[player]);
            float len = fmin(normFloat3(delta), 2.5); //TODO: use player's strength stat
            delta = mulFloat3(normaliseFloat3(delta),len);
            world->velocity[flare] = {delta.x,delta.y,delta.z};
            addToLog("You throw a flare");
            return 1;
        }
    }
    return 0;
}

void Game::getFlashlight()
{
    entity_t light = world->createEntity();
    world->mask[light] = COMP_OWNED | COMP_PICKABLE | COMP_COUNTER;
    sprintf(world->pickable[light].name,"Flashlight");
    world->pickable[light].type = ITM_LIGHT;
    world->pickable[light].stack = 1;
    world->pickable[light].maxStack = 1;
    world->counter[light].max = 100;
    world->counter[light].count = 100;
    world->counter[light].on = 0;
    world->counter[light].type = CNT_ELECTRIC;
}

void Game::getFlute()
{
    entity_t flute = world->createEntity();
    world->mask[flute] = COMP_OWNED | COMP_PICKABLE;
    sprintf(world->pickable[flute].name,"Flute");
    world->pickable[flute].type = ITM_INSTRUMENT;
    world->pickable[flute].stack = 1;
    world->pickable[flute].maxStack = 1;
}

void Game::getAnchors()
{
    int numAnchors = 6;
    entity_t anch = world->createEntity();
    world->mask[anch] = COMP_OWNED | COMP_PICKABLE | COMP_ANCHOR;
    sprintf(world->pickable[anch].name,"Anchor");
    world->pickable[anch].type = ITM_ANCHOR;
    world->pickable[anch].stack = numAnchors;
    world->pickable[anch].maxStack = 10;
}

void Game::getStartingFoods()
{
    //Two bars of chocolate
    entity_t ent = world->createEntity();
    world->mask[ent] = COMP_OWNED | COMP_PICKABLE | COMP_IS_EDIBLE;
    sprintf(world->pickable[ent].name,"Chocolate");
    world->pickable[ent].type = ITM_CHOCOLATE;
    world->is_visible[ent].tex = 33;
    world->is_visible[ent].tex_side = 33;
    world->pickable[ent].stack = 2;
    world->pickable[ent].maxStack = 10;
    world->edible[ent].calories = 300;
    world->edible[ent].moodMod = 45;
    world->edible[ent].quench = -.05;
    world->edible[ent].bottle = 0;

    // One big sandwich
    ent = world->createEntity();
    world->mask[ent] = COMP_OWNED | COMP_PICKABLE | COMP_IS_EDIBLE;
    sprintf(world->pickable[ent].name,"Big Tuna Sandwich");
    world->is_visible[ent].tex = 34;
    world->is_visible[ent].tex_side = 34;
    world->pickable[ent].type = ITM_CHOCOLATE;
    world->pickable[ent].stack = 1;
    world->pickable[ent].maxStack = 1;
    world->edible[ent].calories = 1200;
    world->edible[ent].moodMod = 30;
    world->edible[ent].quench = -.1;
    world->edible[ent].bottle = 0;
    
    // Bottle of apple juice
    ent = world->createEntity();
    world->mask[ent] = COMP_OWNED | COMP_PICKABLE | COMP_IS_EDIBLE;
    sprintf(world->pickable[ent].name,"Apple Juice");
    world->is_visible[ent].tex = 35;
    world->is_visible[ent].tex_side = 35;
    world->pickable[ent].type = ITM_CHOCOLATE;
    world->pickable[ent].stack = 1;
    world->pickable[ent].maxStack = 1;
    world->edible[ent].calories = 50;
    world->edible[ent].moodMod = 20;
    world->edible[ent].quench = 2;
    world->edible[ent].bottle = 1;
}
void Game::getFlares(int num)
{
    int i;
    entity_t flares = 0;
    for(i=0; i<maxEntities; i++)
    {
        if(world->mask[i] & (COMP_OWNED))
        {
            if(world->pickable[i].type != ITM_FLARE)
                continue;
            if(world->pickable[i].stack + num <= world->pickable[i].maxStack)
            {
                flares = i;
                break;
            }
        }
    }
    if(! flares )
    {
        flares = world->createEntity();
        world->mask[flares] = COMP_OWNED | COMP_PICKABLE;
        sprintf(world->pickable[flares].name,"Flares");
        world->pickable[flares].type = ITM_FLARE;
        world->pickable[flares].stack = 0;
        world->pickable[flares].maxStack = 5;
    }
    world->pickable[flares].stack += num;
}
void Game::getBandages(int num)
{
    int i;
    entity_t bandages = 0;
    for(i=0; i<maxEntities; i++)
    {
        if(world->mask[i] & (COMP_OWNED))
        {
            if(world->pickable[i].type == ITM_BANDAGE && world->pickable[i].stack + num <= world->pickable[i].maxStack)
            {
                bandages = i;
                break;
            }
        }
    }
    if(! bandages )
    {
        bandages = world->createEntity();
        world->mask[bandages] = COMP_BANDAGE | COMP_OWNED | COMP_PICKABLE;
        sprintf(world->pickable[bandages].name,"Bandages");
        world->pickable[bandages].type = ITM_BANDAGE;
        world->pickable[bandages].stack = 0;
        world->pickable[bandages].maxStack = 10;
    }
    world->pickable[bandages].stack += num;
}

float Game::useBandage()
{
    // find a flare
    int ent;
    for(ent=0; ent < maxEntities; ent++)
    {
        if(world->mask[ent] & COMP_PICKABLE && world->pickable[ent].type == ITM_BANDAGE && world->pickable[ent].stack > 0)
        {
            return useBandage(ent);
        }
    }
    return 0;
}

float Game::useBandage(entity_t bandage)
{
    if(pc.bleed > 0)
    {
        addToLog("You use a bandage");

        world->pickable[bandage].stack --;
        if(world->pickable[bandage].stack < 1)
            world->destroyEntity(bandage);
        pc.bleed = fmax(0,pc.bleed - 10);
        if(pc.bleed == 0)
            addToLog("You manage to stop the bleeding");
        return 1;
    }
    return 0;
}

float Game::eat(entity_t food)
{
    // decide how much of the food you're gonna eat. Man, how?
    // maybe don't eat unless hungry/thirsty
    if(world->edible[food].calories > 0 && pc.hungerLevel == 3 && world->edible[food].quench <= 0)
    {
        addToLog("You're not hungry.");
        return 0;
    }
    if(world->edible[food].quench > 0 && pc.thirstLevel == 3)
    {
        addToLog("You're not thirsty.");
        return 0;
    }
    
    char s[32];
    sprintf(s,"You consume the %s", world->pickable[food].name);
    addToLog(s);
    
    pc.hunger -= (world->edible[food].calories/2400.0f);
    pc.mood += (world->edible[food].moodMod);
    pc.thirst -= world->edible[food].quench;
    world->pickable[food].stack --;
    if(world->pickable[food].stack < 1)
    {
        if(world->edible[food].bottle > 0)
        {
            //TODO: make entity into empty bottle
            world->mask[food] = COMP_OWNED | COMP_PICKABLE;
            world->pickable[food].type = ITM_BOTTLE;
            sprintf(world->is_visible[food].lookString,"empty bottle");
            sprintf(world->pickable[food].name,"empty bottle");
            world->is_visible[food].tex = 39;
            world->is_visible[food].tex_side = 39;
        }else
            world->destroyEntity(food);
    }
    return 1;
}

float Game::eatSlime(entity_t slime)
{
    float t = eat(slime);
    if(t > 0)
        switch(world->slime[slime])
        {
            case SLM_LUMO:
                world->mask[player] = world->mask[player] | COMP_OMNILIGHT;
                world->light_source[player].brightness += world->light_source[slime].brightness;
                world->light_source[player].color = world->light_source[slime].color;
                addToLog("Your skin glows.");
                break;
            default:
                printf("ate an unimplemented slime\n");
        }
    return t;
}

float Game::useBottle(entity_t bottle)
{
    addToLog("You try to use the bottle");
    return 1;
}
    
float Game::useAnchor(entity_t anchor)
{
    // TODO: if next to solid
    
    // TODO: if not already an anchor here
    
    world->pickable[anchor].stack --;
    if(world->pickable[anchor].stack < 1)
        world->destroyEntity(anchor);

    addToLog("You set an anchor");
    entity_t a = world->createEntity();
    world->mask[a] = COMP_ANCHOR | COMP_IS_VISIBLE | COMP_POSITION;
    sprintf(world->is_visible[a].lookString,"Anchor");
    world->is_visible[a].tex = 267;
    world->is_visible[a].tex_side = 267;
    world->position[a] = world->position[player];
    return 1;
}

float Game::playInstrument(entity_t inst)
{
    char s[64];
    
    float success = frand(0,1);
    
    // modify success by current mood. Mood is ..0 to 100..
    // flute should be effective when mood is not great, but not terrible.
    float moodMod = fmin(pc.mood/50,2);
    success = success * moodMod;
    if(success > 0.9)
    {
        sprintf(s,"The tones of your %s harmonize with their twisted echoes",world->pickable[inst].name);
        pc.mood += 10;
    }else if(success > 0.3)
    {
        sprintf(s,"You play your %s",world->pickable[inst].name);
        pc.mood += 1;
    }else
    {
        sprintf(s,"The echoes of your %s sound like tortured souls",world->pickable[inst].name);
        pc.mood -= 1;
    }
    addToLog(s);
    return 10;
}

float Game::toggleLight(entity_t light)
{
    if(world->counter[light].on) // if on, turn off
    {
        char s[32];
        sprintf(s,"The %s switches off.",world->pickable[light].name);
        addToLog(s);
        world->mask[light] -= (COMP_DIRLIGHT + COMP_CAN_MOVE);
        world->counter[light].on = 0;
    }else if(world->counter[light].count > 0) // if not empty, turn on
    {
        char s[32];
        sprintf(s,"The %s turns on.",world->pickable[light].name);
        addToLog(s);

        world->mask[light] += (COMP_DIRLIGHT + COMP_CAN_MOVE);
        world->counter[light].on = 1;
        world->light_source[light].brightness = 20;
        world->light_source[light].color = {1,1,1};
        world->light_source[light].width = 15; // +-15°
        world->move_type[light] = MOV_WIELDED;
    }else
    {
        char s[32];
        sprintf(s,"The %s has no power.",world->pickable[light].name);
        addToLog(s);

    }
    return 0;
}

float Game::jump()
{
    // no double-jump! Can't jump if you're free falling
    if(world->move_type[player] & MOV_FREE)
        return 0;
    
    addToLog("You jump");
    
    // jumping costs energy. Should cost according to velocity...
    pc.energy -= 20;
    
    // if standing about, jump up
    if(look == 0)
    {
        world->velocity[player] = {0,0,1.6};
        world->move_type[player] = MOV_FREE;
        return 1;
    }
    // if Looking at a point, jump there
    // move player, look to center of her position
    world->position[player] = PosInt2Float3_rounded(Float32PosInt(world->position[player]));
    world->position[look] = PosInt2Float3_rounded(Float32PosInt(world->position[look]));
    
    world->move_type[player] = MOV_FREE;
    Float3 delta = diffFloat3(world->position[look], world->position[player]);
    float len = fmin(normFloat3(delta), 1.6); //TODO: use player's str or jump stat
    delta = mulFloat3(normaliseFloat3(delta),len);
    world->velocity[player] = {delta.x,delta.y,delta.z};
    
    // turn off Look mode
    toggleLook();
    return 1;
}

void Game::collision(entity_t ent, float dV)
{
    //printf("collision %d %f\n", ent, dV);
    // right now, player is the only entity that cares about this.
    //TODO: Should make health energy etc a bunch of components.
    if(ent == player)
    {
        if( dV > 2 ) // TODO: relate damage cutoff to "toughness" or something
        { /** You take a hit. get a damage value
           — If your armour value > damage, armour takes the damage (“clothes get ripped”).
           — else if damage is low and bruising < maxEnergy, you get a bruise. maxEnergy is decreased.
           — else if damage is high, you get a bleeding injury. recovery is decreased, possibly into the negative.
           */
            
            float damage = dV-1;
            if(pc.armour > damage)
            {
                pc.armour -= damage;
                addToLog("You land badly and rip your clothes");
            }else if( damage < 2.5 && pc.bruise + damage < pc.maxEnergy) //TODO: relate this "2.5" to "skin toughness"
            {
                pc.bruise += damage*5;
                addLabel("Bruise",2,{1,.5,0});
                addToLog("You land badly and knock the wind out of you");
            }else{
                addToLog("You get a bad scrape");
                addLabel("Cut",2,{1,0,0});
                pc.bleed += damage*3;
            }
        }
    }
}

//TODO: call this on a window draw event, not on game update loop
void Game::updateLabels(float timeStep)
{
   // printf("updatelabel %f\n",timeStep);
    entity_t ent;
    
    for(ent = 0; ent<maxEntities; ent++)
    {
        if(world->mask[ent] & COMP_LABEL)
        {
            if(world->counter[ent].on > 0)
            {
               // printf("updatelabel %f - %f\n",world->counter[ent].count, timeStep);
                world->counter[ent].count -= timeStep;
                if(world->counter[ent].count <= 0) // counter runs out!
                {
                    world->destroyEntity(ent);
                }
            }
        }
    }
}

void Game::updateCounters(float timeStep)
{
    entity_t ent;
    
    for(ent = 0; ent<maxEntities; ent++)
    {
        if(world->mask[ent] & COMP_COUNTER)
        {
            if(world->mask[ent] & COMP_LABEL)
                continue;
            if(world->counter[ent].on > 0)
            {
                world->counter[ent].count -= timeStep;
                if(world->counter[ent].count <= 0) // counter runs out!
                {
                    switch(world->counter[ent].type)
                    {
                        case CNT_FLARE: world->destroyEntity(ent); break;
                        case CNT_ELECTRIC: toggleLight(ent); break; // TODO: generalise pls
                    }
                }
            }
        }
    }
}

void Game::heal(float timeStep)
{
    float h = fmax(0,pc.healing*timeStep*(1-fmin(1,pc.hunger)));
    float amtHealed = 0;
    // if you're bleeding, there's a chance to heal the bleeding. This is shown by th emission of a spot of blood.
    if(pc.bleed > 0 && frand(0,10) < pc.bleed)
    {
        pc.bleed = fmax(0,pc.bleed - h);
        if(pc.bleed == 0)
        {
            addToLog("You stop bleeding");
        }
        //TODO: this is the ugliest blood I've ever seen.
        else if( world->move_type[player] & MOV_WALK){
            entity_t blood = world->createEntity();
            world->mask[blood] = COMP_POSITION | COMP_IS_VISIBLE | COMP_CAN_MOVE;
            world->position[blood] = world->position[player];
            world->is_visible[blood].tex = 5;
            world->is_visible[blood].tex_side = 6;
            sprintf(world->is_visible[blood].lookString,"a puddle of blood");
            world->move_type[blood] = MOV_FREE;
        }
        amtHealed = h;
    }else {
        pc.bruise = fmax(0,pc.bruise - h);
        amtHealed = h;
    }
    float hungerGrowth = amtHealed/100.00;
    pc.hunger += hungerGrowth;
}

char* Game::getMoodDescription(float mood)
{
    char* m = moodDesc;
    if(mood < 0)
    {
        sprintf(m,"Despairing");
        pc.moodLevel = 0;
    }else if(mood < 10)
    {
        sprintf(m,"Despondent");
        pc.moodLevel = 1;
    }else if(mood < 25)
    {
        sprintf(m,"Miserable");
        pc.moodLevel = 2;
    }else if(mood < 40)
    {
        sprintf(m,"Unhappy");
        pc.moodLevel = 3;
    }else if(mood < 60)
    {
        sprintf(m,"Managing");
        pc.moodLevel = 4;
    }else if(mood < 85)
    {
        sprintf(m,"Ok");
        pc.moodLevel = 5;
    }else if(mood < 100)
    {
        sprintf(m,"Focused");
        pc.moodLevel = 6;
    }else if(mood < 120)
    {
        sprintf(m,"Determined");
        pc.moodLevel = 7;
    }else if(mood < 200)
    {
        pc.moodLevel = 8;
        sprintf(m,"Thriving");
    }else
    {
        pc.moodLevel = 9;
        sprintf(m,"Euphoric");
    }
    
    return m;
}

/***** DEPRECATED ******/
float Game::lighting(Float3 p)
{
    // find ents with the right components
    entity_t ent;
    
    p = sumFloat3(p, {0,0,.5});
    int mask = COMP_OMNILIGHT | COMP_DIRLIGHT;
    float light = 0;
 //   printf("\t%f %f %f\n",p.x,p.y,p.z);
    
    for(ent = 0; ent<maxEntities; ent++)
    {
        if(world->mask[ent] & mask)
        {
            // check distance and visibility to this entity
            Float3 sourcePos = {world->position[ent].x,world->position[ent].y,world->position[ent].z};
            sourcePos = sumFloat3(sourcePos, {0.5,0.5,0.5});
   
            // if directional, check whether it's within light cone
            if(world->mask[ent] & COMP_DIRLIGHT)
            {
                // direction between point p and the light's position
                Float3 pointDir = normaliseFloat3(diffFloat3(p, sourcePos));
                
                // all dir-lights have the player's direction? yeah for now why not.
                // TODO: implement dirlights with their own direction
                Float3 lightDir = {0,0,0};
                if(look != 0) // if player is Looking about
                {
                    printf("foo\n");
                    Float3 la = PosInt2Float3(lookAt);
                    la.z -= 0.5;
                    lightDir = normaliseFloat3(diffFloat3({la.x,la.y,la.z}, world->position[player]));
                    if(isnan(lightDir.x) || isnan(lightDir.y) || isnan(lightDir.z))
                    {
                        lightDir.x = 0;lightDir.y = 0; lightDir.z = -1;
                    }
                
                }else
                {
                    printf("bar\n");
                    // light direction from player's facing
                    lightDir = normaliseFloat3(pc.facing);
                    if(isnan(lightDir.x) || isnan(lightDir.y) || isnan(lightDir.z))
                    {
                        lightDir.x = 0;lightDir.y = 0; lightDir.z = -1;
                    }
                }
                
                // TODO: fix for short ranges? point down more when shining short distance at empty air?
                //lightDir = normaliseFloat3({lightDir.x,lightDir.y,(float)(lightDir.z-0)});
                // what is the angle between? arccos( v1 dot v2 )
                float dot = dotFloat3(lightDir, pointDir);
                if(dot <= 0)
                    continue;
                float angle = fabs(acosf(dot)*180.0/M_PI); // in degrees
                if(angle > world->light_source[ent].width)
                    continue;
            }
            
            if(pointVisibility(sourcePos, p) \
                || pointVisibility(sumFloat3(sourcePos,{-0.45,0,0}),p) \
                || pointVisibility(sumFloat3(sourcePos,{0,-0.45,0}),p) \
                || pointVisibility(sumFloat3(sourcePos,{0.45,0,0}),p) \
                || pointVisibility(sumFloat3(sourcePos,{0,0.45,0}),p) \
                || pointVisibility(sumFloat3(sourcePos,{0,0,-0.25}),p) \
                || pointVisibility(sumFloat3(sourcePos,{0,0,0.45}),p) \
               )
            {
                float distance = distFloat3(sourcePos, p);
                light += world->light_source[ent].brightness / (distance*distance);
     //           printf("\tdist %f, light %f\n",distance,light);
            }
            //TODO (for better vertical coverage): try shifting sourcePos around north/south and east/west and retesting
        }
    }
    return light;
}

Float3 Game::lighting3f(Float3 p)
{
#ifdef OMNISIGHT
    return {1,1,1};
#endif
    Float3 light = {0,0,0};
    // find ents with the right components
    entity_t ent;
    
    p = sumFloat3(p, {0,0,.5});
    int mask = COMP_OMNILIGHT | COMP_DIRLIGHT;
    for(ent = 0; ent<maxEntities; ent++)
    {
        if(world->mask[ent] & mask)
        {
            // check distance and visibility to this entity
            Float3 sourcePos = {world->position[ent].x,world->position[ent].y,world->position[ent].z};
            sourcePos = sumFloat3(sourcePos, {0.5,0.5,0.5});
            
            //if source is very far and quite weak, abort here.
            float distance = distFloat3(sourcePos, p);
            float l = world->light_source[ent].brightness / (distance*distance);
            
            if(l < 0.05)
                continue;
            
            float close = 1.5;
            
            // if directional, check whether it's within light cone and beyond "close" distance
            if(world->mask[ent] & COMP_DIRLIGHT && distance > close)
            {
                //sumFloat3(p,{0,0,-0.5});
                // direction between point p and the light's position
                Float3 pointDir = normaliseFloat3(diffFloat3(p, sourcePos));
                
                // all dir-lights have the player's direction? yeah for now why not.
                // TODO: implement dirlights with their own direction
                Float3 lightDir = {0,0,0};
                if(look != 0) // if player is Looking about
                {
                    Float3 la = PosInt2Float3(lookAt);
                    //la.z += 0.5;
                    lightDir = normaliseFloat3(diffFloat3({la.x,la.y,la.z}, world->position[player]));
                    if(isnan(lightDir.x) || isnan(lightDir.y) || isnan(lightDir.z))
                    {
                        lightDir.x = 0;lightDir.y = 0; lightDir.z = -1;
                    }
                    
                }else
                {
                    // light direction from player's facing
                    lightDir = normaliseFloat3(pc.facing);
                    if(isnan(lightDir.x) || isnan(lightDir.y) || isnan(lightDir.z))
                    {
                        lightDir.x = 0;lightDir.y = 0; lightDir.z = -1;
                    }
                }
                
                lightDir = normaliseFloat3({lightDir.x,lightDir.y,(float)(lightDir.z-0)});
                // what is the angle between? arccos( v1 dot v2 )
                float dot = dotFloat3(lightDir, pointDir);
                if(dot <= 0)
                    continue;
                float angle = fabs(acosf(dot)*180.0/M_PI); // in degrees
                
                // hacky fix for short distances. Look ok tho.
                if(distance < 3)
                    angle *= 0.5;
                
                if(angle > world->light_source[ent].width)
                    continue;
            }
            
            if(pointVisibility(sourcePos, p) \
               || pointVisibility(sumFloat3(sourcePos,{-0.45,0,0}),p) \
               || pointVisibility(sumFloat3(sourcePos,{0,-0.45,0}),p) \
               || pointVisibility(sumFloat3(sourcePos,{0,0,-0.45}),p) \
               || pointVisibility(sumFloat3(sourcePos,{0.45,0,0}),p) \
               || pointVisibility(sumFloat3(sourcePos,{0,0.45,0}),p) \
               || pointVisibility(sumFloat3(sourcePos,{0,0,0.45}),p) \
               )
            {
                light = sumFloat3(light,mulFloat3(world->light_source[ent].color,l));
            }
            //TODO (for better vertical coverage): try shifting sourcePos around north/south and east/west and retesting
        }
    }
    return minFloat3({1,1,1},light);    
}

int Game::pointVisibility(Float3 f, Float3 t)
{
    Float3 delta = diffFloat3(t, f);
    float distance = normFloat3(delta);
    float stepSize = 0.2;
    delta = mulFloat3(delta, stepSize/distance);
    
    Float3 p = f;
    PosInt pi = Float32PosInt(p);
    
    int i;
    for(i=0; i<distance/stepSize-1; i++)
    {
        MapTile * tile = getTile(pi.x,pi.y,pi.z);

        //if the current tile is not transparent, end it.
        if(! (tile->propmask & TP_ISTRANSPARENT))
        {
            if(distance - i*stepSize < .5)
                return 1;

            return 0;
        }
        
        // step along the line
        p = sumFloat3(f,mulFloat3(delta, i));
        pi = Float32PosInt(p);
    }
    return 1;
}

int Game::visibility(PosInt from, PosInt to)
{
    if(! eyesOpen)
        return 0;
#ifdef OMNISIGHT
    return 1;
#endif
    if(manhattanPosInt(from, to) == 1)
        return 1;
 
    
    
    // step along several lines from from to to. check
    Float3 f = PosInt2Float3(from);
    f = sumFloat3(f, {0.5,0.5,.95});
    
    
    // top face
    Float3 t = {(float)(to.x+0.5), (float)(to.y+0.5), (float)(to.z+1)};
    if(pointVisibility(f,t))
        return 1;
    
    // bottom face
    t = {(float)(to.x+0.5), (float)(to.y+0.5), (float)(to.z)};
    if(pointVisibility(sumFloat3(f, {0,0,-.5}),t))
        return 1;
    
    
    // left
    t = {(float)(to.x), (float)(to.y+0.5), (float)(to.z+.5)};
    if(pointVisibility(f,t))
        return 1;
    
    // right
    t = {(float)(to.x+1), (float)(to.y+0.5), (float)(to.z+.5)};
    if(pointVisibility(f,t))
        return 1;
    
    // north
    t = {(float)(to.x+0.5), (float)(to.y+1), (float)(to.z+.5)};
    if(pointVisibility(f,t))
        return 1;
    
    // south
    t = {(float)(to.x+0.5), (float)(to.y), (float)(to.z+.5)};
    if(pointVisibility(f,t))
        return 1;
    
    return -1;
}

// returns the coordinates of the map cell for given global coords
PosInt Game::getCellCoords(int x, int y, int z)
{
    PosInt p;
    
    // fix for -0
    if(x < 0)
        x -= cellSizeXY-1;
    if(y < 0)
        y -= cellSizeXY-1;
    if(z < 0)
        z -= cellSizeZ-1;
    
    p.x = x/cellSizeXY;
    p.y = y/cellSizeXY;
    p.z = z/cellSizeZ;
    return p;
}

// returns address of loaded mapcell at the specified global coordinates
MapCell * Game::getCellatCoords(int x, int y, int z)
{
    // cell 1 1 1 is the cell at cellcoords.
    // cell 2 1 1 is the cell at cellcoords + {1,0,0}
    // cell 1 0 1 is the cell att cellcoords + {0,-1,0}
    // got it?
    
    // cellcoord 0,0,0 spans from 0,0,0 to sizexy,sizexy,sizez
    // cellcoord 1,0,0 spans from sizexy,0,0 to sizexy*2,sizexy,sizez
    // cellcoord -1,0,0 spans from -sizexy,0,0 to 0,0,0
            
    PosInt cc = getCellCoords(x,y,z);
   // printf("%d %d %d got cc %d %d %d\n",x,y,z,cc.x,cc.y,cc.z);
    
    int cx = cc.x-cellCoords.x+1;
    int cy = cc.y-cellCoords.y+1;
    int cz = cc.z-cellCoords.z+1;
    
    if(cx < 0 || cx > 2 || cy < 0 || cy > 2 || cz < 0 || cz > 2)
    {
        //fprintf(stderr,"asking for invalid cell %d %d %d in getCellatCoords(%d %d %d),cellCoords=%d %d %d\n",cx,cy,cz,x,y,z,cellCoords.x,cellCoords.y,cellCoords.z);
        return NULL;
    }
    //printf("getCellCoords %d %d %d got cell %d %d %d\n",x,y,z,cc.x,cc.y,cc.z);
    return cell[cx][cy][cz];
}


void Game::setTileType(PosInt p, TileType tt)
{
    MapCell *c = getCellatCoords(p); // gets the cell that owns the coords
    c->setGlobalTileType(p, tt);
}
MapTile* Game::getTile(int x, int y, int z)
{
    MapCell *c = getCellatCoords(x,y,z); // gets the cell that owns the coords
    return c->getGlobalTile(x,y,z);
}
MapTile* Game::getTile(float x, float y, float z)
{
    Float3 p = {x,y,z};
    return getTile(p);
}
MapTile* Game::getTile(PosInt p)
{
    return getTile(p.x,p.y,p.z);
}
MapTile* Game::getTile(Float3 p)
{
    return getTile(Float32PosInt_rounded(p));
}

int Game::isInBounds(PosInt p)
{
    MapCell *c = getCellatCoords(p.x,p.y,p.z); // gets the cell that owns the coords
    if (c == NULL)
        return 0;
    else
        return 1;
}

float Game::wasSeen(MapTile* tile){ return tile->wasSeen;}

void Game::setWasSeen(MapTile* tile, float light)
{
    if(wasSeen(tile) < light)
        tile->wasSeen=light;
}
