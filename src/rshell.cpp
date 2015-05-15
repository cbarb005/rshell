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
bool isSymbol(string&);
bool isSemiColon(string &s);
bool validSymbol(string&);
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
		string sym=""; //will hold collected symbol ('||' vs '|') 
		vector<string> cmdvect; 

		char_separator<char> delim("","<&|#;>");
		toknizer parser(userinput,delim);

		for(toknizer::iterator it=parser.begin();it!=parser.end();++it)
		{
			if(*it=="#") { break; } //finish reading input if comment
			if(*it=="&" || *it=="|" || *it==";" ) //doesn't check validity/meaning of symbol yet
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
		for(unsigned n=0;n<cmdvect.size();++n)
		{
			cout << n << ": " << cmdvect.at(n) << endl;
		}
		int x=syntaxCheck(cmdvect);
		if(x==0)
		{	
			cout << "All valid!\n";
			//executor(cmdvect);
		}
	}
	//end of while loop


	return 0;
} //end of main

void executor(vector<string> &vect)
{
	bool success=false;

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
				if(success==false || i==0 )
				{
					return;
				}
				else	{  continue;	}
			}
			if(vect.at(i)=="||")
			{
				if(success==false && i!=0)
				{	
					continue;
				}
				else if(success==true || i==0  )	{ return; }
			}
			if(vect.at(i)==";" && i!=0)
			{
				continue;
			}
		}
		else
		{
			
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
		//after initial parsing, vector passed in should always have
		//commands in even indices, and any symbols in odd indices.
		if(i%2==0) //if even index...
		{
			if(isSymbol(vect.at(i))) //...and is a symbol...
			{
				cerr << "Syntax error: missing argument.\n"; 
				return -1;  //...argument must be missing
			}
			else { continue; } //if not a symbol, assume to be intended command
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
		return false; //assume it's not an attempt at connector/redirection symbol
	}
	return true;
}

bool validSymbol(string &str)
{
	//only supports few connectors now
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
