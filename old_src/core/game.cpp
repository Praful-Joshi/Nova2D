#include "game.hpp"

#include <mutex>
#include <utility>

#include "peer.hpp"

struct EntityState {
    int x, y, width, height, clientID;
    double velocityX, velocityY;
    bool hasGravity, isMovable, isControllable;
    SDL_Color color;
};
std::unordered_map<int, EntityState> previousEntityStates;

void Game::addEntity(const std::shared_ptr<Entity>& entity) {
    int entityID = entity->getEntityID();  // Assuming Entity has this method
    entities[entityID] = entity;           // Store the entity by its ID
}

void Game::updatePreviousEntityStates(Game& game) {
    auto gameEntities = game.getEntities();
    for (const auto& [entityID, entity] : gameEntities) {
        previousEntityStates[entityID] = EntityState{
            entity->getRect().x,         entity->getRect().y,     entity->getRect().w,
            entity->getRect().h,         entity->getClientID(),   entity->getVelocityX(),
            entity->getVelocityY(),      entity->getHasGravity(), entity->getisMovable(),
            entity->getIsControllable(), entity->getColor()};
    }
}

void Game::removeEntity(const std::shared_ptr<Entity>& entity) {
    entities.erase(entity->getEntityID());
}

// Property management
void Game::addProperty(Property propertyType, int entityId, std::shared_ptr<Attribute> attribute) {
    properties[propertyType][entityId] = std::move(attribute);
}

void Game::updateProperties(Property propertyType, Timeline* timeline) {
    auto it = properties.find(propertyType);
    if (it != properties.end()) {
        for (const auto& [entityId, attribute] : it->second) {
            auto entity = getEntity(entityId);
            if (entity != nullptr && attribute) {
                // Use the updated method signature
                attribute->update(*entity, *this);
            }
        }
    }
}

std::shared_ptr<Entity> Game::getEntity(const int& GUID) {
    auto it = entities.find(GUID);  // Find the entity with the specified GUID
    if (it != entities.end()) {
        return it->second;  // Return the shared_ptr to the entity
    }
    return nullptr;  // Return nullptr if the entity is not found
}

bool Game::entityHasProperty(Property propertyType, int entityId) const {
    auto propertyIt = properties.find(propertyType);
    if (propertyIt != properties.end()) {
        const auto& entityMap = propertyIt->second;
        return entityMap.find(entityId) != entityMap.end();
    }
    return false;  // Property or entity ID not found
}

std::vector<std::string> Game::serializeEntitiesServerSide(Game& game) {
    std::vector<std::string> jsonStrings;
    std::mutex game_entities_mutex;
    game_entities_mutex.lock();
    auto gameEntities = game.getEntities();
    for (const auto& [entityID, entity] : gameEntities) {
        boost::property_tree::ptree tree;

        // Get current state
        auto currentState = EntityState{
            entity->getRect().x,         entity->getRect().y,     entity->getRect().w,
            entity->getRect().h,         entity->getClientID(),   entity->getVelocityX(),
            entity->getVelocityY(),      entity->getHasGravity(), entity->getisMovable(),
            entity->getIsControllable(), entity->getColor()};
        if (useEfficientCommunication && didAllPlayersJoin) {
            bool stateChanged = true;
            const auto& prevState = previousEntityStates[entityID];
            stateChanged =
                (prevState.x != currentState.x || prevState.y != currentState.y ||
                 prevState.width != currentState.width || prevState.height != currentState.height ||
                 prevState.clientID != currentState.clientID ||
                 prevState.velocityX != currentState.velocityX ||
                 prevState.velocityY != currentState.velocityY ||
                 prevState.hasGravity != currentState.hasGravity ||
                 prevState.isMovable != currentState.isMovable ||
                 prevState.isControllable != currentState.isControllable ||
                 prevState.color.r != currentState.color.r ||
                 prevState.color.g != currentState.color.g ||
                 prevState.color.b != currentState.color.b ||
                 prevState.color.a != currentState.color.a);
            if (stateChanged) {
                tree.put("EntityID", entity->getEntityID());
                tree.put("ClientID", entity->getClientID());
                tree.put("x", entity->getRect().x);
                tree.put("y", entity->getRect().y);
                tree.put("width", entity->getRect().w);
                tree.put("height", entity->getRect().h);
                tree.put("velocityX", entity->getVelocityX());
                tree.put("velocityY", entity->getVelocityY());
                tree.put("hasGravity", entity->getHasGravity());
                tree.put("isMovable", entity->getisMovable());
                tree.put("isControllable", false);
                tree.put("color.r", entity->getColor().r);
                tree.put("color.b", entity->getColor().b);
                tree.put("color.g", entity->getColor().g);
                tree.put("color.a", entity->getColor().a);

                // Convert ptree to a JSON string
                std::ostringstream buf;
                boost::property_tree::write_json(buf, tree);
                jsonStrings.push_back(buf.str());
                previousEntityStates[entityID] = currentState;
                continue;
            } else {
                continue;
            }
        }

        tree.put("EntityID", entity->getEntityID());
        tree.put("ClientID", entity->getClientID());
        tree.put("x", entity->getRect().x);
        tree.put("y", entity->getRect().y);
        tree.put("width", entity->getRect().w);
        tree.put("height", entity->getRect().h);
        tree.put("velocityX", entity->getVelocityX());
        tree.put("velocityY", entity->getVelocityY());
        tree.put("hasGravity", entity->getHasGravity());
        tree.put("isMovable", entity->getisMovable());
        tree.put("isControllable", false);
        tree.put("color.r", entity->getColor().r);
        tree.put("color.b", entity->getColor().b);
        tree.put("color.g", entity->getColor().g);
        tree.put("color.a", entity->getColor().a);

        // Convert ptree to a JSON string
        std::ostringstream buf;
        boost::property_tree::write_json(buf, tree);
        jsonStrings.push_back(buf.str());
        previousEntityStates[entityID] = currentState;
    }
    game_entities_mutex.unlock();
    return jsonStrings;
}

