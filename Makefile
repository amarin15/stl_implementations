ifeq ($(OS),Windows_NT)
	RM = rd /s /q
else
	RM = rm -rf
endif


all:
	mkdir -p build

	# Install dependencies, generate cmake solution,
	# build with debug symbols and run unit tests.
	cd build \
		&& conan install --build=missing .. \
		&& cmake -DCMAKE_BUILD_TYPE=Debug -S .. \
		&& make \
		&& ctest --output-on-failure

.PHONY: clean
clean:
	${RM} build
