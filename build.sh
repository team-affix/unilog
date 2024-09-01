
# Build SWIPL
# https://github.com/SWI-Prolog/swipl-devel/blob/master/CMAKE.md

cd swipl
rm -rf build
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

# Copy libswipl.so.9 to the pwd
cp -f swipl/build/src/libswipl.so.9 build/

cd build

# Instruct the linker that the library can be found here 
#     (omitted due to addition of -rpath option to compilation below)
#export LD_LIBRARY_PATH=$(pwd):$LD_LIBRARY_PATH

# Build our application
rm -rf calc
swipl-ld -goal true -o calc ../src/calc.c ../src/calc.pl -Wl,-rpath,'$ORIGIN'

# https://www.swi-prolog.org/pldoc/man?section=plld
