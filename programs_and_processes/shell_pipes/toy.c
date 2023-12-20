#include <stdio.h>
#include <wait.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

void handle_sigint() {
    return;
}

void ignore_SIGTTOU () {
    return;
}

/*
    This will capture stops, continues, and terminations from children
    We'll use it to keep track of the state of background processes
*/
void handle_sigchld() {
    printf("(entering sigchld handler in toy.c)\n");
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED )) > 0) {
        if (WIFEXITED(status)) { 
            printf("Child %d terminated with exit code %d\n", pid, WEXITSTATUS(status));
            continue;
        }

        if (WIFSIGNALED(status)) {
            printf("Child %d was terminated with signal %d\n", pid, WTERMSIG(status));
            continue;
        }

        if (WIFSTOPPED(status)) {
            printf("Child %d was stopped by signal %d\n", pid, WSTOPSIG(status));
            continue;
        }

        if (WIFCONTINUED(status)) {
            printf("Child %d was continued\n", pid);
        }
    }
}

int main () {
    signal(SIGINT, handle_sigint);
    printf("Parent: %d\n", getpid());
    setbuf(stdout, NULL); // Write to STDOUT immediately; don't buffer
    // signal(SIGCHLD, handle_sigchld);

    pid_t child_pid;

    child_pid = fork();

    if (child_pid == -1) {
        perror("fork");
    }

    if (child_pid == 0) {
        // child
        printf("Child: %d\n", getpid());
        setpgid(0, 0);

        struct sigaction default_action;
        struct sigaction ignore_action;
        ignore_action.sa_handler = SIG_IGN;
        
        sigaction(SIGTTOU, &ignore_action, &default_action); /* Ignore SIGTTOU */
        if (tcsetpgrp(STDIN_FILENO, getpid()) == -1)
            perror("tcsetpgrp - child");
        sigaction(SIGTTOU, &default_action, NULL); /* Reinstate default SIGTTOU disposition */

        execlp("./cnt", "./cnt", NULL);
    }

    if (setpgid(child_pid, child_pid) == -1 && errno != EACCES) {
        perror("setpgid - parent");
    }
    tcsetpgrp(STDIN_FILENO, child_pid);
    
    int status;
    waitpid(child_pid, &status, WUNTRACED);
    /*
        This will look very similar to handle_sigchld, but it will only apply to foreground
        processes.  For that reason, we can simplify these conditions (ignore continue for example)
    */
    if (WIFEXITED(status)) { 
        printf("Foreground child %d terminated with exit code %d\n", child_pid, WEXITSTATUS(status));
    }

    if (WIFSIGNALED(status)) {
        printf("Foreground child %d was terminated with signal %d\n", child_pid, WTERMSIG(status));
    }

    if (WIFSTOPPED(status)) {
        printf("Foreground child %d was stopped by signal %d\n", child_pid, WSTOPSIG(status));
    }
    sleep(1);
    printf("> ");
}