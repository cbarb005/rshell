#rshell

This is a very, very basic unix-shell, like bash, but less functional for now.
It can almost execute everything bash does, as long as it is already a ```/bin/``` executable, along with  built-in ```exit``` and ```cd``` commands. 

Compound commands are supported, but only with the ```&&```,```||```, and ```;``` connectors.
It is worth noting that when stringing commands together, there is no inherent "order of operations"; commands connected by ```&&``` only rely on whether the previous argument succeeded or not, while ```||``` relies on its failure. 

Input/output redirection and piping are supported to a limited extent as outlined in the Bugs & Other Documentation section below. 

```cd``` only takes a single argument, as in ```cd fakedir```; anything more than this will give an error as to the number of arguments. 



##Installation

```
git clone https://github.com/cbarb005/rshell.git
cd rshell
git checkout hw0
make
bin/rshell
```

##Bugs & Other  Documentation

Sometimes, a phantom empty line appears after executing a command. So far, this only seems to happen with compound commands, and not consistently.

If not a valid command , such as ```lss || echo hello```, `perror` will return a message, but will NOT echo. This only occurs with this specific connector.

A statement like ```pwd && && && && && && ls``` will no longer run. Commands may no longer start or end with a connector. 

Currently, input/output redirection only supports a single operation per command, unless a connector is used.
Occasionally, when running ```wc < somefile.txt```, the space at the very beginning of the returned line is missing. 

Piping and connectors are not simultaneously handled, so a command such as ```ls | ls && echo bye``` must be instead given as ```ls | ls``` and ```echo bye```.
Multiple pipes as of this moment are not supported, and is a major bug in this program. 

```cd``` has a bug in which after a nonexistent directory passed in. 
The program returns an error message as expected, but afterwards, attempts such as ```cd ..``` and  ```cd dir``` also return the same error, until the command ```cd -``` is used, thus restoring all the previous commands.

After stopping a program with ^Z and resuming it with either `fg` or `bg`, or resuming and then ^C, it does not automatically return to the prompt; you must press enter.
Sometimes, `fg` and `bg` break and the function hangs.
This does not happen consistently.
On its own, ^C works fine, but as mentioned above, may get stuck when used with `fg` or `bg`.

For `ls`, any combination of `-l` and `-R` does not work. 
They work fine on their own and each combined with `-a`, but `-R` doesn't play well with others. 
Errors for `stat` may appear even if everything is output correctly.

Occasionally, the colors on ls may mess up output.
For this reason, colored output for ls is available by using the flag "-cc" to enable colors for a single execution of `ls`.

