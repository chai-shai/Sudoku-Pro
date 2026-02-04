cmake_minimum_required(VERSION 3.16)
project(Sudoku VERSION 0.1 LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_mixer REQUIRED)

file(GLOB SOURCES "src/*.cpp" "src/imgui/*.cpp")

add_executable(sudoku ${SOURCES})

add_compile_options(-O2 -s)
add_custom_command(
  TARGET sudoku POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "${CMAKE_CURRENT_SOURCE_DIR}/res"
  "$<TARGET_FILE_DIR:sudoku>/res"
  COMMENT "Copying resources to build directory..."
)

target_link_libraries(sudoku PRIVATE SDL2::SDL2)
target_link_libraries(sudoku PRIVATE SDL2_ttf::SDL2_ttf)
target_link_libraries(sudoku PRIVATE SDL2_mixer::SDL2_mixer)
