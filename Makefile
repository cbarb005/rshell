#for now, makefile only has rshell to make

VPATH = src
FLAGS = -Wall -Werror -ansi -pedantic

all: bin  rshell cp

rshell: bin 
	g++  $(FLAGS) src/rshell.cpp  -o  ./bin/rshell  
cp: 
	g++ $(FLAGS) src/cp.cpp -o ./bin/cp
bin:
	[ ! -d bin ] && mkdir bin

clean:
	rm -rf bin
