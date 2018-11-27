#ifndef CIRCUITROUTER_SHELL_H
#define CIRCUITROUTER_SHELL_H

#include "lib/vector.h"
#include <sys/types.h>

typedef struct {
    pid_t pid;
    int status;
    double exec_time;
} child_t;

void waitForChild();
void printChildren();
void signalHandler(int sig);

#endif /* CIRCUITROUTER_SHELL_H */
