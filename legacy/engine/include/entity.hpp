#ifndef ENTITY_HPP
#define ENTITY_HPP

#include "timeline.hpp"

class Entity {
public:
    Entity(int x, int y, int w, int h, SDL_Color color, double velY, double velX, bool gravity,
           bool control, bool movable, int EntityID, int ClientID)
        : velocityY(velY),
          velocityX(velX),
          hasGravity(gravity),
          isControllable(control),
          isMovable(movable),
          color(color),
          EntityID(EntityID),
          ClientID(ClientID) {
        rect.x = x;
        rect.y = y;
        rect.w = w;
        rect.h = h;
    }
    virtual ~Entity() = default;
    // Getters
    SDL_Rect& getRect() { return rect; }
    [[nodiscard]] double getVelocityY() const { return velocityY; }
    [[nodiscard]] double getVelocityX() const { return velocityX; }
    [[nodiscard]] bool getHasGravity() const { return hasGravity; }
    [[nodiscard]] bool getIsControllable() const { return isControllable; }
    [[nodiscard]] bool getisMovable() const { return isMovable; }
    [[nodiscard]] SDL_Color getColor() const { return color; }
    [[nodiscard]] int getEntityID() const { return EntityID; }
    [[nodiscard]] int getClientID() const { return ClientID; }
    [[nodiscard]] int getSpawnPointId() const { return spawnPointId; }

    // Setters
    void setRect(SDL_Rect& newRect) { rect = newRect; }
    void setVelocityY(double vy) { velocityY = vy; }
    void setVelocityX(double vx) { velocityX = vx; }
    void setHasGravity(bool gravity) { hasGravity = gravity; }
    void setIsControllable(bool controllable) { isControllable = controllable; }
    void setColor(SDL_Color newColor) { color = newColor; }
    void setEntityID(int newID) { EntityID = newID; }
    void setClientID(int newID) { ClientID = newID; }
    void setIsMovable(bool movable) { isMovable = movable; }
    void setSpawnPointId(int spawnID) { spawnPointId = spawnID; }

    void setRectWidth(int w) { rect.w = w; };
    void setRectHeight(int h) { rect.h = h; };
    void setRectX(int x) { rect.x = x; };
    void setRectY(int y) { rect.y = y; };

private:
    SDL_Rect rect;        // rectangle shape
    double velocityY;     // vertical velocity
    double velocityX;     // horizontal velocity
    bool hasGravity;      // gravity applies to entity or not
    bool isControllable;  // is controlled by input
    bool isMovable;       // is movable by other game objects
    SDL_Color color;      // color
    int EntityID;         // Entity ID
    int spawnPointId;
    int ClientID;  // Client ID
};

#endif
