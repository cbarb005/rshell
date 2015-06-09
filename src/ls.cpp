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
#include <map>
#include <set>
#include <ios>
#include <iomanip>
#include <ctype.h>

using namespace std;

void dirStream(set<char>&, char*);
void formatNormal( vector<char*> &);
void formatLong( vector<char*> &);
void setflags(set<char>&,bool&,bool&,bool&);
void totalSize(vector<char*>&, blkcnt_t&);
int longestName(vector<char*>&,int&);
int flagID(char*,set<char>&);
bool isDot(char*); //specifically for ./ or ../
bool isHiddenFile(char*); //any hidden file
bool isFlag(char*);
void setMap(map<int,string> &month);
bool compare_char(char* lhs, char*rhs);
void fileHandle(set<char> &flagSet, char* file);
bool isDirectory(char*);
bool isExecutable(char*);

int main(int argc, char**argv)
{	
	vector<char*> dirc;	
	set<char> flags;

	vector<char*> file; //for passing in file as parameter

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

		if(-1==(stat(argv[i],&buf))) { perror("stat"); }
		//if it is a directory
		if(buf.st_mode & S_IFDIR)
		{
			dirc.push_back(argv[i]); //captures directory
		}
		if(buf.st_mode & S_IFREG)
		{
			file.push_back(argv[i]);
		}

	}

	if(dirc.size()==0) //if no directory was specifically passed in
	{
		char cwd[]=".";
		dirc.push_back(cwd); //assume current working directory
	}

	if(file.size() > 0)
	{
		for(unsigned i=0; i<file.size();++i)
		{
			fileHandle(flags,file.at(i));
		}
		
	}
	else if(dirc.size() > 0)
	{
		for(unsigned i=0; i<dirc.size();++i) 
		{	
			dirStream(flags,dirc.at(i));
		}
	}
	
	return 0;
}

void fileHandle(set<char> &flagSet, char* file)
{
	bool a,l,R;
	setflags(flagSet,a,l,R);

	vector<char*> outputFile; //will hold single file
	outputFile.push_back(file);

	if(l)
	{
		formatLong(outputFile);
	}
	else
	{
		formatNormal(outputFile);
	}
	return;
}

void dirStream(set<char> &flagSet,char* dirc)
{
	
	DIR *dirptr;
	struct dirent *info;
	
	bool a,l,R;

	setflags(flagSet,a,l,R); //"activates" bools based on if flag is in set

	vector<char*> output;
	vector<char*> recursiVect;

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

		string path(dirc);
		string temp(info->d_name); //for recursive function

		output.push_back(info->d_name);	
		
		if(R && info->d_type==DT_DIR && !isDot(info->d_name))
		{	
			string newPath=path+"/"+temp;

			//adds directory to queue of directories to "recurse" through

			char* v= (const_cast<char*>(newPath.c_str()));
			//cerr << "new path: " << newPath << endl;
			//cerr << v <<  endl;
			recursiVect.push_back(v);
		}
	}

	//sort(output.begin(),output.end(),compare_char);

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
	for(unsigned i=0;i<recursiVect.size();++i)
	{
		cout << "vect: " << recursiVect.at(i) << endl;
		dirStream(flagSet,recursiVect.at(i));
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

		map<int,string> month;
		setMap(month);

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
		cout << setw(6) << setfill(' ') << buf.st_size << " ";
		
		//time
		//time_t mtime=buf.st_mtime;
		struct tm timep;

		//time(&mtime);
		if(NULL==(localtime_r(&buf.st_mtime,&timep)));
		cout << setw(3) << month[timep.tm_mon] << " ";
		cout << setw(3) << setfill(' ') << timep.tm_mday << " ";
		cout << setw(2) << setfill('0') << timep.tm_hour << ":" << setw(2) << setfill('0') << timep.tm_min << " ";
		//filename
		cout << v.at(i);
		cout << endl;
		cout << flush;

	}	
		
	cout << endl;
	return;
}

void formatNormal(vector<char*> &v)
{
	int sum = 0;
	int display=100;
	int max = longestName(v,sum);
	//string blue = "\e[34m";
	//string green = "\e[32m";
	//string grey = "\e[100m";
	//string clear = "\x1b[0m";
	
	
	for(unsigned i=0; i<v.size();++i)
	{
		display-=max;
		if(display < max)
		{
			cout << endl;
			display=100;
		}
		
		cout << left << setw(max+2) << v.at(i);
	}

	cout << endl;

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

bool isDirectory(char *c)
{
	struct stat buf;

	if(-1==stat(c,&buf)) { perror("stat"); }
	if(buf.st_mode & S_IFDIR)
	{
		return true;
	}
	else
	{
		return false;
	}
}
bool isExecutable(char *c)
{
	struct stat buf;

	if(-1==stat(c,&buf)) { perror("stat"); }
	if(buf.st_mode & S_IXUSR)
	{
		return true;
	}
	else
	{
		return false;
	}
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

void setMap(map<int,string> &month)
{
	month[1] = "Jan";
	month[2] = "Feb";
	month[3] = "Mar";
	month[4] = "Apr";
	month[5] = "May";
	month[6] = "Jun";
	month[7] = "Jul";
	month[8] = "Aug";
	month[9] = "Sep";
	month[10] = "Oct";
	month[11] = "Nov";
	month[12] = "Dec";

	return;

}

bool compare_char(char* lhs, char*rhs)
{
	return (tolower(lhs[0]) < tolower(rhs[0]));
}
