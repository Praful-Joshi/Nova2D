#ifndef TAG_TAGGING_HPP
#define TAG_TAGGING_HPP

#include <cstdint>
#include <memory>
#include <vector>

#include "game.hpp"
#include "message.hpp"
#include "tag_player.hpp"

// Tag's chase rules, kept in each player's TagPlayerAttribute: who is It, who is frozen, who
// has Tag-back Immunity, and how long each player has been It. The Host decides; every Peer
// applies the Host's decisions with these functions. Times are the Peer's Timeline time in ms.
namespace tagging {

// How long It stays frozen at the start of Playing (or when It is reassigned)
constexpr int64_t IT_FREEZE_MS = 2'000;
// How long a player who tagged someone can't be tagged back by them
constexpr int64_t IMMUNITY_MS = 2'000;

// The given Peer's player's Tag state, or null if its player isn't in the game
std::shared_ptr<TagPlayerAttribute> playerTag(const Game& game, int peerId);

// Peer ID of It, or 0 if nobody is It
int it(const Game& game);

// Forgets the last Round: nobody is It, frozen or Immune, and every It time is 0
void reset(Game& game);

// Makes the player It, frozen until frozenUntil; nobody is Immune
void makeIt(Game& game, int peerId, int64_t frozenUntil);

// A Tagging: `to` becomes It, and `from` is Immune against them until immuneUntil
void tag(Game& game, int from, int to, int64_t immuneUntil);

// The participant It is touching and may tag now, or 0. Nobody is tagged by a frozen or hidden
// It, and hidden, Spectating and Immune players can't be tagged. Lowest Peer ID first.
int findTagging(Game& game, const std::vector<int>& participants, int64_t now);

// The It time of each participant whose player is in the game, in participant order
std::vector<ItTime> itTimes(const Game& game, const std::vector<int>& participants);
void setItTimes(Game& game, const std::vector<ItTime>& itTimes);

// Host: puts the Tag state into the Host's broadcast, with times as durations from now
void writeHostState(const Game& game, const std::vector<int>& participants, int64_t now,
                    HostState& state);
// Takes the Tag state in the Host's broadcast as this Peer's own
void followHostState(Game& game, const HostState& state, int64_t now);

// The participants in the game who were It for the least time (more than one on a tie)
std::vector<int> winners(const Game& game, const std::vector<int>& participants);

}  // namespace tagging

#endif
