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
        NavalBattleEngine();

        // --- Setup ---
        const Fleet& getFleetForPlayer(Player p) const;

        PlaceShipResult placeShip(Player p, int ID, coord pos, int rotation);

        ValidatePlacementResult validatePlacement(Player p, int ID, coord pos, int rotation) const;

        ReadyUpResult readyUp(Player p);

        bool isPlayerReady(Player p);

        // --- Gameplay ---
        FireResult fire(Player p, coord target);

        // --- Queries ---
        Phase phase() const;
        Player getWinner() const;
        Player currentTurn() const;
        std::string nameForId(int id) const;

        int boardRows();
        int boardCols();

        const std::set<coord>& getHitsForPlayer(Player p) const;
        const std::set<coord>& getMissesForPlayer(Player p) const;

        BoardView boardViewForPlayer(Player p) const;

private:
    Fleet& getMutableFleetForPlayer(Player p);

    std::set<coord>& getHitsForPlayer(Player p);
    std::set<coord>& getMissesForPlayer(Player p);

    GridView ownGrid(Player p) const;
    GridView opponentGrid(Player p) const;

	Fleet createFleetFromBlueprint(FleetBlueprint blueprint);

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

    Phase _phase;
    Player _currentPlayer;
    std::pair<int, int> _boardDimensions;

	int _nextVehicleId = 0;

    FleetBlueprint const& getBaseFleetBlueprint();

    enum class FleetStatusBits {
        outOfBounds = 0,
        overlapping = 1,
        unplaced    = 2
    };

    std::bitset<8> checkFleetStatus(Fleet f);

    PlayerData& getDataForPlayer(Player p);

    const PlayerData& getDataForPlayer(Player p) const;
};

} // namespace NavalBattle
