#include "NavalBattleSession.h"
#include "NavalBattleSession.h"
#include "Action.h"

#include <string>

using namespace NavalBattle;

NavalBattleSession::NavalBattleSession(const GameId& id, const UserId& playerOneId, const UserId& playerTwoId) {
	_userToPlayerMap[playerOneId] = Player::one;
	_userToPlayerMap[playerTwoId] = Player::two;
	_playerToUserMap[Player::one] = playerOneId;
	_playerToUserMap[Player::two] = playerTwoId;
	_playerToUserMap[Player::none] = "";
	_gameId = id;
}

GameId NavalBattleSession::getGameId() const {
	return _gameId;
}

bool NavalBattleSession::isFinished() const {
	return _engine.phase() == Phase::finished;
}

AddressedMessageBundle NavalBattleSession::handleAction(const UserId& user, const SessionAction& action) {
	AddressedMessageBundle a;
	SessionActionResult s;
	Player p = playerFor(user);
	if (p == Player::none) {
		s.success = false;
		s.error = SessionActionResultError::userNotFound;
		return a.addMessage(ToUser(user), s);
	}

	switch (action.type) {
		case SessionActionType::PlaceShip: {
			s = handlePlaceShip(p, action);
			break;
		}
		case SessionActionType::Ready: {
			s = handleReady(p);
			break;
		}
		case SessionActionType::Fire: {
			s = handleFire(p, action);
			break;
		}
		case SessionActionType::CheckPlacement: {
			// Only return the action result with preview data, no snapshot
			s = handleCheckPlacement(p, action);
			return a.addMessage(ToUser(user), s);
		}
		case SessionActionType::Rematch: {
			s = handleRematch(p);
			return processRematchRequest(user, p, s);
		}
		default: {
			s.success = false;
			s.error = SessionActionResultError::unknownAction;
			break;
		}
	}

	a.addMessage(ToUser(user), s);
	a.addMessageBundle(getSnapshotMessageBundleForUser(user));
	if (s.success && s.type != SessionActionResultType::PlaceShipResult) {//do not alert opponent on ship placement or failed actions
		a.addMessage(ToUser(opponentForUser(user)), s);
		a.addMessageBundle(getSnapshotMessageBundleForUser(opponentForUser(user)));
	}

	return a;
}

AddressedMessageBundle NavalBattleSession::getSnapshotMessageBundles() {
	AddressedMessageBundle answer;

	UserSnapshot p1Snapshot;
	p1Snapshot.currentUser = _playerToUserMap[_engine.currentTurn()];
	p1Snapshot.phase = _engine.phase();
	p1Snapshot.userView = UserView(_playerToUserMap[Player::one], _engine.boardViewForPlayer(Player::one));
	p1Snapshot.youReady = _engine.isPlayerReady(Player::one);
	p1Snapshot.opponentReady = _engine.isPlayerReady(opponent(Player::one));

	UserSnapshot p2Snapshot;
	p2Snapshot.currentUser = p1Snapshot.currentUser;
	p2Snapshot.phase = p1Snapshot.phase;
	p2Snapshot.userView = UserView(_playerToUserMap[Player::two], _engine.boardViewForPlayer(Player::two));
	p2Snapshot.youReady = _engine.isPlayerReady(Player::two);
	p2Snapshot.opponentReady = _engine.isPlayerReady(opponent(Player::two));
	
	answer.addMessage(ToUser(_playerToUserMap[Player::one]), p1Snapshot);
	answer.addMessage(ToUser(_playerToUserMap[Player::two]), p2Snapshot);

	return answer;
}

AddressedMessageBundle NavalBattleSession::getStartupInfoMessageBundles() {
	AddressedMessageBundle answer;

	StartupInfo p1startupInfo(
		_engine.phase(),
		_playerToUserMap[Player::one],
		_playerToUserMap[opponent(Player::one)],
		_gameId,
		_engine.getFleetForPlayer(Player::one),
		UserView(_playerToUserMap[Player::one], _engine.boardViewForPlayer(Player::one)),
		_engine.boardRows(),
		_engine.boardCols()
	);

	StartupInfo p2startupInfo(
		_engine.phase(),
		_playerToUserMap[Player::two],
		_playerToUserMap[opponent(Player::two)],
		_gameId,
		_engine.getFleetForPlayer(Player::two),
		UserView(_playerToUserMap[Player::two], _engine.boardViewForPlayer(Player::two)),
		_engine.boardRows(),
		_engine.boardCols()
	);

	answer.addMessage(ToUser(_playerToUserMap[Player::one]), p1startupInfo);
	answer.addMessage(ToUser(_playerToUserMap[Player::two]), p2startupInfo);
	
	return answer;
}

AddressedMessageBundle NavalBattleSession::getSnapshotMessageBundleForUser(const UserId& u) {
	AddressedMessageBundle answer;
	UserSnapshot snapshot;
	snapshot.currentUser = _playerToUserMap[_engine.currentTurn()];
	snapshot.phase = _engine.phase();
	snapshot.userView = UserView(u, _engine.boardViewForPlayer(playerFor(u)));
	snapshot.youReady = _engine.isPlayerReady(playerFor(u));
	snapshot.opponentReady = _engine.isPlayerReady(opponent(playerFor(u)));
	answer.addMessage(ToUser(u), snapshot);
	return answer;
}

