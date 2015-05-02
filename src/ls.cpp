#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <string>
#include <vector>
#include <set>


using namespace std;

void all( );
void list( );
void recursive();
void dirStream(set<char>&,char*);
void setflags(set<char>&,bool&,bool&,bool&);
int flagID(char*,set<char>&);
bool isDotFile(char*);
bool isFlag(char*);

int main(int argc, char**argv)
{	
	vector<char*> dirc;	
	set<char> flags;

	for(int i=0; i<argc;++i)
	{
		struct stat buf;
		 
		//if argv[i] starts with a "-", designate as flag
		if(isFlag(argv[i]))
		{	
			int j =flagID(argv[i],flags); //pull flags out
			if(j==-1) { break;  } 
			else { continue;}

		}

		stat(argv[i],&buf);
		//if it is a directory
		if(buf.st_mode & S_IFDIR)
		{
			dirc.push_back(argv[i]); //captures directory
			cout << "main 2\n";
		}
		if(buf.st_mode & S_IFREG)
		{
			//stuff?
		}

	}

	if(dirc.size()==0) //if no directory was specifically passed in
	{
		char cwd[]=".";
		dirc.push_back(cwd); //assume current working directory
	}
	
	for(unsigned i=0; i<dirc.size();++i) 
	{
		dirStream(flags,dirc[i]);
	}
	return 0;
}

void dirStream(set<char> &flagSet,char* dirc)
{
	DIR *dirptr;
	struct dirent *fileInfo;
	bool a,l,R;
	setflags(flagSet,a,l,R);

	if(R)
	{
		cout << dirc << ":" << endl;
	}
	if((dirptr=opendir(dirc))==NULL)
	{
		perror("opendir");
	}
	errno=0;
	while(NULL != (fileInfo=readdir(dirptr)))
	{	
		if(isDotFile(fileInfo->d_name) && !a)
		{ continue; } //skip over dot files

		cout << fileInfo->d_name << " ";

		if(R && fileInfo->d_type==DT_DIR)
		{	

			//recursion here I guess
		}

	}
	if(errno != 0)
	{
		perror("readdir");
	}
	if(-1 == closedir(dirptr))
	{
		perror("closedir");
	}
		
	return;
}

void setflags(set<char> &flags,bool& a,bool& l,bool& R)
{
	if(flags.find('a') != flags.end())	{	a=true;	}
	else { a=false; }

	if(flags.find('l') != flags.end())	{	l=true;	}
	else { l=false; }

	if(flags.find('R') != flags.end())	{	R=true;	}
	else { R=false; }

	return;
}
int flagID(char *flag, set<char> &flagSet)
{
	string str(flag);
	unsigned valid=1; 
	if(str.size()==1 && str=="-")
	{
		cerr << "Error: empty flag\n";
		return -1;
	}
	size_t pos=str.find("a");
	if(pos != string::npos)
	{
		flagSet.insert('a');
		++valid;
	}
	size_t pos2=str.find("l");
	if(pos2 != string::npos)
	{
		flagSet.insert('l');
		++valid;
	}	
	size_t pos3=str.find("R");
	if(pos3 != string::npos)
	{
		flagSet.insert('R');
		++valid;
	}		
	if(valid != str.size())
	{	
		cerr << "Invalid flag detected.\n";
		return -1;
	}


	return 0;
}
bool isDotFile(char* c)
{	
	string file(c);
	size_t pos= file.find(".");
	//if not found at all, return false
	if(pos == string::npos) { return false;}
	//if found in first position, return true;
	else if(pos==0)
	{
		return true;
	}
	//if found elsewhere besides beginning, not a dot file
	else {return false;}
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
