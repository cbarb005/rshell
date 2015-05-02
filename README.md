#rshell

This is a very, very basic unix-shell, like bash, but less functional for now.
It can almost execute everything bash does, as long as it is already a ```/bin/``` executable, along with a built-in ```exit``` command. 

Compound commands are supported, but only with the ```&&```,```||```, and ```;``` connectors.
It is worth noting that when stringing commands together, there is no inherent "order of operations"; commands connected by ```&&``` only rely on whether the previous argument succeeded or not, while ```||``` relies on its failure. 


##Installation
```
git clone https://github.com/cbarb005/rshell.git
cd rshell
git checkout hw0
make
bin/rshell
```

##Bugs
###rshell
When incorrectly entering an argument such as `|| pwd`, the program will ignore the fact there is no argument preceding the connector and will execute the rest. This is not the case for `pwd ||`, which would produce an error message.


Sometimes, a phantom empty line appears after executing a command. So far, this only seems to happen with compound commands, and not consistently.


If not a valid command, `perror` will return a message, but continue, even in a compound statement like `ls && pwdd && echo hello` and echo anyways.
Similarly, a statement like ```pwd && && && && && && ls``` will also run both anyways, despite the missing arguments in between. Technically, doing nothing would easily succeed anyways, but if the expectation is that it would not run, then yes, it is a bug. 


###ls
When using -R, will not open directory, even if it is there.
Output sometimes truncates a word and then continues it on the next line.
Not really a bug, but month is output as number. 

