add_library(NavalBattleSessionManager
	NavalBattleSessionManager.cpp
	NavalBattleSessionManager.h
)

target_include_directories(NavalBattleSessionManager PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(NavalBattleSessionManager PUBLIC
		NavalBattleSession
		RemoteTerminalMessaging
	PRIVATE
		NavalBattleJsonProtocol
)

set_property(TARGET NavalBattleSessionManager PROPERTY CXX_STANDARD 20)
