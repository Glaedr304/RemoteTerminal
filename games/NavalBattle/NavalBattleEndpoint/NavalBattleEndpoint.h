#pragma once

#include "IRemoteTerminalEndpoint.h"
#include "NavalBattleSessionManager.h"
#include "SessionTypes.h"
#include "GameEntities.h"

class NavalBattleEndpoint : public IRemoteTerminalEndpoint {
public:
	NavalBattleEndpoint(NavalBattle::GameMode mode = NavalBattle::GameMode::classic)
		: _gameMode(mode), _sessionManager(mode) {}

	virtual WireMessageResult onUnauthenticatedMessage(std::string_view message) override;

	virtual WireMessageResult onAuthenticatedMessage(const UserId& userID, std::string_view message) override;

	virtual std::string routePath() override;

private:
	NavalBattle::GameMode _gameMode;
	NavalBattle::NavalBattleSessionManager _sessionManager;

	AddressedWireMessageBundle routeMessagesToWireFormat(const NavalBattle::AddressedMessageBundle& b);
};
