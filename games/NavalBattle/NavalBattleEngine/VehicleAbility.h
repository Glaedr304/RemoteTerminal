#pragma once

#include "GameEntities.h"

namespace NavalBattle {
	class VehicleAbility {
		VehicleAbilityType _type = VehicleAbilityType::Apache;
		VehicleAbilityUsagePolicy _usagePolicy = VehicleAbilityUsagePolicy::limited;
		int _remainingUses = 0;

	public:
		VehicleAbility(VehicleAbilityType type, VehicleAbilityUsagePolicy policy, int allowedUses = 0) :
			_type(type),
			_usagePolicy(policy),
			_remainingUses(allowedUses)
		{
		};

		bool canUse() const {
			if (_usagePolicy == VehicleAbilityUsagePolicy::unlimited)
				return true;
			return _remainingUses > 0;
		}

		bool use() {
			bool used = canUse();
			if (used)
				_remainingUses--;
			return used;
		}

		VehicleAbilityType getType() const {
			return _type;
		}
	};
}
