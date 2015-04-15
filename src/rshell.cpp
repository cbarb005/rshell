#include <iostream>
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
		cout << "$";
		string userinput;
		getline(cin,userinput);
		//parse userinput into tokens
		string cmd="";
		char_separator<char> delim(" ","&|#");
		toknizer parser(userinput,delim);
		for(toknizer::iterator it=parser.begin();it!=parser.end();++it)
		{
			if(*it=="exit")
			{
				exit(0);
			}
			if(*it=="#")
			{
				break;
			}
			
			cmd+=(*it);

			
			

		}

		//group tokens based on connectors into separate commands
		//if exit command is found, exit shell
		//fork based on commands and connectors, then use execvp
		//once child processes succeed or fail, return to parent
		//print out newline and clear buffer?

	}

	return 0;
}
