#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>

int node(int id);
int node_init(int new_id);
void node_destruct();
void *update_bitmap();
void *node_write(void* i);
void *node_read(void* i);
void *other_node_read(void* i);

#ifndef NUM_SENSORS
#define NUM_SENSORS 8 
#define NUM_NODES 2 
#endif
