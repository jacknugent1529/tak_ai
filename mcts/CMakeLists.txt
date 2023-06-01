cmake_minimum_required(VERSION 3.5)
project(takMCTS)

set(CMAKE_CXX_STANDARD 17)

include_directories(src)
include_directories(/usr/local/include)
set(CMAKE_PREFIX_PATH ../../libtorch) # NOTE: change based on libtorch location

find_package(Boost COMPONENTS system log program_options json REQUIRED)
find_package(Torch REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${TORCH_INCLUDE_DIRS})

add_library(takMCTSLib src/game.cpp src/mcts_bot.cpp src/ai_model.cpp)

add_executable(takMCTS main.cpp)
add_executable(takTUI tui.cpp)
set(CMAKE_BUILD_TYPE Release)

target_link_libraries(takMCTS takMCTSLib ${Boost_LIBRARIES} ${TORCH_LIBRARIES})
target_link_libraries(takTUI takMCTSLib ${Boost_LIBRARIES} ${TORCH_LIBRARIES})