//
//  Movement.cpp
//  HPRL
//
//  Created by Marcus Holm on 10/10/17.
//  Copyright © 2017 Marcus Holm. All rights reserved.
//

#include "Movement.hpp"

int Movement::isWalkable(PosInt p)
{
   // MapCell * c = game->getCellCoords(p.x,p.y,p.z);
    MapTile * tile = game->getTile(p.x,p.y,p.z);
    
    // walkable if tile is air and tile below is solid
    if(tile->propmask & TP_AIR )
    {
        tile = game->getTile(p.x,p.y,p.z-1);
        if(tile->propmask & TP_SOLID)
            return 1;
        // is also walkable if tile is air and current tile below is slippery
        PosInt p2 = Float32PosInt_rounded(w->position[game->getPlayerEntity()]);
        if(game->getTile(p2.x,p2.y,p2.z-1)->propmask & TP_SLIPPERY)
            return 1;
    }
    // walkable also if tile is water?
    return (tile->propmask & TP_WATER) ;

    //return 0;
}

int Movement::isSwimmable(PosInt p)
{
    MapTile * tile = game->getTile(p.x,p.y,p.z);
    
    // can swim to any tile with water, also walkable tiles. Dude will stop swimming if on land.
    if(tile->propmask & TP_WATER)
        return 1;
    else
        return isWalkable(p);
    
    return 0;
}

int Movement::isClimbable(PosInt p)
{
    MapTile * tile = game->getTile(p.x,p.y,p.z);
    
    // climbable iff tile is air/water and a grippable tile is anywhere nearby
    if(tile->propmask & (TP_AIR | TP_WATER))
    {
        PosInt pt;
        
        //TODO: Reorganise this disaster plz
        pt = {p.x-1,p.y,p.z};
        if(game->getTile(pt.x,pt.y,pt.z)->propmask & TP_GRIPPABLE)
            return 1;
        pt = {p.x+1,p.y,p.z};
        if(game->getTile(pt.x,pt.y,pt.z)->propmask & TP_GRIPPABLE)
            return 1;
        pt = {p.x,p.y-1,p.z};
        if(game->getTile(pt.x,pt.y,pt.z)->propmask & TP_GRIPPABLE)
            return 1;
        pt = {p.x,p.y+1,p.z};
        if(game->getTile(pt.x,pt.y,pt.z)->propmask & TP_GRIPPABLE)
            return 1;
        pt = {p.x,p.y,p.z-1};
        if(game->getTile(pt.x,pt.y,pt.z)->propmask & TP_GRIPPABLE)
            return 1;
        pt = {p.x,p.y,p.z+1};
        if(game->getTile(pt.x,pt.y,pt.z)->propmask & TP_GRIPPABLE)
            return 1;
        // cardinal + 1
        pt = {p.x-1,p.y+1,p.z};
        if(game->getTile(pt.x,pt.y,pt.z)->propmask & TP_GRIPPABLE)
            return 1;
        pt = {p.x+1,p.y+1,p.z};
        if(game->getTile(pt.x,pt.y,pt.z)->propmask & TP_GRIPPABLE)
            return 1;
        pt = {p.x+1,p.y-1,p.z};
        if(game->getTile(pt.x,pt.y,pt.z)->propmask & TP_GRIPPABLE)
            return 1;
        pt = {p.x,p.y+1,p.z+1};
        if(game->getTile(pt.x,pt.y,pt.z)->propmask & TP_GRIPPABLE)
            return 1;
        pt = {p.x+1,p.y,p.z-1};
        if(game->getTile(pt.x,pt.y,pt.z)->propmask & TP_GRIPPABLE)
            return 1;
        pt = {p.x+1,p.y,p.z+1};
        if(game->getTile(pt.x,pt.y,pt.z)->propmask & TP_GRIPPABLE)
            return 1;
        //cardinal - 1
        pt = {p.x-1,p.y-1,p.z};
        if(game->getTile(pt.x,pt.y,pt.z)->propmask & TP_GRIPPABLE)
            return 1;
        pt = {p.x-1,p.y,p.z-1};
        if(game->getTile(pt.x,pt.y,pt.z)->propmask & TP_GRIPPABLE)
            return 1;
        pt = {p.x,p.y-1,p.z-1};
        if(game->getTile(pt.x,pt.y,pt.z)->propmask & TP_GRIPPABLE)
            return 1;
        pt = {p.x,p.y-1,p.z+1};
        if(game->getTile(pt.x,pt.y,pt.z)->propmask & TP_GRIPPABLE)
            return 1;
        pt = {p.x,p.y+1,p.z-1};
        if(game->getTile(pt.x,pt.y,pt.z)->propmask & TP_GRIPPABLE)
            return 1;
        pt = {p.x+1,p.y,p.z+1};
        if(game->getTile(pt.x,pt.y,pt.z)->propmask & TP_GRIPPABLE)
            return 1;
        
    }
    return 0;
}

