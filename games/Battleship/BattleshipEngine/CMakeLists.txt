# CMakeList.txt : CMake project for NavalBattleEngine, include source and define
# project specific logic here.
#

add_library(NavalBattleEngine STATIC
    "NavalBattleEngine.cpp"
 "GameEntities.h"  "Ship.h" "Ship.cpp" "Fleet.h" "Fleet.cpp" "coord.h" "coord.cpp")

target_include_directories(NavalBattleEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_property(TARGET NavalBattleEngine PROPERTY CXX_STANDARD 20)

# TODO: Add tests and install targets if needed.
