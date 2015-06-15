#include <iostream>
#include <vector>
#include <string>
#include <stack>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <boost/tokenizer.hpp>

using namespace std;
using namespace boost;

void prompt();
int cmdSyntaxCheck(string &s);
int syntaxCheck(vector<string>&vect);
bool isCD(string &s);
bool isConnector(string&);
bool isSymbol(string&);
bool isSemiColon(string &s);
bool isFG(string &s);
bool isBG(string &s);
bool validSymbol(string&);
void cdExecutor(string &s);
bool cdCheck(string &s);
void executor(vector<string> &vect);
void pipeExecutor(vector<string>& );
void commandParser(vector<string> &v, string str);
void ioType(string &s, bool&, bool&, bool&);
bool hasPipe(vector<string> &v);
int pipeCounter(vector<string> &v);
char* stringConverter(string &s);
void removeSymbol(vector<string> &v, string str,string &arg);
void handleSig(int signum);
void handleStop(int signum);
void handleChild(int signum);

stack<int> stopped; //stores pid's of processes that have been stopped
bool success=false;
bool bgCalled=false;
bool fgCalled=false;
int cid=0;
int currid;
typedef tokenizer<char_separator<char> > toknizer;
int main()
{
	struct sigaction sig_act;
	struct sigaction sig_stp;
	struct sigaction sig_chld;

	sig_act.sa_handler=handleSig;
	sig_stp.sa_handler=handleStop;
	sig_chld.sa_handler=handleChild;

	sig_act.sa_flags=SA_RESTART;
	sig_stp.sa_flags=SA_RESTART;
	if(-1==sigaction(SIGINT,&sig_act,NULL)) { perror("sigaction");}
	if(-1==sigaction(SIGTSTP, &sig_stp,NULL)) { perror("sigaction");}
	if(-1==sigaction(SIGCHLD, &sig_chld,NULL)) { perror("sigaction");}


	while(1)
	{
		//prints simple prompt, gets user input
		prompt();
		cin.clear();
		string userinput;
		getline(cin,userinput);
		
		//declares tokenizer and storage for tokens
		string cmd=""; //gathers entire command until connector or end
		string sym=""; //will hold collected symbol ('||' vs '|') 
		vector<string> cmdvect; 

		char_separator<char> delim("","<&|#;>");
		toknizer parser(userinput,delim);

		for(toknizer::iterator it=parser.begin();it!=parser.end();++it)
		{
			if(*it=="#") { break; } //finish reading input if comment
			if(*it=="&" || *it=="|" || *it==";" ) 
			{
				if(!cmd.empty()) //if there is currently a command recorded, push it onto vector, start reading in symbol
				{
					cmdvect.push_back(cmd);
					cmd.clear();
					sym+=(*it);
				}
				else if(cmd.empty() ||  !sym.empty()) 
				{
					sym+=(*it);
				}
			}
			//if none of cases above apply, can be assumed to be command
			else
			{ 
				if(!sym.empty())
				{
					cmdvect.push_back(sym);
					cmd+=(*it);
					sym.clear();
					//"empties" connector and resumes building command
				}
				else //otherwise, just keep building command
				{
					cmd+=(*it);
					string temp=*it;
					if(!isSymbol(temp)) //to avoid spaces in ">>"
					{ cmd+=" "; }//spaces out commands like "ls -a"
				}
			}

		} //end tokenizer loop

		if(!cmd.empty())
		{
			cmdvect.push_back(cmd); //adds last of input
		}
		if(!sym.empty())
		{
			cmdvect.push_back(sym);//and trailing connectors
		}
		int x=syntaxCheck(cmdvect);
		if(x==0)
		{
			if(hasPipe(cmdvect)) { pipeExecutor(cmdvect); }
			else { executor(cmdvect); } 
		}
	}
	//end of while loop

	return 0;
} //end of main

