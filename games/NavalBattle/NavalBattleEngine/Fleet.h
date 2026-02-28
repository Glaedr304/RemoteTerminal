#pragma once

#include "Ship.h"
#include "Plane.h"
#include<map>

namespace NavalBattle {

class Fleet {
private:
	std::vector<Ship> ships;
	std::vector<Plane> planes;
	void buildHitmap();
	std::map<coord, Ship*>& getHitmap();
	bool hitmapUpToDate = false;
	std::map<coord, Ship*> hitmap;

public:
	enum class hitFleetError {
		coordNotInFleet,
		coordAlreadyHit
	};

	struct hitFleetResult {
		bool success;
		int hitID;
		bool sunk;
		hitFleetError error;
	};

	void addShip(const Ship& ship);

	void placeShip(VehicleId id, coord pos, int rotation);

	void addPlane(const Plane& plane);

	void placePlane(VehicleId id, coord pos);

	const std::vector<Ship>& getShips() const;
	
	Fleet(std::vector<Ship> ships);
	Fleet();

	hitFleetResult hitFleet(coord c);

	bool isDefeated() const;


};

} // namespace NavalBattle
