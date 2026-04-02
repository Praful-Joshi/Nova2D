// core/attribute.hpp
#ifndef ATTRIBUTE_HPP
#define ATTRIBUTE_HPP

class Entity;  // Forward declaration to avoid circular dependencies
class Game;    // Forward declaration for the Game class

class Attribute {
public:
    virtual ~Attribute() = default;

    // Base update method should be virtual and can have a default implementation
    virtual void update(Entity& entity, Game& game) = 0;
};

#endif  // ATTRIBUTE_HPP
