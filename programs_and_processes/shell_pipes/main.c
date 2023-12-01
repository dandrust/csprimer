/*
    STRETCH GOAL IDEAS:
        * modularize the code - linking/modules/static vars, etc
*/ 

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#define CMD_BUFFER_SIZE 1024
#define MAX_JOBS 8
#define MAX_PIPES MAX_JOBS - 1
#define MAX_ARGS 16
#define TOKEN_EXIT "exit"
#define TOKEN_HELP "help"
#define TOKEN_LOGOUT "logout"
#define TOKEN_ECHO "echo"
#define TOKEN_PIPE "|"

#define DEBUG 0

volatile pid_t child_proc;

void render_prompt() {
    printf("> ");
    fflush(stdout);
}

void handle_sigint() {
    if(!child_proc) { 
        return;
    }

    kill(child_proc, SIGINT);
}

struct child_proc {
    int ifd;
    int ofd;
    char *args[MAX_ARGS];
    pid_t pid;
};

struct pipe {
    struct child_proc* writer;
    struct child_proc* reader;
    int write_fd;
    int read_fd;
};

void debug_child_proc(struct child_proc *job) {
    if (!DEBUG) return;
    int a = 0;

    printf("[%d] Job: ", getpid());

    while (job->args[a] != NULL) {
        printf("%s ", job->args[a]);
        a++;
    }

    printf("(Reads from %d; writes to %d)\n", job->ifd, job->ofd);
}

void debug_pipe(struct pipe *p) {
    if (!DEBUG) return;

    printf("Write: %d; Read: %d\n", p->write_fd, p->read_fd);
}

int parse_command_line(char *buffer, struct child_proc *jobs[], struct pipe *pipes[]) {
    int pipefd[2];
    int job_no = 0;
    int arg_no = 0;
    int pipe_no = 0;

    jobs[job_no] = malloc(sizeof(struct child_proc));

    jobs[job_no]->ifd = STDIN_FILENO;  // we don't know any differently yet
    jobs[job_no]->ofd = STDOUT_FILENO; // we don't know any differently yet

    char *last_token;
    jobs[job_no]->args[arg_no] = strtok(buffer, " \t\n");
    last_token = jobs[job_no]->args[arg_no];
    if (DEBUG) printf("read: %s (job %d, arg %d)\n", last_token, job_no, arg_no);

    while (last_token != NULL) { // until we reach end of string
        jobs[job_no]->args[++arg_no] = strtok(NULL, " \t\n"); // get the next token
        last_token = jobs[job_no]->args[arg_no];
        if (DEBUG) printf("read: %s (job %d, arg %d)\n", last_token, job_no, arg_no);

        if (jobs[job_no]->args[arg_no] != NULL && strcmp(TOKEN_PIPE, jobs[job_no]->args[arg_no]) == 0) { // if the next token is a pipe
            if (DEBUG) printf("pipe detected!\n");
            pipes[pipe_no] = malloc(sizeof(struct pipe));
            jobs[job_no]->args[arg_no] = NULL; //      write NULL to args[i]

            // Setup a new pipe
            if (pipe(pipefd) == -1) {
                perror("pipe");
                exit;
            };

            pipes[pipe_no]->read_fd  = pipefd[0];
            pipes[pipe_no]->write_fd = pipefd[1];

            // Set writer
            jobs[job_no]->ofd = pipes[pipe_no]->write_fd;
            pipes[pipe_no]->writer = jobs[job_no];
            
            // Pointer accounting (point to next job)
            job_no++;   //      increment the job index
            jobs[job_no] = malloc(sizeof(struct child_proc));
            if (DEBUG) printf("incremented job no to %d\n", job_no);
            arg_no = -1;   //      set arg index to zero
            if (DEBUG) printf("reset arg no to %d\n", arg_no);

            // Set reader
            jobs[job_no]->ifd = pipes[pipe_no]->read_fd;
            jobs[job_no]->ofd = STDOUT_FILENO; // we don't know any differently yet
            pipes[pipe_no]->reader = jobs[job_no];

            // Inc pipe number
            pipe_no++;
        }
    }
    return job_no + 1;
}

