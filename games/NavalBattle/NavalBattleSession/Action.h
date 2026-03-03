#pragma once

#include "coord.h"
#include "GameEntities.h"
#include <variant>

namespace NavalBattle {

enum class SessionActionType {
    PlaceShip,
    Ready,
    Fire,
    FireAntiAircraft,
    CheckPlacement,
    Rematch,
    ActivateAbility
};

struct PlaceShipData {
    int shipId;
    int rotation;
    coord position;
};

struct ReadyData {};

struct RematchData {};

struct FireData {
    coord target;
};

struct FireAntiAircraftData {
    coord target;
};

struct ActivateAbilityData {
    int vehicleId;
    VehicleAbilityAction abilityAction;
};

// Note: CheckPlacement uses PlaceShipData directly, not in the variant
using SessionActionData = std::variant<PlaceShipData, ReadyData, FireData, FireAntiAircraftData, RematchData, ActivateAbilityData>;

struct SessionAction {
    SessionActionType type;
    SessionActionData data;
};

} // namespace NavalBattle
