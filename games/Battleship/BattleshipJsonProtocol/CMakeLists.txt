add_library(NavalBattleJsonProtocol
	NavalBattleJsonProtocol.cpp
	NavalBattleJsonProtocol.h
)

target_include_directories(NavalBattleJsonProtocol PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(NavalBattleJsonProtocol PUBLIC
	NavalBattleSession
	RemoteTerminalMessaging
    jsoncpp_static
)

set_property(TARGET NavalBattleEngine PROPERTY CXX_STANDARD 20)
