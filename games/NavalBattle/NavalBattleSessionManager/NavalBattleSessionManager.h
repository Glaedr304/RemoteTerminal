#pragma once
#include "NavalBattleSession.h"
#include "EndpointTypes.h"
#include "GameEntities.h"
#include <map>

namespace NavalBattle {

struct MessageResult {
	SenderAction senderAction = SenderAction::None; //what the session manager requests to happen to the sender
	UserId userToBind; //include the name of the user who sent the message
	AddressedMessageBundle addressedMessages; //messages to be sent in the format they travel on the wire
};

class NavalBattleSessionManager {
public:
	NavalBattleSessionManager(GameMode mode = GameMode::classic) : _gameMode(mode) {}

	MessageResult handleJoinRequest(const JoinRequest& request);

	MessageResult handleActionRequest(const ActionRequest& request);

	void destroySession(GameId g);

	NavalBattleSession* findSession(GameId g);

private:
	AddUserToGameResult buildSuccessfulJoinResponse(bool readyToStart) const;

	AddUserToGameResult buildFailedJoinResponse(AddUserToGameError error) const;

	MessageResult buildImmediateJoinResult(const UserId& userId, SenderAction senderAction, const AddUserToGameResult& response) const;

	void addReconnectRestoreMessages(MessageResult& result, NavalBattleSession* session, const UserId& userId) const;

	MessageResult handleInProgressGameJoin(
		const UserId& userId,
		NavalBattleSession* session
	) const;

	MessageResult handleNewLobbyJoin(
		const UserId& userId,
		const GameId& gameId
	);

	MessageResult handleExistingLobbyOwnerJoin(
		const UserId& userId
	) const;

	MessageResult handleSecondPlayerJoin(
		const UserId& userId,
		const GameId& gameId,
		const UserId& firstUser
	);

	GameMode _gameMode;
	std::map<GameId, NavalBattleSession*> _gameIdToSessionMap;
	std::map<GameId, UserId> _lobbyGames;
};

} // namespace NavalBattle
