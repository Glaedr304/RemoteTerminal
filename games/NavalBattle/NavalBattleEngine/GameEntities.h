#pragma once

#include "coord.h"
#include "VehicleAbility.h"
#include <string>
#include <variant>
#include <vector>
#include <map>
#include <set>

namespace NavalBattle {

enum class Player {
	none = 0,
	one = 1,
	two = 2
};

inline Player opponent(Player p) {
	return (p == Player::one) ? Player::two : Player::one;
}

enum class Phase {
	setup,
	playing,
	finished
};

enum class FireError {
	outOfBounds,
	notYourTurn
};

struct FireResult {
	bool success;
	bool isHit;
	bool isSink;
	int hitId;
	FireError error;
};

enum class PlaceShipError {
	WrongPhase,
	OverlapsAnotherShip,
	OutOfBounds,
	invalidID
};

struct PlaceShipResult {
	bool success;
	PlaceShipError error;
};

struct ValidatePlacementResult {
	bool valid;
	PlaceShipError error;
	std::set<coord> coords;
};

enum class ReadyUpError {
	fleetNotPlaced,
	fleetPlacementInvalid
};

struct ReadyUpResult {
	bool success;
	ReadyUpError error;
};

enum class SquareState {
	miss,
	ship,
	hit
};

using GridView = std::map<coord, SquareState>;
using SquareView = GridView::value_type;

struct BoardView {
	GridView ownGrid;
	GridView opponentGrid;
};

struct TorpedoData {
	enum class FiringPattern {
		vertical,
		horizontal
	};
	FiringPattern firingPattern;
	coord startPoint;
};

struct ExocetData {
	coord target;
};

struct ApacheData {
	enum class FiringPattern {
		vertical,
		horizontal
	};
	FiringPattern firingPattern;
	coord target;
};

struct TomahawkData {
	enum class FiringPattern {
		plus,
		x
	};
	FiringPattern firingPattern;
	coord target;
};

struct RelocateData {
	int shipId;
	coord target;
};

struct ScanData {
	coord target;
};

struct RevealData {
	enum class FiringPattern {
		square,
		diamond
	};
	FiringPattern firingPattern;
	coord target;
};

using VehicleAbilityActionData = std::variant<
	TorpedoData,
	ExocetData,
	ApacheData,
	TomahawkData,
	RelocateData,
	ScanData,
	RevealData
>;

struct VehicleAbilityAction {
	VehicleAbilityType type;
	VehicleAbilityActionData data;
};

enum class ActivateAbilityResultError {
	none,
	outOfBounds,
	notYourTurn,
	notYourShip,
	shipSunk,
	noSuchAbility
};

struct TorpedoResultData {
	bool isHit;
};

struct BulkFireResultData {
	bool isHit;
};

struct RelocateResultData {}; //no data

struct ScanResultData {
	bool isFound = false;
};

struct RevealResultData {
	std::set<coord> hitsRevealed;
};

using ActivateAbilityResultData = std::variant<
	TorpedoResultData,
	BulkFireResultData,
	RelocateResultData,
	ScanResultData,
	RevealResultData
>;

struct ActivateAbilityResult {
	bool success = false;
	ActivateAbilityResultError error = ActivateAbilityResultError::none;
	ActivateAbilityResultData data;
};

using VehicleId = int;

struct ShipBlueprint {
	std::set<coord> coords;
	std::string name;
	std::vector<VehicleAbility> abilities;
	bool canHoldPlanes = false;
};

struct PlaneBlueprint {
	std::string name;
	std::vector<VehicleAbility> abilities;
};

struct FleetBlueprint {
	std::vector<ShipBlueprint> ships;
	std::vector<PlaneBlueprint> planes;
};

} // namespace NavalBattle
