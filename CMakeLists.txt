cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(triumf2023_test)

find_package(Geant4 REQUIRED ui_all vis_all)

set(ROOT_DIR "/usr/local/root_6.36.06/cmake")
find_package(ROOT REQUIRED COMPONENTS Core RIO Tree)

include(${Geant4_USE_FILE})

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB_RECURSE sources ${PROJECT_SOURCE_DIR}/src/*.cc)

file(GLOB MACRO_FILES "macros/*.mac")
file(COPY ${MACRO_FILES} DESTINATION ${CMAKE_BINARY_DIR}/)

add_executable(sim sim.cc ${sources})
target_link_libraries(
    sim
    PRIVATE
    ${Geant4_LIBRARIES}
    ROOT::Core
    ROOT::RIO
    ROOT::Tree
)

add_custom_target(Tutorial DEPENDS sim)




