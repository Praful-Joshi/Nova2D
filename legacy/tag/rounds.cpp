#include "rounds.hpp"

#include <algorithm>
#include <sstream>
#include <utility>

bool Rounds::isSpectator(int peerId) const {
    if (phase_ == RoundPhase::LOBBY) return false;
    return std::find(participants_.begin(), participants_.end(), peerId) == participants_.end();
}

int64_t Rounds::remainingMs() const {
    if (phase_ == RoundPhase::LOBBY) return 0;
    return std::max<int64_t>(0, endsAt_ - timeline_.getTime());
}

void Rounds::enter(RoundPhase phase, int round, std::vector<int> participants,
                   int64_t remainingMs) {
    phase_ = phase;
    round_ = round;
    participants_ = std::move(participants);
    std::sort(participants_.begin(), participants_.end());
    setRemainingMs(remainingMs);
}

void Rounds::setRemainingMs(int64_t remainingMs) { endsAt_ = timeline_.getTime() + remainingMs; }

bool Rounds::advance() {
    if (phase_ != RoundPhase::COUNTDOWN || !isTimeUp()) return false;
    // Playing starts when the Countdown ends, carrying over any time the tick overshot
    const int64_t overshoot = timeline_.getTime() - endsAt_;
    enter(RoundPhase::PLAYING, round_, participants_, PLAYING_MS - overshoot);
    return true;
}

namespace {
// Order of phases within a Round; a Lobby after a Round (too few players left) comes last
int rank(int round, RoundPhase phase) {
    if (phase == RoundPhase::LOBBY) return round == 0 ? 0 : 4;
    return static_cast<int>(phase);
}
}  // namespace

int Rounds::compare(int round, RoundPhase phase) const {
    if (round != round_) return round > round_ ? 1 : -1;
    const int theirs = rank(round, phase);
    const int mine = rank(round_, phase_);
    if (theirs != mine) return theirs > mine ? 1 : -1;
    return 0;
}

std::string joinPeerIds(const std::vector<int>& ids) {
    std::string text;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (i > 0) text += ',';
        text += std::to_string(ids[i]);
    }
    return text;
}

std::vector<int> splitPeerIds(const std::string& text) {
    std::vector<int> ids;
    std::istringstream input(text);
    std::string item;
    while (std::getline(input, item, ',')) {
        if (!item.empty()) ids.push_back(std::stoi(item));
    }
    return ids;
}
