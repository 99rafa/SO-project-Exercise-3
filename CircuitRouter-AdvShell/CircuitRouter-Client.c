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
	char* server_pipename= (char*) malloc(sizeof(char)*100);
	char* text_buffer= (char*) malloc(sizeof(char)*100);
	// Every client will have their own pipe because all the clients have different pids
	pid_t pid = getpid();  
    char string_pid[6];
    sprintf(string_pid, "%d", pid);
	char ext[6]=".pipe", client_pipename[50] = "/tmp/CircuitRouter-Client";
	strcat(client_pipename, string_pid);
	strcat(client_pipename, ext);
	strcpy(server_pipename,argv[1]); /*argv[1]-pathname*/
    strcat(server_pipename, ext);
    fd_set fds;
    int rv;        
    int maxfd;     /*There is no need to unlink the pipe given that all the client pipes have different names */
    if (mkfifo (client_pipename, 0777) == -1) {
        perror("Error: Making Fifo");
        exit (1);
    }
    if ((fAdv = open (server_pipename,O_WRONLY)) == -1) {
    	perror("Unexpected error while opening named pipe");
        exit (-1); 
    }
    if ((fClient = open (client_pipename,O_RDONLY |O_NONBLOCK )) == -1) {
    	perror("Unexpected error while opening named pipe");
        exit (-1); 
    }
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
           	if (write(fAdv, client_pipename, 50) == -1) {
     			perror("Unexpected error while writing in named pipe");
           		exit(-1);
           }
			if(write(fAdv, text_buffer, TAMCMD)==-1) {
				perror("Unexpected error while writing in named pipe");
           		exit(-1);
			}

        }
        if (FD_ISSET(fClient, &fds)) { 
        	int rd = read(fClient,text_buffer, 100);
            if(rd > 0) {
            	printf("%s\n", text_buffer);
           }
           else if( rd == -1) {
           	if (errno !=EWOULDBLOCK) {
	           	perror("Unexpected error while reading from named pipe");
	           	exit(-1);
           }
           }
        }

		
	}
	if(close(fClient) == -1) {
        perror("Unexpected error while closing named pipe");
        exit(-1);
    }
	if( unlink(client_pipename)==-1) {
		 perror("Unexpected error while unlinking named pipe");
        exit(EXIT_FAILURE);
	}
	free(text_buffer);
	free(server_pipename);
	exit(0);
}