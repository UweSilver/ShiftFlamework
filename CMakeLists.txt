cmake_minimum_required(VERSION 3.26)
project(shift_flamework)

set(CMAKE_CXX_STANDARD 20)

file(GLOB sources source/*.cpp)
file(GLOB autogenerated_source out/auto_generated_source/handmade_export.cpp) # use hand crafted source for now
file(GLOB includes include/*.hpp)
add_executable(game ${sources} ${includes} ${autogenerated_source})
target_include_directories(game PUBLIC include lib/assimp/include)

file(GLOB runtime_sources runtime/source/*.cpp)
file(GLOB runtime_includes runtime/include/*.hpp)
add_library(runtime SHARED ${runtime_sources} ${runtime_includes})
target_include_directories(runtime PUBLIC runtime/include)

if(EMSCRIPTEN)
    set_target_properties(game PROPERTIES SUFFIX ".html")
    target_link_options(game PRIVATE "-sUSE_WEBGPU=1" "-sUSE_GLFW=3" "-sALLOW_MEMORY_GROWTH")
else()
    set(DAWN_FETCH_DEPENDENCIES ON)
    # debug -> set_target_properties(game PROPERTIES VS_DEBUGGER_ENVIRONMENT "DAWN_DEBUG_BREAK_ON_ERROR=1")
    add_subdirectory("lib/dawn" EXCLUDE_FROM_ALL)
    target_link_libraries(game PRIVATE webgpu_dawn webgpu_cpp webgpu_glfw)
endif()

set(BUILD_STATIC_LIBS ON)
set(BUILD_ASSIMP_TOOLS ON)
set(ASSIMP_BUILD_STATIC_LIB ON)

add_subdirectory(lib/assimp)
target_link_libraries(game PUBLIC assimp)