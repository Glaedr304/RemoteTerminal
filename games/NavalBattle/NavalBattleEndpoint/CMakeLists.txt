add_library(NavalBattleEndpoint
	NavalBattleEndpoint.cpp
)

target_include_directories(NavalBattleEndpoint PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(NavalBattleEndpoint
	PUBLIC
		RemoteTerminalMessaging
		NavalBattleSessionManager
	PRIVATE
		NavalBattleJsonProtocol
)