int main () {
    char cmd_buffer[CMD_BUFFER_SIZE];
    struct child_proc *jobs[MAX_JOBS];
    struct pipe *pipes[MAX_PIPES];

    signal(SIGINT, handle_sigint);

    render_prompt();
    
    while (1) {
        if (feof(stdin)) { 
            printf("\n");
            return 0;
        }

        fgets(cmd_buffer, CMD_BUFFER_SIZE, stdin);

        int job_cnt = parse_command_line(cmd_buffer, jobs, pipes);
        int pipe_cnt = job_cnt - 1;

        if (DEBUG) printf("found %d jobs\n", job_cnt);

        if (jobs[0]->args[0] == NULL) { 
            render_prompt();
            continue;
        }
        
        if ((strcmp(TOKEN_EXIT, jobs[0]->args[0])) == 0 || (strcmp(TOKEN_LOGOUT, jobs[0]->args[0])) == 0) {
            return 0;
        }
        
        if ((strcmp(TOKEN_HELP, jobs[0]->args[0])) == 0) {
            printf("help is on the way!\n");
            render_prompt();
            continue;
        }

        for (int i = 0; i < job_cnt; i++) {
            debug_child_proc(jobs[i]);
            if (DEBUG) printf("[%d] Forking child %d\n", getpid(), i);
            
            jobs[i]->pid = fork();

            if (jobs[i]->pid < 0) {
                if (DEBUG) printf("Unable to fork\n");
            } else if (jobs[i]->pid == 0) {
                // Child Process
                
                if (DEBUG) printf("[%d] Hello from child (%d) process\n", getpid(), i);
                // Fiddle with IO streams
                if (DEBUG) printf("[%d] Checking input stream (%d)...\n", getpid(), jobs[i]->ifd);
                if (jobs[i]->ifd != STDIN_FILENO) {
                    if (DEBUG) printf("\t[%d] Dup2-ing fd (%d) to STDIN (%d)\n", getpid(), jobs[i]->ifd, STDIN_FILENO);
                    dup2(jobs[i]->ifd, STDIN_FILENO);
                }

                if (DEBUG) printf("[%d] Checking output stream (%d)...\n", getpid(), jobs[i]->ofd);
                if (jobs[i]->ofd != STDOUT_FILENO) {
                    if (DEBUG) printf("\t[%d] Dup2-ing fd (%d) to stdout (%d)\n", getpid(), jobs[i]->ofd, STDOUT_FILENO);
                    dup2(jobs[i]->ofd, STDOUT_FILENO);
                }

                // Close any fd's that aren't needed from the pipe array
                for(int p = 0; p < pipe_cnt; p++) {
                    if (pipes[p]->writer != jobs[i]) {
                        // if (DEBUG) printf("\t[%d] Closing pipe %d writer (fd %d)\n", getpid(), p, pipes[p].write_fd);
                        if (-1 == close(pipes[p]->write_fd)) perror("close (write)");
                    }

                    if (pipes[p]->reader != jobs[i]) {
                        // if (DEBUG) printf("\t[%d] Closing pipe %d reader (fd %d)\n", getpid(), p, pipes[p].read_fd);
                        if (-1 == close(pipes[p]->read_fd)) perror("close (read)");
                    }
                }

                int rc_exec = execvp(jobs[i]->args[0], jobs[i]->args);
                if (rc_exec == -1) { 
                    return 0;
                }
            }
        }

        for(int p = 0; p < pipe_cnt; p++) {
            if (DEBUG) printf("[%d] Closing pipe %d read-side on main thread (%d)\n", getpid(), p, pipes[p]->read_fd);
            if (-1 == close(pipes[p]->read_fd)) perror("close (read)");
            
            if (DEBUG) printf("[%d] Closing pipe %d write-side on main thread (%d)\n", getpid(), p, pipes[p]->write_fd);
            if (-1 == close(pipes[p]->write_fd)) perror("close (read)");

            free(pipes[p]);
        }

        for (int i = 0; i < job_cnt; i++) {
            if (DEBUG) printf("[%d] Waiting on child %d running '%s' (pid %d)\n", getpid(), i, jobs[i]->args[0], jobs[i]->pid);
            waitpid(jobs[i]->pid, NULL, 0);
            if (DEBUG) printf("[%d] Child %d running '%s' (pid %d) has returned\n", getpid(), i, jobs[i]->args[0], jobs[i]->pid);
            free(jobs[i]);
        }

        render_prompt();
    }
}
