SRC = dram_functions.c
OBJ = $(addsuffix .o, $(basename $(SRC)))
TARGET = $(basename $(OBJ))
CC = gcc
CCFLAGS = 

all: $(TARGET)

dram_functions: dram_functions.o util.o
	${CC} ${CCFLAGS} -o $@ dram_functions.o util.o

util.o: util.c util.h
	${CC} ${CCFLAGS} -c $<

.c.o:
	${CC} ${CCFLAGS} -c $<

clean:
	rm -f *.o $(TARGET) 
