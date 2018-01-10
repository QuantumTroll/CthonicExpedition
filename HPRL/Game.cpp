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
    srandom(2);
    
    // create a player character
    
    pc.name="Mary-Sue";
    pc.mood=1;
    pc.hasClimbed = 0;
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

// create a player entity
    player = world->createEntity();
    world->mask[player] = COMP_IS_PLAYER_CONTROLLED | COMP_IS_VISIBLE | COMP_CAN_MOVE | COMP_POSITION | COMP_VELOCITY | COMP_OMNILIGHT;
  //  world->is_player_controlled[player].is_player_controlled = 1;
  //  world->is_visible[player].is_visible = 1;
    world->position[player] = {(float)(cellSizeXY*2.5),(float)(cellSizeXY*2.5),(float)(cellSizeZ*28.5-2)};
    world->velocity[player] = {0,0,0};
    world->is_visible[player].tex = 266; // some thing
    world->is_visible[player].tex_side = 266; // some thing
    sprintf(world->is_visible[player].lookString,"yourself, %s",pc.name.c_str());
    world->move_type[player] = MOV_FREE; // default movement mode
    world->light_source[player].brightness = 1.5; // reduce when not debugging
    lookAt = Float32PosInt( world->position[player]);
    
    // add starting gear to inventory
    getBandages(3);
    getFlares(5);
    getStartingFoods();
    getAnchors();
    
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
            case 'w': input = KEY_NORTH; break;
            case 's': input = KEY_SOUTH; break;
            case 'a': input = KEY_WEST; break;
            case 'd': input = KEY_EAST; break;
            case '>': input = KEY_DOWN; break;
            case '<': input = KEY_UP; break;
            case 'f': input = KEY_FLARE; break;
            case 'l': input = KEY_LOOK; break;
            case 'j': input = KEY_JUMP; break;
            case '.': input = KEY_WAIT; break;
            case 'c': input = KEY_CLIMB; break;
            case 'i': input = KEY_INVENTORY; break;
            case 'b': input = KEY_BANDAGE; break;
            case 'o': input = KEY_ORIENTEER; break;
            case 'x': input = KEY_CLOSEEYES; break;
            case 'e': input = KEY_EXAMINE; break;
            default: input = KEY_NONE;
        }
    }else {
        // a menu is open. send key directly to menu handler
        currentMenu->menuHandler(this,inchar);
    }
    
    inputs.push(input);
    turnOn();
}

void Game::inventoryMenuHandler_s(void * t,char c)
{
    ((Game*)t)->inventoryMenuHandler(c);
}
void Game::inventoryMenuHandler(char c)
{
    if(c == 'i')
    {
        exitMenu();
        return;
    }
    int opt = c-'a';
    entity_t ent = currentMenu->options[opt].ent;
    
    switch (world->pickable[ent].type)
    {
        case ITM_BANDAGE: useBandage(ent); break;
        case ITM_FLARE: wieldFlare(ent); break;
        case ITM_CHOCOLATE: eat(ent); break;
        case ITM_ANCHOR: useAnchor(ent); break;
        default: printf("pushed %c, got item %s\n",c,world->pickable[ent].name);
    }
    exitMenu();
}

void Game::displayInventory()
{
    currentMenu = (Menu*) malloc(sizeof(Menu));
    currentMenu->menuHandler = &inventoryMenuHandler_s;
    sprintf(currentMenu->name,"Inventory");
    
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
void Game::doSystems()
{
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
            displayInventory();
        }else if(input & KEY_BANDAGE)
        {
            timeStep = useBandage();
        }else if(input & KEY_ORIENTEER)
        {
            char s[32];
            sprintf(s,"You are in cell %d %d %d",cellCoords.x,cellCoords.y,cellCoords.z);
            addToLog(s);
        }else if(input & KEY_CLOSEEYES)
        {
            toggleCloseEyes();
        }else if(input & KEY_EXAMINE)
        {
            lookReport();
        }
    }
    // step simulation
    // grab an event. take timestep. //TODO: fix this so it works as intended. Maybe Looking/Floating should take a teensy bit of time? Or maybe control input shouldn't be stupid and trigger spurious doSystems() calls. //update: maybe fixed now?
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
    turnOff();
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
        if(pc.hunger < 0.125)
            sprintf(s,"Your stomach is content. %f",pc.hunger);
        else if(pc.hunger < 0.5)
            sprintf(s,"You feel like you could eat. %f",pc.hunger);
        else if(pc.hunger < 1)
            sprintf(s,"You are hungry. %f",pc.hunger);
        else
            sprintf(s,"You are starving. %f",pc.hunger);
        addToLog(s);
        
        if(pc.thirst < 0.125)
            sprintf(s,"You feel hydrated. %f",pc.thirst);
        else if(pc.thirst < 0.5)
            sprintf(s,"You feel a little thirsty. %f",pc.thirst);
        else if(pc.thirst < 0.75)
            sprintf(s,"You are thirsty. %f",pc.thirst);
        else
            sprintf(s,"You are parched. %f",pc.thirst);
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

