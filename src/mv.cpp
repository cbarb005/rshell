#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>

using namespace std;

bool isdir(char* c) {
	struct stat buff;
	if (stat(c, &buff) == -1) {
		perror("stat");
		exit(EXIT_FAILURE);
	}
	if (buff.st_mode & S_IFDIR)
		return true;
	else
		return false;
}

void moveit(char*file1, const char* file2) {
	errno = 0;
	if (link(file1,file2) == -1) {
		perror("link");
		exit(EXIT_FAILURE);
	}
	if (unlink(file1) == -1) {
		perror("unlinK");
		exit(EXIT_FAILURE);
	}
}

bool exists(char* file) {
	struct stat buff;
	if (stat(file, &buff) == -1) {
		return false;
	}
	return true;
}

int main(int agrc, char** argv) {
	if(!exists(argv[1])) {
		perror("stat");
		return -1;
	}

	if (exists(argv[2])) {
		if (isdir(argv[2])) {
			string s1 = argv[1];
			string s2 = argv[2];
			string s  = s2 + "/" + s1;	
			moveit(argv[1], s.c_str());
		}
		else {
			cout << argv[2]	<< "already exists." << endl;
		}
	}
	//rename file1
	else 
		moveit(argv[1], argv[2]);	

	return 0;
}
