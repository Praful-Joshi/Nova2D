#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#include "entity.hpp"

class Platform : public Entity {
public:
    // Constructor for Platform with specific movement properties
    Platform(int x, int y, int w, int h, SDL_Color color, double velY, double velX, bool gravity,
             int EntityID, int ClientID, int startX, int endX, int startY, int endY)
        : Entity(x, y, w, h, color, velY, velX, false, false, false, EntityID, ClientID),
          startX(startX),
          endX(endX),
          startY(startY),
          endY(endY),
          directionX(1),
          directionY(1) {}  // Default direction is forward

    // Moves the platform along its axis for delta_time, turning around at its limits.
    // Horizontal when endX > startX, vertical when endY > startY.
    void update(double delta_time);
    // Platform-specific getters and setters
    [[nodiscard]] int getStartX() const { return startX; }
    [[nodiscard]] int getEndX() const { return endX; }
    [[nodiscard]] int getStartY() const { return startY; }
    [[nodiscard]] int getEndY() const { return endY; }
    // How far the platform moved in its last update, so riders can move with it
    [[nodiscard]] double getLastDeltaX() const { return lastDeltaX; }
    [[nodiscard]] double getLastDeltaY() const { return lastDeltaY; }

private:
    int startX, endX, startY, endY;  // Start and end positions for the platform's movement
    int directionX, directionY;      // Direction of movement (+1 or -1)
    double lastDeltaX = 0, lastDeltaY = 0;
};

#endif
