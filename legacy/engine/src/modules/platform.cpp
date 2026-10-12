#include "platform.hpp"

void Platform::update(int64_t delta_time) {
    if (startY == 0 && endY == 0) {
        // Move the platform horizontally
        setRectX((int)(getRect().x + getVelocityX() * (double)delta_time * directionX));
        // Change horizontal direction if the platform reaches the horizontal boundaries
        if (getRect().x <= startX) {
            directionX = 1;
        }
        if (getRect().x >= endX) {
            directionX = -1;
        }
    } else if (startX == 0 && endX == 0) {
        // Move the platform vertically
        setRectY((int)(getRect().y + getVelocityY() * (double)delta_time * directionY));
        // Change vertical direction if the platform reaches the vertical boundaries
        // Change horizontal direction if the platform reaches the horizontal boundaries
        if (getRect().y <= startX) {
            directionX = 1;
        }
        if (getRect().y >= endX) {
            directionX = -1;
        }
    }
}
