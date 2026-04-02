#include "control_handler.hpp"
#ifdef __linux__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#include <cstring>

void ControlHandler::onEvent(const Event& event) {
    if (event.type == EventType::CONTROL) {
        updateGameState(*event.context.entity, *event.context.game);
    }
}

void ControlHandler::updateGameState(Entity& entity, Game& game) {
    const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);

    Timeline* timeline = game.getTimeline();
    int64_t elapsed_time = timeline->getTime();
    static int64_t last_time = elapsed_time;

    // Calculate delta time since last frame
    int64_t delta_time = elapsed_time - last_time;
    last_time = elapsed_time;

    // Retrieve gravity attribute from property-centric structure
    auto gravityAttr = entity.getHasGravity();
    bool hasGravity = gravityAttr;

    // Define keys of interest
    SDL_Scancode keysOfInterest[] = {SDL_SCANCODE_UP,    SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT,
                                     SDL_SCANCODE_RIGHT, SDL_SCANCODE_T,    SDL_SCANCODE_P,
                                     SDL_SCANCODE_1,     SDL_SCANCODE_2,    SDL_SCANCODE_3};

    // Process key presses and releases
    for (SDL_Scancode key : keysOfInterest) {
        bool isPressed = currentKeyStates[key];
        bool wasPressed = previousKeyStates[key];

        if (!wasPressed && isPressed) {
            // Key was just pressed
            keySequence.push_back(
                {key, static_cast<Uint32>(elapsed_time / 1000)});  // Convert to milliseconds
            if (entity.getisMovable() && entity.getIsControllable())
                checkForSequences(key, static_cast<Uint32>(elapsed_time / 1000), entity, game);
        }

        if (wasPressed && !isPressed) {
            // Key was just released (if needed, handle key releases here)
        }
    }

    // Handle movement based on current key states and properties
    if (!hasGravity) {
        if (currentKeyStates[SDL_SCANCODE_UP]) {
            entity.setRectY(entity.getRect().y - entity.getVelocityY() * delta_time);
        }
        if (currentKeyStates[SDL_SCANCODE_DOWN]) {
            entity.setRectY(entity.getRect().y + entity.getVelocityY() * delta_time);
        }
    }
    if (currentKeyStates[SDL_SCANCODE_LEFT]) {
        entity.setRectX(entity.getRect().x - entity.getVelocityX() * delta_time);
    }
    if (currentKeyStates[SDL_SCANCODE_RIGHT]) {
        entity.setRectX(entity.getRect().x + entity.getVelocityX() * delta_time);
    }

    // Jumping logic when gravity is applied
    if (hasGravity && currentKeyStates[SDL_SCANCODE_UP] && entity.getVelocityY() == 0) {
        entity.setVelocityY(-5.0f);  // Set a jump velocity
    }

    // Check for scaling mode toggle
    if (currentKeyStates[SDL_SCANCODE_T]) {
        game.toggleScalingMode();
    }

    // Pause game if 'P' is pressed
    if (currentKeyStates[SDL_SCANCODE_P]) {
        if (timeline->isPaused()) {
            timeline->unpause();
        } else {
            timeline->pause();
        }
    }

    // Speed change based on key press
    if (currentKeyStates[SDL_SCANCODE_1]) {
        timeline->changeTic(500'000);  // Double speed
    }
    if (currentKeyStates[SDL_SCANCODE_2]) {
        timeline->changeTic(1'000'000);  // Normal speed
    }
    if (currentKeyStates[SDL_SCANCODE_3]) {
        timeline->changeTic(2'000'000);  // Half speed
    }

    // Check for chords
    checkForChords(currentKeyStates, entity, game, static_cast<Uint32>(elapsed_time / 1000));

    // Update previousKeyStates for next frame
    memcpy(previousKeyStates, currentKeyStates, sizeof(previousKeyStates));
}

// Helper function to check for chords (simultaneous key presses)
void ControlHandler::checkForChords(const Uint8* currentKeyStates, Entity& entity, Game& game,
                                    Uint32 timestamp) {
    Uint32 currentTime = SDL_GetTicks();  // Get current time in milliseconds

    // pressing 'S' and 'H' together triggers the shrink mechanic
    if (currentKeyStates[SDL_SCANCODE_S] && currentKeyStates[SDL_SCANCODE_H]) {
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
