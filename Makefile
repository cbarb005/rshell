#for now, makefile only has rshell to make

VPATH = src
FLAGS = -Wall -Werror  -pedantic

all: bin  rshell cp ls

rshell: bin 
	g++  $(FLAGS) src/rshell.cpp  -o  ./bin/rshell  

cp: bin 
	g++ $(FLAGS) src/cp.cpp -o ./bin/cp


ls: bin
	g++ $(FLAGS) src/ls.cpp -o ./bin/ls

bin:
	[ ! -d bin ] && mkdir bin

clean:
	rm -rf bin
