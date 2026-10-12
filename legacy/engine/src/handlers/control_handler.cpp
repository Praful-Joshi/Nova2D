#include "control_handler.hpp"
#ifdef __linux__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#include <cstring>

#include "control.hpp"

void ControlHandler::onEvent(const Event& event) {
    if (event.type == EventType::CONTROL) {
        updateGameState(*event.context.entity, *event.context.game);
    }
}

void ControlHandler::updateGameState(Entity& entity, Game& game) {
    const Keys& keys = game.getInput();
    auto control = game.getAttribute<ControlAttribute>(Property::Control, entity.getEntityID());
    if (!control) return;

    const int64_t elapsed_time = game.getTimeline()->getTime();

    // Movement and ability keys, tracked for double-tap sequences
    const SDL_Scancode keysOfInterest[] = {SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT,
                                           SDL_SCANCODE_RIGHT};

    // Process key presses and releases
    for (SDL_Scancode key : keysOfInterest) {
        bool isPressed = keys.held(key);
        bool wasPressed = previousKeyStates[key];

        if (!wasPressed && isPressed) {
            // Key was just pressed
            keySequence.push_back({key, static_cast<Uint32>(elapsed_time)});
            if (entity.getisMovable() && entity.getIsControllable())
                checkForSequences(key, static_cast<Uint32>(elapsed_time), entity, game);
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

    // Check for chords
    checkForChords(keys, entity, game, static_cast<Uint32>(elapsed_time));

    // Remember this frame's keys for edge detection next frame
    for (SDL_Scancode key : keysOfInterest) previousKeyStates[key] = keys.held(key);
}

// Helper function to check for chords (simultaneous key presses)
void ControlHandler::checkForChords(const Keys& keys, Entity& entity, Game& game,
                                    Uint32 timestamp) {
    const Uint32 currentTime = timestamp;  // Timeline time

    // pressing 'S' and 'H' together triggers the shrink mechanic
    if (keys.held(SDL_SCANCODE_S) && keys.held(SDL_SCANCODE_H)) {
        if (!isShrunk) {
            // Shrink the entity
            SDL_Rect rect = entity.getRect();
            rect.w /= 2;  // Halve the width
            rect.h /= 2;  // Halve the height
            entity.setRect(rect);

            isShrunk = true;
            shrinkStartTime = currentTime;
        }
    }

    // Check if shrink duration has passed
    if (isShrunk && currentTime - shrinkStartTime >= 5000) {  // 5 seconds duration
        // Restore original size
        SDL_Rect rect = entity.getRect();
        rect.w *= 2;  // Restore width
        rect.h *= 2;  // Restore height
        entity.setRect(rect);

        isShrunk = false;
    }

    // Add more chords in future
}

// Helper function to check for sequences (specific key press patterns)
void ControlHandler::checkForSequences(SDL_Scancode key, Uint32 timestamp, Entity& entity,
                                       Game& game) {
    // Define the direction keys
    SDL_Scancode directionKeys[] = {SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT,
                                    SDL_SCANCODE_RIGHT};

    // Remove old entries from keySequence (older than SEQUENCE_TIMEOUT milliseconds)
    const Uint32 SEQUENCE_TIMEOUT = 250;  // quarter of a second
    while (!keySequence.empty() && (timestamp - keySequence.front().second) > SEQUENCE_TIMEOUT) {
        keySequence.pop_front();
    }

    // Check if the last two key presses are the same direction key
    if (keySequence.size() >= 2) {
        auto it = keySequence.rbegin();
        SDL_Scancode lastKey = it->first;
        ++it;
        SDL_Scancode secondLastKey = it->first;

        // Check if both keys are the same and are direction keys
        if (lastKey == secondLastKey) {
            bool isDirectionKey = false;
            for (SDL_Scancode dirKey : directionKeys) {
                if (lastKey == dirKey) {
                    isDirectionKey = true;
                    break;
                }
            }
            if (isDirectionKey) {
                // Attempt to teleport the entity 200 pixels in the direction
                int teleportDistance = 200;
                int targetX = entity.getRect().x;
                int targetY = entity.getRect().y;

                switch (lastKey) {
                    case SDL_SCANCODE_UP:
                        targetY -= teleportDistance;
                        break;
                    case SDL_SCANCODE_DOWN:
                        targetY += teleportDistance;
                        break;
                    case SDL_SCANCODE_LEFT:
                        targetX -= teleportDistance;
                        break;
                    case SDL_SCANCODE_RIGHT:
                        targetX += teleportDistance;
                        break;
                    default:
                        break;
                }

                // Check if another entity is at the target position
                SDL_Rect targetRect = entity.getRect();
                targetRect.x = targetX;
                targetRect.y = targetY;

                bool positionOccupied = false;
                const auto& entities = game.getEntities();  // Map of ID and std::shared_ptr<Entity>

                for (const auto& pair : entities) {
                    std::shared_ptr<Entity> otherEntityPtr = pair.second;
                    if (otherEntityPtr.get() != &entity) {  // Don't compare with self
                        SDL_Rect otherRect = otherEntityPtr->getRect();
                        if (SDL_HasIntersection(&targetRect, &otherRect)) {
                            positionOccupied = true;
                            break;
                        }
                    }
                }

                if (!positionOccupied) {
                    // Teleport the entity
                    entity.setRectX(targetX);
                    entity.setRectY(targetY);
                } else {
                    // Cannot teleport because position is occupied
                    // Optional: Provide feedback to the player
                }

                // Clear the key sequence to prevent immediate retriggering
                keySequence.clear();
            }
        }
    }
}
