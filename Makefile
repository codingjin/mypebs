CC = g++
CFLAGS = -g -w -fPIC
INCLUDES = -I/usr/local/include
LIBS = -lm -lpthread -ldl -lpfm

OBJ1 = gaussian
SRC1 = gaussian.cpp

OBJ2 = pebs
SRC2 = pebsmain.c pebs.c



all: $(SRC1) $(SRC2)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(OBJ1) $(SRC1) $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(OBJ2) $(SRC2) $(LIBS)

.PHONY: clean

clean:
	rm $(OBJ1) $(OBJ2)
