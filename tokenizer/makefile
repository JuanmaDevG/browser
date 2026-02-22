.PHONY= clean

CC:=g++
FLAGS:= -g -Wall -Werror -std=c++11 -I.
DEBUG:= #-D DEBUG
LIBDIR=lib
INCLUDEDIR=include
_OBJ= tokenizador.o
OBJ= $(patsubst %,$(LIBDIR)/%,$(_OBJ))

all: practica1

practica1: src/main.cpp $(OBJ)
	$(CC) $(OPTIONS) $(DEBUG) -I$(INCLUDEDIR) src/main.cpp $(OBJ) -o practica1

$(LIBDIR)/%.o: $(LIBDIR)/%.cpp $(OBJ)
	$(CC) $(OPTIONS) $(DEBUG) -c -I$(INCLUDEDIR) -o $@ $<

clean:
	rm -f $(OBJ) practica1
