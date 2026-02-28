#include "NavalBattleEngine.h"
#include "GameEntities.h"
#include "unordered_set"

using namespace NavalBattle;

NavalBattleEngine::NavalBattleEngine() :
    _currentPlayer(Player::none),
    _phase(Phase::setup),
    _p1Data(createFleetFromBlueprint(getBaseFleetBlueprint())),
    _p2Data(createFleetFromBlueprint(getBaseFleetBlueprint())),
    _pNoneData(Fleet()),
    _boardDimensions(10,10)
{
}

const Fleet& NavalBattleEngine::getFleetForPlayer(Player p) const{
    return getDataForPlayer(p).fleet;
}

// only checks the overall status of the fleet
// not the specific result of changing ship s
PlaceShipResult NavalBattleEngine::placeShip( Player p, int ID, coord pos, int rotation ) {
    PlaceShipResult r;
    r.success = true;

    //only place in setup phase
    if (phase() != Phase::setup) {
        r.error = PlaceShipError::WrongPhase;
        r.success = false;
        return r;
    }

    // Validate the placement
    auto validation = validatePlacement(p, ID, pos, rotation);
    
    if (!validation.valid) {
        r.success = false;
        r.error = validation.error;
        return r;
    }

    // Validation passed, now actually place the ship
    Fleet& fleet = getMutableFleetForPlayer(p);
    fleet.placeShip(ID, pos, rotation);

    return r;
}

ValidatePlacementResult NavalBattleEngine::validatePlacement(Player p, int ID, coord pos, int rotation) const {
    ValidatePlacementResult r;
    r.valid = true;

    const Fleet& fleet = getFleetForPlayer(p);
    
    // Find the ship
    const Ship* targetShip = nullptr;
    for (const Ship& s : fleet.getShips())
        if (s.getId() == ID)
            targetShip = &s;
    
    if (targetShip == nullptr) {
        r.valid = false;
        r.error = PlaceShipError::invalidID;
        return r;
    }
    
    // Collect coords of other placed ships
    std::unordered_set<coord> occupied;
    for (const Ship& s : fleet.getShips()) {
        if (s.getId() == ID)
            continue;
        if (s.isPlaced()) {
            for (const coord& c : s.getCoords()) {
                occupied.insert(c.applyTransform(s.getPos(), s.getRotation()));
            }
        }
    }
    
    // Calculate placement coords and check validity
    for (const coord& c : targetShip->getCoords()) {
        coord transformed = c.applyTransform(pos, rotation);
        
        // Check out of bounds - don't include invalid coords in result
		if (!isValidCoord(transformed)) {
             r.valid = false;
             r.error = PlaceShipError::OutOfBounds;
             continue;//cannot be out of bounds and overlapping
        }
        
        r.coords.insert(transformed);
        
        // Check overlap
        if (r.valid && occupied.find(transformed) != occupied.end()) {
            r.valid = false;
            r.error = PlaceShipError::OverlapsAnotherShip;
        }
    }
    
    return r;
}

//build hitmaps and move to gamplay of other player is ready
ReadyUpResult NavalBattleEngine::readyUp(Player p) {
    ReadyUpResult answer;
    auto status = checkFleetStatus(getFleetForPlayer(p));
    if (status.any()) {
        answer.success = false;

        if (status.test((int)FleetStatusBits::unplaced))
            answer.error = ReadyUpError::fleetNotPlaced;
        else if (status.test((int)FleetStatusBits::overlapping)) 
            answer.error = ReadyUpError::fleetPlacementInvalid;
        else if (status.test((int)FleetStatusBits::outOfBounds)) 
            answer.error = ReadyUpError::fleetPlacementInvalid;

        return answer;
    }
    getDataForPlayer(p).isReady = true;
    if (getDataForPlayer(Player::one).isReady && getDataForPlayer(Player::two).isReady) {
        _phase = Phase::playing;
        _currentPlayer = Player::one;
    }

    answer.success = true;
    return answer;
}

bool NavalBattleEngine::isPlayerReady(Player p) {
    return getDataForPlayer(p).isReady;
}

// --- Gameplay ---
FireResult NavalBattleEngine::fire(Player p, coord target) {
    FireResult answer;
    
    if (_currentPlayer != p) {
        answer.success = false;
        answer.error = FireError::notYourTurn;
        return answer;
    }

	if (!isValidCoord(target)) {
        answer.success = false;
        answer.error = FireError::outOfBounds;
        return answer;
    }

    auto& f = getMutableFleetForPlayer(opponent(p));
    auto r = f.hitFleet(target);

    if (r.success) {
        answer.success = true;
        answer.isHit = true;
        getHitsForPlayer(p).insert(target);
        answer.isSink = r.sunk;
        answer.hitId = r.hitID;
    }
    else {
        if(r.error != Fleet::hitFleetError::coordAlreadyHit) //not a true miss if this coord was hit before
            getMissesForPlayer(p).insert(target);
        answer.success = true;
        answer.isHit = false;
    }

    if (f.isDefeated()) {
        _phase = Phase::finished;
        _currentPlayer = Player::none;
    }
    else
        if (answer.success)
            _currentPlayer = opponent(p);

    return answer;
}

// --- Queries ---
Phase NavalBattleEngine::phase() const {
    return _phase;
}

