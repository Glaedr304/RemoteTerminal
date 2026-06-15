#pragma once
#include "NavalBattleSession.h"
#include "EndpointTypes.h"
#include "GameEntities.h"
#include <map>
#include <random>

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
	AddUserToGameResult buildSuccessfulJoinResponse(bool readyToStart, const std::string& connectionToken) const;

	AddUserToGameResult buildFailedJoinResponse(AddUserToGameError error, const std::string& connectionToken) const;

	MessageResult buildImmediateJoinResult(const UserId& userId, SenderAction senderAction, const AddUserToGameResult& response) const;

	void addReconnectRestoreMessages(MessageResult& result, NavalBattleSession* session, const UserId& userId) const;

	MessageResult handleInProgressGameJoin(
		const UserId& userId,
		NavalBattleSession* session,
		bool validReconnectToken,
		const std::string& connectionToken
	) const;

	MessageResult handleNewLobbyJoin(
		const UserId& userId,
		const GameId& gameId,
		const std::string& connectionToken
	);

	MessageResult handleExistingLobbyOwnerJoin(
		const UserId& userId,
		bool validReconnectToken,
		const std::string& connectionToken
	) const;

	MessageResult handleSecondPlayerJoin(
		const UserId& userId,
		const GameId& gameId,
		const UserId& firstUser,
		const std::string& connectionToken
	);

	std::string generateConnectionToken();

	bool isReconnectTokenValid(const UserId& userId, const std::string& connectionToken) const;

	std::string rotateConnectionToken(const UserId& userId);

	GameMode _gameMode;
	std::map<GameId, NavalBattleSession*> _gameIdToSessionMap;
	std::map<GameId, UserId> _lobbyGames;
	std::map<UserId, std::string> _userReconnectTokens;
	std::mt19937_64 _rng{std::random_device{}()};
};

} // namespace NavalBattle