float Movement::move(Key input, entity_t ent)
{
    Float3 *v = &(w->velocity[ent]);
    v->x = 0;
    v->y = 0;
    v->z = 0;
    float time = 0;

    switch(input)
    {
        case(KEY_NORTH): v->y = 1; break;
        case(KEY_SOUTH): v->y = -1; break;
        case(KEY_EAST): v->x = 1; break;
        case(KEY_WEST): v->x = -1; break;
        case(KEY_UP): v->z = 1; break;
        case(KEY_DOWN): v->z = -1; break;
        case(KEY_WAIT): time = 1; break;
        default: fprintf(stderr,"non-move input sent to Movement\n");
    }
    return time;
}

float Movement::swim(Key input, entity_t ent)
{
    PosInt p = Float32PosInt(w->position[ent]);
    Float3 *v = &(w->velocity[ent]);
    v->x = 0;
    v->y = 0;
    v->z = 0;
    float time = 1;
    switch(input)
    {
        case(KEY_NORTH):
            if(isSwimmable({p.x,p.y+1,p.z}))
                v->y = 1;
            else if(isWalkable({p.x,p.y+1,p.z+1})){ // get up on shore
                v->y = isqrt2;
                v->z = isqrt2;
                time = sqrt2;
            }
            break;
        case(KEY_SOUTH):
            if(isSwimmable({p.x,p.y-1,p.z}))
                v->y = -1;
            else if(isWalkable({p.x,p.y-1,p.z+1})){ // get up on shore
                v->y = -isqrt2;
                v->z = isqrt2;
                time = sqrt2;
            }
            break;
        case(KEY_EAST):
            if(isSwimmable({p.x+1,p.y,p.z}))
                v->x = 1;
            else if(isWalkable({p.x+1,p.y,p.z+1})){ // get up on shore
                v->x = isqrt2;
                v->z = isqrt2;
                time = sqrt2;
            }
            break;
        case(KEY_WEST):
            if(isSwimmable({p.x-1,p.y,p.z}))
                v->x = -1;
            else if(isWalkable({p.x-1,p.y,p.z+1})){ // get up on shore
                v->x = -isqrt2;
                v->z = isqrt2;
                time = sqrt2;
            }
            break;
            
        case(KEY_UP):
            if(isSwimmable({p.x,p.y,p.z+1}))
                v->z = 1;
            break;
        case(KEY_DOWN):
            if(isSwimmable({p.x,p.y,p.z-1}))
                v->z = -1;
            break;
        case(KEY_WAIT):
            time = 1; break;
        default: fprintf(stderr,"non-move input sent to Movement\n");
    }
    // add water tile's velocity
    float ws = game->getTile(p)->flowSpeed;
    Direction wd = game->getTile(p)->flowDir;
    Float3 wv = {0,0,0};
    switch(wd)
    {
        case DIR_EAST: wv.x += ws; break;
        case DIR_WEST: wv.x -= ws; break;
        case DIR_NORTH: wv.y += ws; break;
        case DIR_SOUTH: wv.y -= ws; break;
        case DIR_UP: wv.z += ws; break;
        case DIR_DOWN: wv.z -= ws; break;
        default: printf("no flow direction");
    }
    printf("water flow %f in %d\n",ws,wd);
    *v = sumFloat3(*v,wv);
    return time;
}

float Movement::climb(Key input, entity_t ent)
{
    // if energy is negative, chance of letting go.
    Character* pc = game->getCharacter();
    if(pc->energy < 0)
    {
        //TODO: this makes *everything* de-climb if player's energy<0
        w->move_type[ent] = MOV_FREE;
        return 1;
    }
    
   // printf("trying to climb\n");
    PosInt p = Float32PosInt(w->position[ent]);
    Float3 *v = &(w->velocity[ent]);
    v->x = 0;
    v->y = 0;
    v->z = 0;
    float time = 1;
    switch(input)
    {
        case(KEY_NORTH):
            if(isClimbable({p.x,p.y+1,p.z}))
                v->y = 1;
            break;
        case(KEY_SOUTH):
            if(isClimbable({p.x,p.y-1,p.z}))
                v->y = -1;
            break;
        case(KEY_EAST):
            if(isClimbable({p.x+1,p.y,p.z}))
                v->x = 1;
            break;
        case(KEY_WEST):
            if(isClimbable({p.x-1,p.y,p.z}))
                v->x = -1;
            break;
            //can't walk up or down. Must climb. Good way of toggling mode?
        case(KEY_UP):
            if(isClimbable({p.x,p.y,p.z+1}))
                v->z = 1;
            break;
        case(KEY_DOWN):
            if(isClimbable({p.x,p.y,p.z-1}))
                v->z = -1;
            break;
        case(KEY_WAIT):
            time = 1; break;
        default: fprintf(stderr,"non-move input sent to Movement\n");
    }
    return time;
}

