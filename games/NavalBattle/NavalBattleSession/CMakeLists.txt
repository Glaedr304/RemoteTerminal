add_library(NavalBattleSession
	NavalBattleSession.cpp
	NavalBattleSession.h
	Action.h
	SessionTypes.h
)

target_include_directories(NavalBattleSession PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(NavalBattleSession PUBLIC
	NavalBattleEngine
	RemoteTerminalCore
)
