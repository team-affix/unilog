REMOTE_LIBSWIPL := swipl/build/src/libswipl.so.9
LIBSWIPL := build/libswipl.so.9
LIBUNI   := build/libuni.so
MAINBIN  := build/uni
TESTBIN  := build/test

all: $(LIBSWIPL) $(LIBUNI) $(MAINBIN) $(TESTBIN) test

$(REMOTE_LIBSWIPL):
	# Build SWIPL
	# https://github.com/SWI-Prolog/swipl-devel/blob/master/CMAKE.md

	# NOTE: distinct terminal sessions are given to each distinct command.
	#     hence the `&& \` for each cd command.
	cd swipl && \
	mkdir -p build && \
	cd build && \
	cmake -G Ninja .. && \
	ninja && \
	ctest -j 8 && \
	ninja install

$(LIBSWIPL): $(REMOTE_LIBSWIPL)
	# Create the root build output
	mkdir -p build

	# Copy libswipl.so.9 to the pwd
	cp -f $(REMOTE_LIBSWIPL) build/
	

$(LIBUNI): $(LIBSWIPL)
	##########################
	#### COMPILE UNI LIB #####
	##########################

	# Build our library
	swipl/build/src/swipl-ld -shared -goal true -o $(LIBUNI) src_lib/*.cpp src_lib/*.pl

	##########################
	##########################

$(MAINBIN): $(LIBUNI)
	##########################
	#### COMPILE UNI MAIN ####
	##########################

	# Build our application
	g++ -std=c++20 -Wall -g -o $(MAINBIN) src_main/*.cpp $(LIBUNI) $(LIBSWIPL)

	# Link manually to the library which will be expected to sit alongside the executable.
	patchelf --set-rpath '$$ORIGIN' $(MAINBIN)

	##########################
	##########################

$(TESTBIN): $(LIBUNI)
	###########################
	#### COMPILE UNI TESTS ####
	###########################

	# Build our test application
	g++ -std=c++20 -Wall -g -o $(TESTBIN) src_test/*.cpp $(LIBUNI) $(LIBSWIPL)

	# Link manually to the library which will be expected to sit alongside the executable.
	patchelf --set-rpath '$$ORIGIN' $(TESTBIN)

test: $(TESTBIN)
	$(TESTBIN)

clean:
	# Only remove the local build folder, but not swipl's
	rm -rf ./build

deep_clean: clean
	# Remove the swipl build folder as well.
	rm -rf ./swipl/build

