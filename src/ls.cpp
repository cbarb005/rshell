#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <set>

using namespace std;

void all( );
void list( );
void recursive();
void flagID(char*,set<char>&);
bool isFlag(char*);

int main(int argc, char**argv)
{	
	char *dirc;	
	set<char> flags;

	for(int i=0; i<argc;++i)
	{
		struct stat buf;
		 
		//if argv[i] starts with a "-", designate as flag
		if(isFlag(argv[i]))
		{
			flagID(argv[i],flags);
		}

		stat(argv[i],&buf);
		//if it is a directory
		if(buf.st_mode & S_IFDIR)
		{
			dirc=argv[i]; //captures directory
		}

	}
	
	if(dirc !=NULL)	{cout << "directory found";	}
	cout << endl << flags.size();

	return 0;
}

void flagID(char *flag, set<char> &flagSet)
{
	//function adds detected flags into set
	char* a=strchr(flag,'a');
	if(a != NULL) { flagSet.insert('a'); }

	char* l=strchr(flag,'l');
	if(l != NULL) { flagSet.insert('l'); }

	char* R=strchr(flag,'R');
	if(R != NULL) { flagSet.insert('R'); }
	
	/*possibly check flag input too? if invalid flag
	  passed, then output error*/
	return;
}

bool isFlag(char *c)
{
	char* result=strchr(c,'-');
	if(result==NULL)
	{
		return false;
	}
	else
	{
		return true;
	}
}
