


# Goal:
have potentially thousands or millions of npc's in a city or battle.
Have potentially thousands of players in an area


## Tools:
- compute shaders can be used to do ai math for many entities at once
- a position/rotation update might be 40 bytes per player/npc. A server with 1gbps internet could send 125mb/s (bit and bytes). but we could frequent update the closest players frequently, and then everyone much slower:

    * server_mb_per_s = n × [num_close_players·bytes_per_player_update·close_update_hz + (n−1−num_close_players)·bytes_per_player_update·far_update_hz]
    * server_mb_per_s = bytes_per_player_update·far_update_hz·n² + n·(num_close_players·bytes_per_player_update·close_update_hz − bytes_per_player_update·far_update_hz·(1+num_close_players))
    * standard quadratic form an² + bn + num_close_players = 0:
    * a = bytes_per_player_update·far_update_hz
    * b_coef = num_close_players·bytes_per_player_update·close_update_hz − bytes_per_player_update·far_update_hz·(1+num_close_players)
    * c_coef = −server_mb_per_s
    * n = [−b_coef + √(b_coef² − 4·a·c_coef)] / (2a)
    
    + You can try the python script 'possible_player_count.py' for this math to get an estimate for different values
    + but if you at a far distance update frequency of 1Hz you can get like 1700 players. And at .25hz you could have up to 3167

- So if you do like 20-40 nearby players/npcs at 10hz, and then the rest at increasingly small frequencies you could have like 2000+ entities in the scene which would probably
exceed the render distance. But that would be server wide.
- So I think you'd need to spin up servers to handle regions
as needed. But probably for most daily life stuff a server would just be handling a handful of npcs around each player,
and player machines could make background npcs in a city or
something. But during battles with multiple players you'd need more coordination. 


## Issues to Solutions:
- communicating positions of 1000s or npcs and players efficiently
- for server npc groups with client interpolation of individual npcs
    + clients could keep individual npc's aways from themselves
    + 

|issue|solution|
|-----|--------|
|clients could keep individual npc's away from themselves | server could probability estimate how many hits they should take if group is close |
| clients could claim they hit an npc everytime | npcs are at fixed offsets from group position. so server could calculate if the hit made it |
| at fixed offset melee fighting might be strange | client will have to report melee engagement, then report damage delt/taken, and the server will have to validate with estimates |
| players could drive vehicles through a battle, possibly effecting multiple npcs by running them over | server is given vehicle position and knows it's bounds, it can determine which npcs can be hit roughly by group positions |
| for large player events, we could have potentially thousands of players in a 1024 area | server will have to provision more instances to handle that. We'll use the tiered positioin/rotation update system to only send frequent updates for the closest players |
| if distant players are updated only every few seconds players can't snipe other players | If in a sniping mode, we could just send the player information of where're they're looking. Or not have sniping stuff |
| groups could get stuck walking through tight spaces | we could have different group formations in different scenarios, some coud be seemingly random

