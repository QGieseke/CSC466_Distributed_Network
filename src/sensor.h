#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>

int sensor_init(int new_id);
void sensor_destruct();
int sensor(int id_val);

#ifndef NUM_NODES
#define NUM_NODES 2
#endif