std::vector<std::string> Game::serializeEntitiesClientSide(Game& game) {
    std::vector<std::string> jsonStrings;
    std::mutex game_entities_mutex;
    game_entities_mutex.lock();
    auto gameEntities = game.getEntities();
    for (const auto& [entityID, entity] : gameEntities) {
        if (entity->getIsControllable()) {
            boost::property_tree::ptree tree;
            tree.put("EntityID", entity->getEntityID());
            tree.put("ClientID", entity->getClientID());
            tree.put("x", entity->getRect().x);
            tree.put("y", entity->getRect().y);
            tree.put("width", entity->getRect().w);
            tree.put("height", entity->getRect().h);
            tree.put("velocityX", entity->getVelocityX());
            tree.put("velocityY", entity->getVelocityY());
            tree.put("hasGravity", entity->getHasGravity());
            tree.put("isMovable", entity->getisMovable());
            tree.put("isControllable", entity->getIsControllable());
            tree.put("color.r", entity->getColor().r);
            tree.put("color.b", entity->getColor().b);
            tree.put("color.g", entity->getColor().g);
            tree.put("color.a", entity->getColor().a);

            // Convert ptree to a JSON string
            std::ostringstream buf;
            boost::property_tree::write_json(buf, tree);
            jsonStrings.push_back(buf.str());
        }
    }
    game_entities_mutex.unlock();
    return jsonStrings;
}

void Game::deserializeEntitiesServerSide(const std::vector<std::string>& jsons, Game& game,
                                         Peer& peer) {
    std::mutex game_entities_mutex;
    game_entities_mutex.lock();
    for (const auto& json : jsons) {
        boost::property_tree::ptree tree;
        std::istringstream iss(json);

        // Read the JSON string into the property tree
        boost::property_tree::read_json(iss, tree);

        // Extract values from the property tree
        int EntityID = tree.get<int>("EntityID");
        int ClientID = tree.get<int>("ClientID");
        int x = tree.get<int>("x");
        int y = tree.get<int>("y");
        int width = tree.get<int>("width");
        int height = tree.get<int>("height");
        double velocityX = tree.get<double>("velocityX");
        double velocityY = tree.get<double>("velocityY");
        bool hasGravity = tree.get<bool>("hasGravity");
        bool isControllable = tree.get<bool>("isControllable");
        bool isMovable = tree.get<bool>("isMovable");

        // Extract color values
        SDL_Color color = {static_cast<Uint8>(tree.get<int>("color.r")),
                           static_cast<Uint8>(tree.get<int>("color.g")),
                           static_cast<Uint8>(tree.get<int>("color.b")),
                           static_cast<Uint8>(tree.get<int>("color.a"))};

        auto gameEntities = game.getEntities();

        if (gameEntities.find(EntityID) != gameEntities.end()) {
            // Update existing entity
            auto entity = gameEntities[EntityID];
            entity->setColor(color);
            entity->setClientID(ClientID);
            entity->setRectX(x);
            entity->setRectY(y);
            entity->setRectWidth(width);
            entity->setRectHeight(height);
            entity->setVelocityX(velocityX);
            entity->setVelocityY(velocityY);
            entity->setHasGravity(hasGravity);
            entity->setIsMovable(isMovable);
            entity->setIsControllable(false);
        } else {
            // Create new entity if it doesn't exist
            auto newEntity =
                std::make_shared<Entity>(x, y, width, height, color, velocityX, velocityY,
                                         hasGravity, false, isMovable, EntityID, ClientID);
            game.addEntity(newEntity);
            game.addProperty(Property::Collide, EntityID, std::make_shared<CollisionAttribute>());
            if (hasGravity) {
                game.addProperty(Property::Gravity, EntityID, std::make_shared<GravityAttribute>());
            }
            if (isControllable) {
                game.addProperty(Property::Control, EntityID, std::make_shared<ControlAttribute>());
            }
            if (isMovable) {
                game.addProperty(Property::Move, EntityID, std::make_shared<MovementAttribute>());
            }
        }

        if (ClientID == noPlayersJoining && !didAllPlayersJoin) {
            didAllPlayersJoin = true;
        }

        if (ClientID == -1) {
            if (gameEntities.find(EntityID) != gameEntities.end()) {
                // this entity is present locally
                peer.send_data(game.serializeEntitiesServerSide(game));
                game.removeEntity(game.getEntity(EntityID));
            }
        }
    }
    game_entities_mutex.unlock();
}

