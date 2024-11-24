SWIPL_INCLUDE_PATH := /usr/lib/swi-prolog/include/

REMOTE_LIBSWIPL := /usr/lib/swi-prolog/lib/x86_64-linux/libswipl.so.9
LIBSWIPL 		:= build/libswipl.so.9
LIBUNI   		:= build/libuni.so
LIBUNI_TEST     := build/libuni_test.so
MAINBIN  		:= build/uni
TESTBIN  		:= build/test

all: $(LIBSWIPL) $(LIBUNI) $(MAINBIN) $(TESTBIN)

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
	g++ -std=c++20 -Wall -g -DUNIT_TEST -o $(TESTBIN) src/*.cpp $(LIBUNI_TEST) $(LIBSWIPL) -I$(SWIPL_INCLUDE_PATH)

	# Link manually to the library which will be expected to sit alongside the executable.
	patchelf --set-rpath '$$ORIGIN' $(TESTBIN)

# $(MAINBIN): $(wildcard src_main/*.cpp) $(wildcard src_main/*.hpp) $(LIBUNI) $(LIBSWIPL)
# 	##########################
# 	#### COMPILE UNI MAIN ####
# 	##########################

# 	# Build our application
# 	g++ -std=c++20 -Wall -g -o $(MAINBIN) src_main/*.cpp $(LIBUNI) $(LIBSWIPL) -I$(SWIPL_INCLUDE_PATH)

# 	# Link manually to the library which will be expected to sit alongside the executable.
# 	patchelf --set-rpath '$$ORIGIN' $(MAINBIN)

# 	##########################
# 	##########################

test: $(TESTBIN)

clean:
	# Remove the local build folder
	rm -rf ./build
