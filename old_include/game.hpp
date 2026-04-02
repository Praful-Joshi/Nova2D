#ifndef GAME_HPP
#define GAME_HPP

#ifndef _WIN32
#include <unistd.h>

#include <mutex>
#include <thread>
#else
#include <windows.h>
#endif

// #if (defined (WIN32))
// #include <zhelpers.hpp>
// #endif

#include <utility>
#include <vector>

#include "property.hpp"
// #include <zmq.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <unordered_map>

#include "attribute.hpp"
#include "collisions.hpp"
#include "control.hpp"
#include "gravity.hpp"
#include "movement.hpp"
#include "peer.hpp"
#include "scaling.hpp"

class EventManager;

class Game {
public:
    Game(int control, double gravity, int curW, int curH, int ClientID, bool useEfficientComm,
         int noplayersJoining)
        : ClientID(ClientID) {
        controllableSpeed = control;
        gravityStrength = gravity;
        cur_w = curW;
        cur_h = curH;
        useEfficientCommunication = useEfficientComm;
        noPlayersJoining = noplayersJoining;
    }

    // void addEntity(Entity* entity);
    void addEntity(const std::shared_ptr<Entity>& entity);
    void removeEntity(const std::shared_ptr<Entity>& entity);
    void updatePreviousEntityStates(Game& game);

    std::shared_ptr<Entity> getEntity(const int& GUID);

    bool entityHasProperty(Property propertyType, int entityId) const;

    // Getters and Setters
    std::unordered_map<int, std::shared_ptr<Entity>>& getEntities() {
        return entities;  // Return the map of entities as a reference
    }

    int getControllableSpeed() const { return controllableSpeed; }
    void setControllableSpeed(int speed) { controllableSpeed = speed; }

    std::unordered_map<int, SDL_Rect>& getOriginalPositions() { return originalPositions; }
    void setOriginalPositions(std::unordered_map<int, SDL_Rect>& gameOriginalPositions) {
        originalPositions = std::move(gameOriginalPositions);
    }

    double getGravityStrength() const { return gravityStrength; }
    void setGravityStrength(double gravity) { gravityStrength = gravity; }

    int getCurrentWidth() const { return cur_w; }
    void setCurrentWidth(int width) { cur_w = width; }

    int getCurrentHeight() const { return cur_h; }
    void setCurrentHeight(int height) { cur_h = height; }

    Timeline* getTimeline() const { return timeline; }
    void setTimeline(Timeline& gameTimeline) { timeline = &gameTimeline; }

    bool getIsProportionalScaling() const { return isProportionalScaling; };
    void toggleScalingMode() { isProportionalScaling = !isProportionalScaling; };

    EventManager* getEventManager() const { return eventManager; }
    void setEventManager(EventManager& eventMgr) { eventManager = &eventMgr; }

    void addProperty(Property propertyType, int entityId, std::shared_ptr<Attribute> attribute);
    void updateProperties(Property propertyType, Timeline* timeline);

    // void updateEntitySizes(ScalingMode scalingMode, std::unordered_map<Entity*, SDL_Rect>
    // originalSizes, int cur_w, int cur_h);

    std::vector<std::string> serializeEntitiesServerSide(Game& game);
    std::vector<std::string> serializeEntitiesClientSide(Game& game);
    void deserializeEntitiesServerSide(const std::vector<std::string>& jsons, Game& game,
                                       Peer& peer);
    void deserializeEntitiesClientSide(const std::vector<std::string>& jsons, Game& game,
                                       Peer& peer);

    // std::vector<std::string> serializeEntitiesClientSide();
    // std::vector<std::string> serializeEntitiesServerSide();

    // void deserializeEntitiesClientSide(const std::vector<std::string>& jsons, Game& game);
    // void deserializeEntitiesServerSide(const std::vector<std::string>& jsons, Game& game);

    // void PublishGameState();

    // void ConnectToServer(const std::string &sub_address, const std::string &pub_address);

    // void UpdateGameState(bool initializeGame);

    // void initializeGame(const std::vector<std::string> &jsons, Game &game);

    // current client ID
    const int ClientID;
    std::vector<SDL_Scancode> keysPressed;

private:
    // List of entities
    // std::vector<Entity*> entities;

    // Map of maps for property-centric architecture
    std::unordered_map<Property, std::unordered_map<int, std::shared_ptr<Attribute>>> properties;
    // Map of GUID -> Entity
    std::unordered_map<int, std::shared_ptr<Entity>>
        entities;  // Map of entity IDs to entity objects

    // default controllable shape speed
    int controllableSpeed = 3;
    // default gravity
    double gravityStrength = 0.1;
    // current window width
    int cur_w;
    // current window height
    int cur_h;

    std::unordered_map<int, SDL_Rect> originalPositions;

    // timeline
    Timeline* timeline;

    bool isProportionalScaling = false;  // Track current scaling mode

    // Current scaling mode
    // ScalingMode scalingMode = ScalingMode::ConstantSize;

    // Stores the initial size of each entity for proportional scaling
    std::unordered_map<Entity*, SDL_Rect> originalSizes;

    bool useEfficientCommunication = false;
    int noPlayersJoining = 0;
    bool didAllPlayersJoin = false;

    EventManager* eventManager;
    // Client Sockets
    // zmq::context_t context;
    // zmq::socket_t  publisher;
    // zmq::socket_t  subscriber;
};

#endif
