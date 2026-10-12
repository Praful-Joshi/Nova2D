#include "control.hpp"

#include <algorithm>

namespace {
// Fraction of a cooldown that has passed since it started, or 1 if it never started
double cooldownProgress(const std::optional<int64_t>& start, int64_t cooldown, int64_t now) {
    if (!start || cooldown <= 0) return 1.0;
    return std::clamp(static_cast<double>(now - *start) / static_cast<double>(cooldown), 0.0, 1.0);
}
}  // namespace

void ControlAttribute::update(Entity& entity, Game& game) {
    // Raise a control event
    EventContext context = {&game, &entity};
    Event controlEvent(EventType::CONTROL, context, 3, 3);
    game.getEventManager()->raise(controlEvent);
}

double ControlAttribute::dashReadiness(int64_t now) const {
    return cooldownProgress(lastDashTime, dashCooldown, now);
}

double ControlAttribute::shrinkReadiness(int64_t now) const {
    if (shrunk) return 0.0;
    return cooldownProgress(shrinkEndTime, shrinkCooldown, now);
}
