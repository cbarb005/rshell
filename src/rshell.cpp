#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <boost/tokenizer.hpp>

using namespace std;
using namespace boost;

int cmdSyntaxCheck(string &s);
int syntaxCheck(vector<string>&vect);
bool isCD(string &s);
bool isConnector(string&);
bool isSymbol(string&);
bool isSemiColon(string &s);
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

//sigHandler()
typedef tokenizer<char_separator<char> > toknizer;
int main()
{
	//sigaction();
	while(1)
	{
		//prints simple prompt, gets user input
		cout << "$ ";
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
	bool success=true;

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

		int fdIO;
		if(in) { fdIO = open(argStr.c_str(), O_RDONLY);}
		else if (out) { fdIO=open(argStr.c_str(),O_WRONLY | O_TRUNC | O_CREAT,S_IRUSR | S_IWUSR); }
		else if (app) { fdIO=open(argStr.c_str(),O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);}
		if(-1==fdIO) { perror("open"); };

		//fork and attempt to execute using execvp
		pid_t pid=fork();
		if(pid==-1) { perror("fork"); }  //error with fork
		else if(pid==0) //child
		{
			if(in) { dup2(fdIO,0); if(-1==(close(fdIO))) { perror("close");}  }
			if(out || app) { dup2(fdIO,1);  if(-1==(close(fdIO))) { perror("close");} }
			success = true; //"temporary" based on execvp's success or failure
			if(execvp(argv[0],argv)==-1) {	perror("execvp");	}
			success = false;  //if it fails, set it back to false
			_exit(0);
		}
		else //parent
		{	
			if(wait(0)==-1) { perror("wait"); }
			if(in) {  close(fdIO); }
			if(out || app) {  close(fdIO); }
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
		int fdIO;
		if(in) { fdIO = open(argStr.c_str(), O_RDONLY);}
		else if (out) { fdIO=open(argStr.c_str(),O_WRONLY | O_TRUNC | O_CREAT,S_IRUSR | S_IWUSR); }
		else if (app) { fdIO=open(argStr.c_str(),O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);}
		if(-1==fdIO) { perror("open"); };
		
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
	for(int j=0;j<pipeCnt*2;++j) { close(fds[j]); }
	for(int j=0;j<=pipeCnt;++j)
	{
		wait(0);
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
	char* request=stringConverter(v.at(1));
	//char* past=getenv("OLDPWD");
	//char* curr=getenv("PWD");
//	if(request=="-") { char* temp=curr; curr=past; past=temp; }
	cout << "*" << request << "*";
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
		if(*it=="exit")
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
