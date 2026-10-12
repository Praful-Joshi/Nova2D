#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <algorithm>

// A Peer's own view into the world, given as an offset from World Coordinates.
// Scrolling moves the Camera, never the Entities: a point at World (x, y) is drawn at screen
// (x - getX(), y - getY()) before any proportional scaling. The view is the design window size,
// and the Camera is kept so the view stays inside the world.
class Camera {
public:
    Camera(int viewWidth, int viewHeight)
        : viewW(viewWidth), viewH(viewHeight), worldW(viewWidth), worldH(viewHeight) {}

    // Top-left corner of the view, in World Coordinates
    [[nodiscard]] double getX() const { return x; }
    [[nodiscard]] double getY() const { return y; }
    [[nodiscard]] int getViewWidth() const { return viewW; }
    [[nodiscard]] int getViewHeight() const { return viewH; }
    [[nodiscard]] int getWorldWidth() const { return worldW; }
    [[nodiscard]] int getWorldHeight() const { return worldH; }

    // The world spans (0, 0) to (width, height); a world smaller than the view doesn't scroll
    void setWorldSize(int width, int height) {
        worldW = width;
        worldH = height;
        setPosition(x, y);
    }

    void setPosition(double newX, double newY) {
        x = std::clamp(newX, 0.0, static_cast<double>(std::max(0, worldW - viewW)));
        y = std::clamp(newY, 0.0, static_cast<double>(std::max(0, worldH - viewH)));
    }
    void scrollBy(double dx, double dy) { setPosition(x + dx, y + dy); }

private:
    double x = 0;
    double y = 0;
    int viewW, viewH;
    int worldW, worldH;
};

#endif
