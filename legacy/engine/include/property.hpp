// property.hpp
#ifndef PROPERTY_HPP
#define PROPERTY_HPP

// Custom is a game-defined Attribute; the engine never updates it, games fetch it with
// Game::getAttribute
enum class Property {
    Control,
    Collide,
    Move,
    Gravity,
    SpawnPoint,
    DeathZone,
    SideBoundary,
    Custom
};

#endif  // PROPERTY_HPP
