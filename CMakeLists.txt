cmake_minimum_required(VERSION 3.20)
project(Boosterik VERSION 1.0.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
  message(FATAL_ERROR "Boosterik is a Windows application (Win32 + GDI+).")
endif()

# Static runtime: users do not need the Visual C++ Redistributable
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  add_compile_options(/utf-8 /W3 /permissive-)
else()
  add_compile_options(-Wall -finput-charset=UTF-8)
endif()
add_compile_definitions(UNICODE _UNICODE NOMINMAX _WIN32_WINNT=0x0A00)

file(GLOB_RECURSE BOOSTERIK_SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(Boosterik WIN32 ${BOOSTERIK_SOURCES} resources.rc)
target_compile_definitions(Boosterik PRIVATE BOOSTERIK_VERSION="${PROJECT_VERSION}")
target_link_libraries(Boosterik PRIVATE gdiplus winmm dwmapi psapi shell32 ole32 user32 gdi32 advapi32 magnification)
if(NOT MSVC)
  target_link_options(Boosterik PRIVATE -mwindows -static)
endif()

# Put assets / custom content / configs next to the exe
add_custom_command(TARGET Boosterik POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:Boosterik>/assets
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/custom $<TARGET_FILE_DIR:Boosterik>/custom
  COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Boosterik>/configs)

# Tests (cmake -S . -B build -DBOOSTERIK_TESTS=ON)
option(BOOSTERIK_TESTS "Build tests" OFF)
if(BOOSTERIK_TESTS)
  enable_testing()
  add_executable(test_util tests/test_util.cpp src/Core/Util.cpp)
  target_include_directories(test_util PRIVATE src)
  target_link_libraries(test_util PRIVATE advapi32)
  add_test(NAME util COMMAND test_util)
endif()
