# CMAKE_CXX_STANDARD was introduced in CMake 3.1.
cmake_minimum_required (VERSION 3.1)
project(stl_implementations)

# Build with C++11.
set (CMAKE_CXX_STANDARD 11)


set(INC_FOLDER   "${PROJECT_SOURCE_DIR}/include")
set(TESTS_FOLDER "${PROJECT_SOURCE_DIR}/unit_tests")

# If there are no CMakeLists.txt changes when a source is added
# or removed then the generated build system cannot know when
# to ask CMake to regenerate. So it's good practice to list all
# the included files manually.

set(INC_FILES
)

set(TESTS_FILES
    ${TESTS_FOLDER}/unordered_map_test.cpp
    ${TESTS_FOLDER}/flat_hash_map_test.cpp
)


# Make the compiler also search for include files in this location.
include_directories($INC_FOLDER)


option(BUILD_TESTS "Build and run unit tests." ON)
if (BUILD_TESTS)
    # Add the subdirectory containing the CMakeLists.txt for the gtest library.
    add_subdirectory(googletest)
    include_directories("${PROJECT_SOURCE_DIR}/googletest/googletest/include")
    
    # Add the unit tests executable target.
    add_executable(run_unit_tests ${TESTS_FILES} ${INC_FILES})
    target_link_libraries(run_unit_tests gtest_main)

else (BUILD_TESTS)
    message(STATUS "BUILD_TESTS option is OFF.")

endif (BUILD_TESTS)