void executor(vector<string> &vect)
{
	
	for(unsigned i=0;i<vect.size();++i)
	{
		if(isConnector(vect.at(i)))
		{
			if(vect.at(i)=="&&")
			{ 
				if(success==false) { return; }
				else {  continue; }
			}
			if(vect.at(i)=="||")
			{
				if(success==false)	{ continue; }
				else if(success==true )	{ return; }
			}
			if(vect.at(i)==";")	{ continue;	}
		}
		else if(isCD(vect.at(i))) 
		{
			if(cdCheck(vect.at(i)))
			{ cdExecutor(vect.at(i));
			continue; }
		}
		else if(isFG(vect.at(i)))
		{
			if(stopped.size()>0)
			{
				fgCalled=true;
				if(-1==(kill(stopped.top(),SIGCONT)))
				{ perror("kill"); }
				currid=stopped.top();
				stopped.pop();
			}
			else { cerr << "Error: no process to foreground.\n"; }
			continue;
		}
		else if(isBG(vect.at(i)))
		{
			if(stopped.size()>0)
			{	
				bgCalled=true;
				if(-1==(kill(stopped.top(),SIGCONT)))
				{ perror("kill"); }
				stopped.pop();
			}
			else
			{
				cerr << "Error: no process to background.\n";
			}
			continue;
		}

		//otherwise can be assumed to be a command
		bool in=false;
		bool out=false;
		bool app=false;
		ioType(vect.at(i),in,app,out);
		
		//parse vect.at(i) into smaller vector
		vector<string>argvect;
		string argStr="";
		if(in || out || app) { removeSymbol(argvect,vect.at(i),argStr); }
		else { commandParser(argvect,vect.at(i)); }

		//store vector size for array allocation
		const size_t sz=argvect.size();
		char**argv=new char*[sz+1]; 
		for(unsigned j=0;j<sz+1;++j)
		{
			if(j<sz)//using strdup since it dynamically allocates on its own
			{ 
				argv[j]=strdup(argvect.at(j).c_str()); 
			}
			else if(j==sz)	{ argv[j]=NULL; }//adds null at end
		}

		int fdIO=0;
		if(in)
		{ 
			fdIO = open(argStr.c_str(), O_RDONLY);
			if (fdIO==-1) { perror("open");}
		}
		else if (out)
		{ 
			fdIO=open(argStr.c_str(),O_WRONLY | O_TRUNC | O_CREAT,S_IRUSR | S_IWUSR);
			if(-1==fdIO) { perror("open"); }
		}
		else if (app)
		{
			fdIO=open(argStr.c_str(),O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
			if(-1==fdIO) { perror("open"); }
		}

		//fork and attempt to execute using execvp
		pid_t pid=fork();
		int status;
		if(pid==-1) { perror("fork"); }  //error with fork
		else if(pid==0) //child
		{

			if(in)
			{
				if(-1==(dup2(fdIO,0))) { perror("dup2"); }
				if(-1==(close(fdIO))) { perror("close");}
			}
			if(out || app)
			{
				if(-1==(dup2(fdIO,1))) { perror("dup2"); }
				if(-1==(close(fdIO))) { perror("close");}
			}
			
			if(execvp(argv[0],argv)==-1) {  perror("execvp");  }
			//if it fails, set it back to false
			_exit(1);
		}
		else //parent
		{	
			cid=pid;
			
			int w=waitpid(pid,&status,WUNTRACED);
			if(w==-1)
			{
				perror("wait");
			}
			if (WIFEXITED(status) > 0)
			{
				if (WEXITSTATUS(status) == 0) {  success=true;}
				else	{ success=false; }

			}				

			if(in) {  if(-1==close(fdIO)) { perror("close"); } }
			if(out || app) {  if(-1==close(fdIO)) {perror("close"); } }


		}
		
		//deallocates argv as well as strdup's dynamic memory
		for(unsigned i=0;i<sz+1;++i) {	free(argv[i]);	}
		delete [] argv;
	}
	return;
}
//not an elegant solution, but could be merged with executor down the line
void pipeExecutor(vector<string> &vect)
{
	//before loop, count pipe
	int pipeCnt=pipeCounter(vect);
	
	int*fds=new int[pipeCnt]; //dynamically allocated to avoid -pedantic error for variable array size
	int cmdCntr=1;
	int cmdCnt=pipeCnt+1;
	//open pipes
	for(int i=0;i<pipeCnt;++i)
	{
		if((pipe(fds+i*2))==-1)	{ perror("pipe");}
	}

	for(unsigned i=0;i<vect.size();++i)
	{
		if(isConnector(vect.at(i)))
		{
			if(vect.at(i)=="|") { continue; }
		}

		//otherwise can be assumed to be a command
		bool in=false;
		bool out=false;
		bool app=false;
		ioType(vect.at(i),in,app,out);
		
		//parse vect.at(i) into smaller vector
		vector<string>argvect;
		string argStr="";
		if(in || out || app) { removeSymbol(argvect,vect.at(i),argStr); }
		else { commandParser(argvect,vect.at(i)); }

		//store vector size for array allocation
		const size_t sz=argvect.size();
		char**argv=new char*[sz+1]; 
		
		for(unsigned j=0;j<sz+1;++j)
		{
			if(j<sz)//using strdup since it dynamically allocates on its own
			{ 
				argv[j]=strdup(argvect.at(j).c_str()); 
			}
			else if(j==sz)	{ argv[j]=NULL; }//adds null at end
		}
		
		//IO redirection
		int fdIO=0;
		if(in)
		{ 
			fdIO = open(argStr.c_str(), O_RDONLY);
			if(fdIO==-1) { perror("open"); }
		}
		else if (out)
		{
			fdIO=open(argStr.c_str(),O_WRONLY | O_TRUNC | O_CREAT,S_IRUSR | S_IWUSR);
			if(fdIO==-1) { perror("open"); }
		}
		else if (app)
		{
			fdIO=open(argStr.c_str(),O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
			if(-1==fdIO) { perror("open"); }
		}
		
		//fork and attempt to execute using execvp
		pid_t pid=fork();
		if(pid==-1) { perror("fork"); }  //error with fork
		else if(pid==0) //child
		{
			if(in)
			{
				if ((dup2(fdIO,0))==-1) { perror("dup2"); } 
				if(((close(fdIO))==-1)) { perror("close");} 
			}
			if(out || app)
			{
				if ((dup2(fdIO,1))==-1) { perror("dup2"); }
				if((close(fdIO))==-1) { perror("close");}
			}
			if(cmdCntr>1) //if there is input to be redirected
			{
				if ((dup2(fds[cmdCntr-2],0))==-1) { perror("dup2"); }
				if ((close(fds[cmdCntr-2]))==-1) { perror("close"); }
			}

			if(cmdCntr!=cmdCnt) //if there is output to be redirected
			{ 
				if ((dup2(fds[cmdCntr],1))==-1) { perror("dup2"); }
				if ((close(fds[cmdCntr]))==-1) { perror("close"); }
			}
			if(execvp(argv[0],argv)==-1) {	perror("execvp");	}
			_exit(0);
		}
		else //parent
		{	
			++cmdCntr;

			if(in) {  if ((close(fdIO))==-1) { perror("close");} }
			if(out || app) {  if ((close(fdIO))==-1) { perror("close");} }
			
			//for(int j=0; j<pipeCnt*2; ++j) { close(fds[j]); }

		}

		//deallocates argv as well as strdup's dynamic memory
		for(unsigned i=0;i<sz+1;++i) {	free(argv[i]);	}
		delete [] argv;
	}
	//once done with loop
	for(int j=0;j<pipeCnt*2;++j)
	{
		if(-1==(close(fds[j])))
		{ perror("close"); }
	}
	for(int j=0;j<=pipeCnt;++j)
	{
		if(-1==(wait(0)))
		{ perror("wait"); }
	}
	
	delete [] fds;
	return;
}


int pipeCounter(vector<string> &v)
{
	int cnt=0;
	for(unsigned i=0;i<v.size();++i)
	{
		if(v.at(i)=="|") { ++cnt; }
	}
	return cnt;
}

void ioType(string &s, bool& in, bool& append, bool& out)
{
	vector<string> v;
	commandParser(v,s);
	for(unsigned i=0;i<v.size();++i)
	{
		if(v.at(i)=="<") { in=true; } 
		else if (v.at(i)==">") { out=true; }
		else if (v.at(i)==">>") { append=true; }
	}
	return;
}
void cdExecutor(string &s)
{
	vector<string> v;
	commandParser(v,s);
	char* home=getenv("HOME");
	char* past=getenv("OLDPWD");
	char* curr=getenv("PWD");

	if(v.size()==1)     //if command is just "cd", go to home dir
	{
		if(-1==(chdir(home)))
		{ perror("chdir"); } 
		setenv("PWD",home,1); 
		setenv("OLDPWD",curr,1);
		return;
	}
	else if (v.at(1)==".") { return; } //do nothing, already in needed dir 
	else if (v.at(1)=="/") 
	{
		const char*absPath="/";
		if(-1==(chdir(absPath)))
		{ perror("chdir"); }
		setenv("PWD",absPath,1);
		setenv("OLDPWD",curr,1);	

	}
	else if(v.at(1)=="-")  //switch current and past directory
	{
		if(-1==(chdir(past)))
		{ perror("chdir"); }
		setenv("PWD",past,1); 
		setenv("OLDPWD",curr,1);
	}
	else	
	{	
		setenv("OLDPWD",curr,1); //set past to current
		char* temp=curr; //temp string for strcat
		char* request=stringConverter(v.at(1));   //requested dir
		if (0!=strncmp(request,"/",1)) { strcat(temp,"/"); }
		strcat(temp,request); 
		errno=0;
		chdir(temp); 
		if(errno==2)
		{
			perror("No such directory exists.");
			setenv("OLDPWD",past,1); //revert back to past, reversing change
			errno=0;
			return;
		}
		else if (errno==0) { setenv("PWD",temp,1); } //continues change
		else { perror("chdir"); }
	}
	return;
}

bool cdCheck(string &s)
{
	vector<string> v;
	commandParser(v,s); 
	if(v.size()>2)
	{ 
		cerr << "Error: too many arguments passed in.\n";
		return false;
	}
	else { return true;}
}


void commandParser(vector<string> &v, string str)
{
	char_separator<char> delim(" ");
	toknizer parser(str,delim);
	for(toknizer::iterator it=parser.begin();it!=parser.end();++it)
	{
		if(*it=="exit" && it==parser.begin())
		{
			exit(0);
		}
		else { v.push_back(*it); }
	}
	return;
}
char* stringConverter(string &s)
{
	char* str=new char[s.size()+1];
	strcpy(str,s.c_str());
	return str;
}
void removeSymbol(vector<string> &v, string str,string &arg)
{
	//in order to pass into exec, parses symbols out, and get IO file
	bool symFound=false;
	char_separator<char> delim(" ","<>");
	toknizer parser(str,delim);
	for(toknizer::iterator it=parser.begin();it!=parser.end();++it)
	{
		if(*it=="exit") { exit(0); }
		if(*it=="<" || *it==">") { symFound=true; }
		else if(!symFound) { v.push_back(*it); }
		else if(symFound) { arg=(*it); } //input/output file
	}
	return;
}

int syntaxCheck(vector<string> &vect)
{
	//checks input for invalid syntax in connectors
	for(unsigned i=0; i<vect.size();++i)
	{
		//after initial parsing, vector passed in should always have
		//commands in even indices, and any symbols in odd indices.
		if(i%2==0) //if even index...
		{
			if(isConnector(vect.at(i))) //...and is a symbol...
			{
				cerr << "Syntax error: missing argument.\n"; 
				return -1;  //...argument must be missing
			}
			else if(isSymbol(vect.at(i))) //if io redirection symbol present
			{
				if(-1==(cmdSyntaxCheck(vect.at(i)))) //check it too
				{ return -1; }
				else { continue;}
			}
			else { continue; } //if "vanilla" command, just carry on
		}
		else if(i%2!=0 ) //if odd index 
		{
			
			if(!validSymbol(vect.at(i))) //and symbol is not valid
			{
				cerr << "Syntax error: invalid operator.\n";
				return -1; //will return an error
			}
			else if(isSymbol(vect.at(i)) && i+1==vect.size())
			{
				//if symbol but last one, argument is missing
				cerr << "Syntax error: missing argument.\n"; 
				return -1;
			}
			else //if valid, just continue checking
			{
				continue;
			}
		}
	}
	return 0; //no syntax errors found
}

int cmdSyntaxCheck(string &s)
{
	vector<string> strv;
	commandParser(strv, s);
	for(unsigned i=0;i<strv.size();++i)
	{
		if(i==0) //similar to SyntaxCheck, but smaller scale
		{
			if(isSymbol(strv.at(i)))
			{ cerr << "Error: missing argument.\n"; return -1; }
		}
		else if(i+1==strv.size())
		{
			if(isSymbol(strv.at(i)) && i+i==strv.size())
			{ cerr << "Error: missing argument.\n"; return -1;}
		}

	}

	return 0;
}
bool hasPipe(vector<string> &v)
{
	for(unsigned i=0;i<v.size();++i)
	{	
		if(v.at(i)=="|") { return true; }
	}
	return false;
}

bool isSemiColon(string &s) //literally only to allow ";" as a valid command ending.
{
	size_t sc=s.find(";");
	if(sc==string::npos) { return false;}
	return true;
}
	
bool isConnector(string &s)
{
	//function just checks if token is supposed to be a connector
	size_t i=s.find("&");
	size_t j=s.find("|");
	size_t k=s.find(";");

	if(i==string::npos && j==string::npos && k==string::npos)
	{
		return false;
	}
	return true;
}

bool isSymbol(string &s)
{
	size_t a=s.find(">");
	size_t b=s.find("<");
	size_t noSym = string::npos; //"no such symbol"
	//if no symbols are found in the string
	if(a==noSym && b==noSym) 
	{
		return false; //assume it's not an attempt at redirection symbol
	}
	return true;
}

bool isCD(string &s)
{
	size_t pos=s.find("cd");
	if(pos !=string::npos && pos==0)
	{
		return true;
	}
	return false;
}

bool isFG(string &s)
{
	size_t pos=s.find("fg");
	if(pos !=string::npos)
	{
		return true;
	}
	return false;
}

bool isBG(string &s)
{
	size_t pos=s.find("bg");
	if(pos !=string::npos)
	{
		return true;
	}
	return false;
}

bool validSymbol(string &str)
{
	if(str=="&&") { return true; }
	else if(str=="||") { return true;}
	else if(str==";") { return true;}
	else if(str=="|") { return true;}
	else if(str=="<") { return true;}
	else if(str==">") { return true;}
	else if(str==">>") {return true;}
	//else if(str=="<<<") {return true;} //if time allows
	return false;
}
void prompt()
{
	string t="~";
	char buffer[PATH_MAX];
	char* cwd=getcwd(buffer, PATH_MAX);
	if(cwd==NULL) { perror("getcwd"); }
	char* home=getenv("HOME");
	string cwdstr(cwd);
	string homestr(home);
	size_t pos = cwdstr.find(homestr);
	if(pos != string::npos)
	{
		int sz=homestr.size();
		cwdstr.replace(0,sz,t);
	}
	
	struct passwd *pw;
	if(NULL==(pw=getpwuid(getuid())))
	{ perror("getpwuid"); }
	char *user=pw->pw_name;
	char host[128];
	if(-1==(gethostname(host,128)))
	{
		perror("gethostname");
	}

	cout << user << "@";
	cout << host << ": ";;
	cout << cwdstr;	

	cout << " $ ";
	return;
}
void handleSig(int signum) 
{
	if(cid==0) //if a child
	{
		if(-1==(kill(getpid(),SIGINT)))
		{ perror("kill"); }
	}
	else
	{
		return;
	}
}

void handleStop(int signum)
{ 
	if(cid==0)
	{
		if(-1==(kill(getpid(),SIGSTOP)))
		{perror("kill"); }
	}
	else
	{
		stopped.push(cid);
		cerr << "\nProcess [" << cid << "] stopped.\n";
		return;
	}
}

void handleChild(int signum)
{
	while(((waitpid(-1,NULL, WNOHANG)))>0)
	{
		;
	}
	return;

}
