#include "platform.hpp"

void Platform::update(double delta_time) {
    const double startXPos = getX();
    const double startYPos = getY();
    if (endX > startX) {
        double x = getX() + getVelocityX() * delta_time * directionX;
        if (x <= startX) {
            x = startX;
            directionX = 1;
        }
        if (x >= endX) {
            x = endX;
            directionX = -1;
        }
        setPosition(x, getY());
    } else if (endY > startY) {
        double y = getY() + getVelocityY() * delta_time * directionY;
        if (y <= startY) {
            y = startY;
            directionY = 1;
        }
        if (y >= endY) {
            y = endY;
            directionY = -1;
        }
        setPosition(getX(), y);
    }
    lastDeltaX = getX() - startXPos;
    lastDeltaY = getY() - startYPos;
}
