#include "NavalBattleSessionManager.h"
#include "EndpointTypes.h"

using namespace NavalBattle;

AddUserToGameResult NavalBattleSessionManager::buildSuccessfulJoinResponse(bool readyToStart) const {
	AddUserToGameResult response;
	response.success = true;
	response.readyToStart = readyToStart;
	return response;
}

AddUserToGameResult NavalBattleSessionManager::buildFailedJoinResponse(AddUserToGameError error) const {
	AddUserToGameResult response;
	response.success = false;
	response.readyToStart = false;
	response.error = error;
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
	NavalBattleSession* session
) const {
	(void)session;
	return buildImmediateJoinResult(userId, SenderAction::RejectMessage, buildFailedJoinResponse(AddUserToGameError::gameFull));
}

MessageResult NavalBattleSessionManager::handleNewLobbyJoin(
	const UserId& userId,
	const GameId& gameId
) {
	_lobbyGames.insert({ gameId, userId });
	return buildImmediateJoinResult(userId, SenderAction::Bind, buildSuccessfulJoinResponse(false));
}

MessageResult NavalBattleSessionManager::handleExistingLobbyOwnerJoin(
	const UserId& userId
) const {
	return buildImmediateJoinResult(userId, SenderAction::TerminateSession, buildFailedJoinResponse(AddUserToGameError::userAlreadyInGame));
}

MessageResult NavalBattleSessionManager::handleSecondPlayerJoin(
	const UserId& userId,
	const GameId& gameId,
	const UserId& firstUser
) {
	MessageResult result;
	result.userToBind = userId;
	result.senderAction = SenderAction::Bind;

	NavalBattleSession* session = new NavalBattleSession(gameId, firstUser, userId, _gameMode);
	_gameIdToSessionMap[gameId] = session;
	_lobbyGames.erase(gameId);

	result.addressedMessages.addMessage(ToUser(userId), buildSuccessfulJoinResponse(true));

	AddressedMessageBundle startupMessages = session->getStartupInfoMessageBundles();
	for (const AddressedMessage& m : startupMessages)
		result.addressedMessages.addMessage(m.address, m.message);

	return result;
}

MessageResult NavalBattleSessionManager::handleJoinRequest(const JoinRequest& request) {
	UserId u = request.userId;
	GameId g = request.gameId;

	// game is already in progress
	auto inProgressGame = _gameIdToSessionMap.find(g);
	if (inProgressGame != _gameIdToSessionMap.end())
		return handleInProgressGameJoin(u, inProgressGame->second);

	auto lobbyGame = _lobbyGames.find(g);
	// game does not exist yet
	if (lobbyGame == _lobbyGames.end())
		return handleNewLobbyJoin(u, g);

	// game exists only in lobby and this user is trying to join twice
	if (lobbyGame->second == u)
		return handleExistingLobbyOwnerJoin(u);

	// this is the second user
	const UserId firstUser = lobbyGame->second;
	return handleSecondPlayerJoin(u, g, firstUser);
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
