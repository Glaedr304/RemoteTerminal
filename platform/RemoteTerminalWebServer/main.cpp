#include "RemoteTerminalWebSocketController.h"
#include "RemoteTerminalEndpointRegistry.h"
#include "GamesHttpController.h"
#include "NavalBattleEndpoint.h"
#include "TicTacToeEndpoint.h"
#include <drogon/drogon.h>

int main(int argc, char* argv[]) {
    RemoteTerminalEndpointRegistry registry;

    NavalBattleEndpoint* navalBattleEndpoint = new NavalBattleEndpoint();
    registry.registerEndpoint("/ws/" + navalBattleEndpoint->routePath(), navalBattleEndpoint);

    TicTacToeEndpoint* ticTacToeEndpoint = new TicTacToeEndpoint();
    registry.registerEndpoint("/ws/" + ticTacToeEndpoint->routePath(), ticTacToeEndpoint);

	getRemoteTerminalWebSocketManager().setEndpointRegistry(&registry);

    int port = argc >= 2 ? std::stoi(argv[1]) : 8080; //allow port selection as runtime arg, default to 8080
    auto& app = drogon::app();
    app.addListener("127.0.0.1", port);
	app.setThreadNum(1); //single-threaded to avoid concurrency issues in game logic
    app.run();
}
