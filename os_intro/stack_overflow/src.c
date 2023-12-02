#include <stdio.h>

void f(int depth, long int last_frame_pointer, long int bottom) {
    printf ("depth: %d \t(%p) \t %ld (delta: %ldb)\n", depth, &depth, bottom - (long)&depth, last_frame_pointer - (long)&depth);
    f(++depth, (long)&depth, bottom);
}

void start() { 
    int depth = 0;
    f(depth, 0, (long)&depth);
}

int main() {
    start();
}

