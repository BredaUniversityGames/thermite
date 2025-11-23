# engine/CMakeLists.txt

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "Thermite currently only supports x64 architecture")
endif()

if (NOT MSVC)
    message(FATAL_ERROR "Thermite currently only supports building with MSVC.")
endif()

# collect all files (do we want to enforce .cpp .hpp?)
file(GLOB_RECURSE THERMITE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

add_library(ThermiteEngine STATIC ${THERMITE_SOURCES})

# add include path
target_include_directories(ThermiteEngine 
PUBLIC ${CMAKE_SOURCE_DIR}  
PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

# precomp possible..
#target_precompile_headers(ThermiteEngine PRIVATE Thermite/pch.h)

# adding libraries (graphite?)
#add_library(Graphite STATIC IMPORTED)
#set_target_properties(Graphite PROPERTIES
#    IMPORTED_LOCATION "path/to/Graphite.lib"
#    INTERFACE_INCLUDE_DIRECTORIES "path/to/graphite/includes"
#)
#target_link_libraries(ThermiteEngine PUBLIC Graphite)

# VS filters
#source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${THERMITE_SOURCES})

# Warning level
target_compile_options(ThermiteEngine PRIVATE /W4 /permissive-)