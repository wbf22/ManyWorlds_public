
# Server

All the code in this directory is anything that is server specific.

When a player plays on their own, they will actually be running the server, but the server won't be exposed on any ports. 

A server could also be run by running the project in a different mode where all the graphic stuff is disabled. (We'll have to set that up in a bit.) Hopefully we can use all the same classes without having to make any duplicates.

Things that the server should handle
- block edits (player or NPC)
- water edits (reported by clients)
- NPC movement and actions
- world or universe events
- economies
- positions of objects
- player data


Basically anything that happens during play.

Things that the client should handle
- terrain generation (except player and NPC edits)
- building generation
- water flow (low priority calculations, also don't spread like minecraft)
- animal/alien NPC generation
- physics

For physics enabled objects the client will have to report the position of physics objects. This might be ok if we can get some decent server rules to prevent people from hijacking this.

I'm thinking the client will basically be a dumb ui with the exception of calculating terrain.

## Security
Login and Register requests are done over TCP to ensure a clients ip address is what they say it is. After that the server just accepts any UDP requests from the ip address as coming from the client (can be spoofed if hacker knows clients ip. Probably just a risk on public wifi). To prevent rando UDP spamming, the server only accepts requests from ip addresses that are logged in. 


## Large Servers
It may be beneficial to allow the partitioning of a server into different world spaces if it gets really large. (Like if the server was trying to handle thousands of clients) We'd want to have a way to do this easily. Each server could manage a certain region of the universe. 

Some things would have to be transferred between partitions
- player data
- maybe universe wide events

One server would send the players data to another when they switch. You might also need the servers to share some secret key so the transfer can be verified (so players can't log themselves in with whatever items they want)



## Communication
A lot of games use binary protocol for efficient communication. (cheaper than Json)

Here's how minecraft does it https://wiki.vg/Protocol

We might do that as well. Something like the first 2 bytes (0-65,635) would indicate the command.
The request might look like this:
- 2 bytes (command)
- Repeats of the following
    - 1 byte (field)
    - 2 bytes (value length)
    - n bytes (value)

Using utf-8 for strings

If an object has nested values then they'll have to be flattened so the whole request can work this way. 

To send lists of objects we'll just have to append the index onto the end of each field.

So this object

```
{
    "name" : "michael",
    "hobbies" : [
        {
            "name" : "baseball",
            "skill" : 10
        },
        {
            "name" : "cooking",
            "skill" : 7
        }
    ]
}
```

Would become

```
name=1
hobbie.name.1=2
hobbie.skill.1=3
hobbie.name.2=4
hobbie.skill.2=5


<command><1><micheal><2><baseball><3><10><4><cooking><5><cooking><5><7>
```

This means you'll probably send lists of a fixed length, or not send lists. 

Alternatively for complex object you could send a json string instead. The example above would become:
```
name=1
hobbies=2

<command><1><micheal><2><[ { "name" : "baseball", "skill" : 10 }, { "name" : "cooking", "skill" : 7 } ]>
```
For complex objects that'd probably be the best but you'd have to take the performance overhead of parsing. We might be able to make a pretty efficient parser though, but it'd still be more overhead and network traffic.


## Hard Hit Endpoints

- player/npc position updates
- hit/attack
- block querying for areas

We'll put these on a seperate port I think so 