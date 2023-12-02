#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>

int main () { 
    struct stat s;
    int a = 'a';
    int fd = open("test.txt", O_CREAT | O_TRUNC | O_WRONLY);
    long b = 0;

    for (int i = 0; i < 100000; i++) {
        write(fd, &a, 1);
    
        fstat(fd, &s);
        
        if (b < s.st_blocks) {
            b = s.st_blocks;
            printf("[%d] (%ld)%ld bytes on disk\n", i,s.st_blocks, s.st_blocks * 512);
        }
    }

    close(fd);
    }