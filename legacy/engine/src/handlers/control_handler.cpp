#include "control_handler.hpp"
#ifdef __linux__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#include <algorithm>
#include <cmath>

#include "control.hpp"

namespace {

// Direction keys: each runs or jumps while held and dashes when double-tapped
constexpr SDL_Scancode DIRECTION_KEYS[] = {SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT,
                                           SDL_SCANCODE_RIGHT};

SDL_Rect roundedRect(double x, double y, int w, int h) {
    return SDL_Rect{static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)), w, h};
}

// True if the area overlaps a solid: an immovable Entity with the Collide property. Zones,
// SpawnPoints and other players don't count.
bool overlapsSolid(Game& game, const Entity& self, const SDL_Rect& area) {
    for (const auto& [id, other] : game.getEntities()) {
        if (id == self.getEntityID() || other->getisMovable()) continue;
        if (!game.entityHasProperty(Property::Collide, id)) continue;
        const SDL_Rect otherRect = other->getRect();
        if (SDL_HasIntersection(&area, &otherRect)) return true;
    }
    return false;
}

// Keeps a w x h rectangle at (x, y) inside the bounds, if there are any
void clampToBounds(const ControlAttribute& control, int w, int h, double& x, double& y) {
    if (!control.getBounds()) return;
    const SDL_Rect& b = *control.getBounds();
    x = std::clamp(x, static_cast<double>(b.x), static_cast<double>(b.x + b.w - w));
    y = std::clamp(y, static_cast<double>(b.y), static_cast<double>(b.y + b.h - h));
}

// Teleports the Entity DASH_DISTANCE towards the key, stopping at the bounds. Returns false,
// leaving it where it is, if that spot is inside a solid or no different from where it is.
bool tryDash(Entity& entity, Game& game, const ControlAttribute& control, SDL_Scancode key) {
    double x = entity.getX();
    double y = entity.getY();
    switch (key) {
        case SDL_SCANCODE_UP:
            y -= ControlHandler::DASH_DISTANCE;
            break;
        case SDL_SCANCODE_DOWN:
            y += ControlHandler::DASH_DISTANCE;
            break;
        case SDL_SCANCODE_LEFT:
            x -= ControlHandler::DASH_DISTANCE;
            break;
        case SDL_SCANCODE_RIGHT:
            x += ControlHandler::DASH_DISTANCE;
            break;
        default:
            return false;
    }
    const SDL_Rect& rect = entity.getRect();
    clampToBounds(control, rect.w, rect.h, x, y);
    if (x == entity.getX() && y == entity.getY()) return false;
    if (overlapsSolid(game, entity, roundedRect(x, y, rect.w, rect.h))) return false;

    entity.setPosition(x, y);
    return true;
}

// A direction key went down: two presses of the same key within the window dash
void onDirectionPressed(Entity& entity, Game& game, ControlAttribute& control, SDL_Scancode key,
                        int64_t now) {
    const bool isDoubleTap = control.lastTapKey == key &&
                             now - control.lastTapTime <= ControlHandler::DOUBLE_TAP_WINDOW;
    control.lastTapKey = key;
    control.lastTapTime = now;
    if (!isDoubleTap) return;

    // A third press starts a new double-tap rather than completing another one
    control.lastTapKey.reset();
    // A dash that fails doesn't start the cooldown
    if (control.isDashReady(now) && tryDash(entity, game, control, key)) control.recordDash(now);
}

// Halves the Entity's size about its bottom centre, so its feet stay where they were
void shrink(Entity& entity, ControlAttribute& control, int64_t now) {
    const SDL_Rect rect = entity.getRect();
    const int w = rect.w / 2;
    const int h = rect.h / 2;
    control.startShrink(now, rect.w, rect.h);
    entity.setRectWidth(w);
    entity.setRectHeight(h);
    entity.setPosition(entity.getX() + (rect.w - w) / 2.0, entity.getY() + (rect.h - h));
}

// Grows the Entity back to full size with its feet where they are: about its bottom centre, or
// keeping its right or left edge if that's the only way to fit. Returns false, leaving it
// shrunk, if there's no room (e.g. under the low wall), so it never grows into a solid.
bool tryGrow(Entity& entity, Game& game, const ControlAttribute& control) {
    const SDL_Rect rect = entity.getRect();
    const int w = control.getFullWidth();
    const int h = control.getFullHeight();
    const double y = entity.getY() - (h - rect.h);
    const double candidates[] = {entity.getX() - (w - rect.w) / 2.0, entity.getX() - (w - rect.w),
                                 entity.getX()};
    for (double x : candidates) {
        double fitX = x;
        double fitY = y;
        clampToBounds(control, w, h, fitX, fitY);
        if (overlapsSolid(game, entity, roundedRect(fitX, fitY, w, h))) continue;
        entity.setRectWidth(w);
        entity.setRectHeight(h);
        entity.setPosition(fitX, fitY);
        return true;
    }
    return false;
}

// Holding S and H together shrinks the Entity for the shrink duration
void updateShrink(Entity& entity, Game& game, ControlAttribute& control, const Keys& keys,
                  int64_t now) {
    if (control.isShrunk()) {
        if (now - control.getShrinkStartTime() >= control.getShrinkDuration() &&
            tryGrow(entity, game, control)) {
            control.endShrink(now);
        }
    } else if (keys.held(SDL_SCANCODE_S) && keys.held(SDL_SCANCODE_H) &&
               control.isShrinkReady(now)) {
        shrink(entity, control, now);
    }
}

}  // namespace

void ControlHandler::onEvent(const Event& event) {
    if (event.type == EventType::CONTROL) {
        updateGameState(*event.context.entity, *event.context.game);
    }
}

void ControlHandler::updateGameState(Entity& entity, Game& game) {
    auto control = game.getAttribute<ControlAttribute>(Property::Control, entity.getEntityID());
    if (!control) return;
    // A frozen Entity acts as if no keys are held
    const Keys keys = control->isFrozen() ? Keys{} : game.getInput();

    const int64_t now = game.getTimeline()->getTime();
    const bool canUseAbilities = entity.getisMovable() && entity.getIsControllable();

    // Dash on a double-tap of a direction key, counting only the tick each key goes down
    for (SDL_Scancode key : DIRECTION_KEYS) {
        if (canUseAbilities && keys.held(key) && !control->previousKeys.held(key)) {
            onDirectionPressed(entity, game, *control, key, now);
        }
    }

    // Run while left or right is held
    double velocityX = 0;
    if (keys.held(SDL_SCANCODE_LEFT)) velocityX -= control->getRunSpeed();
    if (keys.held(SDL_SCANCODE_RIGHT)) velocityX += control->getRunSpeed();
    entity.setVelocityX(velocityX);

    if (entity.getHasGravity()) {
        // Jump only while standing on something
        if (keys.held(SDL_SCANCODE_UP) && entity.isGrounded()) {
            entity.setVelocityY(-control->getJumpSpeed());
            entity.setGrounded(false);
        }
    } else {
        // Without gravity, up and down move freely
        double velocityY = 0;
        if (keys.held(SDL_SCANCODE_UP)) velocityY -= control->getRunSpeed();
        if (keys.held(SDL_SCANCODE_DOWN)) velocityY += control->getRunSpeed();
        entity.setVelocityY(velocityY);
    }

    if (canUseAbilities) updateShrink(entity, game, *control, keys, now);

    // Remember this frame's keys for edge detection next frame
    control->previousKeys = keys;
}
