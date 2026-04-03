#pragma once

#include "coord.h"
#include "GameEntities.h"
#include <variant>

namespace NavalBattle {

enum class SessionActionType {
    PlaceShip,
    PlacePlane,
    Ready,
    Fire,
    FireAntiAircraft,
    CheckPlacement,
    CheckPlanePlacement,
    CheckAbility,
    Rematch,
    ActivateAbility
};

struct PlaceShipData {
    int shipId;
    int rotation;
    coord position;
};

struct PlacePlaneData {
    int planeId;
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

struct CheckAbilityData {
    int vehicleId;
    VehicleAbilityActionData abilityData;
};

// Note: CheckPlacement uses PlaceShipData directly, not in the variant
using SessionActionData = std::variant<PlaceShipData, PlacePlaneData, ReadyData, FireData, FireAntiAircraftData, RematchData, ActivateAbilityData, CheckAbilityData>;

struct SessionAction {
    SessionActionType type;
    SessionActionData data;
};

} // namespace NavalBattle
