cmake_minimum_required(VERSION 3.10)
project(LD2420_Tests)

set(CMAKE_CXX_STANDARD 14)

include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(kalman_tests test_kalman.cpp)
target_include_directories(kalman_tests PRIVATE ../libraries/LD2420_Ultimate/src)
target_link_libraries(kalman_tests PRIVATE Catch2::Catch2WithMain)
