#R'shell

This is a very, very basic unix-shell, like bash, but less functional for now.

##Installation

'git clone https://github.com/cbarb005/rshell.git
cd rshell
git checkout hw0
make
bin/rshell`


##Bugs

When incorrectly entering an argument such as `|| pwd`, the program will ignore the fact there is no argument preceding the connector and will execute the rest. This is not the case for `pwd ||`, which would produce an error message.


