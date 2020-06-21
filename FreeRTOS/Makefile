#
# Ngenda Henry, June 2020
#

LIB_NAME = librtos
RTOS_ARCH = ARM_CM4F

SRCS += $(wildcard source/*.c)
SRCS += $(wildcard source/portable/GCC/$(RTOS_ARCH)/*.c)
SRCS += $(wildcard source/portable/MemMang/*4.c)

CPPFLAGS += -Iconfig/
CPPFLAGS += -Isource/include/
CPPFLAGS += -Isource/portable/GCC/$(RTOS_ARCH)/

OBJECTS := $(SRCS:.c=.o)

PREFIX ?= arm-none-eabi-

CC			:= $(PREFIX)gcc
LD			:= $(PREFIX)gcc
AR			:= $(PREFIX)ar
GDB			:= $(PREFIX)gdb
DEBUG		:= -ggdb3
FP_FLAGS	?= -mfloat-abi=hard -mfpu=fpv4-sp-d16
ARCH_FLAGS	= -mthumb -mcpu=cortex-m4 $(FP_FLAGS)
ARFLAGS 	= rcs

CFLAGS 		= -Os -std=c99 \
		  		-Wall -Wextra -Wimplicit-function-declaration \
		  		-Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes \
		  		-Wundef -Wshadow -fno-common -Wstrict-prototypes \
		  		-ffunction-sections -fdata-sections -MD

.PHONY: clean

all: $(LIB_NAME)

$(LIB_NAME): $(OBJECTS)
	$(AR) $(ARFLAGS) lib/$@.a $^
	ln -s ../portable/GCC/$(RTOS_ARCH)/portmacro.h ./source/include/portmacro.h

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(ARCH_FLAGS) -o $@ -c $<
	
clean:
	rm -f lib/*.a $(SRCS:.c=.d) $(OBJECTS) ./source/include/portmacro.h
