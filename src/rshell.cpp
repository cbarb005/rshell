#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <boost/tokenizer.hpp>

using namespace std;
using namespace boost;

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
			if(*it=="exit")	{ exit(0);} //exits program
			if(*it=="#") { break; } //finish reading input if comment
			if(*it=="&")
			{
				cnntr+=(*it);
				++it; //goes to next to see if valid &&
				if(*it=="&")
				{
					cmdvect.push_back(cmd);
					cnntr+=(*it);
					cmdvect.push_back(cnntr);
				}
				else { cout << "Invalid connector\n"; }
				//empty out cnntr for next time
				cnntr.clear();
				cmd.clear();
			}
			if(*it=="|")
			{
				cnntr+=(*it);
				++it;
				if(*it=="|")
				{
					cmdvect.push_back(cmd);
					cnntr+=(*it);
					cmdvect.push_back(cnntr);
				}
				else
				{
					cmdvect.push_back(cmd);
					cmdvect.push_back(cnntr);
				}
				cnntr.clear();
				cmd.clear();
			}
			if(*it==";")
			{
				//just push onto vector, two ;; = do nothing for now 
			}


			cmd+=(*it);


		}
		
		for(unsigned i=0;i<cmdvect.size();++i)
		{
			cout << cmdvect.at(i) << endl;
		}
		//if exit command is found, exit shell
		//fork based on commands and connectors, then use execvp
		//once child processes succeed or fail, return to parent
		//print out newline and clear buffer?

	}

	return 0;
}