void Game::toggleLook()
{
    if(look == 0) // turn on look mode
    {
        printf("look mode on\n");
        look = world->createEntity();
        world->mask[look] = COMP_IS_PLAYER_CONTROLLED | COMP_CAN_MOVE | COMP_POSITION | COMP_VELOCITY | COMP_IS_VISIBLE;
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
    world->mask[flare] = COMP_POSITION | COMP_IS_VISIBLE | COMP_OMNILIGHT | COMP_CAN_MOVE | COMP_VELOCITY | COMP_COUNTER;
    world->light_source[flare].brightness = 5.0;
    world->is_visible[flare].tex = 4;
    world->is_visible[flare].tex_side = 4;
    sprintf(world->is_visible[flare].lookString,"a lit flare");
    world->position[flare] = pos;
    world->velocity[flare] = {0,0,0};
    world->move_type[flare] = MOV_FREE;
    world->counter[flare].type = CNT_FLARE;
    world->counter[flare].max = 25; //lasts for 25 steps
    world->counter[flare].count = world->counter[flare].max;
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
        world->mask[flare] = world->mask[flare] ^ COMP_IS_VISIBLE;
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
    world->pickable[ent].stack = 2;
    world->pickable[ent].maxStack = 10;
    world->edible[ent].calories = 300;
    world->edible[ent].moodMod = 10;
    world->edible[ent].quench = -.05;

    // One big sandwich
    ent = world->createEntity();
    world->mask[ent] = COMP_OWNED | COMP_PICKABLE | COMP_IS_EDIBLE;
    sprintf(world->pickable[ent].name,"Big Tuna Sandwich");
    world->pickable[ent].type = ITM_CHOCOLATE;
    world->pickable[ent].stack = 1;
    world->pickable[ent].maxStack = 1;
    world->edible[ent].calories = 1200;
    world->edible[ent].moodMod = 5;
    world->edible[ent].quench = -.1;
    
    // Bottle of apple juice
    ent = world->createEntity();
    world->mask[ent] = COMP_OWNED | COMP_PICKABLE | COMP_IS_EDIBLE;
    sprintf(world->pickable[ent].name,"Apple Juice");
    world->pickable[ent].type = ITM_CHOCOLATE;
    world->pickable[ent].stack = 1;
    world->pickable[ent].maxStack = 1;
    world->edible[ent].calories = 50;
    world->edible[ent].moodMod = 10;
    world->edible[ent].quench = 2;
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
    char s[32];
    sprintf(s,"You consume the %s", world->pickable[food].name);
    addToLog(s);
    pc.hunger -= (world->edible[food].calories/2400.0f);
    pc.mood += (world->edible[food].moodMod);
    pc.thirst -= world->edible[food].quench;
    world->pickable[food].stack --;
    if(world->pickable[food].stack < 1)
        world->destroyEntity(food);
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
                addToLog("You land badly and knock the wind out of you");
            }else{
                addToLog("You get a bad scrape");
                pc.bleed += damage*3;
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
            world->counter[ent].count -= timeStep;
            if(world->counter[ent].count <= 0) // counter runs out!
            {
                switch(world->counter[ent].type)
                {
                    case CNT_FLARE: world->destroyEntity(ent); break;
                    case CNT_ELECTRIC: /*TODO: turn off */; break;                    
                }
            }
        }
    }
}

void Game::heal(float timeStep)
{
    float h = fmax(0,pc.healing*timeStep*(1-pc.hunger));
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

float Game::lighting(Float3 p)
{
    // find ents with the right components
    entity_t ent;
    
    p = sumFloat3(p, {0,0,.5});
    int mask = COMP_OMNILIGHT;
    float light = 0;
 //   printf("\t%f %f %f\n",p.x,p.y,p.z);
    
    for(ent = 0; ent<maxEntities; ent++)
    {
        if(world->mask[ent] & mask)
        {
            // check distance and visibility to this entity
            Float3 sourcePos = {world->position[ent].x,world->position[ent].y,world->position[ent].z};
            sourcePos = sumFloat3(sourcePos, {0.5,0.5,0.5});
   //         printf("\tsource at %f %f %f\n",sourcePos.x,sourcePos.y,sourcePos.z);
            //int isVisible = pointVisibility(sourcePos, p);
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

int Game::pointVisibility(Float3 f, Float3 t)
{
    Float3 delta = diffFloat3(t, f);
    float distance = normFloat3(delta);
    float stepSize = 0.1;
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
    if(manhattanPosInt(from, to) == 1)
        return 1;
 
    if(! eyesOpen)
        return 0;
    
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
        fprintf(stderr,"asking for invalid cell in getCellatCoords(%d %d %d),cellCoords=%d %d %d",x,y,z,cellCoords.x,cellCoords.y,cellCoords.z);
        return NULL;
    }
    //printf("getCellCoords %d %d %d got cell %d %d %d\n",x,y,z,cc.x,cc.y,cc.z);
    return cell[cx][cy][cz];
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

float Game::wasSeen(MapTile* tile){ return tile->wasSeen;}

void Game::setWasSeen(MapTile* tile, float light)
{
    if(wasSeen(tile) < light)
        tile->wasSeen=light;
}
