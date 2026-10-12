#ifndef ENTITY_HPP
#define ENTITY_HPP

#include <cmath>
#include <cstdint>

#include "timeline.hpp"

class Entity {
public:
    Entity(int x, int y, int w, int h, SDL_Color color, double velY, double velX, bool gravity,
           bool control, bool movable, int EntityID, int ClientID)
        : posX(x),
          posY(y),
          velocityY(velY),
          velocityX(velX),
          hasGravity(gravity),
          isControllable(control),
          isMovable(movable),
          color(color),
          EntityID(EntityID),
          spawnPointId(-1),
          ClientID(ClientID) {
        rect.x = x;
        rect.y = y;
        rect.w = w;
        rect.h = h;
    }
    virtual ~Entity() = default;
    // Getters
    // Rounded view of the Entity's position and size; change it through the setters
    [[nodiscard]] const SDL_Rect& getRect() const { return rect; }
    // Exact position in World Coordinates
    [[nodiscard]] double getX() const { return posX; }
    [[nodiscard]] double getY() const { return posY; }
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
    void setRect(const SDL_Rect& newRect) {
        rect = newRect;
        posX = newRect.x;
        posY = newRect.y;
    }
    void setPosition(double x, double y) {
        posX = x;
        posY = y;
        rect.x = static_cast<int>(std::lround(x));
        rect.y = static_cast<int>(std::lround(y));
    }
    void moveBy(double dx, double dy) { setPosition(posX + dx, posY + dy); }
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
    void setRectX(int x) { setPosition(x, posY); };
    void setRectY(int y) { setPosition(posX, y); };

    // Time step: each Entity tracks its own last update time on the Timeline
    void startClock(int64_t now) { lastTime = now; }
    // Records the Timeline time at the start of a frame and returns the time since the last frame
    int64_t beginFrame(int64_t now) {
        frameDelta = now - lastTime;
        lastTime = now;
        return frameDelta;
    }
    [[nodiscard]] int64_t getFrameDelta() const { return frameDelta; }
    // Game time, in Timeline units, that the current update step covers
    [[nodiscard]] double getDeltaTime() const { return deltaTime; }
    void setDeltaTime(double dt) { deltaTime = dt; }

    // Standing on something: set when collision pushes the Entity up out of a solid Entity
    [[nodiscard]] bool isGrounded() const { return grounded; }
    [[nodiscard]] int getGroundEntityId() const { return groundEntityId; }
    void setGrounded(bool isOnGround, int groundId = -1) {
        grounded = isOnGround;
        groundEntityId = isOnGround ? groundId : -1;
    }

private:
    SDL_Rect rect;        // rectangle shape
    double posX, posY;    // exact position
    double velocityY;     // vertical velocity
    double velocityX;     // horizontal velocity
    bool hasGravity;      // gravity applies to entity or not
    bool isControllable;  // is controlled by input
    bool isMovable;       // is movable by other game objects
    SDL_Color color;      // color
    int EntityID;         // Entity ID
    int spawnPointId;
    int ClientID;  // Client ID

    int64_t lastTime = 0;
    int64_t frameDelta = 0;
    double deltaTime = 0;
    bool grounded = false;
    int groundEntityId = -1;
};

#endif
