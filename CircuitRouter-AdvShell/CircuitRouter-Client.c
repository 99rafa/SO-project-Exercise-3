#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#define TAMCMD 100 /*size of command*/

int main (int argc, char** argv) {
	int fAdv, fClient;
	char* pipename= (char*) malloc(sizeof(char)*100);
	char* text_buffer= (char*) malloc(sizeof(char)*100);
	char ext[6]=".pipe";
	strcpy(pipename,argv[1]); /*argv[1]-pathname*/
    strcat(pipename, ext);
    unlink(pipename);
    if (mkfifo (pipename, 0777) < 0) exit (-1);
	if ((fAdv = open (pipename,O_WRONLY)) < 0) exit (-1);
	while (1) {
		fgets(text_buffer, 100, stdin);
		write(fAdv, text_buffer, TAMCMD);
	}
	unlink(pipename);
	close(fAdv); 
	free(text_buffer);
	free(pipename);
	exit(0);
}