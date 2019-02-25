# STL implementations  [![Build Status](https://travis-ci.com/amarin15/stl_implementations.svg?token=Sixm9dAdgNAi8nS7fenD&branch=master)](https://travis-ci.com/amarin15/stl_implementations)

This repo was created with an educational purpose in mind, so use at your own risk. That being said, the header-only implementations cover most of their STL interfaces and their APIs have been fully tested using `gtest`. They avoid complexities such as allocators or hints so that they're easier to follow.

Currently implemented:
- [`unordered_map`](https://github.com/amarin15/stl_implementations/blob/master/include/si_unordered_map.h) using chaining. You can also take a look at the [unit tests](https://github.com/amarin15/stl_implementations/blob/master/unit_tests/unordered_map_test.cpp) and the [performance chart](https://amarin15.github.io/stl_implementations/hash_maps_performance.html) against  `std::unordered_map`.
