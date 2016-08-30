CFLAGS := -O2 -mtune=cortex-a8 -mfloat-abi=hard -mfpu=neon -ansi -std=gnu99 -ftree-vectorize -ftree-vectorizer-verbose=10
WARNINGS := -Wall -Wextra -Wno-unused-parameter -Wmissing-prototypes
WARNINGS=
ASSEMBLER := -Wa,-mimplicit-it=thumb
CC=arm-linux-gnueabihf-gcc
AR=arm-linux-gnueabihf-ar
BUILD_FOLDER=./build/

override CFLAGS += $(WARNINGS) $(ASSEMBLER)
LIBS := -lm

lib: libmathneon.a

all: math_debug

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


math_debug: clean math_debug.o libmathneon.a #untested
	$(CC) -ggdb $(LDFLAGS) -o $@ $^ $(LIBS)

$(BUILD_FOLDER)%.o:: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

%.a::
	$(AR) rcs $@ $^
	@echo Successfully built $@

clean:
	$(RM) math_debug $(BUILD_FOLDER)*.o *.a
