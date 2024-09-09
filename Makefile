SWIPL_INCLUDE_PATH := /usr/lib/swi-prolog/include/

REMOTE_LIBSWIPL := /usr/lib/swi-prolog/lib/x86_64-linux/libswipl.so.9
LIBSWIPL 		:= build/libswipl.so.9
LIBUNI   		:= build/libuni.so
MAINBIN  		:= build/uni
TESTBIN  		:= build/test

all: $(LIBSWIPL) $(LIBUNI) $(MAINBIN) $(TESTBIN)

$(LIBSWIPL):
	# Create the root build output
	mkdir -p build

	# Copy libswipl.so.9 to the pwd
	cp -f $(REMOTE_LIBSWIPL) build/

$(LIBUNI): $(LIBSWIPL) $(wildcard src_lib/*.cpp) $(wildcard src_lib/*.pl)
	##########################
	#### COMPILE UNI LIB #####
	##########################

	# Build our library
	swipl-ld -std=c++20 -g -shared -goal true -o $(LIBUNI) src_lib/*.cpp src_lib/*.pl

	##########################
	##########################

$(MAINBIN): $(wildcard src_main/*.cpp) $(LIBUNI) $(LIBSWIPL)
	##########################
	#### COMPILE UNI MAIN ####
	##########################

	# Build our application
	g++ -std=c++20 -Wall -g -o $(MAINBIN) src_main/*.cpp $(LIBUNI) $(LIBSWIPL) -I$(SWIPL_INCLUDE_PATH)

	# Link manually to the library which will be expected to sit alongside the executable.
	patchelf --set-rpath '$$ORIGIN' $(MAINBIN)

	##########################
	##########################

$(TESTBIN): $(wildcard src_test/*.cpp) $(LIBUNI) $(LIBSWIPL)
	###########################
	#### COMPILE UNI TESTS ####
	###########################

	# Build our test application
	g++ -std=c++20 -Wall -g -o $(TESTBIN) src_test/*.cpp $(LIBUNI) $(LIBSWIPL) -I$(SWIPL_INCLUDE_PATH)

	# Link manually to the library which will be expected to sit alongside the executable.
	patchelf --set-rpath '$$ORIGIN' $(TESTBIN)

test: $(TESTBIN)
	$(TESTBIN)

clean:
	# Remove the local build folder
	rm -rf ./build
