
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

# Copy libswipl.so.9 to the pwd
cp -f swipl/build/src/libswipl.so.9 build/

cd src

# Build our application
../swipl/build/src/swipl-ld -goal true -o ../build/calc calc.c calc.pl

cd ..

# Link manually to the library which will be expected to sit alongside the executable.
patchelf --set-rpath '$ORIGIN' build/calc
