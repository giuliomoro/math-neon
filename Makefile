CC=clang

ifeq ($(CC),clang)
  COMPILER_CFLAGS=-no-integrated-as
  BUILD_FOLDER=./build_clang/
else
  COMPILER_CFLAGS=--fast-math
  BUILD_FOLDER=./build_gcc/
endif

$(shell mkdir -p $(BUILD_FOLDER))
CFLAGS := -O2 -mtune=cortex-a8 -mfloat-abi=hard -mfpu=neon -ansi -std=gnu99 -ftree-vectorize -I./ $(COMPILER_CFLAGS)
TEST_CFLAGS := -I/usr/xenomai/include
WARNINGS := -Wall -Wextra -Wno-unused-parameter -Wmissing-prototypes
WARNINGS=
ASSEMBLER := -Wa,-mimplicit-it=arm
AR=arm-linux-gnueabihf-ar

override CFLAGS += $(WARNINGS) $(ASSEMBLER)
LIBS := -lm 

lib: libmathneon.a

C_SRCS := $(wildcard *.c)
C_OBJS := $(addprefix $(BUILD_FOLDER),$(notdir $(C_SRCS:.c=.o)))
C_DEPS := $(addprefix $(BUILD_FOLDER),$(notdir $(C_SRCS:.c=.d)))

libmathneon.a: $(C_OBJS)
#libmathneon.a: math_acosf.o math_ldexpf.o math_powf.o math_sqrtfv.o \
	math_asinf.o math_expf.o math_log10f.o math_runfast.o math_tanf.o \
	math_atan2f.o  math_fabsf.o math_logf.o math_sincosf.o math_tanhf.o \
	math_atanf.o math_floorf.o math_mat2.o math_sinf.o math_vec2.o \
	math_ceilf.o math_fmodf.o math_mat3.o math_sinfv.o math_vec3.o \
	math_cosf.o math_frexpf.o math_mat4.o math_sinhf.o math_vec4.o \
	math_coshf.o math_invsqrtf.o math_modf.o math_sqrtf.o

SRCS = $(wildcard *.c)
OBJS= $(addprefix $(BUILD_FOLDER),$(notdir $(SRCS:.c=.o)))

#$(warning $(SRCS) $(OBJS))
XENOMAI_LDLIBS=-lrt -lxenomai -lnative -L/usr/xenomai/lib
test: $(BUILD_FOLDER)test.o $(BUILD_FOLDER)libmathneon.a
	$(CC) $(BUILD_FOLDER)test.o -o test_$(CC) $(LIBS) $(BUILD_FOLDER)libmathneon.a $(XENOMAI_LDLIBS)

$(BUILD_FOLDER)%.o: %.c
	$(CC) $(TEST_CFLAGS) $(CFLAGS) -o $@ -c $< 

%.a: $(OBJS)
	$(AR) rcs $@ $^
	@echo Successfully built $@

.PHONY: math_debug
math_debug:math_debug_$(CC)

math_debug_$(CC): $(BUILD_FOLDER)math_debug.o $(BUILD_FOLDER)libmathneon.a 
	$(CC) -ggdb $(LDFLAGS) -o $@ $^ $(LIBS) $(TEST_CFLAGS) $(XENOMAI_LDLIBS)

clean:
	$(RM) math_debug $(BUILD_FOLDER)*.o *.a
