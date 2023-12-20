#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

void interrupt() {
    printf("SIGINT from count.c\n");
}

void term_stop() {
    printf("SIGTSTP from count.c\n");
}

void cont() {
    printf("SIGCONT from count.c\n");
}

void sigttou() {
    printf("SIGTTOU from count.c\n");
}

int main() {
    // signal(SIGINT, interrupt);
    // signal(SIGTSTP, term_stop);
    // signal(SIGCONT, cont);
    // signal(SIGTTOU, sigttou);

    int i;
    for(i = 0; i < 20; i++) {
        sleep(1);
        printf("%d\n", i);
        
    }
}