
/*
// Projeto SO - exercise 3, version 1
// Sistemas Operativos, DEI/IST/ULisboa 2018-19
*/

#include "lib/commandlinereader.h"
#include "lib/vector.h"
#include "CircuitRouter-AdvShell.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "lib/timer.h"

#define COMMAND_EXIT "exit"
#define COMMAND_RUN "run"
#define ERROR_MSG "Command not supported."
#define STDIN 0

#define MAXARGS 3
#define BUFFER_SIZE 100
#define TAMCMD 100 /*size of command*/

TIMER_T startTime;
TIMER_T stopTime;
vector_t *children; /* global array of child processes*/
int runningChildren = 0;

void waitForChild() {
    while (1) {
        child_t *child = malloc(sizeof(child_t));
        if (child == NULL) {
            perror("Error allocating memory");
            exit(EXIT_FAILURE);
        }
        child->pid = wait(&(child->status));
        if (child->pid < 0) {
            if (errno == EINTR) {
                 // Este codigo de erro significa que chegou signal que interrompeu a espera
                 //   pela terminacao de filho; logo voltamos a esperar 
                free(child);
                continue;
            } else {
                perror("Unexpected error while waiting for child.");
                exit (EXIT_FAILURE);
            }
        }
        vector_pushBack(children, child);
        return;
    }
}

void printChildren() {
    for (int i = 0; i < vector_getSize(children); ++i) {
        child_t *child = vector_at(children, i);
        int status = child->status;
        pid_t pid = child->pid;
        double exec_time = child->exec_time;
        if (pid != -1) {
            const char* ret = "NOK";
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                ret = "OK";
            }
            printf("CHILD EXITED: (PID=%d; return %s; %lf s )\n", pid, ret, exec_time);
        }
    }
    puts("END.");
}

void SignalHandler(int sig)
{
    child_t *child = malloc(sizeof(child_t));
        if (child == NULL) {
            perror("Error allocating memory");
            exit(EXIT_FAILURE);
        }
        child->pid  = waitpid(-1, (&(child->status)), WNOHANG);
        TIMER_READ(stopTime);
        if (child->pid < 0) {
            if (errno == EINTR) {
                /* Este codigo de erro significa que chegou signal que interrompeu a espera
                   pela terminacao de filho; logo voltamos a esperar */
                free(child);
                
            } else {
                perror("Unexpected error while waiting for child.");
                exit (EXIT_FAILURE);
            }
        }
        vector_pushBack(children, child);
  child->exec_time= TIMER_DIFF_SECONDS(startTime, stopTime);
  runningChildren--;
}

int main (int argc, char** argv) {
    int fAdv, fClient, mode=0;
    char *args[MAXARGS + 1];
    char buffer[BUFFER_SIZE];
    int MAXCHILDREN = -1;
    
    char* text_buffer= (char*) malloc(sizeof(char)*100);
    fd_set fds;
    int rv;
    int maxfd;
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SignalHandler;

    
    if(argv[1] != NULL){
        MAXCHILDREN = atoi(argv[1]);
    }

    children = vector_alloc(MAXCHILDREN);
    printf("Welcome to CircuitRouter-AdvShell\n");
    unlink("/tmp/CircuitRouter-AdvShell.pipe");
    if (mkfifo ("/tmp/CircuitRouter-AdvShell.pipe", 0777) < 0) {
        perror("Error: Making Fifo");
        exit (1);
    }
    if ((fAdv = open ("/tmp/CircuitRouter-AdvShell.pipe",O_NONBLOCK)) < 0) exit (-1); 
     
    while (1) {
        int numArgs;
        FD_ZERO(&fds);
        FD_SET(STDIN, &fds);
        FD_SET(fAdv, &fds);  
        maxfd = (fAdv > STDIN)?fAdv:STDIN;  
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
            mode=1; 

        }

        if (FD_ISSET(fAdv, &fds)) {
            read(fAdv, text_buffer, 100);
            mode=2;
        }
        strcpy(buffer, text_buffer);
        numArgs = readLineArguments(args, MAXARGS+1, buffer, BUFFER_SIZE,mode);

        /* EOF (end of file) do stdin ou comando "sair" */
        if (numArgs < 0 || (numArgs > 0 && (strcmp(args[0], COMMAND_EXIT) == 0))) {
            if (mode==2) {
                if ((fClient = open ("/tmp/CircuitRouter-Client.pipe",O_WRONLY)) < 0) exit (-1);
                write(fClient, ERROR_MSG,23);
                close(fClient);
         }
            else {
            printf("CircuitRouter-AdvShell will exit.\n--\n");

            /* Espera pela terminacao de cada filho */
            while (runningChildren > 0) {
                pause();
                
            }

            printChildren();
            printf("--\nCircuitRouter-AdvShell ended.\n");
            break;
        }
        }

        else if (numArgs > 0 && strcmp(args[0], COMMAND_RUN) == 0){
            int pid;
            if (numArgs < 2) {
                printf("%s: invalid syntax. Try again.\n", COMMAND_RUN);
                continue;
            }
            if (MAXCHILDREN != -1 && runningChildren >= MAXCHILDREN) {
                pause();
                
            }
           // if (sigaction(SIGCHLD, &act, NULL) < 0) {
            //perror ("sigaction");
    //            return 1;
   // }
            sigaction(SIGCHLD, &sa, NULL);
            pid = fork();
            TIMER_READ(startTime);
            if (pid < 0) {
                perror("Failed to create new process.");
                exit(EXIT_FAILURE);
            }

            if (pid > 0) {
                runningChildren++;
                printf("%s: background child started with PID %d.\n", COMMAND_RUN, pid);
                continue;
            } else {
                char seqsolver[] = "../CircuitRouter-SeqSolver/CircuitRouter-SeqSolver";
                char *newArgs[3] = {seqsolver, args[1], NULL};
                execv(seqsolver, newArgs);
                perror("Error while executing child process"); // Nao deveria chegar aqui
                exit(EXIT_FAILURE);
            }
        }

        else if (numArgs == 0){
            /* Nenhum argumento; ignora e volta a pedir */
            continue;
        }
        else
            printf("Unknown command. Try again.\n");

    }

    for (int i = 0; i < vector_getSize(children); i++) {
        free(vector_at(children, i));
    }
    close (fAdv);
    unlink("/tmp/CircuitRouter-AdvShell.pipe");
    vector_free(children);
    free(text_buffer);
    return EXIT_SUCCESS;
}
