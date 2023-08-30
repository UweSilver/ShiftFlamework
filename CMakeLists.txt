cmake_minimum_required(VERSION 3.26)
project(shift_flamework)

set(CMAKE_CXX_STANDARD 20)

file(GLOB sources source/*.cpp)
file(GLOB includes  include/*.hpp)
add_executable(game ${includes} ${sources})

if(EMSCRIPTEN)
    set_target_properties(game PROPERTIES SUFFIX ".html")
    target_link_options(game PRIVATE "-sUSE_WEBGPU=1" "-sUSE_GLFW=3")
else()
    set(DAWN_FETCH_DEPENDENCIES ON)
    add_subdirectory("lib/dawn" EXCLUDE_FROM_ALL)
    target_link_libraries(game PRIVATE webgpu_dawn webgpu_cpp webgpu_glfw)
endif()