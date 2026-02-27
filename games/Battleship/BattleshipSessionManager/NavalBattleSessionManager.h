#pragma once
#include "NavalBattleSession.h"
#include "EndpointTypes.h"
#include <map>

namespace NavalBattle {

struct MessageResult {
	SenderAction senderAction = SenderAction::None; //what the session manager requests to happen to the sender
	UserId userToBind; //include the name of the user who sent the message
	AddressedMessageBundle addressedMessages; //messages to be sent in the format they travel on the wire
};

class NavalBattleSessionManager {
public:
	MessageResult handleJoinRequest(const JoinRequest& request);

	MessageResult handleActionRequest(const ActionRequest& request);

	void destroySession(GameId g);

	NavalBattleSession* findSession(GameId g);

private:

	std::map<GameId, NavalBattleSession*> _gameIdToSessionMap;
	std::map<GameId, UserId> _lobbyGames;
};

} // namespace NavalBattle