void Game::deserializeEntitiesClientSide(const std::vector<std::string>& jsons, Game& game,
                                         Peer& peer) {
    std::mutex game_entities_mutex;
    game_entities_mutex.lock();
    auto gameEntities = game.getEntities();
    for (const auto& json : jsons) {
        boost::property_tree::ptree tree;
        std::istringstream iss(json);

        // Read the JSON string into the property tree
        boost::property_tree::read_json(iss, tree);

        // Extract values
        int EntityID = tree.get<int>("EntityID");
        int ClientID = tree.get<int>("ClientID");
        if (ClientID == -1) {
            if (gameEntities.find(EntityID) != gameEntities.end()) {
                // this entity is present locally
                game.removeEntity(game.getEntity(EntityID));
                continue;
            }
        }
        if (ClientID == -2) {
            // server left make new server
            if (game.ClientID == EntityID + 1) {
                // make this instance server
                peer = Peer(true, game.ClientID);
                game.removeEntity(game.getEntity(EntityID));
            }
        }
        int x = tree.get<int>("x");
        int y = tree.get<int>("y");
        int width = tree.get<int>("width");
        int height = tree.get<int>("height");
        double velocityX = tree.get<double>("velocityX");
        double velocityY = tree.get<double>("velocityY");
        bool hasGravity = tree.get<bool>("hasGravity");
        bool isControllable = tree.get<bool>("isControllable");
        bool isMovable = tree.get<bool>("isMovable");

        // Extract color values
        SDL_Color color = {static_cast<Uint8>(tree.get<int>("color.r")),
                           static_cast<Uint8>(tree.get<int>("color.g")),
                           static_cast<Uint8>(tree.get<int>("color.b")),
                           static_cast<Uint8>(tree.get<int>("color.a"))};

        if (gameEntities.find(EntityID) != gameEntities.end()) {
            // Update existing entity
            auto entity = gameEntities[EntityID];
            entity->setColor(color);
            entity->setRectX(x);
            entity->setRectY(y);
            entity->setRectWidth(width);
            entity->setRectHeight(height);
            entity->setVelocityX(velocityX);
            entity->setVelocityY(velocityY);
            entity->setHasGravity(hasGravity);
            entity->setIsMovable(isMovable);
            if (entity->getClientID() == game.ClientID) {
                entity->setIsControllable(true);  // Allow control for the current client
            } else {
                entity->setIsControllable(false);  // Disable control for other clients
            }
        } else {
            // Create a new entity
            auto newEntity =
                std::make_shared<Entity>(x, y, width, height, color, velocityX, velocityY,
                                         hasGravity, false, isMovable, EntityID, ClientID);
            game.addEntity(newEntity);
            game.addProperty(Property::Collide, EntityID, std::make_shared<CollisionAttribute>());
            if (hasGravity) {
                game.addProperty(Property::Gravity, EntityID, std::make_shared<GravityAttribute>());
            }
            if (isControllable) {
                game.addProperty(Property::Control, EntityID, std::make_shared<ControlAttribute>());
            }
            if (isMovable) {
                game.addProperty(Property::Move, EntityID, std::make_shared<MovementAttribute>());
            }
        }
    }
    game_entities_mutex.unlock();
}