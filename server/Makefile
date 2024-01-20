EXECUTABLE_NAME = aesdsocket
CROSS_COMPILE = 0

ifeq ($(CROSS_COMPILE),0)
CC = gcc
else
CC = $(CROSS_COMPILE)gcc
endif

#ARCH = aarch64 (It is used to define the architecture of target device, not used in this code)
C_STANDARD = gnu99
C_WARNINGS = -Wpedantic -Wall -Wextra -std=$(C_STANDARD)
CFLAGS = $(C_WARNINGS)

COMPILER_CALL = $(CC) $(CFLAGS)

all: build
default: build

build: aesdsocket.o
	@$(COMPILER_CALL) aesdsocket.o -o $(EXECUTABLE_NAME)	

clean:
	@rm -f *.o && rm -f $(EXECUTABLE_NAME)

aesdsocket.o:
	@$(COMPILER_CALL) aesdsocket.c -c