include(scripts/cmake/code_modules.cmake)

vs_configuration_setup()

cmake_minimum_required(VERSION 3.25)
project(Thermite LANGUAGES CXX)

# hide away ZERO_CHECK and ALL_BUILD
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")

### Engine specific

# use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# define THERMITE options 
option(THERMITE_EDITOR_BUILD "Build editor version" ON)
option(THERMITE_DEBUG_BUILD "Thermite in debug build" ON)

# output binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# compile commands for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

### Compilation settings
option(WARNINGS_AS_ERRORS "Enable warnings as errors" OFF)

# add engine
add_subdirectory(engine)


### Game specific
