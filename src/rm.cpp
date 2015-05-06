#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <errno.h>
#include <dirent.h>
#include <vector>
using namespace std;

bool isDot(char*);
int isDir(const char*);
void recursiveRm(char*c,bool&);
void removeDir(char*,bool&);
void unlinkFile(const char*);

int main(int argc, char**argv)
{
	bool recursFlag=false;
	string possFlag(argv[1]);
	if(argc <=1) 
	{
		cout << "Error: insufficient arguments\n";
	}

	if(possFlag=="-r")
	{
		recursFlag=true;
	}
	int i;
	if(recursFlag) {i=2;}
	else { i =1; }

	for(i;i<argc;++i)
	{
		int a=isDir(argv[i]);
		if(a==0) //if not directory, assume it's a file
		{
			unlinkFile(argv[i]);	
		}
		else if(a==1)
		{
			removeDir(argv[i],recursFlag);
		}

	}
return 0;
}

void recursiveRm(char*c,bool &flag)
{
	DIR *dirptr;
	struct dirent *info;
	vector<char*> dirVect;

	if(NULL==(dirptr=opendir(c)))
	{
		perror("opendir");
	}
	errno=0;
	while(NULL!= (info=readdir(dirptr)))
	{
		string path(c);
		string temp(info->d_name);
		path+="/"+temp;
		

		int z=isDir(path.c_str());
		if(z==0 )
		{	
			unlinkFile(path.c_str());
		}
		else if(z==1 && !isDot(info->d_name))
		{	
			char*v=const_cast<char*>(path.c_str());
			dirVect.push_back(v);
		}
	}
	if(errno != 0) { perror("readdir"); }
	if(-1==closedir(dirptr)) { perror("closedir"); }

	for(unsigned i=0; i<dirVect.size();++i)
	{
		removeDir(dirVect.at(i),flag);
	}
	removeDir(c, flag);
	return;
}

void removeDir(char*c, bool &flag)
{
	errno=0; //errno 39 indicates directory is not empty(not including . .. files)
	int result=rmdir(c);
	if(errno==0)
	{
		return; //was a success
	}
	if (errno==39 && flag)//if recursive flag, then use helper function to recursively remove
	{
		//recursive deletion
		recursiveRm(c,flag);
		return;
	}
	else if(errno==39 && !flag) //if no recursive flag and directory contains a file, then warn user
	{
		cerr << "Error: directory is not empty.\n";
		return;
	}
	else
	{
		perror("rmdir");
		return;
	}
}

void unlinkFile(const char*c) 
{
	errno=0;
	if(-1==(unlink(c)))
	{
		perror("unlink");
		return;
	}
	return;
}

int isDir(const char* c) //"bool" function to check if it's a directory
{

	struct stat buf;
	errno=0;
	if(-1==(stat(c,&buf)))
	{
		perror("stat");
		return -1; //if error, return -1 to indicate
	}
	//otherwise, return true/1 or false/0
	if(S_ISDIR(buf.st_mode))	{ 	return 1;	}
	else	{ 	return 0;	}
}

bool isDot(char*c)
{
	string dot(c);
	if(dot=="." || dot =="..")
	{
		return true;
	}
	else
	{
		return false;
	}
}
