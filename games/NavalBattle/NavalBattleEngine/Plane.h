#pragma once

#include "coord.h"
#include "VehicleAbility.h"
#include "GameEntities.h"
#include <string>
#include <set>

namespace NavalBattle
{
	class Plane
	{
	private:
		coord _pos;
		bool _isOnCarrier = true;

		std::vector<VehicleAbility> _abilities;

		bool _isDestroyed = false;

		VehicleId _id;
		std::string _name;

	public:
		enum class HitPlaneError {
			notOnShip,
			isDestroyed,
			none
		};

		struct HitPlaneResult {
			bool success = false;
			bool destroyed;
			HitPlaneError error = HitPlaneError::none;
		};

		Plane(const Plane& other);
		Plane(PlaneBlueprint blueprint, VehicleId id, coord pos = coord::unspecified);

		bool isDestroyed() const;
		int getId() const;
		std::string getName() const;
		coord getPos() const;
		void setPos(coord pos);
		HitPlaneResult hit(coord where);
		bool isPlaced() const;

		bool hasAbility(const VehicleAbilityType& abilityType) const;

		const std::vector<VehicleAbility>& getAbilities() const;

		const static PlaneBlueprint reconPlane;
	};

}
