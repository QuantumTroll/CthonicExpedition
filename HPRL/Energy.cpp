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
    pc->energy += pc->recover*fmax(pc->oxygen,0)*timestep;
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
    moveCost += moveCost*(fmax(0,pc->thirst)*fmax(0,pc->thirst)); // being thirsty makes you tired faster.
    pc->energy -= moveCost*timestep;
    
    if(pc->energy < 0)
        game->addLabel("Exhausted!",1,{1,1,1});
    else if(pc->energy+pc->recover < moveCost)
        game->addLabel("Strained!",1,{1,1,1});
    else if(pc->energy+pc->recover < 2*moveCost )
        game->addLabel("Tired",1,{1,1,1});
    
    pc->hunger += moveCost*timestep / (100*500.0); // we're consuming calories here. Have 100 energy. How many full recoveries before you're starving? 1000 of them?
    int hL = game->getHungerLevel(pc->hunger);
    if(hL != pc->hungerLevel)
    {
        pc->hungerLevel = hL;
        char s[32];
        switch(hL)
        {
            case 0: sprintf(s,"Starving!"); break;
            case 1: sprintf(s,"Hungry"); break;
            case 2: sprintf(s,"Peckish"); break;
            case 3: sprintf(s,"Sated"); break;
            default: sprintf(s,"??");
        }
        game->addLabel(s,3,{1,.8,.8});
    }
    pc->thirst += timestep*pc->bleed / 1000.0; // thirst just slowly grows with time. Much faster while you bleed.
    
    int tL = game->getThirstLevel(pc->thirst);
    if(tL != pc->thirstLevel)
    {
        pc->thirstLevel = tL;
        char s[32];
        switch(tL)
        {
            case 0: sprintf(s,"Parched!"); break;
            case 1: sprintf(s,"Thirsty"); break;
            case 2: sprintf(s,"Less hydrated"); break;
            case 3: sprintf(s,"Hydrated"); break;
            default: sprintf(s,"??");
        }
        game->addLabel(s,3,{.8,.8,1});
    }
    
    //TODO: update pc->oxygen
    // if inside water and tile above not air
    PosInt p = Float32PosInt(world->position[player]);
    PosInt p2 = sumPosInt(p, {0,0,1});
    if(game->getTile(p)->propmask & TP_WATER && !(game->getTile(p2)->propmask & TP_AIR) )
    {
        pc->oxygen -= 0.2; // TODO: equipment & slimes will affect this
    }else {
        pc->oxygen = fmin(1,pc->oxygen+0.3);        
    }
    
    int oL = game->getOxygenLevel(pc->oxygen);
    if(oL != pc->oxygenLevel)
    {
        char s[32];
        if(oL < pc->oxygenLevel)
        {
            switch(oL)
            {
                case 0: sprintf(s,"Fainting!"); break;
                case 1: sprintf(s,"Holding breath"); break;
                case 2: sprintf(s,"Holding breath "); break;
                case 3: sprintf(s,"Magicarp"); break; // shouldn't show up, should it?
                default: sprintf(s,"??");
            }
            game->addLabel(s,3,{1,.7,.7});
        }else {
            switch(oL)
            {
                case 0: sprintf(s,"Goldeen"); break;    // shouldn't show up, should it?
                case 1: sprintf(s,"Catching breath"); break;
                case 2: sprintf(s,"Catching breath "); break;
                case 3: sprintf(s,"Caught breath"); break;
                default: sprintf(s,"??");
            }
            game->addLabel(s,3,{.7,1,.7});
        }
        
        pc->oxygenLevel = oL;
    }
    
    // mood evolution varies depending on many factors
    float mm = getMoodMods();
    pc->mood += timestep*mm;
    int mL = pc->moodLevel;
    //pc->moodLevel
    game->getMoodDescription(pc->mood);
    if(mL != pc->moodLevel)
    {
        //game->addLabel(game->getMoodDescription(pc->mood),2,{1,1,1});
    }
    printf("mood %f\n",pc->mood);
}

// mood evolution varies depending on light, hunger, thirst, injury, mutation, number of held items, scientific progress ?, discovered cave features ?
float EnergySystem::getMoodMods()
{
    float mm = 0;
    
    // entities with mood effects
    int ent;
    for(ent = 0; ent<maxEntities; ent++)
    {
        // check for wielded light sources
        //TODO: also check for other types of light than omnilight, e.g. spotlight
        if(world->mask[ent] & (COMP_OMNILIGHT | COMP_OWNED | COMP_CAN_MOVE) && world->move_type[ent] & MOV_WIELDED)
        {
            mm += 1;
            printf("mood +1 wielded light\n");
        }
        
        // just count how many things we still have. More is better.
        // TODO: I question this a little. Is it comforting to have a pebble in your pocket?
      /*  if(world->mask[ent] & COMP_OWNED)
        {
            mm += 0.05;
        }*/
    }
    // whole, dry, and warm clothing is positive. The opposite is not.
    // default armour is 3
    mm -= 0.1*(3-pc->armour);
    printf("mood %f torn clothes\n",- 0.1*(3-pc->armour));
    
    // hunger is depressing. Hunger is in [0, 1...), 0 not hungry
    mm -= fmax(0,pc->hunger)*fmax(0,pc->hunger);
    printf("mood %f hunger\n",-pc->hunger*pc->hunger);
    
    // thirst is distressing. Thirst is in same range as hunger.
    mm -= fmax(0,pc->thirst)*fmax(0,pc->thirst);
    printf("mood %f thirst\n",-pc->thirst*pc->thirst);
    
    // bruising hurts. Can be from 0 to 100
    mm -= 0.01*(pc->bruise);
    
    // bleeding is very scary. Can be from 0 to 10 or higher
    mm -= 0.5*pc->bleed;
    
    //TODO: exhaustion is depressing. resting feels good.
    if(pc->energy > pc->maxEnergy*0.9)
    {
        mm += 0.2;
        printf("mood +0.2 rested\n");
    }else{
        mm -= (1 - pc->energy/pc->maxEnergy);
        printf("mood %f rested\n", -(1 - pc->energy/pc->maxEnergy));
    }
    //TODO: there will be mutations that make cave life more palatable
    
    
    return mm;
}
