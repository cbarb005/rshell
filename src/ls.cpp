#include <iostream>
#include <stdio.h>
#include <sys/types.h>

using namespace std;

void -all( );
void -list( );
void -recursive();
bool isFlag(char*);
bool isDir(char*);

int main(int argc, char**argv)
{
	for(int i=0; i<argc;++i)
	{
		//write functions to check: 
		//if argv[i] starts with a "-", designate as flag
		//if argv[i] is a file, and if reg or linked
		//if it is a directory
	}

	//parse flags letter by letter
	//based on letter, call each function


	return 0;
}


