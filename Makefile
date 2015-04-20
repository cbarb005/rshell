#for now, makefile only has rshell to make

VPATH = src
FLAGS = -Wall -Werror -ansi -pedantic

all: bin  rshell 

rshell: bin 
	g++  $(FLAGS) src/rshell.cpp  -o  ./bin/rshell  

bin:
	[ ! -d bin ] && mkdir bin

clean:
	rm -rf bin
