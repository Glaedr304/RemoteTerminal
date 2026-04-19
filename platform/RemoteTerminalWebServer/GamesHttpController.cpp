#include "GamesHttpController.h"
#include <json/json.h>

void GamesHttpController::listGames(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    Json::Value games(Json::arrayValue);

    // Naval Battle
    Json::Value navalbattle;
    navalbattle["id"] = "navalbattle";
    navalbattle["name"] = "Naval Battle";
    navalbattle["description"] = "Classic naval combat strategy game";
    navalbattle["route"] = "com.titohq.navalbattle";
    navalbattle["url"] = "/navalbattle/";
    navalbattle["icon"] = "\u2693";
    navalbattle["minPlayers"] = 2;
    navalbattle["maxPlayers"] = 2;
    games.append(navalbattle);

    // Advanced Naval Battle
    Json::Value advancednavalbattle;
    advancednavalbattle["id"] = "advancednavalbattle";
    advancednavalbattle["name"] = "Advanced Naval Battle";
    advancednavalbattle["description"] = "Naval combat with advanced weaponry";
    advancednavalbattle["route"] = "com.titohq.advancednavalbattle";
    advancednavalbattle["url"] = "/advancednavalbattle/";
    advancednavalbattle["icon"] = "\u2694";
    advancednavalbattle["minPlayers"] = 2;
    advancednavalbattle["maxPlayers"] = 2;
    games.append(advancednavalbattle);

    // Tic Tac Toe
    Json::Value tictactoe;
    tictactoe["id"] = "tictactoe";
    tictactoe["name"] = "Tic Tac Toe";
    tictactoe["description"] = "Classic X and O game";
    tictactoe["route"] = "com.titohq.tictactoe";
    tictactoe["url"] = "/tictactoe/";
    tictactoe["icon"] = "\u274C";
    tictactoe["minPlayers"] = 2;
    tictactoe["maxPlayers"] = 2;
    games.append(tictactoe);

    Json::Value response;
    response["games"] = games;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->addHeader("Access-Control-Allow-Origin", "*");
    callback(resp);
}
