# CMAKE_CXX_STANDARD was introduced in CMake 3.1.
cmake_minimum_required (VERSION 3.1)
project(stl_implementations)

# Build with C++17 (std::is_invocable is used).
set (CMAKE_CXX_STANDARD 17)

# Enable warnings.
if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()


# If there are no CMakeLists.txt changes when a source is added
# or removed then the generated build system cannot know when
# to ask CMake to regenerate. So it's good practice to list all
# the included files manually.

set(INC_FOLDER   "${PROJECT_SOURCE_DIR}/include")
set(TESTS_FOLDER "${PROJECT_SOURCE_DIR}/unit_tests")
set(INC_FILES
    ${INC_FOLDER}/si_unordered_map.h
    ${INC_FOLDER}/si_flat_hash_map.h
    ${INC_FOLDER}/si_tuple.h
)
set(TESTS_FILES
    ${TESTS_FOLDER}/unordered_map_test.cpp
    ${TESTS_FOLDER}/flat_hash_map_test.cpp
    ${TESTS_FOLDER}/tuple_test.cpp
)

# Set up file tabs for Visual Studio.
source_group(includes   FILES ${INC_FILES})
source_group(unit_tests FILES ${TESTS_FILES})

# Tell the compiler to also search for includes in this location.
include_directories(${INC_FOLDER})


# Also include the boost headers location (for endianness).
find_package(Boost 1.68)
if(Boost_FOUND)
    include_directories( ${Boost_INCLUDE_DIRS} )
endif()


# Add the subdirectory containing the CMakeLists.txt for gtest.
add_subdirectory(googletest)
include_directories("${PROJECT_SOURCE_DIR}/googletest/googletest/include")

# Add the unit tests executable target.
add_executable(run_unit_tests ${TESTS_FILES} ${INC_FILES})
target_link_libraries(run_unit_tests gtest_main)

# Add a test to be run by ctest. This enables us to run the
# unit tests from the build scripts.
enable_testing()
add_test(UnitTests run_unit_tests)

