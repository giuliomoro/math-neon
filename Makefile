CC=clang

ifeq ($(CC),clang)
  COMPILER_CFLAGS=-no-integrated-as
  BUILD_FOLDER=./build_clang/
else
  COMPILER_CFLAGS=--fast-math
  BUILD_FOLDER=./build_gcc/
endif

$(shell mkdir -p $(BUILD_FOLDER))
CFLAGS := -O2 -mtune=cortex-a8 -mfloat-abi=hard -mfpu=neon -ansi -std=gnu99 -ftree-vectorize -I./ $(COMPILER_CFLAGS) -Wno-return-type
TEST_CFLAGS := `/usr/xenomai/bin/xeno-config --skin=native --cflags`
WARNINGS := -Wall -Wextra -Wno-unused-parameter -Wmissing-prototypes
WARNINGS=
ASSEMBLER := -Wa,-mimplicit-it=arm
AR=arm-linux-gnueabihf-ar

override CFLAGS += $(WARNINGS) $(ASSEMBLER)
LIBS := -lm 

SOLIB_EXT :=so.1

lib: libmathneon.a libmathneon.$(SOLIB_EXT)

C_SRCS := $(filter-out math_debug.c,$(wildcard math_*.c))
C_OBJS := $(addprefix $(BUILD_FOLDER),$(notdir $(C_SRCS:.c=.o)))
C_OBJS_PIC := $(addprefix $(BUILD_FOLDER),$(notdir $(C_SRCS:.c=.pic.o)))
C_DEPS := $(addprefix $(BUILD_FOLDER),$(notdir $(C_SRCS:.c=.d)))

libmathneon.a: $(C_OBJS)
	@nm -a $(C_OBJS) | grep main && { echo "Error: there is a main() among the library objects" >&2 ; return 1; } || return 0
	ar rcs $@ $^

libmathneon.$(SOLIB_EXT): $(C_OBJS_PIC)
	@nm -a $(C_OBJS_PIC) | grep main && { echo "Error: there is a main() among the library objects" >&2 ; return 1; } || return 0
	$(CC) $(LDFLAGS) $(C_OBJS_PIC) -shared -Wl,-soname,libmathneon.$(SOLIB_EXT) -o $@

SRCS = $(wildcard *.c)
OBJS= $(addprefix $(BUILD_FOLDER),$(notdir $(SRCS:.c=.o)))

#$(warning $(SRCS) $(OBJS))
XENOMAI_LDLIBS= `/usr/xenomai/bin/xeno-config --skin=native --ldflags`
test: $(BUILD_FOLDER)test.o $(BUILD_FOLDER)libmathneon.a
	$(CC) $(BUILD_FOLDER)test.o -o test_$(CC) $(LIBS) $(BUILD_FOLDER)libmathneon.a $(XENOMAI_LDLIBS)

$(BUILD_FOLDER)test.o: test.c
	$(CC) $(TEST_CFLAGS) $(CFLAGS) -o $@ -c $<
$(BUILD_FOLDER)math_debug.o: math_debug.c
	$(CC) $(TEST_CFLAGS) $(CFLAGS) -o $@ -c $<
$(BUILD_FOLDER)%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
$(BUILD_FOLDER)%.pic.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $< -fPIC

%.a: $(OBJS)
	$(AR) rcs $@ $^
	@echo Successfully built $@

.PHONY: math_debug
math_debug:math_debug_$(CC)

math_debug_$(CC): $(BUILD_FOLDER)math_debug.o $(BUILD_FOLDER)libmathneon.a 
	$(CC) -ggdb $(LDFLAGS) -o $@ $^ $(LIBS) $(TEST_CFLAGS) $(XENOMAI_LDLIBS)

clean:
	$(RM) math_debug $(BUILD_FOLDER)*.o *.a

install:
	install libmathneon.a libmathneon.$(SOLIB_EXT) /usr/lib/
	cd /usr/lib && rm -rf libmathneon.so && ln -s libmathneon.$(SOLIB_EXT) libmathneon.so
	install math_neon.h /usr/include/

.PHONY: libcheck
