# Create build folder if it doesn't exist
mkdir -p build
cd build

# Generate solution and build
cmake ..
cmake --build .

# Run unit tests
ctest --output-on-failure

