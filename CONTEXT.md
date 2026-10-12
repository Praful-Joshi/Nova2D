# Nova2D

A 2D game engine with peer-to-peer multiplayer, plus the games built to showcase it. Two engines coexist while the original course-project code is fixed up and then ported into the new architecture.

## Language

### Engines

**Legacy Engine**:
The original course-project engine in `legacy/engine/`, built around Entities, Attributes and an EventManager. It is being repaired and showcased now, and ported into the Nova Engine later.
_Avoid_: old code, old engine, v1

**Entity**:
A coloured rectangle in a Legacy Engine game world, such as a player, platform or zone. It has no behaviour of its own.
_Avoid_: object, actor, sprite

**Property**:
A kind of behaviour an Entity can be given in the Legacy Engine, such as Gravity, Collide or DeathZone.
_Avoid_: component, trait

**Attribute**:
One Entity's instance of a Property, holding that Entity's data for that behaviour.
_Avoid_: component (until the Nova Engine port, where Attributes become ECS components)

**Custom Event**:
An event defined by a game rather than by the Legacy Engine, identified by name (for example TAGGED or ROUND_END).
_Avoid_: game event, user event

**World Coordinates**:
Positions in the shared game world. Every Peer agrees on them, and they are what gets sent over the network.
_Avoid_: screen position, absolute position

**Camera**:
A Peer's own view into the world, given as an offset from World Coordinates. Scrolling moves the Camera, never the Entities.
_Avoid_: viewport, scroll offset

**Nova Engine**:
The rewrite in `engine/` (namespace `Nova`) that follows the roadmap phases. Legacy Engine features are eventually ported into it.
_Avoid_: new engine, engine v2

### Multiplayer

**Peer**:
One running instance of a networked game, identified by a Peer ID. Every Peer talks directly to every other Peer.
_Avoid_: client, server, node

**Host**:
The Peer that currently has authority over shared state, such as the Round timer and who is It. Being Host is a role, not a different kind of Peer.
_Avoid_: server, master peer, masterPeer

**Host Migration**:
Handing the Host role to the surviving Peer with the lowest Peer ID when the current Host leaves.
_Avoid_: failover, server switch

**Lobby**:
The waiting phase before the first Round, when Peers can see who has joined. It ends once the expected number of Peers is present.
_Avoid_: waiting room, matchmaking

**Spectator**:
A Peer that joined while a Round was already running. It watches until the next Round begins, then plays.
_Avoid_: observer, late joiner

### Tag (showcase game)

**Tag**:
A networked platformer for 2–4 players built on the Legacy Engine to showcase it. One player is It at any time.
_Avoid_: tag game, demo

**It**:
The player currently chasing the others. Touching another player passes It to them.
_Avoid_: tagger, chaser, seeker

**Round**:
One timed session of Tag. When it ends, the player who was It for the least total time wins.
_Avoid_: match, game

**Tagging**:
The act of It touching another player, which makes that player It. Only the Host decides whether a Tagging happened.
_Avoid_: tag (when you mean the act, to keep it distinct from the game's name), catch, hit

**Tag-back Immunity**:
A short window after a Tagging during which the new It cannot tag the player who just tagged them.
_Avoid_: cooldown, grace period
