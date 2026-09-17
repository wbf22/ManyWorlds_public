# Version Migration
If we every touch generation code between versions (highly likely) then planets or places could competely change between versions.

So we need a way to not completly upend what players have done


## favored solution
For each generated area that a player has edited:
- save a version number with the area
- this folder should contain a hook that can be compiled with the src code into a C ABI lib
- the game then calls the hook when generating that area. 
    + (so it might generate the galaxy in the same place down to the planet and chunk the player edited, but untouched chunks and celestial objects can be generated with current server code)

player edits should include:
- block edits
- significant npc interactions


## game justification
A game update is the modifcation of the fabric of the universe by super gods (me haha, ok..)

It rewrites some of the universe each time




