add_executable(NavalBattleServer
    "main.cpp"
)

target_link_libraries(NavalBattleServer PRIVATE
    NavalBattleEndpoint
    NavalBattleJsonProtocol
)

set_property(TARGET NavalBattleServer PROPERTY CXX_STANDARD 20)