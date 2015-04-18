#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <boost/tokenizer.hpp>

using namespace std;
using namespace boost;

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
		//cout << "it: "<< *it << endl;
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
				//empty out cnntr for next time
				//cnntr.clear();
				//cmd.clear();
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

		}
		if(!cmd.empty())
		{
			cmdvect.push_back(cmd); //adds last of input
		}
		if(cnntr==";")
		{
			cmdvect.push_back(cnntr);//and trailing ; connectors
		}

		executor(cmdvect); //pass in parsed command to be executed (or not)
	}
	//end of while loop
	
	return 0;
}

void executor(vector<string> &vect)
{
	//bool success; //used in long compound statements
	for(unsigned i=0; i<vect.size();++i)
	{
		if(isConnector(vect.at(i)) && validConnector(vect.at(i)))
		{
			cout << vect.at(i) << "is connector" << endl;
		}
	}


	return;
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
	if(str=="&&") { return true; }
	else if(str=="||") { return true;}
	else if(str==";") { return true;}
	return false;
}
