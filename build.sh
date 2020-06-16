# Install conan dependencies
mkdir -p build
cd build
conan install ..

# Generate solution and build with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug -S ..
make

# Run unit tests
ctest --output-on-failure
