# CthonicExpedition
A roguelike inspired by At the Mountains of Madness, by HP Lovecraft

Version 0.0.0 — an early demo intended for a limited audience. 

## Dependencies:

* FLTK and its dependencies. See FLTK.org. 
* That's all, I think! 

## Build Instructions:

make or use XCode project

## Usage: 

./cthonicexpedition [window_width_in_pixels]

If you don't like the world you generate, go to Game.cpp:init() and provide a different integer to srandom() (around line 56).

## Controls:

* Arrow keys to move north/south/east/west.
* '.' to stand still
* < to go up
* SHIFT to go down (don't ask)
* 'c' to try to climb or let go
* 'l' for look mode
  * 'f' to throw a flare
  * 'j' to jump
  * 'e' to examine a tile
* 'i' for inventory
  * 'x' to close inventory. 
  * other keys to use/activate/consume items
* 'd' to drop items
* 'e' to examine yourself and report on status
* 'x' to close/open your eyes 

There are other controls, too, but they are more development/debugging tools than actual player controls. 
If a key doesn't suit your tastes, go into Game.cpp:addInput() and change it. Or even better: write me a control scheme parser.

