#ifndef CONTROL_ATTRIBUTE_HPP
#define CONTROL_ATTRIBUTE_HPP

#include <cstdint>
#include <optional>

#include "attribute.hpp"
#include "eventmanager.hpp"
#include "game.hpp"
#include "input.hpp"

// One controlled Entity's input settings and input state. The shared ControlHandler keeps no
// state of its own, so every controlled Entity's key history, dash and shrink are independent.
class ControlAttribute : public Attribute {
public:
    // Speeds are in pixels per Timeline unit; durations and cooldowns are in Timeline units
    explicit ControlAttribute(double runSpeed = 0.35, double jumpSpeed = 0.8,
                              int64_t dashCooldown = 10000, int64_t shrinkDuration = 5000,
                              int64_t shrinkCooldown = 8000)
        : runSpeed(runSpeed),
          jumpSpeed(jumpSpeed),
          dashCooldown(dashCooldown),
          shrinkDuration(shrinkDuration),
          shrinkCooldown(shrinkCooldown) {}

    void update(Entity& entity, Game& game) override;

    [[nodiscard]] double getRunSpeed() const { return runSpeed; }
    [[nodiscard]] double getJumpSpeed() const { return jumpSpeed; }
    [[nodiscard]] int64_t getDashCooldown() const { return dashCooldown; }
    [[nodiscard]] int64_t getShrinkDuration() const { return shrinkDuration; }
    [[nodiscard]] int64_t getShrinkCooldown() const { return shrinkCooldown; }

    // World Coordinates a dash can't leave; unbounded unless set
    void setBounds(const SDL_Rect& worldBounds) { bounds = worldBounds; }
    [[nodiscard]] const std::optional<SDL_Rect>& getBounds() const { return bounds; }

    // While blocked (e.g. frozen), the Entity can't start a dash or a shrink
    void setAbilitiesBlocked(bool blocked) { abilitiesBlocked = blocked; }
    [[nodiscard]] bool areAbilitiesBlocked() const { return abilitiesBlocked; }

    // How ready each ability is at Timeline time now: 0 just used, 1 ready
    [[nodiscard]] double dashReadiness(int64_t now) const;
    [[nodiscard]] double shrinkReadiness(int64_t now) const;
    [[nodiscard]] bool isDashReady(int64_t now) const {
        return !abilitiesBlocked && dashReadiness(now) >= 1.0;
    }
    [[nodiscard]] bool isShrinkReady(int64_t now) const {
        return !abilitiesBlocked && shrinkReadiness(now) >= 1.0;
    }

    // Input state, kept by the ControlHandler from frame to frame
    Keys previousKeys;
    // The last direction key that went down, and when, for double-tap detection
    std::optional<SDL_Scancode> lastTapKey;
    int64_t lastTapTime = 0;

    void recordDash(int64_t now) { lastDashTime = now; }

    [[nodiscard]] bool isShrunk() const { return shrunk; }
    [[nodiscard]] int64_t getShrinkStartTime() const { return shrinkStartTime; }
    // Width and height to grow back to when the shrink ends
    [[nodiscard]] int getFullWidth() const { return fullWidth; }
    [[nodiscard]] int getFullHeight() const { return fullHeight; }
    void startShrink(int64_t now, int width, int height) {
        shrunk = true;
        shrinkStartTime = now;
        fullWidth = width;
        fullHeight = height;
    }
    // The shrink cooldown counts from when the Entity is back at full size
    void endShrink(int64_t now) {
        shrunk = false;
        shrinkEndTime = now;
    }

private:
    double runSpeed;
    double jumpSpeed;
    int64_t dashCooldown;
    int64_t shrinkDuration;
    int64_t shrinkCooldown;

    std::optional<SDL_Rect> bounds;
    bool abilitiesBlocked = false;

    std::optional<int64_t> lastDashTime;
    bool shrunk = false;
    int64_t shrinkStartTime = 0;
    std::optional<int64_t> shrinkEndTime;
    int fullWidth = 0;
    int fullHeight = 0;
};

#endif  // CONTROL_ATTRIBUTE_HPP
