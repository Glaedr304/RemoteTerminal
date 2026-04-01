#pragma once

#include "CoreTypes.h"
#include "GameEntities.h"
#include "Fleet.h"
#include "Action.h"
#include <string>
#include <variant>
#include <unordered_map>
#include <unordered_set>

// Hash specialization for std::pair<coord, UserId>
template<>
struct std::hash<std::pair<coord, UserId>> {
    std::size_t operator()(const std::pair<coord, UserId>& p) const noexcept {
        std::size_t h1 = std::hash<coord>{}(p.first);
        std::size_t h2 = std::hash<UserId>{}(p.second);
        return h1 ^ (h2 << 1);
    }
};

namespace NavalBattle {

    enum class TransientSquareState {
        invalidPlacement,
        validPlacement,
        targetedSquare,
        torpedoUp,
        torpedoDown,
        torpedoLeft,
        torpedoRight
    };

    enum class SessionActionResultError {
        //session-level errors
        shipNotFound,     //the specified shippId is not in this session
        userNotFound,     //the specified userId is not in this session
        unknownAction,    //the action requested is not valid

        //gameplay violations
        wrongPhase,       //action cannot be performed in this game phase
        invalidPlacement, //tried to so something with invalid coordinates
        notYourTurn,      //tried to do something when not allowed to act

        //ability errors
        vehicleNotFound,  //the specified vehicleId is not in this session
        vehicleSunk,      //the vehicle is sunk and cannot use abilities
        noSuchAbility,    //the vehicle does not have the requested ability

        //catch all for engine
        internalError     //generic error
    };

    enum class SessionActionResultType {
        FireResult,
        FireAntiAircraftResult,
        ReadyResult,
        PlaceShipResult,
        PlacePlaneResult,
        TransientOverlayResult,
        RematchResult,
        ActivateAbilityResult
    };

    struct PlaceShipResultData {
        //does not need data, will be enough to see if result was successful
    };

    struct PlacePlaneResultData {
        //does not need data, will be enough to see if result was successful
    };

    struct ReadyResultData {
        //does not need data, will be enough to see if result was successful
    };

    struct FireResultData {
        bool isHit = false;
        bool isSunk = false;
        std::string sunkName = "";
        int hitId = 0;
    };

    struct FireAntiAircraftResultData {
        bool isHit = false;
        bool isDestroyed = false;
        std::string destroyedName = "";
        int hitId = 0;
    };

    struct TransientOverlayData {
        std::unordered_map<std::pair<coord, UserId>, std::unordered_set<TransientSquareState>> overlay;
    };

    struct RematchResultData {
        // no data needed
    };

    using SessionActionResultData = std::variant<PlaceShipResultData, PlacePlaneResultData, ReadyResultData, FireResultData, FireAntiAircraftResultData, TransientOverlayData, RematchResultData, ActivateAbilityResult>;

    struct SessionActionResult {
        bool success = false;
        UserId actingUser; // the user who performed this action
        SessionActionResultError error = SessionActionResultError::internalError;
        SessionActionResultType type = SessionActionResultType::FireResult;
        SessionActionResultData data = FireResultData();
    };

    struct UserView {
        UserId userId;
        BoardView boardView;
    };

    struct FleetView {
        std::vector<Ship> yourShips;
        std::vector<Ship> opponentShips;
        std::vector<Plane> yourPlanes;
        std::vector<Plane> opponentPlanes;
    };

    struct UserSnapshot {
        Phase phase;
        UserId currentUser;
        UserView userView;
        FleetView fleetView;
        bool youReady;
        bool opponentReady;
    };

    struct StartupInfo {
        Phase phase;
        UserId you;
        UserId opponent;
        GameId gameId;
        UserView userView;
        FleetView fleetView;
        int boardRows;
        int boardCols;
    };

    enum class AddUserToGameError {
        userAlreadyInGame,
        gameFull
    };

    struct AddUserToGameResult {
        bool success = false;
        bool readyToStart = false;
        AddUserToGameError error;
    };

    struct RematchRequest {
        UserId requestingUser;
    };

    struct RematchStart {
        // Signal to both players that rematch is starting
    };

    using OutboundMessage = std::variant<UserSnapshot, StartupInfo, SessionActionResult, AddUserToGameResult, RematchRequest, RematchStart>;

    struct AddressedMessage {
        OutboundMessageReciever address;
        OutboundMessage message;
    };

    class AddressedMessageBundle {
    public:
        AddressedMessageBundle& addMessage(const OutboundMessageReciever& reciever, const OutboundMessage& message) {
            messages.emplace_back(AddressedMessage{ reciever, message });
            return *this;
        }

        AddressedMessageBundle& addMessageBundle(const AddressedMessageBundle& bundle) {
            for (const auto& msg : bundle.messages) {
                messages.emplace_back(msg);
            }
            return *this;
        }

        auto begin() { return messages.begin(); }
        auto end() { return messages.end(); }

        auto begin() const { return messages.begin(); }
        auto end() const { return messages.end(); }

    private:
        std::vector<AddressedMessage> messages;
    };

    struct JoinRequest {
        UserId userId;
        GameId gameId;
    };

    struct ActionRequest {
        GameId gameId;
        UserId userId;
        SessionAction action;
    };

} // namespace NavalBattle

template<>
struct std::hash<NavalBattle::TransientSquareState> {
    std::size_t operator()(const NavalBattle::TransientSquareState& state) const noexcept {
        return std::hash<int>()(static_cast<int>(state));
    }
};