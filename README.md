# STL implementations  [![Build Status](https://travis-ci.org/amarin15/stl_implementations.svg?branch=master)](https://travis-ci.org/amarin15/stl_implementations)

This repo was created with an educational purpose in mind, so use at your own risk. That being said, the header-only implementations cover most of their STL interfaces and their APIs have been tested using `gtest`. They avoid complexities such as allocators or hints so that they're easier to follow (and not because I'm lazy, ok?).

Currently implemented:
- [`unordered_map`](https://github.com/amarin15/stl_implementations/blob/master/include/si_unordered_map.h) using chaining. Unit tests [here](https://github.com/amarin15/stl_implementations/blob/master/unit_tests/unordered_map_test.cpp) and a performance chart against `std::unordered_map` [here](https://amarin15.github.io/stl_implementations/hash_maps_performance.html).
- [`flat_hash_map`](https://github.com/amarin15/stl_implementations/blob/master/include/si_flat_hash_map.h) using open addressing with quadratic probing. Aims to implement the [`absl::flat_hash_map`](https://abseil.io/docs/cpp/guides/container)  presented [`here`](https://www.youtube.com/watch?v=ncHmEUmJZf4) (without the actual SSE instructions though). Shares interface unit tests with [`unordered_map`](https://github.com/amarin15/stl_implementations/blob/master/unit_tests/unordered_map_test.cpp) and has specific unit tests [here](https://github.com/amarin15/stl_implementations/blob/master/unit_tests/flat_hash_map_test.cpp). Still needs load testing and a shootout graph against the maps above.
- [`tuple`](https://github.com/amarin15/stl_implementations/blob/master/include/si_tuple.h). Unit tests [here](https://github.com/amarin15/stl_implementations/blob/master/unit_tests/tuple_test.cpp).

