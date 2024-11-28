SWIPL_INCLUDE_PATH := /usr/lib/swi-prolog/include/

REMOTE_LIBSWIPL := /usr/lib/swi-prolog/lib/x86_64-linux/libswipl.so.9
LIBSWIPL 		:= build/libswipl.so.9
LIBUNI   		:= build/libuni.so
LIBUNI_TEST     := build/libuni_test.so
MAINBIN  		:= build/uni
TESTBIN  		:= build/test

all: $(LIBSWIPL) $(LIBUNI_TEST) $(LIBUNI) $(MAINBIN) $(TESTBIN)

$(LIBSWIPL):
	# Create the root build output
	mkdir -p build

	# Copy libswipl.so.9 to the pwd
	cp -f $(REMOTE_LIBSWIPL) build/

$(LIBUNI_TEST): $(LIBSWIPL) $(wildcard src/*.cpp) $(wildcard src/*.hpp) $(wildcard src/*.pl)
	##############################
	#### COMPILE UNI TEST LIB ####
	##############################

	# Build our library
	swipl-ld -c++ g++ -cc-options,"-std=c++20 -g -DUNIT_TEST" -shared -goal true src/*.cpp ./src/unilog.pl -o $(LIBUNI_TEST)

	##############################
	##############################

$(TESTBIN): $(wildcard src/*.cpp) $(wildcard src/*.hpp) $(LIBUNI_TEST) $(LIBSWIPL)
	##############################
	#### COMPILE UNI TEST BIN ####
	##############################

	# Build our test application
	#g++ 
	swipl-ld \
		-c++ g++ \
		-cc-options,"-std=c++20 -Wall -g -DUNIT_TEST -I$(SWIPL_INCLUDE_PATH)" \
		-goal true src/*.cpp $(LIBUNI_TEST) $(LIBSWIPL) ./src/unilog.pl -o $(TESTBIN)

	# Link manually to the library which will be expected to sit alongside the executable.
	patchelf --set-rpath '$$ORIGIN' $(TESTBIN)

$(LIBUNI): $(LIBSWIPL) $(wildcard src/*.cpp) $(wildcard src/*.hpp) $(wildcard src/*.pl)
	##############################
	#### COMPILE UNI MAIN LIB ####
	##############################

	# Build our library
	swipl-ld -c++ g++ -cc-options,"-std=c++20 -g" -shared -goal true src/*.cpp ./src/unilog.pl -o $(LIBUNI)

	##############################
	##############################

$(MAINBIN): $(wildcard src/*.cpp) $(wildcard src/*.hpp) $(LIBUNI) $(LIBSWIPL)
	##############################
	#### COMPILE UNI MAIN BIN ####
	##############################

	# Build our test application
	#g++ 
	swipl-ld \
		-c++ g++ \
		-cc-options,"-std=c++20 -Wall -g -I$(SWIPL_INCLUDE_PATH)" \
		-goal true src/*.cpp $(LIBUNI) $(LIBSWIPL) ./src/unilog.pl -o $(MAINBIN)

	# Link manually to the library which will be expected to sit alongside the executable.
	patchelf --set-rpath '$$ORIGIN' $(MAINBIN)

test: $(TESTBIN)

main: $(MAINBIN)

clean:
	# Remove the local build folder
	rm -rf ./build
