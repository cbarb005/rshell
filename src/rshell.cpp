#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <boost/tokenizer.hpp>

using namespace std;
using namespace boost;

int syntaxCheck(vector<string>&vect);
bool isConnector(string&);
bool validConnector(string&);
void executor(vector<string> &vect);
void commandParser(vector<string> &v, string str);

typedef tokenizer<char_separator<char> > toknizer;
int main()
{
	while(1)
	{
		//prints simple prompt, gets user input
		cout << "$ ";
		string userinput;
		getline(cin,userinput);
		
		//declares tokenizer and storage for tokens
		string cmd=""; //gathers entire command until connector or end
		string cnntr=""; //will hold connector
		vector<string> cmdvect;

		char_separator<char> delim("","&|#;");
		toknizer parser(userinput,delim);

		for(toknizer::iterator it=parser.begin();it!=parser.end();++it)
		{
			if(*it=="#") { break; } //finish reading input if comment
			if(*it=="&" || *it=="|" || *it==";")
			{
				if(!cmd.empty())
				{
					cmdvect.push_back(cmd);
					cmd.clear();
					cnntr+=(*it);
				}
				else if(cmd.empty() && !cnntr.empty())
				{
					cnntr+=(*it);
				}
			}
			//if none of cases above apply, can be assumed to be command
			else
			{ 
				if(!cnntr.empty())
				{
					cmdvect.push_back(cnntr);
					cmd+=(*it);
					cnntr.clear();
					//"empties" connector and resumes building command
				}
				else //otherwise, just keep building command
				{
					cmd+=(*it);
					cmd+=" "; //spaces out commands like "ls -a"
				}
			}

		} //end tokenizer loop

		if(!cmd.empty())
		{
			cmdvect.push_back(cmd); //adds last of input
		}
		if(!cnntr.empty())
		{
			cmdvect.push_back(cnntr);//and trailing connectors
		}

		int x=syntaxCheck(cmdvect);
		if(x==0)
		{
			executor(cmdvect);
		}
	}
	//end of while loop
	
	return 0;
} //end of main

void executor(vector<string> &vect)
{
	bool success=false;
	bool preArg=false;

	for(unsigned i=0;i<vect.size();++i)
	{
		if(vect.at(i)=="exit")
		{
			cout << "is exit" << endl;
		}
		//checks if current string is connector
		else if(isConnector(vect.at(i)))
		{
			if(vect.at(i)=="&&")
			{ 
				if(success==false || i==0 || preArg==false)
				{
					return;
				}
				else	{ preArg=false;  continue;	}
			}
			if(vect.at(i)=="||")
			{
				if(success==false && i!=0)
				{	
					continue;
				}
				else if(success==true || i==0 || preArg==false)	{ return; }
			}
			if(vect.at(i)==";" && i!=0)
			{
				continue;
			}
		}
		else
		{
			preArg=true;
		}
		//otherwise can be assumed to be a command
		
		//parse vect.at(i) into smaller vector
		vector<string>argvect;
		commandParser(argvect,vect.at(i));

		//store vector size for array allocation
		const size_t sz=argvect.size();
		char**argv=new char*[sz+1]; //REMEMBER- delete at end
		
		for(unsigned j=0;j<sz+1;++j)
		{
			if(j<sz)//using strdup since it dynamically allocates on its own
			{
				argv[j]=strdup(argvect.at(j).c_str()); 
			}
			else if(j==sz) //adds null at end
			{
				argv[j]=NULL;
			}
		}
		
		//fork and attempt to execute using execvp
		pid_t pid=fork();
		if(pid==-1) //error with fork
		{
			perror("fork");
			exit(1);
		}
		else if(pid==0) //child
		{

			if(execvp(argv[0],argv)==-1)
			{
				success=false;  //redundant maybe?
				perror("execvp");
				exit(1);
			}
			
			_exit(0);
		}
		else //parent
		{	
			if(wait(0)==-1)
			{
				perror("wait");
				exit(1);
			}
			success=true;
		}
		
		//deallocates argv as well as strdup's dynamic memory
		for(unsigned i=0;i<sz+1;++i)
		{
			delete [] argv[i];
		}
		delete [] argv;
	}
	return;
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
		v.push_back(*it);
	}

	return;
}

int syntaxCheck(vector<string> &vect)
{
	//checks input for invalid syntax in connectors
	for(unsigned i=0; i<vect.size();++i)
	{
		if(isConnector(vect.at(i))) 
		{
			//ensures argument for && and || is complete
			if((i==0 || i+1==vect.size()) && vect.at(i)!=";")
			{
				cerr << "Connector syntax error: missing argument\n";
				return -1; 
			}
			if(!validConnector(vect.at(i)))
			{
				cerr << "Connector syntax error: invalid connector\n";
				return -1;
			}
		}
	}
	return 0; //no syntax errors found
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

bool validConnector(string &str)
{
	//only supports few connectors now
	if(str=="&&") { return true; }
	else if(str=="||") { return true;}
	else if(str==";") { return true;}
	return false;
}