UserId NavalBattleSession::opponentForUser(const UserId& u) {
	return _playerToUserMap[opponent(_userToPlayerMap[u])];
}

Player NavalBattleSession::playerFor(const UserId& user) const {
	auto p = _userToPlayerMap.find(user);
	if (p == _userToPlayerMap.end())
		return Player::none;
	return p->second;
}

SessionActionResult NavalBattleSession::handlePlaceShip(Player p, const SessionAction& a) {
	SessionActionResult answer;
	
	PlaceShipData psd = std::get<PlaceShipData>(a.data);
	auto r = _engine.placeShip(p, psd.shipId, psd.position, psd.rotation);

	answer.success = r.success;
	answer.type = SessionActionResultType::PlaceShipResult;

	if (answer.success) {
		answer.data = PlaceShipResultData();
	}
	else {
		switch (r.error) {
			case PlaceShipError::OverlapsAnotherShip:
			case PlaceShipError::OutOfBounds : {
				answer.error = SessionActionResultError::invalidPlacement;
				break;
			}
			case PlaceShipError::WrongPhase: {
				answer.error = SessionActionResultError::wrongPhase;
				break;
			}
			case PlaceShipError::invalidID: {
				answer.error = SessionActionResultError::shipNotFound;
				break;
			}
		}
	}
	
	return answer;;
}

SessionActionResult NavalBattleSession::handleFire(Player p, const SessionAction& a) {
	SessionActionResult answer;
	FireResult r = _engine.fire(p, std::get<FireData>(a.data).target);

	answer.success = r.success;
	answer.type = SessionActionResultType::FireResult;

	if (answer.success) {
		FireResultData d;
		d.isHit = r.isHit;
		d.isSunk = r.isSink;
		if (r.isSink) {
			d.sunkName = _engine.nameForId(r.hitId);
			d.hitId = r.hitId;
		}
		answer.data = d;
	}
	else {
		switch (r.error) {
			case FireError::outOfBounds: {
				answer.error = SessionActionResultError::invalidPlacement;
				break;
			}
			case FireError::notYourTurn: {
				answer.error = SessionActionResultError::notYourTurn;
				break;
			}
		}
	}

	return answer;
}

SessionActionResult NavalBattleSession::handleReady(Player p) {
	SessionActionResult answer;
	ReadyUpResult r = _engine.readyUp(p);
	answer.success = r.success;
	answer.type = SessionActionResultType::ReadyResult;
	
	if (answer.success) {
		answer.data = ReadyResultData();
	}
	else {
		switch (r.error) {
			//may want to refine these errors for clarity
			case ReadyUpError::fleetNotPlaced: {
				answer.error = SessionActionResultError::invalidPlacement;
				break;
			}
			case ReadyUpError::fleetPlacementInvalid: {
				answer.error = SessionActionResultError::notYourTurn;
				break;
			}
		}
	}

	return answer;
}

SessionActionResult NavalBattleSession::handleCheckPlacement(Player p, const SessionAction& a) {
	SessionActionResult answer;
	answer.success = true;
	answer.type = SessionActionResultType::CheckPlacementResult;
	
	PlaceShipData psd = std::get<PlaceShipData>(a.data);
	auto r = _engine.validatePlacement(p, psd.shipId, psd.position, psd.rotation);
	
	CheckPlacementResultData data;
	data.valid = r.valid;
	data.coords = r.coords;
	answer.data = data;
	
	return answer;
}

SessionActionResult NavalBattleSession::handleRematch(Player p) {
	SessionActionResult answer;
	answer.type = SessionActionResultType::RematchResult;
	answer.data = RematchResultData{};
	
	// Guard: Only allow rematch when game is finished
	if (_engine.phase() != Phase::finished) {
		answer.success = false;
		answer.error = SessionActionResultError::wrongPhase;
		return answer;
	}
	
	answer.success = true;
	
	// Mark this player as wanting a rematch
	if (p == Player::one) {
		_playerOneWantsRematch = true;
	} else if (p == Player::two) {
		_playerTwoWantsRematch = true;
	}
	
	return answer;
}

AddressedMessageBundle NavalBattleSession::processRematchRequest(const UserId& user, Player p, const SessionActionResult& result) {
	AddressedMessageBundle a;
	
	// If rematch request failed, just send error to user
	if (!result.success) {
		a.addMessage(ToUser(user), result);
		return a;
	}
	
	// Check if both players want a rematch
	if (_playerOneWantsRematch && _playerTwoWantsRematch) {
		// Reset the engine for a new game
		_engine = NavalBattleEngine();
		_playerOneWantsRematch = false;
		_playerTwoWantsRematch = false;
		
		// Send rematch confirmation to the user who completed the rematch
		a.addMessage(ToUser(user), result);
		
		// Send rematch start to both players
		RematchStart rematchStart{};
		a.addMessage(ToUser(_playerToUserMap[Player::one]), rematchStart);
		a.addMessage(ToUser(_playerToUserMap[Player::two]), rematchStart);
		// Send new setup info to both players
		a.addMessageBundle(getStartupInfoMessageBundles());
		return a;
	}
	
	// Send rematch request to opponent
	UserId opponent = opponentForUser(user);
	a.addMessage(ToUser(opponent), RematchRequest{user});
	a.addMessage(ToUser(user), result);
	return a;
}
