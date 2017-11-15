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
    world=w; movement = new Movement(); movement->setGame(this);
    energy = new EnergySystem(world, &pc, this);
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
}

void Game::init()
{
    time = 0;
    
    // create a player entity
    player = world->createEntity();
    world->mask[player] = COMP_IS_PLAYER_CONTROLLED | COMP_IS_VISIBLE | COMP_CAN_MOVE | COMP_POSITION | COMP_VELOCITY | COMP_OMNILIGHT;
  //  world->is_player_controlled[player].is_player_controlled = 1;
  //  world->is_visible[player].is_visible = 1;
    world->position[player] = {11,14,5};
    world->velocity[player] = {0,0,0};
    world->is_visible[player].tex = 265; // some thing
    world->move_type[player] = MOV_WALK; // default movement mode
    world->light_source[player].brightness = 2.0; // reduce when not debugging
    lookAt = {0,0,0};
    
    pc.name="Mary-Sue";
    pc.mood=1;
    pc.strength = 10;
    pc.injuries=0;
    pc.maxEnergy = 100;
    pc.energy = 90;
    pc.recover = 10;
    pc.bleed = 0;
    pc.bruise = 0;
    pc.armour = 3;
    pc.healing = 0.01;
    pc.numBandages = 3;
    pc.numFlares = 5;
    pc.nuts = 5;
    pc.rope = 6.31;
    
    look = 0;
    
    //TODO: generate "overworld"?
    
    // generate map cells
    int i, j, k;
    for(i=0; i<3; i++)
        for(j=0; j<3; j++)
            for(k=0; k<3; k++)
                cell[i][j][k] = new MapCell(30,20,i-1,j-1,k-1); //30 xy, 20 z, origin.
    
    addToLog("You wake up in the test cave. There is no escape.");
    turnOn();
}

// TODO: set controls in config file instead of hard-coding here
void Game::addInput(char inchar)
{
    Key input;
    // add input to inputs queue
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
        default: input = KEY_NONE;
    }
    
    inputs.push(input);
    turnOn();
}

void Game::displayInventory()
{
    char s[64];
    sprintf(s,"# of flares: %d",pc.numFlares);
    addToLog(s);
    sprintf(s,"# of bandages: %d",pc.numBandages);
    addToLog(s);
    sprintf(s,"# of anchor nuts: %d",pc.nuts);
    addToLog(s);
    sprintf(s,"meters of rope: %.1f",pc.rope);
    addToLog(s);
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
    printf("%f %f %f\n",world->position[player].x,world->position[player].y,world->position[player].z);
    // process inputs
    while(! inputs.empty())
    {
        input = inputs.front();
        inputs.pop();
        
        if(input & (KEY_UP|KEY_DOWN|KEY_WEST|KEY_EAST|KEY_NORTH|KEY_SOUTH))
        {
         
            timeStep = movement->input(world,input);
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
            timeStep = movement->input(world, input);
        }else if(input & KEY_CLIMB)
        {
            toggleClimb();
        }else if(input & KEY_INVENTORY)
        {
            displayInventory();
        }else if(input & KEY_BANDAGE)
        {
            timeStep = useBandage();
        }
    }
    // step simulation
    // grab an event. take timestep. //TODO: fix this so it works as intended. Maybe Looking/Floating should take a teensy bit of time? Or maybe control input shouldn't be stupid and trigger spurious doSystems() calls.
    energy->exec(timeStep);
    movement->exec(world, timeStep);
    heal(timeStep);
    time += timeStep;
    
    //TODO: check if movement brought player in range of cell edge. if so, offload old and load new cells.

    // update lookAt
    if(look == 0)
    {
        lookAt.x = (int)(world->position[player].x + 0.5);
        lookAt.y = (int)(world->position[player].y + 0.5);
        lookAt.z = (int)(world->position[player].z + 0.5);
        //printf("updating lookat(player): %d %d %d\n",lookAt.x,lookAt.y,lookAt.z);
    }else
    {
        lookAt.x = (int)(world->position[look].x + 0.5);
        lookAt.y = (int)(world->position[look].y + 0.5);
        lookAt.z = (int)(world->position[look].z + 0.5);
    }
    turnOff();
}