float Movement::walk(Key input, entity_t ent)
{
    PosInt p = Float32PosInt(w->position[ent]);
    Float3 *v = &(w->velocity[ent]);
    v->x = 0;
    v->y = 0;
    v->z = 0;
    
    float time = 1;
    switch(input)
    {
        case(KEY_NORTH):
            if(isWalkable({p.x,p.y+1,p.z}))
                v->y = 1;
            else if(isWalkable({p.x,p.y+1,p.z-1})){
                v->y = isqrt2;
                v->z = -1*isqrt2;
                time = sqrt2;
            }else if(isWalkable({p.x,p.y+1,p.z+1})){
                v->y = isqrt2;
                v->z = isqrt2;
                time = sqrt2;
            }
            break;
        case(KEY_SOUTH):
            if(isWalkable({p.x,p.y-1,p.z}))
                v->y = -1;
            else if(isWalkable({p.x,p.y-1,p.z-1})){
                v->y = -isqrt2;
                v->z = -1*isqrt2;
                time = sqrt2;
            }else if(isWalkable({p.x,p.y-1,p.z+1})){
                v->y = -isqrt2;
                v->z = isqrt2;
                time = sqrt2;
            } break;
        case(KEY_EAST):
            if(isWalkable({p.x+1,p.y,p.z}))
                v->x = 1;
            else if(isWalkable({p.x+1,p.y,p.z-1})){
                v->x = isqrt2;
                v->z = -1*isqrt2;
                time = sqrt2;
            }else if(isWalkable({p.x+1,p.y,p.z+1})){
                v->x = isqrt2;
                v->z = isqrt2;
                time = sqrt2;
            } break;
        case(KEY_WEST):
            if(isWalkable({p.x-1,p.y,p.z}))
                v->x = -1;
            else if(isWalkable({p.x-1,p.y,p.z-1})){
                v->x = -isqrt2;
                v->z = -1*isqrt2;
                time = sqrt2;
            }else if(isWalkable({p.x-1,p.y,p.z+1})){
                v->x = -isqrt2;
                v->z = isqrt2;
                time = sqrt2;
            }  break;
        case(KEY_WAIT):
            time = 1; break;
            //can't walk up or down. Must climb. Good way of toggling mode?
        case(KEY_UP):  break;
        case(KEY_DOWN):  break;
        default: fprintf(stderr,"non-move input sent to Movement\n");
    }

    return time;
}

float Movement::input(Key input)
{
    entity_t ent;
    
    float time = 0;
    for(ent = 0; ent<maxEntities; ent++)
    {
        if(w->mask[ent] & mask)
        {
            if(w->mask[ent] & COMP_IS_PLAYER_CONTROLLED)
            {
                // if walking, do walking routine)
                if(w->move_type[ent] & MOV_WALK)
                    time = walk(input, ent);
                // if floating, do basic move
                else if(w->move_type[ent] & MOV_FLOAT)
                    time = move(input, ent);
                else if(w->move_type[ent] & MOV_CLIMB)
                    time = climb(input, ent);
                else if(w->move_type[ent] & MOV_SWIM)
                    time = swim(input, ent);
                else
                // otherwise, you're out of luck.
                    time = 1;
            }
        }
    }
    return time;
}

