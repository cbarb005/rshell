#for now, makefile only has rshell to make

all:
	g++  -Wall -Werror -ansi -pedantic rshell.cpp

rshell: rshell.cpp
	g++  -Wall -Werror -ansi -pedantic rshell.cpp

