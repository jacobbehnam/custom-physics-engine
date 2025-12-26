cmake_minimum_required(VERSION 3.22)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/main.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(UnitTests
        PhysicsTests.cpp
)

target_link_libraries(UnitTests PRIVATE
        GTest::gtest_main
        PhysicsCore
)

include(GoogleTest)
gtest_discover_tests(UnitTests)