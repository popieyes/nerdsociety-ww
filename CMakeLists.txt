cmake_minimum_required(VERSION 3.25) # FetchContent SYSTEM needs 3.25
project(RAYLIB-BOAT CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Dependencies -------------------------------------------------------------------------------
# Web = emcmake (EMSCRIPTEN) or -DPLATFORM=Web. Shaders are GLSL 330 rewritten to GLSL ES 3.00 at load
# time (see SceneShaderLoad), so raylib must use OpenGL ES 3 / WebGL2 (it then links -sMIN_WEBGL_VERSION=2).
if (EMSCRIPTEN OR PLATFORM STREQUAL "Web")
  set(BOAT_WEB ON)
  set(PLATFORM "Web" CACHE STRING "Platform to build for." FORCE)
  set(GRAPHICS "GRAPHICS_API_OPENGL_ES3")
endif()

set(RAYLIB_VERSION 6.0)
find_package(raylib ${RAYLIB_VERSION} QUIET)
if (NOT raylib_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    SYSTEM # raylib headers don't trigger our warnings
  )
  set(FETCHCONTENT_QUIET NO)
  FetchContent_MakeAvailable(raylib)
endif()

# Warnings for our code only
function(boat_warnings target)
  if (MSVC)
    target_compile_options(${target} PRIVATE /W4)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra)
  endif()
endfunction()

# Pure-logic sources: no window/GPU needed at runtime, shared by the game and the tests.
set(BOAT_LOGIC_SOURCES
  src/conductor.cpp
  src/floater.cpp
  src/waves.cpp
  src/weather.cpp
)

# --- Game ---------------------------------------------------------------------------------------
file(GLOB BOAT_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(${PROJECT_NAME} ${BOAT_SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
boat_warnings(${PROJECT_NAME})

# Copy resources/ next to the executable after every build (build/Debug/, build/Release/, ...)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/resources
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
)

if (BOAT_WEB)
  set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
  target_link_options(${PROJECT_NAME} PRIVATE
    -sUSE_GLFW=3 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sASSERTIONS=1 -sWASM=1 -sASYNCIFY
    -sGL_ENABLE_GET_PROC_ADDRESS=1
    --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/resources@resources)
endif()

if (APPLE)
  target_link_libraries(${PROJECT_NAME} PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
endif()

# --- Tests (pure logic, no window) --------------------------------------------------------------
if (NOT BOAT_WEB)
  enable_testing()
  file(GLOB BOAT_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
  add_executable(tests ${BOAT_TEST_SOURCES} ${BOAT_LOGIC_SOURCES})
  target_include_directories(tests PRIVATE src)
  target_link_libraries(tests PRIVATE raylib) # for headers/types; tests never open a window
  boat_warnings(tests)
  add_test(NAME tests COMMAND tests)
endif()
