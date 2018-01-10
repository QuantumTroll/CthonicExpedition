//
//  Energy.cpp
//  HPRL
//
//  Created by Marcus Holm on 11/11/17.
//  Copyright © 2017 Marcus Holm. All rights reserved.
//

#include "Energy.hpp"

void EnergySystem::exec(float timestep)
{
    player = game->getPlayerEntity();
    // recover
    pc->energy += pc->recover*timestep;
    if(pc->energy > pc->maxEnergy - pc->bruise)
        pc->energy = pc->maxEnergy - pc->bruise;
    
    if(pc->bleed > 0)
    {
        pc->energy -= pc->bleed*timestep;
        //game->addToLog("You're bleeding.");
    }
    

    float moveCost = 0;
    // check movement mode
    if(world->move_type[player] & MOV_CLIMB)
    {                
        moveCost = 20;
        PosInt p = Float32PosInt(world->position[player]);
        PosInt p2, p3;
        
        
        // modify cost according to grip
        
        // Number of grippable tiles around
        int numGrippable = 0;
        p2 = {p.x-1,p.y,p.z};
        if(game->getTile(p2)->propmask & TP_GRIPPABLE)
            numGrippable++;
        p2 = {p.x+1,p.y,p.z};
        if(game->getTile(p2)->propmask & TP_GRIPPABLE)
            numGrippable++;
        p2 = {p.x,p.y-1,p.z};
        if(game->getTile(p2)->propmask & TP_GRIPPABLE)
            numGrippable++;
        p2 = {p.x,p.y+1,p.z};
        if(game->getTile(p2)->propmask & TP_GRIPPABLE)
            numGrippable++;
        
        if(numGrippable == 0) // overhead hold or other uncomfortable position
            moveCost += 15;
        else if(numGrippable > 1) // corner or chimney
        {
            moveCost -= 5;
            p2 = {p.x-1,p.y,p.z}; p3 = {p.x+1,p.y,p.z};
            if(game->getTile(p2)->propmask & game->getTile(p3)->propmask & TP_GRIPPABLE) // true chimney
            {
                moveCost -= 5;
            } else {
                p2 = {p.x,p.y-1,p.z}; p3 = {p.x,p.y+1,p.z};
                if(game->getTile(p2)->propmask & game->getTile(p3)->propmask & TP_GRIPPABLE) // true chimney
                    moveCost -= 5;
            }
            
        }
        
        
        // check whether we're hanging on an anchor
        // only if we're hanging still
        if( normFloat3( world->velocity[player]) < 0.1 )
        {
            int i;
            for(i=0; i<maxEntities; i++)
            {
                if(world->mask[i] & COMP_ANCHOR)
                {
                    PosInt anchorPos = Float32PosInt(world->position[i]);
                    if(distPosInt(p,anchorPos) < 1)
                    {
                        moveCost = 0;                        
                    }
                }
            }
        }
        
        // modify cost according to move direction
        float speed = normFloat3(world->velocity[player]);
        if(speed < 0.1) // if just hanging on, consume less energy
            moveCost /= 2;
        else{
            if(world->velocity[player].z > 0) // if we're going up
                moveCost += 10;
        }
    }else if(world->move_type[player] & MOV_WALK)
    {
        float speed = normFloat3(world->velocity[player]);
        if(speed > 0.1) // if just standing still, consume no energy
        {
            moveCost = 5;
            // modify cost according to vertical motion
            if(world->velocity[player].z > 0)
                moveCost = 10;
            else if(world->velocity[player].z < 0)
                moveCost = 4;
        }
    }else if(world->move_type[player] & MOV_FREE)
    {
        // MOV_FREE implies jump or fall. Pretty stressful, I'd say, no recovery.
        moveCost = pc->recover;
    }
    moveCost += moveCost*(pc->thirst*pc->thirst); // being thirsty makes you tired faster.
    pc->energy -= moveCost*timestep;
    
    pc->hunger += moveCost*timestep / (100*500.0); // we're consuming calories here. Have 100 energy. How many full recoveries before you're starving? 1000 of them?
    pc->thirst += timestep*pc->bleed / 1000.0; // thirst just slowly grows with time. Much faster while you bleed.
}