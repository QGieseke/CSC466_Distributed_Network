#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <math.h>

int main();
void cleanup(int signum);
void *read_val(void *r);
void *move_node(void *r);
void *trigger_update(void *i);
void update_map();
double distance(int x1, int y1, int x2, int y2);

