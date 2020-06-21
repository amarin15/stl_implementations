# CMAKE_CXX_STANDARD was introduced in CMake 3.1.
cmake_minimum_required (VERSION 3.1)
project(stl_implementations)

# We need at least C++17 (std::is_invocable is used).
set (CMAKE_CXX_STANDARD 20)

# Warnings are not errors
# (we have a known Wself-assign-overloaded in the shared_ptr unit test)
if (MSVC)
    # all warnings that aren't off by default
    # https://docs.microsoft.com/en-us/cpp/preprocessor/compiler-warnings-that-are-off-by-default
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -pedantic)
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
    ${INC_FOLDER}/si_shared_ptr.h
    ${INC_FOLDER}/si_unique_ptr.h
    ${INC_FOLDER}/si_tuple.h
    ${INC_FOLDER}/si_spmc_queue.h
    ${INC_FOLDER}/si_ring_buffer.h
)
set(TESTS_FILES
    ${TESTS_FOLDER}/unordered_map_test.cpp
    ${TESTS_FOLDER}/flat_hash_map_test.cpp
    ${TESTS_FOLDER}/shared_ptr_test.cpp
    ${TESTS_FOLDER}/unique_ptr_test.cpp
    ${TESTS_FOLDER}/tuple_test.cpp
    ${TESTS_FOLDER}/spmc_queue_test.cpp
    ${TESTS_FOLDER}/ring_buffer_test.cpp
)

# Set up file tabs for Visual Studio.
source_group(includes   FILES ${INC_FILES})
source_group(unit_tests FILES ${TESTS_FILES})

# Tell the compiler to also search for includes in this location.
include_directories(${INC_FOLDER})

# Add the conan dependencies.
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# Add the unit tests executable target.
add_executable(run_unit_tests ${TESTS_FILES} ${INC_FILES})
target_link_libraries(run_unit_tests ${CONAN_LIBS})

# Add a test to be run by ctest. This enables us to run the
# unit tests from the build scripts.
enable_testing()
add_test(UnitTests bin/run_unit_tests)
