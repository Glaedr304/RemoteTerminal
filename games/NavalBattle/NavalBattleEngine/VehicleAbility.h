#pragma once

namespace NavalBattle {

enum class VehicleAbilityType {
	//firing
	Torpedo,
	Exocet,
	Apache,
	Tomahawk,

	//movement
	relocate,

	//scanning
	scan,  //check if there is anything in the area, yes or no
	reveal //report the exact states of all squares
};

enum class VehicleAbilityUsagePolicy {
	unlimited,
	limited
};

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

		VehicleAbilityUsagePolicy getUsagePolicy() const {
			return _usagePolicy;
		}

		int getRemainingUses() const {
			return _remainingUses;
		}
	};
} // namespace NavalBattle
