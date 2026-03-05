#include "NavalBattleSessionManager.h"
#include "EndpointTypes.h"

using namespace NavalBattle;

MessageResult NavalBattleSessionManager::handleJoinRequest(const JoinRequest& request) {
	MessageResult result; //rename to answer at some point

	SenderAction& action = result.senderAction;
	AddressedMessageBundle& messageBundle = result.addressedMessages;
	UserId& userToBind = result.userToBind;

	UserId u = request.userId;
	GameId g = request.gameId;
	userToBind = u;

	//game is already in progress
	auto inProgressGame = _gameIdToSessionMap.find(g);
	if (inProgressGame != _gameIdToSessionMap.end()) {
		action = SenderAction::RejectMessage;
		AddUserToGameResult r;
		r.success = false;
		r.error = AddUserToGameError::gameFull;
		r.readyToStart = false;
		messageBundle.addMessage(ToUser(u), r);
		return result;
	}

	auto lobbyGame = _lobbyGames.find(g);
	//game does not exist yet
	if (lobbyGame == _lobbyGames.end()) {
		action = SenderAction::Bind;
		AddUserToGameResult r;
		_lobbyGames.insert({ g, u });
		r.readyToStart = false;
		r.success = true;
		messageBundle.addMessage(ToUser(u), r);
		return result;
	}

	//game exists only in lobby
	//...and this user is trying to join twice
	if (lobbyGame->second == u) {
		action = SenderAction::TerminateSession;
		AddUserToGameResult r;
		r.success = false;
		r.error = AddUserToGameError::userAlreadyInGame;
		r.readyToStart = false;
		messageBundle.addMessage(ToUser(u), r);
		return result;
	}

	//...and this is the second user
	//can add rand here to assign who is player one and two
	action = SenderAction::Bind;
	NavalBattleSession* s = new NavalBattleSession(g, lobbyGame->second, u, _gameMode);
	_gameIdToSessionMap[g] = s;
	_lobbyGames.erase(lobbyGame);
	AddUserToGameResult r;
	r.success = true;
	r.readyToStart = true;
	messageBundle.addMessage(ToUser(u), r);

	//send both users startup info
	AddressedMessageBundle startupMessages = s->getStartupInfoMessageBundles();
	for (const AddressedMessage& m : startupMessages)
		messageBundle.addMessage(m.address, m.message);

	return result;
}

MessageResult NavalBattleSessionManager::handleActionRequest(const ActionRequest& request){
	MessageResult answer;

	answer.userToBind = request.userId;
	answer.senderAction = SenderAction::None;

	NavalBattleSession* session = findSession(request.gameId);
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
