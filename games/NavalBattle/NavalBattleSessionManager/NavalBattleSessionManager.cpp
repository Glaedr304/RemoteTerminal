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

MessageResult NavalBattleSessionManager::buildImmediateSuccessfulJoinResult(const UserId& userId, SenderAction senderAction, bool readyToStart, const std::string& connectionToken) const {
	return buildImmediateJoinResult(userId, senderAction, buildSuccessfulJoinResponse(readyToStart, connectionToken));
}

MessageResult NavalBattleSessionManager::buildImmediateFailedJoinResult(const UserId& userId, SenderAction senderAction, AddUserToGameError error, const std::string& connectionToken) const {
	return buildImmediateJoinResult(userId, senderAction, buildFailedJoinResponse(error, connectionToken));
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

	return buildImmediateFailedJoinResult(userId, SenderAction::RejectMessage, AddUserToGameError::gameFull, connectionToken);
}

MessageResult NavalBattleSessionManager::handleNewLobbyJoin(
	const UserId& userId,
	const GameId& gameId,
	const std::string& connectionToken
) {
	GameRecord record;
	record.state = GameLifecycleState::lobby;
	record.session = nullptr;
	record.waitingPlayer = userId;
	_games[gameId] = record;
	return buildImmediateSuccessfulJoinResult(userId, SenderAction::Bind, false, connectionToken);
}

MessageResult NavalBattleSessionManager::handleExistingLobbyOwnerJoin(
	const UserId& userId,
	bool validReconnectToken,
	const std::string& connectionToken
) const {
	if (validReconnectToken)
		return buildImmediateSuccessfulJoinResult(userId, SenderAction::Bind, false, connectionToken);

	return buildImmediateFailedJoinResult(userId, SenderAction::TerminateSession, AddUserToGameError::userAlreadyInGame, connectionToken);
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
	GameRecord& record = _games[gameId];
	record.state = GameLifecycleState::inProgress;
	record.session = session;
	record.waitingPlayer.reset();

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

NavalBattleSessionManager::GameRecord* NavalBattleSessionManager::findGameRecord(const GameId& gameId) {
	return const_cast<GameRecord*>(static_cast<const NavalBattleSessionManager*>(this)->findGameRecord(gameId));
}

const NavalBattleSessionManager::GameRecord* NavalBattleSessionManager::findGameRecord(const GameId& gameId) const {
	auto it = _games.find(gameId);
	if (it == _games.end())
		return nullptr;
	return &it->second;
}

MessageResult NavalBattleSessionManager::handleJoinRequest(const JoinRequest& request) {
	UserId u = request.userId;
	GameId g = request.gameId;
	const bool validReconnectToken = isReconnectTokenValid(u, request.connectionToken);
	const std::string connectionToken = rotateConnectionToken(u);

	GameRecord* record = findGameRecord(g);
	if (!record)
		return handleNewLobbyJoin(u, g, connectionToken);

	if (record->state == GameLifecycleState::inProgress)
		return handleInProgressGameJoin(u, record->session, validReconnectToken, connectionToken);

	if (!record->waitingPlayer.has_value())
		return handleNewLobbyJoin(u, g, connectionToken);

	if (record->waitingPlayer.value() == u)
		return handleExistingLobbyOwnerJoin(u, validReconnectToken, connectionToken);

	return handleSecondPlayerJoin(u, g, record->waitingPlayer.value(), connectionToken);
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
	auto record = _games.find(g);
	if (record == _games.end())
		return;

	if (record->second.session)
		delete record->second.session;

	_games.erase(record);
}

NavalBattleSession* NavalBattleSessionManager::findSession(GameId g) {
	GameRecord* record = findGameRecord(g);
	if (!record || record->state != GameLifecycleState::inProgress)
		return nullptr;
	return record->session;
}
