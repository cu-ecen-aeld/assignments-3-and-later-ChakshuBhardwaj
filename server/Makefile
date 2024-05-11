# Makefile for the "aesdsocket" application

# Compiler and flags
CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -g -Wall -Werror
INCLUDE=$(dir $(abspath $(MAKEFILE_LIST)))

# libraries to link
LDFLAGS?= -lm -lpthread -lrt -lthreading -lmanipulate_file -lsignal_func
OBJS=(filter-out aesdsocket.o $(patsubst %.c, %.o, $(wildcard *.c)))
LIBS_TYPE=""

.PHONY: all aesdsocket clean

all: aesdsocket

LIBS_TYPE="a"
libmanipulate_file.$(LIBS_TYPE): manipulate_file
	$(AR) -rc $@ $<.o

libsignal_func.$(LIBS_TYPE): signal_func
	$(AR) -rc $@ $<.o

libthreading.$(LIBS_TYPE): threading
	$(AR) -rc $@ $<.o
	
aesdsocket: libthreading.$(LIBS_TYPE) libmanipulate_file.$(LIBS_TYPE) libsignal_func.$(LIBS_TYPE)
	$(CC) $(CFLAGS) -o $@ $(@).c -L$(INCLUDE) $(LDFLAGS)

manipulate_file:
	$(CC) $(CFLAGS) -fPIC -c $@.c $(LDFLAGS)

signal_func:
	$(CC) $(CFLAGS) -fPIC -c $@.c $(LDFLAGS)

threading:
	$(CC) $(CFLAGS) -fPIC -c $@.c $(LDFLAGS)

clean:	
	$(RM) aesdsocket $(wildcard *.o) $(wildcard *.so) $(wildcard *.a)