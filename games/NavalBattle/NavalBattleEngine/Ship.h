#pragma once

#include "GameEntities.h"
#include "VehicleAbility.h"
#include "coord.h"
#include <string>
#include <set>

namespace NavalBattle {

class Ship {
private:
	coord pos;
	std::set<coord> coords;
	std::set<coord> hits;
	int rotation;
	bool _sunk = false;
	VehicleId _id;
	std::string _name;

	std::vector<VehicleAbility> _abilities;

public:
	enum class hitShipError {
		notOnShip,
		alreadyHit
	};

	struct hitShipResult {
		bool success;
		bool sunk;
		hitShipError error;
	};

	Ship(const Ship& other);
	Ship(ShipBlueprint blueprint, VehicleId id, int rotation = 0, coord pos = coord::unspecified);
	bool isSunk() const;
	int getId() const;
	std::string getName() const;
	int getRotation() const;
	void setRotation(int rotation);
	coord getPos() const;
	void setPos(coord pos);
	hitShipResult hit(coord where);
	bool isPlaced() const;
	const std::set<coord>& getCoords() const;

	const static ShipBlueprint pt;
	const static ShipBlueprint sub;
	const static ShipBlueprint destroyer;
	const static ShipBlueprint battleship;
	const static ShipBlueprint carrier;
};

} // namespace NavalBattle