void Movement::exec(float timestep)
{
    // find ents with the right components
    entity_t ent;
    
    for(ent = 0; ent<maxEntities; ent++)
    {
        if(w->mask[ent] & mask)
        {
            if(w->move_type[ent] & (MOV_FREE)) // update velocity of all free-falling objects
            {
                // do stuff
                Float3 *p = &(w->position[ent]);
                Float3 *v = (Float3*)&(w->velocity[ent]);
                PosInt pi = Float32PosInt(*p);
                
                // if inside water, set move type to swimming if entity can swim
                if(  game->getTile(pi.x,pi.y,pi.z)->propmask & TP_WATER )
                {
                    if(ent == game->getPlayerEntity())
                    {
                        game->addToLog("You start to swim");
                        w->move_type[game->getPlayerEntity()] = MOV_SWIM;
                    }
                }else if(normFloat3(*v) < 0.01 && game->getTile(pi.x,pi.y,pi.z-1)->propmask & TP_SOLID)
                {// if velocity is zero and lying on top of a solid, move on
                    // consider changing movement type. How do I tell what's appropriate?
                    if(ent == game->getPlayerEntity())
                    {
                        game->addToLog("You start to walk");
                        w->move_type[game->getPlayerEntity()] = MOV_WALK;
                    }
                    continue;
                }
                
                // simulate movement until it hits a solid
                float dt = 0.001;
                float t = 0;
                PosInt piOld = pi;
                for(t=0; t<timestep; t+=dt)
                {
                    *v = sumFloat3(*v,{0,0,-1.0f*dt});      // dv = gravity * time
                    *p = sumFloat3(*p, mulFloat3(*v, dt));  // dt = v * time
                    pi = Float32PosInt(*p);
                    
                    if(game->getTile(pi.x,pi.y,pi.z)->propmask & (TP_SOLID))
                    {
                        // tell Game that we just hit something. Give it v to see how hard.
                        game->collision(ent, normFloat3(*v));
                        pi = piOld; // step back and normalise
                        *p = PosInt2Float3(pi);
                        *v = {0,0,0}; // velocity stops
                        break;
                    }
                    piOld = pi;
                }
            }else if(w->move_type[ent] & (MOV_SWIM)) // update velocity of all swimming objects
            {                                   // very similar to freefall
                // do stuff
                Float3 *p = &(w->position[ent]);
                Float3 *v = (Float3*)&(w->velocity[ent]);
                PosInt pi = Float32PosInt(*p);                            
                
                // simulate movement until it hits a solid
                float dt = 0.001;
                float t = 0;
                PosInt piOld = pi;
                for(t=0; t<timestep; t+=dt)
                {
                    *p = sumFloat3(*p, mulFloat3(*v, dt));  // dt = v * time
                    pi = Float32PosInt(*p);
                    
                    if(game->getTile(pi.x,pi.y,pi.z)->propmask & (TP_SOLID))
                    {
                        // tell Game that we just hit something. Give it v to see how hard.
                        game->collision(ent, normFloat3(*v));
                        pi = piOld; // step back and normalise
                        *p = PosInt2Float3(pi);
                        *v = {0,0,0}; // velocity stops
                        break;
                    }
                    piOld = pi;
                }
                // if enters air, stop swimming
                if(game->getTile(*p)->propmask & TP_AIR)
                    w->move_type[ent] = MOV_FREE;
                
                
                v->x = 0;
                v->y = 0;
                v->z = 0;
            }else if(! (w->move_type[ent] & MOV_WIELDED)) {
                // do simpler stuff for other movement modes
                Float3 *p = &(w->position[ent]);
                Float3 *v = &(w->velocity[ent]);
                
                // if entity is "just floating", ignore timestep
                if(w->move_type[ent] & MOV_FLOAT)
                {
                    p->x += v->x;
                    p->y += v->y;
                    p->z += v->z;
                }else // otherwise, do a move
                {
                    p->x += timestep*v->x;
                    p->y += timestep*v->y;
                    p->z += timestep*v->z;
                    
                    //check whether a climbing player should start walking. if on nonslippery walkable surface.
                    //TODO: what about a climbing player who enters water?
                    if(w->move_type[ent] & MOV_CLIMB)
                    {
                        if( game->getHasClimbed())
                        {
                            if(game->getTile((Float3){p->x,p->y,p->z-1})->propmask & TP_SOLID && !(game->getTile((Float3){p->x,p->y,p->z-1})->propmask & TP_SLIPPERY) && !(game->getTile(*p)->propmask & TP_WATER))
                            {
                                w->move_type[ent] = MOV_WALK;
                                w->position[ent] = PosInt2Float3(Float32PosInt(*p));
                                w->velocity[ent] = {0,0,0};
                                game->addToLog("You let go and begin to walk");
                            }
                        }else
                            game->setHasClimbed(1);
                    }
                    
                    // check whether a walking player should start freefalling. if in/on air
                    if(w->move_type[ent] & MOV_WALK)
                    {
                        if(game->getTile((Float3){p->x,p->y,p->z-1})->propmask & (TP_AIR | TP_WATER))
                        {
                            w->move_type[ent] = MOV_FREE;
                        }
                        // check whether a walking player should be swimming.
                        // if inside water, set move type to swimming if entity can swim
                        if(  game->getTile(*p)->propmask & TP_WATER )
                        {
                            if(ent == game->getPlayerEntity())
                            {
                                game->addToLog("You start to swim");
                                w->move_type[game->getPlayerEntity()] = MOV_SWIM;
                            }
                        }
                    }
                }
                v->x = 0;
                v->y = 0;
                v->z = 0;
            }
        }
    }
    
    //move all wielded objects to player's position
    for(ent = 0; ent<maxEntities; ent++)
    {
        if(w->mask[ent] & mask && w->move_type[ent] & MOV_WIELDED)
        {
            w->position[ent] = w->position[game->getPlayerEntity()];
           // printf("setting postion of %d to player position\n", ent);
        }
    }
}
