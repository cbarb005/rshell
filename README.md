#rshell

This is a very, very basic unix-shell, like bash, but less functional for now.
It is worth noting that when stringing commands together, there is no inherent "order of operations"; commands connected by `&&` only rely on whether the previous argument succeeded or not, while `||` relies on its failure. 


##Installation

`git clone https://github.com/cbarb005/rshell.git
cd rshell
git checkout hw0
make
bin/rshell`


##Bugs

When incorrectly entering an argument such as `|| pwd`, the program will ignore the fact there is no argument preceding the connector and will execute the rest. This is not the case for `pwd ||`, which would produce an error message.


Sometimes, a phantom empty line appears after executing a command. So far, this only seems to happen with compound commands, and not consistently.


Compound commands ending with `;` do not execute the last command; `ls ; pwd;` only executes ls, before perror returns with an error message from execvp.


