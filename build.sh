# Generate solution and build with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build

cd build
make

# Run unit tests
ctest --output-on-failure
