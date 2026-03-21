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

	Ship* getShipById(VehicleId id);
	Plane* getPlaneById(VehicleId id);

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

	enum class AbilityAvailabilityError {
		none,
		invalidID,
		vehicleHasNoSuchAbility,
		vehicleDestroyed
	};

	void addShip(const Ship& ship);

	void placeShip(VehicleId id, coord pos, int rotation);

	void addPlane(const Plane& plane);

	void placePlane(VehicleId id, coord pos);

	void placeVehicle(VehicleId id, coord pos, int rotation = 0);

	bool useShipAbility(VehicleId id, VehicleAbilityType ability);

	AbilityAvailabilityError abilityAvailable(VehicleId id, VehicleAbilityType ability) const;

	const Ship* getShipById(VehicleId id) const;

	const Plane* getPlaneById(VehicleId id) const;

	const std::vector<Ship>& getShips() const;

	const std::vector<Plane>& getPlanes() const;

	Fleet(std::vector<Ship> ships);
	Fleet();

	hitFleetResult hitFleet(coord c);
	
	bool wouldBeHit(const coord& c);

	bool isDefeated() const;


};

} // namespace NavalBattle
