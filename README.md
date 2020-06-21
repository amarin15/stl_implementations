# STL implementations  [![Build Status](https://travis-ci.org/amarin15/stl_implementations.svg?branch=master)](https://travis-ci.org/amarin15/stl_implementations)

This repo was created with an educational purpose in mind, so use at your own risk. That being said, the header-only implementations cover most of their STL interfaces and their APIs have been tested using `gtest`. They avoid complexities such as allocators or hints so that they're easier to follow (and not because I'm lazy, ok?).

STL implementations:
- [`unordered_map`](https://github.com/amarin15/stl_implementations/blob/master/include/si_unordered_map.h) using chaining. Unit tests [here](https://github.com/amarin15/stl_implementations/blob/master/unit_tests/unordered_map_test.cpp) and a performance chart against `std::unordered_map` [here](https://amarin15.github.io/stl_implementations/hash_maps_performance.html).
- [`flat_hash_map`](https://github.com/amarin15/stl_implementations/blob/master/include/si_flat_hash_map.h) using open addressing with quadratic probing. Aims to implement the [`absl::flat_hash_map`](https://abseil.io/docs/cpp/guides/container)  presented [`here`](https://www.youtube.com/watch?v=ncHmEUmJZf4) (without the actual SSE instructions though). Shares interface unit tests with [`unordered_map`](https://github.com/amarin15/stl_implementations/blob/master/unit_tests/unordered_map_test.cpp) and has specific unit tests [here](https://github.com/amarin15/stl_implementations/blob/master/unit_tests/flat_hash_map_test.cpp). Still needs load testing and a shootout graph against the maps above.
- [`shared_ptr`](https://github.com/amarin15/stl_implementations/blob/master/include/si_shared_ptr.h). Unit tests [here](https://github.com/amarin15/stl_implementations/blob/master/unit_tests/shared_ptr_test.cpp).
- [`unique_ptr`](https://github.com/amarin15/stl_implementations/blob/master/include/si_unique_ptr.h). Unit tests [here](https://github.com/amarin15/stl_implementations/blob/master/unit_tests/unique_ptr_test.cpp).
- [`tuple`](https://github.com/amarin15/stl_implementations/blob/master/include/si_tuple.h). Unit tests [here](https://github.com/amarin15/stl_implementations/blob/master/unit_tests/tuple_test.cpp).

Thread-safe implementations:
- [Threadsafe stack](https://github.com/amarin15/stl_implementations/blob/master/include/si_threadsafe_stack.h). Unit tests [here](https://github.com/amarin15/stl_implementations/blob/master/unit_tests/threadsafe_stack_test.cpp).
- [Threadsafe queue](https://github.com/amarin15/stl_implementations/blob/master/include/si_threadsafe_queue.h). Unit tests [here](https://github.com/amarin15/stl_implementations/blob/master/unit_tests/threadsafe_queue_test.cpp).
- [Single producer multiple consumer queue](https://github.com/amarin15/stl_implementations/blob/master/include/si_spmc_queue.h). Unit tests [here](https://github.com/amarin15/stl_implementations/blob/master/unit_tests/spmc_queue_test.cpp).
- [Circular buffer](https://github.com/amarin15/stl_implementations/blob/master/include/si_ring_buffer.h). Unit tests [here](https://github.com/amarin15/stl_implementations/blob/master/unit_tests/ring_buffer_test.cpp).
- [Spinlock mutex](https://github.com/amarin15/stl_implementations/blob/master/include/si_spinlock_mutex.h). Unit tests [here](https://github.com/amarin15/stl_implementations/blob/master/unit_tests/spinlock_mutex_test.cpp).

### Build steps
- `make` (should work on both Linux/MacOS as well as Windows)
  - creates a `build` folder if it doesn't already exist
  - resolves dependencies using [Conan](https://conan.io/)
  - generates [CMake](https://cmake.org/) solution
  - builds with [Debug](https://cmake.org/cmake/help/v3.0/variable/CMAKE_BUILD_TYPE.html) symbols
  - runs unit tests using [ctest](https://cmake.org/cmake/help/latest/manual/ctest.1.html)
- `make clean` removes the build folder