Player NavalBattleEngine::getWinner() const{
    if (_phase != Phase::finished)
        return Player::none;
    if (getFleetForPlayer(Player::one).isDefeated())
        return Player::two;
    else
        return Player::one;
}

Player NavalBattleEngine::currentTurn() const {
    return _currentPlayer;
}

std::string NavalBattleEngine::nameForId(int id) const {
    for (const Ship& s : getFleetForPlayer(Player::one).getShips())
        if (s.getId() == id)
            return s.getName();
    for (const Ship& s : getFleetForPlayer(Player::two).getShips())
        if (s.getId() == id)
            return s.getName();

    return "";
}

int NavalBattleEngine::boardRows() const{
    return _boardDimensions.first;
}

int NavalBattleEngine::boardCols() const{
    return _boardDimensions.second;
}

const std::set<coord>& NavalBattleEngine::getHitsForPlayer(Player p) const{
	return getDataForPlayer(p).hits;
}

const std::set<coord>& NavalBattleEngine::getMissesForPlayer(Player p) const{
	return getDataForPlayer(p).misses;
}

BoardView NavalBattleEngine::boardViewForPlayer(Player p) const{
    BoardView b;
    b.ownGrid = ownGrid(p);
    b.opponentGrid = opponentGrid(p);
    return b;
}

GridView NavalBattleEngine::ownGrid(Player p) const {
    std::map<coord, SquareState> occupied;
    //layer from bottom to top so only top is visible
    const Fleet& f = getFleetForPlayer(p);
    for (const Ship& s : f.getShips())
        if(s.isPlaced())
            for (const coord& c : s.getCoords())
                occupied[c.applyTransform(s.getPos(), s.getRotation())] = SquareState::ship;
    for (const auto& c : getMissesForPlayer(opponent(p)))
        occupied[c] = SquareState::miss;
    for (const auto& c : getHitsForPlayer(opponent(p)))
        occupied[c] = SquareState::hit;
    return GridView(occupied);
}

GridView NavalBattleEngine::opponentGrid(Player p) const {
    std::map<coord, SquareState> occupied;
    //layer from bottom to top so only top is visible
    for (const auto& c : getMissesForPlayer(p))
        occupied[c] = SquareState::miss;
    for (const auto& c : getHitsForPlayer(p))
        occupied[c] = SquareState::hit;
    return GridView(occupied);
}

Fleet NavalBattleEngine::createFleetFromBlueprint(FleetBlueprint blueprint) {
    Fleet answer;

    for(const ShipBlueprint& sb : blueprint.ships)
		answer.addShip(Ship(sb, getNextVehicleId()));

	for (const PlaneBlueprint& pb : blueprint.planes)
		answer.addPlane(Plane(pb, getNextVehicleId()));

	return answer;
}

VehicleId NavalBattleEngine::getNextVehicleId() {
    return _nextVehicleId++;
}

bool NavalBattleEngine::isValidCoord(coord c) const {
	//should coord::unspecified be considered invalid? For now, treat it as valid
    return c.d >= 0 && c.d < boardRows() && c.o >= 0 && c.o < boardCols();
}

bool NavalBattleEngine::isValidCoord(const std::set<coord>& coords) const{
    for (coord c : coords)
        if (!isValidCoord(c))
            return false;
    return true;
}

Fleet& NavalBattleEngine::getMutableFleetForPlayer(Player p) {
	return getDataForPlayer(p).fleet;
}

std::set<coord>& NavalBattleEngine::getHitsForPlayer(Player p) {
	return getDataForPlayer(p).hits;
}

std::set<coord>& NavalBattleEngine::getMissesForPlayer(Player p) {
	return getDataForPlayer(p).misses;
}

//default fleet for normal game
FleetBlueprint const& NavalBattleEngine::getBaseFleetBlueprint() {
    static FleetBlueprint baseFleet{
        {
            Ship::carrier,
            Ship::battleship,
            Ship::destroyer,
            Ship::sub,
            Ship::pt,
        }
    };
    return baseFleet;
}

std::bitset<8> NavalBattleEngine::checkFleetStatus(Fleet f) {
    std::bitset<8> answer;
    std::unordered_set<coord> occupied;
    for (Ship s : f.getShips()) {
        if (s.isPlaced()) { //only check ships that are placed
            for (coord c : s.getCoords()) {
                coord transformed = c.applyTransform(s.getPos(), s.getRotation());
				if (!isValidCoord(transformed)){
                     answer.set((int)FleetStatusBits::outOfBounds, true);
					 continue;//cannot be out of bounds and overlapping
				}
                size_t occupiedSize = occupied.size();
                occupied.insert(transformed);
                if (occupiedSize == occupied.size())
                    answer.set((int)FleetStatusBits::overlapping, true);
            }
        }
        else {
            answer.set((int)FleetStatusBits::unplaced, true);
        }
    }
    return answer;
}

NavalBattleEngine::PlayerData& NavalBattleEngine::getDataForPlayer(Player p) {
    if (p == Player::one)
        return _p1Data;
    else if (p == Player::two)
        return _p2Data;

    return _pNoneData;
}

const NavalBattleEngine::PlayerData& NavalBattleEngine::getDataForPlayer(Player p) const {
    return const_cast<NavalBattleEngine*>(this)->getDataForPlayer(p);
}
