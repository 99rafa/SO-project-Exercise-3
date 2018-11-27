#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <sys/select.h>
#include <errno.h>

#define TAMCMD 100 /*size of command*/
#define STDIN 0

int main (int argc, char** argv) {
	int fAdv, fClient;
	char* pipename= (char*) malloc(sizeof(char)*100);
	char* text_buffer= (char*) malloc(sizeof(char)*100);
	char ext[6]=".pipe";
	strcpy(pipename,argv[1]); /*argv[1]-pathname*/
    strcat(pipename, ext);
    fd_set fds;
    int rv;
    int maxfd;
    unlink("/tmp/CircuitRouter-Client.pipe");
    if (mkfifo ("/tmp/CircuitRouter-Client.pipe", 0777) < 0) {
        perror("Error: Making Fifo");
        exit (1);
    }
    if ((fAdv = open (pipename,O_WRONLY)) < 0) exit (-1);
    if ((fClient = open ("/tmp/CircuitRouter-Client.pipe",O_NONBLOCK)) < 0) exit (-1);
	while (1) {
		FD_ZERO(&fds);
        FD_SET(STDIN, &fds);
        FD_SET(fClient, &fds); 
        maxfd = (fClient > STDIN)?fClient:STDIN; 
        rv=select(maxfd+1, &fds, NULL, NULL, NULL);
		if (rv < 0) {
            if (errno == EINTR) {
                continue;
            }
            else {
                printf("Select failed\r\n");
                break;
            }
        }   

        if (FD_ISSET(STDIN, &fds)) { 
            fgets(text_buffer, 100, stdin);
			write(fAdv, text_buffer, TAMCMD);

        }
        if (FD_ISSET(fClient, &fds)) { 
            read(fClient,text_buffer, 100);
            printf("%s\n", text_buffer);
        }

		
	}
	close(fClient); 
	unlink("/tmp/CircuitRouter-Client.pipe");
	free(text_buffer);
	free(pipename);
	exit(0);
}