void Game::toggleLook()
{
    if(look == 0) // turn on look mode
    {
        printf("look mode on\n");
        look = world->createEntity();
        world->mask[look] = COMP_IS_PLAYER_CONTROLLED | COMP_CAN_MOVE | COMP_POSITION | COMP_VELOCITY | COMP_IS_VISIBLE;
        world->is_visible[look].tex = 3;
        world->position[look] = world->position[player];
        world->velocity[look] = world->velocity[player];
        printf("%d %d %d\n",(int)(world->position[look].x+0.5),(int)(world->position[look].y+0.5),(int)(world->position[look].z+0.5));
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

void Game::toggleClimb()
{
    // check surrounding tiles. Is there a grippable one? If so, grip it.
    PosInt p = Float32PosInt(world->position[player]);
    
    if(!(world->move_type[player] & MOV_CLIMB))
    {
        if( movement->isClimbable(p))
        {
            world->move_type[player] = MOV_CLIMB;
            world->position[player] = PosInt2Float3(p);
            addToLog("You start to climb");
        }else
            addToLog("You can't find anything to grip");
    }else
    {
        addToLog("You let go");
        world->move_type[player] = MOV_FREE;
    }
}

float Game::dropFlare()
{
    if(pc.numFlares > 0)
    {
        entity_t flare = world->createEntity();
        world->mask[flare] = COMP_POSITION | COMP_IS_VISIBLE | COMP_OMNILIGHT | COMP_CAN_MOVE | COMP_VELOCITY;
        world->light_source[flare].brightness = 4.0;
        world->is_visible[flare].tex = 4;
        world->position[flare] = world->position[player];
        world->velocity[flare] = {0,0,0};
        addToLog("You drop a flare");
        return 1;
    }
    return 0;
}
float Game::throwFlare()
{
    if(pc.numFlares > 0)
    {
        pc.numFlares --;
        entity_t flare = world->createEntity();
        world->mask[flare] = COMP_POSITION | COMP_IS_VISIBLE | COMP_OMNILIGHT | COMP_CAN_MOVE | COMP_VELOCITY;
        world->light_source[flare].brightness = 4.0;
        world->position[flare] = world->position[player];
        world->move_type[flare] = MOV_FREE;
        world->is_visible[flare].tex = 4;
        Float3 delta = diffFloat3(PosInt2Float3(lookAt), world->position[player]);
        float len = fmin(normFloat3(delta), 4); //TODO: use player's strength stat
        delta = mulFloat3(normaliseFloat3(delta),len);
        world->velocity[flare] = {delta.x,delta.y,delta.z};
        addToLog("You throw a flare");
        return 1;
    }
    return 0;
}
float Game::useBandage()
{
    if(pc.numBandages > 0 && pc.bleed > 0)
    {
        addToLog("You use a bandage");
        pc.numBandages --;
        pc.bleed = fmax(0,pc.bleed - 10);
        if(pc.bleed == 0)
            addToLog("You manage to stop the bleeding");
        return 1;
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
        world->velocity[player] = {0,0,1.5};
        world->move_type[player] = MOV_FREE;
        return 1;
    }
    // if Looking at a point, jump there
    // move player, look to center of her position
    world->position[player] = PosInt2Float3_rounded(Float32PosInt(world->position[player]));
    world->position[look] = PosInt2Float3_rounded(Float32PosInt(world->position[look]));
    
    world->move_type[player] = MOV_FREE;
    Float3 delta = diffFloat3(PosInt2Float3(lookAt), world->position[player]);
    float len = fmin(normFloat3(delta), 1.5); //TODO: use player's str or jump stat
    delta = mulFloat3(normaliseFloat3(delta),len);
    world->velocity[player] = {delta.x,delta.y,delta.z};
    
    // turn off Look mode
    toggleLook();
    return 1;
}

void Game::collision(entity_t ent, float dV)
{
    printf("collision %d %f\n", ent, dV);
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
            }else if( damage < 3 && pc.bruise + damage < pc.maxEnergy) //TODO: relate this "3" to "skin toughness"
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

void Game::heal(float timeStep)
{
    float h = pc.healing*timeStep;
    if(pc.bleed > 0)
    {
        pc.bleed = fmax(0,pc.bleed - h);
        if(pc.bleed == 0)
            addToLog("You stop bleeding");
    }else {
        pc.bruise = fmax(0,pc.bruise - h);
    }
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
    //while(! isEqPosInt(pi, to))
    {
        //TODO: get the right cell
        //if the current cell is not transparent, end it.
        if(! (cell[1][1][1]->tiles[pi.z][pi.x][pi.y].propmask & TP_ISTRANSPARENT))
        {
          //  if(distance < 5)
          //      printf("%d %d: %f\n",pi.x,pi.y,distance - stepSize*i);
            if(distance - i*stepSize < .5)
                return 1;

            return 0;
        }
        
        // step along the line
        //p = sumFloat3(p, delta);
        p = sumFloat3(f,mulFloat3(delta, i));
        pi = Float32PosInt(p);
    }
    return 1;
}

int Game::visibility(PosInt from, PosInt to)
{
    
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
    if(pointVisibility(sumFloat3(f, {0,0,-.85}),t))
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

//TODO: make actually aware of global xyz coords
MapCell * Game::getCellCoords(int x, int y, int z)
{
    MapCell * c = cell[1][1][1];
    int sxy = c->sizexy;
    int sz = c->sizez;
    int cx = 1;
    int cy = 1;
    int cz = 1;
    if(x < 0)
        cx = 0;
    if(x >= sxy)
        cx = 2;
    if(y < 0)
        cy = 0;
    if(y >= sxy)
        cy = 2;
    if(z < 0)
        cz = 0;
    if(z >= sz)
        cz = 2;
    
    return cell[cx][cy][cz];
}