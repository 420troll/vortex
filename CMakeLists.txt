cmake_minimum_required(VERSION 3.20)
project(TROLLfxVortexLab VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(vortex_audio src/audio/wav.cpp src/dsp/morph_engine.cpp)
target_include_directories(vortex_audio PUBLIC src)
if(MSVC)
  target_compile_options(vortex_audio PRIVATE /W4 /permissive-)
else()
  target_compile_options(vortex_audio PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(vortex-render tools/audio_render/main.cpp)
target_link_libraries(vortex-render PRIVATE vortex_audio)

include(CTest)
if(BUILD_TESTING)
  add_executable(vortex-audio-tests tests/audio_tests.cpp)
  target_link_libraries(vortex-audio-tests PRIVATE vortex_audio)
  add_test(NAME vortex-audio-tests COMMAND vortex-audio-tests)
endif()
