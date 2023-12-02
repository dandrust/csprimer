#include <signal.h>
#include <stdio.h>
#include <unistd.h>

void handle_sigwinch(int sig) { 
    printf("window resized!\n");
}

int main() {
    struct sigaction a;
    a.sa_handler = handle_sigwinch;

    int b;
    b = sigaction(SIGWINCH, &a, NULL);
    printf("%d\n", b);

    while (1) {
        sleep(1);
    }
}
