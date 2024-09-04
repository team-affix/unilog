
# Build SWIPL
# https://github.com/SWI-Prolog/swipl-devel/blob/master/CMAKE.md

cd swipl
mkdir build
cd build
cmake -G Ninja ..
ninja
ctest -j 8
ninja install

# Return to root dir
cd ../../

# Create the root build output
mkdir build

LIBSWIPL="libswipl.so.9"

# Copy libswipl.so.9 to the pwd
cp -f swipl/build/src/$LIBSWIPL build/


OUTLIBRARY="libuni"

##########################
#### COMPILE UNI LIB #####
##########################

OUTFILE=$OUTLIBRARY

# Build our library
swipl/build/src/swipl-ld -shared -goal true -o build/$OUTFILE src_lib/*.cpp src_lib/*.pl

##########################
##########################




##########################
#### COMPILE UNI MAIN ####
##########################

OUTFILE="uni"

# Build our application
#swipl/build/src/swipl-ld -goal true -o build/$OUTFILE src_main/*.cpp src_main/*.pl build/$OUTLIBRARY.so
g++ -std=c++20 -Wall -g -o build/$OUTFILE src_main/*.cpp build/$OUTLIBRARY.so build/$LIBSWIPL

# Link manually to the library which will be expected to sit alongside the executable.
patchelf --set-rpath '$ORIGIN' build/$OUTFILE

##########################
##########################




###########################
#### COMPILE UNI TESTS ####
###########################

OUTFILE="test"

# Build our test application
#swipl/build/src/swipl-ld -goal true -o build/$OUTFILE src_test/*.cpp src_test/*.pl build/$OUTLIBRARY.so
g++ -std=c++20 -Wall -g -o build/$OUTFILE src_test/*.cpp build/$OUTLIBRARY.so build/$LIBSWIPL

# Link manually to the library which will be expected to sit alongside the executable.
patchelf --set-rpath '$ORIGIN' build/$OUTFILE
