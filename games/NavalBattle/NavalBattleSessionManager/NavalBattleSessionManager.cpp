#include "NavalBattleSessionManager.h"
#include "EndpointTypes.h"
#include <sstream>

using namespace NavalBattle;

std::string NavalBattleSessionManager::generateConnectionToken() {
	std::ostringstream token;
	token << std::hex << _rng() << _rng();
	return token.str();
}

AddUserToGameResult NavalBattleSessionManager::buildSuccessfulJoinResponse(bool readyToStart, const std::string& connectionToken) const {
	AddUserToGameResult response;
	response.success = true;
	response.readyToStart = readyToStart;
	response.connectionToken = connectionToken;
	return response;
}

AddUserToGameResult NavalBattleSessionManager::buildFailedJoinResponse(AddUserToGameError error, const std::string& connectionToken) const {
	AddUserToGameResult response;
	response.success = false;
	response.readyToStart = false;
	response.error = error;
	response.connectionToken = connectionToken;
	return response;
}

MessageResult NavalBattleSessionManager::buildImmediateJoinResult(const UserId& userId, SenderAction senderAction, const AddUserToGameResult& response) const {
	MessageResult result;
	result.userToBind = userId;
	result.senderAction = senderAction;
	result.addressedMessages.addMessage(ToUser(userId), response);
	return result;
}

void NavalBattleSessionManager::addReconnectRestoreMessages(MessageResult& result, NavalBattleSession* session, const UserId& userId) const {
	result.addressedMessages.addMessageBundle(session->getStartupInfoMessageBundleForUser(userId));
	result.addressedMessages.addMessageBundle(session->getSnapshotMessageBundleForUser(userId));
}

MessageResult NavalBattleSessionManager::handleInProgressGameJoin(
	const UserId& userId,
	NavalBattleSession* session,
	bool validReconnectToken,
	const std::string& connectionToken
) const {
	if (validReconnectToken && session->hasUser(userId)) {
		MessageResult result;
		result.userToBind = userId;
		result.senderAction = SenderAction::Bind;
		result.addressedMessages.addMessage(ToUser(userId), buildSuccessfulJoinResponse(true, connectionToken));
		addReconnectRestoreMessages(result, session, userId);
		return result;
	}

	return buildImmediateJoinResult(userId, SenderAction::RejectMessage, buildFailedJoinResponse(AddUserToGameError::gameFull, connectionToken));
}

MessageResult NavalBattleSessionManager::handleNewLobbyJoin(
	const UserId& userId,
	const GameId& gameId,
	const std::string& connectionToken
) {
	_lobbyGames.insert({ gameId, userId });
	return buildImmediateJoinResult(userId, SenderAction::Bind, buildSuccessfulJoinResponse(false, connectionToken));
}

MessageResult NavalBattleSessionManager::handleExistingLobbyOwnerJoin(
	const UserId& userId,
	bool validReconnectToken,
	const std::string& connectionToken
) const {
	if (validReconnectToken)
		return buildImmediateJoinResult(userId, SenderAction::Bind, buildSuccessfulJoinResponse(false, connectionToken));

	return buildImmediateJoinResult(userId, SenderAction::TerminateSession, buildFailedJoinResponse(AddUserToGameError::userAlreadyInGame, connectionToken));
}

MessageResult NavalBattleSessionManager::handleSecondPlayerJoin(
	const UserId& userId,
	const GameId& gameId,
	const UserId& firstUser,
	const std::string& connectionToken
) {
	MessageResult result;
	result.userToBind = userId;
	result.senderAction = SenderAction::Bind;

	NavalBattleSession* session = new NavalBattleSession(gameId, firstUser, userId, _gameMode);
	_gameIdToSessionMap[gameId] = session;
	_lobbyGames.erase(gameId);

	result.addressedMessages.addMessage(ToUser(userId), buildSuccessfulJoinResponse(true, connectionToken));

	AddressedMessageBundle startupMessages = session->getStartupInfoMessageBundles();
	for (const AddressedMessage& m : startupMessages)
		result.addressedMessages.addMessage(m.address, m.message);

	result.addressedMessages.addMessage(ToUser(firstUser), buildSuccessfulJoinResponse(true, rotateConnectionToken(firstUser)));

	return result;
}

bool NavalBattleSessionManager::isReconnectTokenValid(const UserId& userId, const std::string& connectionToken) const {
	if (connectionToken.empty())
		return false;

	auto it = _userReconnectTokens.find(userId);
	if (it == _userReconnectTokens.end())
		return false;

	return it->second == connectionToken;
}

std::string NavalBattleSessionManager::rotateConnectionToken(const UserId& userId) {
	std::string newToken = generateConnectionToken();
	_userReconnectTokens[userId] = newToken;
	return newToken;
}

MessageResult NavalBattleSessionManager::handleJoinRequest(const JoinRequest& request) {
	UserId u = request.userId;
	GameId g = request.gameId;
	const bool validReconnectToken = isReconnectTokenValid(u, request.connectionToken);
	const std::string connectionToken = rotateConnectionToken(u);

	// game is already in progress
	auto inProgressGame = _gameIdToSessionMap.find(g);
	if (inProgressGame != _gameIdToSessionMap.end())
		return handleInProgressGameJoin(u, inProgressGame->second, validReconnectToken, connectionToken);

	auto lobbyGame = _lobbyGames.find(g);
	// game does not exist yet
	if (lobbyGame == _lobbyGames.end())
		return handleNewLobbyJoin(u, g, connectionToken);

	// game exists only in lobby and this user is trying to join twice
	if (lobbyGame->second == u)
		return handleExistingLobbyOwnerJoin(u, validReconnectToken, connectionToken);

	// this is the second user
	const UserId firstUser = lobbyGame->second;
	return handleSecondPlayerJoin(u, g, firstUser, connectionToken);
}

MessageResult NavalBattleSessionManager::handleActionRequest(const ActionRequest& request){
	MessageResult answer;

	answer.userToBind = request.userId;
	answer.senderAction = SenderAction::None;

	NavalBattleSession* session = findSession(request.gameId);
	if (session)
		answer.addressedMessages = session->handleAction(request.userId, request.action);

	return answer;
}

void NavalBattleSessionManager::destroySession(GameId g) {
	delete _gameIdToSessionMap[g];
	_gameIdToSessionMap.erase(g);
}

NavalBattleSession* NavalBattleSessionManager::findSession(GameId g) {
	auto f = _gameIdToSessionMap.find(g);
	if (f == _gameIdToSessionMap.end())
		return nullptr;
	return (*f).second;
}
