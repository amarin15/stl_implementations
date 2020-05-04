@REM Generate solution and build it with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build

cd build
make

@REM Run unit tests
ctest --output-on-failure
cd ..
