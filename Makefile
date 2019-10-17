CC=gcc
CFLAGS=-Wall -Iinclude/
LIBS=-lpthread -lasound -lm -lrt -lpaho-mqtt3c
# DEPS=def.h fft.h vad_moatt.h pcm_capture.h
SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)

# OBJ=main.o fft.o vad_moatt.o pcm_capture.o

# default: all

.PHONY: run

run: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
	mv src/*.o obj/

.PHONY: clean

clean:
	rm -f obj/*.o run
