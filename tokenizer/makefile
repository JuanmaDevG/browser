.PHONY: clean

CC:=g++
FLAGS:= -g -Wall -Werror -std=c++11 -I.
DEBUG_LOG:= #-D DEBUG
VPATH:=src test
FILES:=$(shell ls src) main.cpp
OUT:=practica1


$(OUT): $(FILES)
	$(CC) $(FLAGS) $(DEBUG_LOG) $^ -o $(OUT)


clean:
	rm practica1
