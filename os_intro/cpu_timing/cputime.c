#include <stdlib.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sched.h>

#define SLEEP_SEC 3
#define NUM_MULS 100000000
#define NUM_MALLOCS 100000
#define MALLOC_SIZE 1000

// TODO define this struct
struct profile_times {
  pid_t pid;
  struct timeval clock_time_start;
  struct timeval user_time_start;
  struct timeval system_time_start;
};

// TODO populate the given struct with starting information
void profile_start(struct profile_times *t) {
  gettimeofday(&(t->clock_time_start), NULL);
  
  struct rusage r;
  getrusage(RUSAGE_SELF, &r);
  t->system_time_start = r.ru_stime;
  t->user_time_start = r.ru_utime;
}

// TODO given starting information, compute and log differences to now
void profile_log(struct profile_times *t) {
  struct timeval b;
  gettimeofday(&b, NULL);

  struct rusage r;
  getrusage(RUSAGE_SELF, &r);

  float clock_time_seconds = b.tv_sec - t->clock_time_start.tv_sec;
  float clock_time_nanoseconds = (b.tv_usec - t->clock_time_start.tv_usec)/1e6;

  float system_time_seconds = r.ru_stime.tv_sec - t->system_time_start.tv_sec;
  float system_time_nanoseconds = (r.ru_stime.tv_usec - t->system_time_start.tv_usec)/1e6;

  float user_time_seconds = r.ru_utime.tv_sec - t->user_time_start.tv_sec;
  float user_time_nanoseconds = (r.ru_utime.tv_usec - t->user_time_start.tv_usec)/1e6;
  
  printf("[%d] real: %fs\t sys: %f\t user:%f\n", t->pid, clock_time_seconds + clock_time_nanoseconds, system_time_seconds + system_time_nanoseconds, user_time_seconds + user_time_nanoseconds);
}

int main(int argc, char *argv[]) {
  struct profile_times t;
  t.pid = getpid();

  u_int16_t cpu;

  printf("[%d, cpu %d] %d muls\n", t.pid, sched_getcpu(), NUM_MULS);
  float x = 1.0;

  profile_start(&t);
  for (int i = 0; i < NUM_MULS; i++)
    x *= 1.1;
  profile_log(&t);

  printf("[%d] %d mallocs\n", t.pid, NUM_MALLOCS);
  profile_start(&t);
  void *p;
  for (int i = 0; i < NUM_MALLOCS; i++)
    p = malloc(MALLOC_SIZE);
  profile_log(&t);

  printf("[%d] sleep %d sec\n", t.pid, SLEEP_SEC);
  profile_start(&t);
  sleep(SLEEP_SEC);
  profile_log(&t);
}

