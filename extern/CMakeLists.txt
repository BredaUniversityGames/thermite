# TODO: link to external libraries here.

# Configure spdlog options 
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS  OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH  OFF CACHE BOOL "" FORCE)
add_subdirectory(spdlog)
# Link spdlog
target_link_libraries(thermite-engine
    PUBLIC spdlog::spdlog
)

# SDL3 - https://github.com/libsdl-org/SDL
set(SDL_SHARED              OFF CACHE BOOL "" FORCE)
set(SDL_STATIC              ON  CACHE BOOL "" FORCE)

# No tests / examples / installs
set(SDL_INSTALL				OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL_TESTS       OFF CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL     ON  CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL_DOCS ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY        OFF CACHE BOOL "" FORCE)
set(SDL_TESTS               OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES            OFF CACHE BOOL "" FORCE)
add_subdirectory(SDL3)
# Link SDL3
target_link_libraries(thermite-engine PRIVATE SDL3::SDL3-static)


# Configure glm
set(GLM_ENABLE_CXX_20 ON CACHE BOOL "" FORCE)
add_subdirectory(glm)
# Link glm
target_link_libraries(thermite-engine PUBLIC glm::glm)

# Configure entt
set(ENTT_INSTALL ON CACHE BOOL "" FORCE)
add_subdirectory(entt)
# Link entt
target_link_libraries(thermite-engine
    PUBLIC EnTT::EnTT
)







