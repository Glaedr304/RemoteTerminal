#include "NavalBattleEngine.h"
#include "GameEntities.h"
#include <unordered_set>

using namespace NavalBattle;

NavalBattleEngine::NavalBattleEngine(GameMode mode) :
    _currentPlayer(Player::none),
    _phase(Phase::setup),
    _p1Data(createFleetFromBlueprint(getBlueprintForMode(mode))),
    _p2Data(createFleetFromBlueprint(getBlueprintForMode(mode))),
    _pNoneData(Fleet()),
    _boardDimensions(getBoardDimensionsForMode(mode))
{
}

const Fleet& NavalBattleEngine::getFleetForPlayer(Player p) const{
	return const_cast<NavalBattleEngine*>(this)->getFleetForPlayer(p);
}

Fleet& NavalBattleEngine::getFleetForPlayer(Player p) {
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
    Fleet& fleet = getFleetForPlayer(p);
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
        if (s.getId() == ID){
            targetShip = &s;
			break;
		}
    
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

ActivateAbilityResult NavalBattleEngine::activateAbility(Player p, int shipId, const VehicleAbilityAction& VehicleAbilityAction) {
    ActivateAbilityResult answer;

    if (_currentPlayer != p) {
        answer.success = false;
        answer.error = ActivateAbilityResultError::notYourTurn;
        return answer;
    }

    auto& f = getDataForPlayer(p).fleet;
	const Ship* s = static_cast<const Fleet&>(f).getShipById(shipId);

    if (s == nullptr) {
        answer.success = false;
        answer.error = ActivateAbilityResultError::notYourShip;
        return answer;
    }

    if (s->isSunk()) {
        answer.success = false;
        answer.error = ActivateAbilityResultError::shipSunk;
        return answer;
    }

    if (!s->hasAbility(VehicleAbilityAction.type)) {
        answer.success = false;
        answer.error = ActivateAbilityResultError::noSuchAbility;
        return answer;
    }

    switch (VehicleAbilityAction.type) {
    case VehicleAbilityType::Torpedo: {
        answer = handleTorpedoAction(p, std::get<TorpedoData>(VehicleAbilityAction.data));
        break;
    }
    case VehicleAbilityType::Exocet: {
        answer = handleExocetAction(p, std::get<ExocetData>(VehicleAbilityAction.data));
        break;
    }
    case VehicleAbilityType::Apache: {
        answer = handleApacheAction(p, std::get<ApacheData>(VehicleAbilityAction.data));
        break;
    }
    case VehicleAbilityType::Tomahawk: {
        answer = handleTomahawkAction(p, std::get<TomahawkData>(VehicleAbilityAction.data));
        break;
    }
    case VehicleAbilityType::relocate: {
        answer = handleRelocateAction(p, std::get<RelocateData>(VehicleAbilityAction.data));
        break;
    }
    case VehicleAbilityType::scan: {
        answer = handleScanAction(p, std::get<ScanData>(VehicleAbilityAction.data));
        break;
    }
    case VehicleAbilityType::reveal: {
        answer = handleRevealAction(p, std::get<RevealData>(VehicleAbilityAction.data));
        break;
    }
    }

    if (answer.success) {
        _currentPlayer = opponent(p);
        f.useShipAbility(shipId, VehicleAbilityAction.type);
    }
    return answer;
}

FireResult NavalBattleEngine::fireAntiAircraft(Player p, coord target) {
    FireResult answer;

    if (phase() != Phase::playing) {
        answer.success = false;
        answer.error = FireError::notYourTurn;
        return answer;
    }

    //to be implemented

    return answer;
}


ActivateAbilityResult NavalBattleEngine::handleTorpedoAction(Player p, TorpedoData d) {
    ActivateAbilityResult answer;
    BulkFireResultData data;

    coord currentPos = d.startPoint;

    std::function<void()> incrementPos;

    if (d.firingPattern == TorpedoData::FiringPattern::vertical)
        //downwards
        if (currentPos.d == 0)
            incrementPos = [&currentPos]() {currentPos.d++; };
    //upwards
        else if (currentPos.d == boardRows() - 1)
            incrementPos = [&currentPos]() {currentPos.d--; };
    //invalid
        else {
            answer.success = false;
            return answer;
        }
    else
        //rightwards
        if (currentPos.o == 0)
            incrementPos = [&currentPos]() {currentPos.o++; };
    //leftwards
        else if (currentPos.o == boardCols() - 1)
            incrementPos = [&currentPos]() {currentPos.o--; };
    //invalid
        else {
            answer.success = false;
            return answer;
        };

    bool isHit = false;
    do {
        if (checkCoord(opponent(p), currentPos)) {
            hitCoord(opponent(p), currentPos);
            isHit = true;
            break;
        }
        else {
            getDataForPlayer(p).revealedMisses.insert(currentPos);
        }
        incrementPos();
    } while (isValidCoord(currentPos));

    //not sure if this actually works
    answer.success = true;
    data.isHit = isHit;
    answer.data = data;

    return answer;
}

ActivateAbilityResult NavalBattleEngine::bulkFire(Player p, const std::set<coord>& targets) {
    ActivateAbilityResult answer;
    BulkFireResultData data;

    if (!isValidCoord(targets)) {
        answer.success = false;
        answer.error = ActivateAbilityResultError::outOfBounds;
        return answer;
    }

    for (const coord& c : targets)
        if (hitCoord(opponent(p), c).success)
            data.isHit = true;

    answer.success = true;
    answer.data = data;
    return answer;
}

ActivateAbilityResult NavalBattleEngine::handleExocetAction(Player p, ExocetData data) {
    ActivateAbilityResult answer;
    BulkFireResultData resultData;

    std::set<coord> targets;
    for (int d = data.target.d - 1; d <= data.target.d + 1; d++)
        for (int o = data.target.o - 1; o <= data.target.o + 1; o++)
            targets.insert(coord({ d,o }));

    return bulkFire(p, targets);
}

ActivateAbilityResult NavalBattleEngine::handleApacheAction(Player p, ApacheData d) {
    std::set<coord> targets;

    targets.insert(d.target);
    if (d.firingPattern == ApacheData::FiringPattern::vertical) {
        targets.insert(d.target + coord({ -1, 0 }));
        targets.insert(d.target + coord({ 1, 0 }));
    }
    else {
        targets.insert(d.target + coord({ 0, -1 }));
        targets.insert(d.target + coord({ 0, 1 }));
    }

    return bulkFire(p, targets);
}

ActivateAbilityResult NavalBattleEngine::handleTomahawkAction(Player p, TomahawkData d) {
    std::set<coord> targets;

    targets.insert(d.target);
    if (d.firingPattern == TomahawkData::FiringPattern::plus)
        for (int i = -1; i < 2; i += 2) {//i is -1 and 1
            targets.insert(d.target + coord({ i, 0 }));
            targets.insert(d.target + coord({ 0, i }));
        }
    else if (d.firingPattern == TomahawkData::FiringPattern::x)
        for (int i = -1; i < 2; i += 2) //i is -1 and 1
            for (int j = -1; j < 2; j += 2) //j is -1 and 1
                targets.insert(d.target + coord({ i, j }));

    return bulkFire(p, targets);
}

ActivateAbilityResult NavalBattleEngine::handleRelocateAction(Player p, RelocateData d) {
    ActivateAbilityResult answer;
    RelocateResultData data;
    answer.data = data;

    if (!isValidCoord(d.target)) {
        answer.success = false;
        answer.error = ActivateAbilityResultError::outOfBounds;
        return answer;
    }

    //ID has already been validated so we should not have to check for nullptr
    const Fleet& fleet = getDataForPlayer(p).fleet;
    const Ship* s = fleet.getShipById(d.shipId);
    getDataForPlayer(p).fleet.placeShip(d.shipId, d.target, s->getRotation());
    answer.success = true;

    return answer;
}

ActivateAbilityResult NavalBattleEngine::handleScanAction(Player p, ScanData data) {
    ActivateAbilityResult answer;
    ScanResultData resultData;

    answer.success = true;

    //generate all squares in the scan pattern
    std::set<coord> scannedSquares;
    for (int d = data.target.d - 1; d <= data.target.d + 1; d++)
        for (int o = data.target.o - 1; o <= data.target.o + 1; o++)
            scannedSquares.insert(coord({ d,o }));

    if (!isValidCoord(scannedSquares)) {
        answer.success = false;
		answer.error = ActivateAbilityResultError::outOfBounds;
        return answer;
    }

    for (const coord& c : scannedSquares)
        if (getDataForPlayer(opponent(p)).fleet.wouldBeHit(c)) {
            getDataForPlayer(p).scansWithHits.insert(scannedSquares);

            resultData.isFound = true;
            answer.data = resultData;
            return answer;
        }

    //scan is clear
    //this is the same as guessing/ missing all scanned squares 
    getDataForPlayer(p).revealedMisses.insert(scannedSquares.begin(), scannedSquares.end());

    resultData.isFound = false;
    answer.data = resultData;
    return answer;
}

ActivateAbilityResult NavalBattleEngine::handleRevealAction(Player p, RevealData d) {
    ActivateAbilityResult answer;
    RevealResultData data;

    std::set<coord> squaresToReveal;
    if (d.firingPattern == RevealData::FiringPattern::square) {
        squaresToReveal.insert(coord({ d.target.d - 1, d.target.o - 1 }));
        squaresToReveal.insert(coord({ d.target.d - 1, d.target.o + 1 }));
        squaresToReveal.insert(coord({ d.target.d + 1, d.target.o - 1 }));
        squaresToReveal.insert(coord({ d.target.d + 1, d.target.o + 1 }));
    }
    else if (d.firingPattern == RevealData::FiringPattern::diamond) {
        squaresToReveal.insert(coord({ d.target.d - 1, d.target.o }));
        squaresToReveal.insert(coord({ d.target.d, d.target.o - 1 }));
        squaresToReveal.insert(coord({ d.target.d, d.target.o + 1 }));
        squaresToReveal.insert(coord({ d.target.d + 1, d.target.o }));
    }
    else {
        answer.success = false;
        answer.error = ActivateAbilityResultError::noSuchAbility;
        return answer;
    }

    if (!isValidCoord(squaresToReveal)) {
        answer.success = false;
        answer.error = ActivateAbilityResultError::outOfBounds;
        return answer;
    }

	answer.success = true;

    for (const coord& c : squaresToReveal)
        if (checkCoord(opponent(p), c)) { // c would be a hit
            getDataForPlayer(p).revealedHits.insert(c);
            data.hitsRevealed.insert(c);
        }
        else
            getDataForPlayer(p).revealedMisses.insert(c);

    answer.data = data;
    return answer;
}

//build hitmaps and move to gamplay if other player is ready
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

bool NavalBattleEngine::isPlayerReady(Player p) const{
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

	Fleet::hitFleetResult r = hitCoord(opponent(p), target);

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

    if (getDataForPlayer(opponent(p)).fleet.isDefeated()) {
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

Fleet::hitFleetResult NavalBattleEngine::hitCoord(Player p, coord target) {
    auto& f = getDataForPlayer(p).fleet;
    auto r = f.hitFleet(target);
    if (r.success) {
        clearScansWithSquareForPlayer(opponent(p), target);
        getDataForPlayer(opponent(p)).hits.insert(target);
    }
    return r;
}

void NavalBattleEngine::clearScansWithSquareForPlayer(Player p, coord c) {
    std::erase_if(getDataForPlayer(p).scansWithHits, [c](const std::set<coord>& s) {
        return s.contains(c);
        });
}

int NavalBattleEngine::boardRows() const{
    return _boardDimensions.first;
}

int NavalBattleEngine::boardCols() const{
    return _boardDimensions.second;
}

bool NavalBattleEngine::checkCoord(Player p, coord where) {
    return getDataForPlayer(p).fleet.wouldBeHit(where);
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

Fleet NavalBattleEngine::createFleetFromBlueprint(const FleetBlueprint& blueprint) {
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

//fleet with abilities for advanced game
FleetBlueprint const& NavalBattleEngine::getAdvancedFleetBlueprint() {
    static FleetBlueprint advancedFleet{
        {
            Ship::advancedCarrier,
            Ship::advancedBattleship,
            Ship::advancedDestroyer,
            Ship::advancedSub,
            Ship::pt,
        },
        {
            Plane::reconPlane,
            Plane::reconPlane
        }
    };
    return advancedFleet;
}

FleetBlueprint const& NavalBattleEngine::getBlueprintForMode(GameMode mode) {
    if (mode == GameMode::advanced)
        return getAdvancedFleetBlueprint();
    return getBaseFleetBlueprint();
}

std::pair<int, int> NavalBattleEngine::getBoardDimensionsForMode(GameMode mode) {
    switch (mode) {
        case GameMode::advanced: return {10, 14};
        case GameMode::classic:  return {10, 10};
    }
    return {10, 10};
}

std::bitset<8> NavalBattleEngine::checkFleetStatus(const Fleet& f) {
    std::bitset<8> answer;
    std::unordered_set<coord> occupied;
    std::unordered_set<coord> planeAllowedCoords; //squares that planes are allowed to occupy

    //check ships
    for (const Ship& s : f.getShips()) {
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

				planeAllowedCoords.insert(transformed);
            }
        }
        else {
            answer.set((int)FleetStatusBits::unplaced, true);
        }
    }

	std::set<coord> planeOccupied;
	//check planes
	for (const Plane& p : f.getPlanes()) {
		if (p.isPlaced()) {
			coord pos = p.getPos();
			if (!planeAllowedCoords.contains(pos)) {
				answer.set((int)FleetStatusBits::outOfBounds, true);
				continue;
			}
			if (planeOccupied.find(pos) != planeOccupied.end())
				answer.set((int)FleetStatusBits::overlapping, true);
			planeOccupied.insert(pos);
		}
        else
        {
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
