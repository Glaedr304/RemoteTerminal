#pragma once

#include "GameEntities.h"
#include "Fleet.h"
#include "Ship.h"
#include "coord.h"
#include <bitset>
#include <unordered_map>

namespace NavalBattle {

class NavalBattleEngine {
    public:
        NavalBattleEngine(GameMode mode = GameMode::classic);

        // --- Setup ---
        const Fleet& getFleetForPlayer(Player p) const;

        PlaceShipResult placeShip(Player p, int ID, coord pos, int rotation);

        ValidateShipPlacementResult validateShipPlacement(Player p, int ID, coord pos, int rotation) const;

        PlacePlaneResult placePlane(Player p, int ID, coord pos);

        ValidatePlanePlacementResult validatePlanePlacement(Player p, int ID, coord pos) const;

        ReadyUpResult readyUp(Player p);

        bool isPlayerReady(Player p) const;

        // --- Gameplay ---
        FireResult fire(Player p, coord target);

        FireResult fireAntiAircraft(Player p, coord target);

        ActivateAbilityResult activateAbility(Player p, int shipId, const VehicleAbilityAction& activateAbilityAction);

        // --- Queries ---
        Phase phase() const;
        Player getWinner() const;
        Player currentTurn() const;
        std::string nameForId(int id) const;
        Fleet::hitFleetResult hitCoord(Player p, coord target);


        void clearScansWithSquareForPlayer(Player p, coord c);

        int boardRows() const;
        int boardCols() const;
        bool checkCoord(Player p, coord where);

        const std::set<coord>& getHitsForPlayer(Player p) const;
        const std::set<coord>& getMissesForPlayer(Player p) const;

        BoardView boardViewForPlayer(Player p) const;


private:

    std::set<coord>& getHitsForPlayer(Player p);
    std::set<coord>& getMissesForPlayer(Player p);

    Fleet& getFleetForPlayer(Player p);

    GridView ownGrid(Player p) const;
    GridView opponentGrid(Player p) const;

    // --- Ability Handlers ---
    ActivateAbilityResult handleTorpedoAction(Player p, TorpedoData d);
    ActivateAbilityResult handleExocetAction(Player p, ExocetData d);
    ActivateAbilityResult handleApacheAction(Player p, ApacheData d);
    ActivateAbilityResult handleTomahawkAction(Player p, TomahawkData d);
    ActivateAbilityResult handleRelocateAction(Player p, RelocateData d);
    ActivateAbilityResult handleScanAction(Player p, ScanData d);
    ActivateAbilityResult handleRevealAction(Player p, RevealData d);

    ActivateAbilityResult bulkFire(Player p, const std::set<coord>& targets);

	Fleet createFleetFromBlueprint(const FleetBlueprint& blueprint);

	VehicleId getNextVehicleId();

    struct PlayerData {
        PlayerData(const Fleet& fleet) : fleet(fleet) {}
        bool isReady = false;
        Fleet fleet;
        std::set<coord> hits;
        std::set<coord> misses;
        std::set<coord> revealedHits;
        std::set<coord> revealedMisses;
        std::set<std::set<coord>> scansWithHits;
    };

    PlayerData _p1Data;
    PlayerData _p2Data;
    PlayerData _pNoneData;

    bool isValidCoord(coord c) const;
    bool isValidCoord(const std::set<coord>& coords) const;

	ActivateAbilityResultError playerMayActivateAbility(Player p, int shipId, const VehicleAbilityType& vehicleAbilityType) const;
	FireError playerMayFire(Player p, coord target) const;
	bool playerMayAct(Player p) const; //check if the player is allowed to perform any turn action in the current game state


    Phase _phase;
    Player _currentPlayer;
    std::pair<int, int> _boardDimensions;

	int _nextVehicleId = 0;

    FleetBlueprint const& getBaseFleetBlueprint();
    FleetBlueprint const& getAdvancedFleetBlueprint();
    FleetBlueprint const& getBlueprintForMode(GameMode mode);
    static std::pair<int, int> getBoardDimensionsForMode(GameMode mode);

    enum class FleetStatusBits {
        outOfBounds = 0,
        overlapping = 1,
        unplaced    = 2
    };

    std::bitset<8> checkFleetStatus(const Fleet& f);

    PlayerData& getDataForPlayer(Player p);

    const PlayerData& getDataForPlayer(Player p) const;

	ActivateAbilityResultError toActivateAbilityResultError(Fleet::AbilityAvailabilityError e) const;
};

} // namespace NavalBattle
