#pragma once

#include "IRemoteTerminalEndpoint.h"
#include "NavalBattleSessionManager.h"
#include "SessionTypes.h"

class NavalBattleEndpoint : public IRemoteTerminalEndpoint {
public:
	virtual WireMessageResult onUnauthenticatedMessage(std::string_view message) override;

	virtual WireMessageResult onAuthenticatedMessage(const UserId& userID, std::string_view message) override;

	virtual std::string routePath() override;

private:
	NavalBattle::NavalBattleSessionManager _sessionManager;

	AddressedWireMessageBundle routeMessagesToWireFormat(const NavalBattle::AddressedMessageBundle& b);
};
