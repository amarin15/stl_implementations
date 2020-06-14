@REM Install conan dependencies
mkdir -p build
cd build
conan install ..

@REM Generate solution and build it with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug -S ..
make

@REM Run unit tests
ctest --output-on-failure
cd ..
