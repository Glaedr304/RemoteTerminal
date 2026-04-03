#include "Plane.h"

using namespace NavalBattle;

NavalBattle::Plane::Plane(const Plane& other) :
	_pos(other._pos),
	_isOnCarrier(other._isOnCarrier),
	_abilities(other._abilities),
	_isDestroyed(other._isDestroyed),
	_id(other._id),
	_name(other._name)
{}

NavalBattle::Plane::Plane(PlaneBlueprint blueprint, VehicleId id, coord pos) :
	_pos(pos),
	_id(id),
	_abilities(blueprint.abilities),
	_name(blueprint.name)
{}

bool Plane::isDestroyed() const {
	return _isDestroyed;
}

int Plane::getId() const {
	return _id;
}

std::string Plane::getName() const {
	return _name;
}

coord Plane::getPos() const {
	return _pos;
}

void Plane::setPos(coord pos) {
	_pos = pos;
}

bool Plane::isOnShip() const {
	return _isOnCarrier;
}

void Plane::isOnShip(bool isOnShip) {
	_isOnCarrier = isOnShip;
}

Plane::HitPlaneResult Plane::hit(coord where) {
	HitPlaneResult answer;

	if (_isDestroyed) {
		answer.success = false;
		answer.error = HitPlaneError::isDestroyed;
		return answer;
	}

	if (where != _pos) {
		answer.success = false;
		answer.error = HitPlaneError::notOnPlane;
		return answer;
	}

	_isDestroyed = true;
	answer.success = true;
	answer.destroyed = true;

	return answer;
}

bool Plane::isPlaced() const {
	return !_pos.isUnspecified();
}

bool Plane::isOnCarrier() const {
	return _isOnCarrier;
}

bool NavalBattle::Plane::hasAbility(const VehicleAbilityType& abilityType) const {
	for (const VehicleAbility& a : _abilities)
		if (a.getType() == abilityType && a.canUse())
			return true;
	return false;
}

const std::vector<VehicleAbility>& Plane::getAbilities() const {
	return _abilities;
}

PlaneBlueprint const Plane::reconPlane{
	"Recon Plane",
	{
		VehicleAbility(VehicleAbilityType::relocate, VehicleAbilityUsagePolicy::unlimited),
		VehicleAbility(VehicleAbilityType::reveal, VehicleAbilityUsagePolicy::unlimited)
	}
};
