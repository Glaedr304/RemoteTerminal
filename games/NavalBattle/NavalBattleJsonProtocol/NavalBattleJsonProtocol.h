#pragma once

#include <string>
#include <json/json.h>
#include "EndpointTypes.h"
#include "GameEntities.h"
#include "coord.h"
#include "Action.h"
#include "SessionTypes.h"
#include "VehicleAbility.h"
#include "Plane.h"

namespace NavalBattle {

Json::Value toJson(const coord& c);
coord coordFromJson(const Json::Value& v);
GameId gameIdFromJson(const Json::Value& v);
UserId userIdFromJson(const Json::Value& v);
Json::Value parseJson(std::string_view s);

Json::Value toJson(const SessionAction& a);
SessionAction sessionActionFromJson(const Json::Value& v);

Json::Value toJson(const FireData& d);
FireData fireDataFromJson(const Json::Value& v);


Json::Value toJson(const ReadyData& d);
ReadyData readyDataFromJson(const Json::Value& v);

Json::Value toJson(const PlaceShipData& d);
PlaceShipData placeShipDataFromJson(const Json::Value& v);

Json::Value toJson(const PlacePlaneData& d);
PlacePlaneData placePlaneDataFromJson(const Json::Value& v);

Json::Value toJson(const RematchData& d);
RematchData rematchDataFromJson(const Json::Value& v);

Json::Value toJson(const FireAntiAircraftData& d);
FireAntiAircraftData fireAntiAircraftDataFromJson(const Json::Value& v);

Json::Value toJson(const ActivateAbilityData& d);
ActivateAbilityData activateAbilityDataFromJson(const Json::Value& v);

Json::Value toJson(const CheckAbilityData& d);
CheckAbilityData checkAbilityDataFromJson(const Json::Value& v);

Json::Value toJson(const VehicleAbilityType& t);
VehicleAbilityType vehicleAbilityTypeFromJson(const Json::Value& v);

Json::Value toJson(const VehicleAbilityUsagePolicy& p);

Json::Value toJson(const VehicleAbility& a);

Json::Value toJson(const Plane& p);

Json::Value toJson(const TorpedoData::FiringPattern& p);
TorpedoData::FiringPattern torpedoFiringPatternFromJson(const Json::Value& v);

Json::Value toJson(const ApacheData::FiringPattern& p);
ApacheData::FiringPattern apacheFiringPatternFromJson(const Json::Value& v);

Json::Value toJson(const TomahawkData::FiringPattern& p);
TomahawkData::FiringPattern tomahawkFiringPatternFromJson(const Json::Value& v);

Json::Value toJson(const RevealData::FiringPattern& p);
RevealData::FiringPattern revealFiringPatternFromJson(const Json::Value& v);

Json::Value toJson(const TorpedoData& d);
TorpedoData torpedoDataFromJson(const Json::Value& v);

Json::Value toJson(const ExocetData& d);
ExocetData exocetDataFromJson(const Json::Value& v);

Json::Value toJson(const ApacheData& d);
ApacheData apacheDataFromJson(const Json::Value& v);

Json::Value toJson(const TomahawkData& d);
TomahawkData tomahawkDataFromJson(const Json::Value& v);

Json::Value toJson(const RelocateData& d);
RelocateData relocateDataFromJson(const Json::Value& v);

Json::Value toJson(const ScanData& d);
ScanData scanDataFromJson(const Json::Value& v);

Json::Value toJson(const RevealData& d);
RevealData revealDataFromJson(const Json::Value& v);

Json::Value toJson(const VehicleAbilityAction& a);
VehicleAbilityAction vehicleAbilityActionFromJson(const Json::Value& v);

Json::Value toJson(const VehicleAbilityActionData& d);

Json::Value toJson(const SessionActionType& t);
SessionActionType sessionActionTypeFromJson(const Json::Value& v);

Json::Value toJson(const SessionActionData& d);

Json::Value toJson(const SessionActionResultType& r);

Json::Value toJson(const SessionActionResult& r);

Json::Value toJson(Phase p);

Json::Value toJson(const std::string& s);

Json::Value toJson(const std::vector<UserView>& us);

Json::Value toJson(const UserView& u);

Json::Value toJson(const BoardView& b);

Json::Value toJson(const GridView& g);

Json::Value toJson(const SquareState& s);

Json::Value toJson(const SquareView& s);

Json::Value toJson(const SessionActionResultData& s);

Json::Value toJson(const SessionActionResultError& s);

Json::Value toJson(const FireResultData& f);

Json::Value toJson(const ReadyResultData& f);

Json::Value toJson(const PlaceShipResultData& f);

Json::Value toJson(const PlacePlaneResultData& f);

Json::Value toJson(const TransientSquareState& s);

Json::Value toJson(const TransientOverlayData& t);

Json::Value toJson(const FireAntiAircraftResultData& f);

Json::Value toJson(const ActivateAbilityResultError& e);

Json::Value toJson(const TorpedoResultData& d);

Json::Value toJson(const BulkFireResultData& d);

Json::Value toJson(const RelocateResultData& d);

Json::Value toJson(const ScanResultData& d);

Json::Value toJson(const RevealResultData& d);

Json::Value toJson(const ActivateAbilityResultData& d);

Json::Value toJson(const ActivateAbilityResult& f);

Json::Value toJson(const RematchResultData& f);

Json::Value toJson(const RematchRequest& r);

Json::Value toJson(const RematchStart& r);

Json::Value toJson(const UserSnapshot& u);

Json::Value toJson(const FleetView& f);

Json::Value toJson(const StartupInfo& s);

Json::Value toJson(const Fleet& f);

Json::Value toJson(const Ship& s);

Json::Value toJson(const std::set<coord> s);

Json::Value toJson(const AddUserToGameResult& r);

Json::Value toJson(const AddUserToGameError& e);

Json::Value toJson(const OutboundMessage& r);

JoinRequest joinRequestFromJson(const Json::Value& v);

ActionRequest actionRequestFromJson(const Json::Value& v);

OutboundWireMessage outboundWireMessageFromJson(const Json::Value v);

} // namespace NavalBattle
