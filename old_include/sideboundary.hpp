#ifndef SIDE_BOUNDARY_ATTRIBUTE_HPP
#define SIDE_BOUNDARY_ATTRIBUTE_HPP

#include "attribute.hpp"
#include "eventmanager.hpp"
#include "game.hpp"

class SideBoundaryAttribute : public Attribute {
public:
    SideBoundaryAttribute(bool isLeft) : isLeftBoundary(isLeft) {}
    void update(Entity& entity, Game& game) override;

private:
    bool isLeftBoundary;
};

#endif  // SIDE_BOUNDARY_ATTRIBUTE_HPP