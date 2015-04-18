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

		char_separator<char> delim(" ","&|#;");
		toknizer parser(userinput,delim);

		for(toknizer::iterator it=parser.begin();it!=parser.end();++it)
		{
			//if(*it=="exit")	{ exit(0);}
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
			cmdvect.push_back(cnntr);//and trailing ; connectors
		}

		int x=syntaxCheck(cmdvect);
		if(x==0)
		{
			executor(cmdvect); //pass in parsed command to be executed 
		}
	}
	//end of while loop
	
	return 0;
}

void executor(vector<string> &vect)
{
	bool success=false;

	for(unsigned i=0;i<vect.size();++i)
	{
		if(isConnector(vect.at(i)))
		{
			if(vect.at(i)=="&&")
			{ 
				if(success==false)
				{
					cout << "first argument failed\n";
					return;
				}
				else
				{
					continue;
				}
			}
			if(vect.at(i)=="||")
			{
				if(success==false && i!=0)
				{	
					cout << "first argument failed, trying second\n";
					continue;
				}
				else if(success==true)
				{
					return;
				}

			}
		}
		//otherwise, fork and attempt to execute using execvp
		//should it fail, set success to false
		pid_t pid=fork();
		if(pid==-1) //error with fork
		{
			perror("fork");
			exit(1);
		}
		else if(pid==0) //child
		{
			
			cout << "execution here!!!\n";
			_exit(0);
		}
		else //parent
		{	
			if(wait(0)==-1)
			{
				perror("wait");
				exit(1);
			}
			
			//if command executes correctly, success set to true
			//then continue with reading through vect
		}

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
				cout << "Connector syntax error: missing argument\n";
				return -1; 
			}
			if(!validConnector(vect.at(i)))
			{
				cout << "Connector syntax error: invalid connector\n";
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
