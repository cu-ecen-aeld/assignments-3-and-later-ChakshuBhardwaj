EXECUTABLE_NAME = writer
CROSS_COMPILE = 0

ifeq ($(CROSS_COMPILE),0)
CC = gcc
else
CC = $(CROSS_COMPILE)gcc
endif

#ARCH = aarch64 (It is used to define the architecture of target device, not used in this code)
C_WARNINGS = -Wpedantic -Wall -Wextra
CFLAGS = $(C_WARNINGS)

COMPILER_CALL = $(CC) $(CFLAGS)

all: build

build: writer.o
	@$(COMPILER_CALL) writer.o -o $(EXECUTABLE_NAME)	

clean:
	@rm -f *.o && rm -f $(EXECUTABLE_NAME)

writer.o:
	@$(COMPILER_CALL) writer.c -c
