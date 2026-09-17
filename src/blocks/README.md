
# blocks

This folder contains all the classes for spawning blocks in the world and things related to that


In the game all world generation blocks are determined client side and nothing is done in the server. 

These blocks planned deterministically World.hpp and Chunk.h based on a seed. The data is then is then handed off to WorldBlockPool.hpp which actually spawns them in the world. (and also despawns them as the player moves)

However, all player edits to world blocks or placing of blocks are recorded on the server, and the server coordinates these events between clients. 

