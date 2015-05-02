#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <string>
#include <algorithm>
#include <vector>
#include <queue>
#include <set>
#include <ios>
#include <iomanip>

using namespace std;

void dirStream(set<char>&,char*);
void formatNormal(vector<char*> &);
void formatLong(vector<char*> &);
void setflags(set<char>&,bool&,bool&,bool&);
void totalSize(vector<char*>&, blkcnt_t&);
int longestName(vector<char*>&,int&);
int flagID(char*,set<char>&);
bool isDot(char*); //specifically for ./ or ../
bool isHiddenFile(char*); //any hidden file
bool isFlag(char*);

int main(int argc, char**argv)
{	
	vector<char*> dirc;	
	set<char> flags;

	for(int i=1; i<argc;++i)
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
			cout << argv[i]; //will just echo file to screen
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
	struct dirent *info;

	bool a,l,R;
	setflags(flagSet,a,l,R); //"activates" bools based on if flag is in set
	queue<char*> dirq;
	vector<char*> output;

	if(R)
	{
		cout << dirc << ":" << endl;
	}
	if((dirptr=opendir(dirc))==NULL)
	{
		perror("opendir");
		exit(1);
	}
	errno=0;
	while(NULL != (info=readdir(dirptr)))
	{	
		if(isHiddenFile(info->d_name) && !a)
		{ continue; } //skip over dot files

		output.push_back(info->d_name);	

		if(R && info->d_type==DT_DIR && !isDot(info->d_name))
		{	
			//adds directory to queue of directories to "recurse" through
			dirq.push(info->d_name);
		}
	}
	if(!l) { formatNormal(output); } //if no -l passed in
	else { formatLong(output); }
	if(errno != 0)
	{
		perror("readdir");
	}
	if(-1 == closedir(dirptr))
	{
		perror("closedir");
	}
	while(!dirq.empty())
	{
		dirc=dirq.front();
		dirq.pop();
		dirStream(flagSet,dirc);
	}
	return;
}
void formatLong(vector<char*> &v)
{
	//get total for output
	blkcnt_t sum=0;
	totalSize(v,sum);
	//total will be based off GNU convention
	cout << "total: " << sum << endl;

	for(unsigned i=0;i<v.size();++i)
	{
		struct stat buf;
		if(-1==(stat(v.at(i),&buf)))
		{
			perror("stat");
		}
		if(buf.st_mode & S_IFDIR) { cout << "d"; }
		else if(buf.st_mode & S_IFREG) {cout << "-"; }
		else if(buf.st_mode & S_IFLNK) { cout << "l";}
		
		
		//permissions
		cout <<
		((S_IRUSR & buf.st_mode) ? "r" : "-") <<
		((S_IWUSR & buf.st_mode) ? "w" : "-") << 
		((S_IXUSR & buf.st_mode) ? "x" : "-") <<
		((S_IRGRP & buf.st_mode) ? "r" : "-") <<
		((S_IWGRP & buf.st_mode) ? "w" : "-") <<
		((S_IXGRP & buf.st_mode) ? "x" : "-") <<
		((S_IROTH & buf.st_mode) ? "r" : "-") <<
		((S_IWOTH & buf.st_mode) ? "w" : "-") <<
		((S_IXOTH & buf.st_mode) ? "x" : "-") << " ";
		
		//number of links
		cout << buf.st_nlink << " ";

		//uid and gid
		struct passwd *pw;
		if(NULL==(pw=getpwuid(buf.st_uid)))
		{ 
			perror("getpwuid");
		}
		cout << pw->pw_name << " " ;

		//get group id
		struct group *grp;
		if(NULL==(grp=getgrgid(buf.st_gid)))
		{
			perror("getgrgid");
		}
		cout << grp->gr_name << " ";

		//size
		cout << setw(6) << buf.st_size << " ";
		
		//time
		//time_t mtime=buf.st_mtime;
		struct tm timep;

		//time(&mtime);
		if(NULL==(localtime_r(&buf.st_mtime,&timep)));
		cout << setw(3) << timep.tm_mon << " "
			 << setw(3) << timep.tm_mday << " ";
		cout << timep.tm_hour << ":" << timep.tm_min << " ";
		//filename
		cout << v.at(i);
		cout << endl;
	}	
		

	return;
}

void formatNormal(vector<char*> &v)
{
	int sum = 0;
	int width = longestName(v,sum);
	if(sum < 55)
	{
		for(unsigned i=0; i<v.size();++i)
		{
			cout <<  v.at(i) << "  " ;
		}
		cout << endl;
	}
	else
	{
		for(unsigned i=0; i<v.size();++i)
		{
			cout << left << setw(width+2) << v.at(i);
		}
		cout << endl;
	}
	return;
}
void totalSize(vector<char*> &v,blkcnt_t& total) //will get total for -l
{
	for(unsigned i=0;i<v.size();++i)
	{
		struct stat buf;
		if(-1==(stat(v.at(i),&buf)))
		{
			perror("stat");
		}
		total+=buf.st_blocks;
	}

	return;
}



//simple sorting function to find column width
int longestName(vector<char*> &v,int& sum) 
{
	int max=0;
	for(unsigned i=0;i<v.size();++i)
	{
		int curr=strlen(v.at(i));
		sum+=curr; //helps track if total length would exceed 55
		if (curr>=max)
		{
			max=curr;
		}
	}
	return max;
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
bool isDot(char* c)
{
	string dir(c);
	if(dir == "." || dir == "..")
	{
		return true;
	}
	else { return false;}
}

bool isHiddenFile(char* c)
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
