#ifndef SIDE_BOUNDARY_ATTRIBUTE_HPP
#define SIDE_BOUNDARY_ATTRIBUTE_HPP

#include "attribute.hpp"
#include "eventmanager.hpp"
#include "game.hpp"

// Marks an Entity as a zone at the left or right edge of the screen. Unlike every other Entity,
// a SideBoundary zone's rect is in screen space (relative to the Camera, before scaling): it is an
// edge of the view rather than a place in the world, so it stays put while the Camera scrolls.
// When the local player enters it, SideBoundaryHandler scrolls the Camera. Zones aren't drawn.
class SideBoundaryAttribute : public Attribute {
public:
    SideBoundaryAttribute(bool isLeft) : isLeftBoundary(isLeft) {}
    void update(Entity& entity, Game& game) override;

private:
    bool isLeftBoundary;
};

#endif  // SIDE_BOUNDARY_ATTRIBUTE_HPP