# Full-mesh networking with Host as a role

Legacy Engine multiplayer uses a full mesh: every Peer binds one ZeroMQ PUB socket and subscribes to every other Peer. There is no central hub. "Host" is a role, meaning authority over shared state (the Round timer, who is It, Tagging decisions, moving platforms), not a different kind of process. We chose this so Host Migration works across machines: when the Host leaves, the surviving Peer with the lowest Peer ID takes over the role, and no socket is rebound and no Peer reconnects.

## Considered Options

- **Star topology (the original course-project design).** The host binds a hub and the other Peers connect to it. It's simpler for 2 players, but if the host leaves, a new hub has to bind and every Peer has to find its address and reconnect. That only works on a single machine, which is why the original Host Migration never worked.

## Consequences

- Connections grow as N×(N−1). That's fine for Tag's 2–4 players (at most 12), but the design won't scale to large lobbies.
- A Peer can't tell from ZeroMQ that another Peer has left, so departures are detected by a heartbeat timeout (or an explicit `LEAVING` message).
- Every Peer receives the Host's state broadcasts, so a newly promoted Host continues from its last received view and needs no state hand-off.
