In order to compile this for Bela (cortex A8, arm-linux-gnueabihf), run:
```
for a in *.c; do arm-linux-gnueabihf-gcc $a -c -O3 -march=armv7-a -mtune=cortex-a8 -mfloat-abi=hard -mfpu=neon -ftree-vectorize -ftree-vectorizer-verbose=3 -std=gnu99 -Wa,-mimplicit-it=thumb || echo ERROR: $a; done
```
Where:
* `-std=gnu99` makes sure the `asm` directive is well understood (and at the same time C99's "'for' loop initial initial declarations" work alright, and
* `-Wa,-mimplicit-it=thumb` fixes the error "Error: thumb conditional instruction should be in IT block -- `bxeq lr'" in math_sqrtfv.c

or simply use the `Makefile`, which also builds the static library.
