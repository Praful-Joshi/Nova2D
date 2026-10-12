#ifndef TAG_ROUNDS_HPP
#define TAG_ROUNDS_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "round_phase.hpp"
#include "timeline.hpp"

// Tag's Round cycle as one Peer sees it: Lobby -> Countdown -> Playing -> Results ->
// Countdown -> ... Phase lengths are Timeline time, so pausing the Timeline stops the clock.
// The Host decides when Rounds start and end; every Peer runs the Countdown's clock itself
// and follows the Host's broadcasts for the rest.
class Rounds {
public:
    static constexpr int64_t COUNTDOWN_MS = 3'000;
    static constexpr int64_t PLAYING_MS = 120'000;
    static constexpr int64_t RESULTS_MS = 5'000;

    explicit Rounds(Timeline& timeline) : timeline_(timeline) {}

    [[nodiscard]] RoundPhase phase() const { return phase_; }
    [[nodiscard]] int round() const { return round_; }
    // Peer IDs playing the current Round, ascending; empty in the Lobby
    [[nodiscard]] const std::vector<int>& participants() const { return participants_; }
    // Watching the current Round rather than playing it (nobody is in the Lobby)
    [[nodiscard]] bool isSpectator(int peerId) const;
    // Timeline time left in the current phase; 0 in the Lobby
    [[nodiscard]] int64_t remainingMs() const;
    [[nodiscard]] bool isTimeUp() const {
        return phase_ != RoundPhase::LOBBY && remainingMs() == 0;
    }

    // Enters a phase with the given time left in it
    void enter(RoundPhase phase, int round, std::vector<int> participants, int64_t remainingMs);
    // Corrects the time left in the current phase
    void setRemainingMs(int64_t remainingMs);
    // Moves from the Countdown to Playing when the Countdown's time is up; true if it did
    bool advance();

    // Whether a state for (round, phase) is ahead of this Peer's, behind it, or the same
    [[nodiscard]] int compare(int round, RoundPhase phase) const;

private:
    Timeline& timeline_;
    RoundPhase phase_ = RoundPhase::LOBBY;
    int round_ = 0;
    std::vector<int> participants_;
    int64_t endsAt_ = 0;  // Timeline time
};

// Peer ID lists as Custom Event parameters ("1,2,4")
std::string joinPeerIds(const std::vector<int>& ids);
std::vector<int> splitPeerIds(const std::string& text);

#endif
