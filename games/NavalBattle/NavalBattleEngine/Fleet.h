#pragma once

#include "Ship.h"
#include<map>

//#include "GameEntities.h"

namespace NavalBattle {

class Fleet {
private:
	std::vector<Ship> ships;
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

	const std::vector<Ship>& getShips() const;
	
	Fleet(std::vector<Ship> ships);
	Fleet();

	hitFleetResult hitFleet(coord c);

	bool isDefeated() const;


};

} // namespace NavalBattle
