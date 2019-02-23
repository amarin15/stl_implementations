mkdir build
cd build

@REM Generate solution for Visual Studio and build it
cmake ..
cmake --build .

@REM Run unit tests
ctest --output-on-failure
cd ..

