#include "main.h"

#define NUM_SENSORS 8
#define NUM_NODES 2 


#include "sensor.h"
#include "node.h"

pid_t sensor_child_id[NUM_SENSORS];
int sensor_ids[NUM_SENSORS];

pid_t node_child_id[NUM_NODES];
int node_ids[NUM_NODES];

pthread_t read_threads[NUM_SENSORS];
pthread_t move_thread[NUM_NODES];

volatile char sensor_node_map[NUM_SENSORS][NUM_NODES];
#define MAX_Y 50
#define MAX_X 50
#define SENSOR_TRANSMIT 10	//how far "away" a sensor ping can be heard
#define NODE_TRANSMIT 20	//how far "away" a node ping can be heard

volatile int sensor_x[NUM_SENSORS];
volatile int sensor_y[NUM_SENSORS];
volatile int node_x[NUM_NODES];
volatile int node_y[NUM_NODES];

volatile char sensor_node_map[NUM_SENSORS][NUM_NODES];
pthread_mutex_t sensor_node_map_lock;
volatile char node_node_map[NUM_NODES][NUM_NODES];
pthread_mutex_t node_node_map_lock;


int main(){
	signal(SIGINT, cleanup);
	//sensor_init(1);
	
	pthread_mutex_init(&sensor_node_map_lock, NULL);
	pthread_mutex_init(&node_node_map_lock, NULL);

	//update_map();
	
	//testing case with full connectivity of network
	if(1){
		memset(sensor_node_map, 1, NUM_SENSORS*NUM_NODES);
		memset(node_node_map, 1, NUM_NODES*NUM_NODES);
	}

	//create new processes for every sensor
	for(int i = 0; i < NUM_SENSORS; i++){
		sensor_child_id[i] = fork();
		//printf("ids: %d\n", sensor_child_id[i]);
		sensor_ids[i] = i;						//linear assignment for now
		if(sensor_child_id[i] == 0){
			sensor(sensor_ids[i]);
			return 0;
		}
		printf("%d, ", sensor_child_id[i]);
	}
	printf("SENSOR ids\n");
	fflush(stdout);

	//create new thread per sensor to manage data distribution
	for(int i = 0; i < NUM_SENSORS; i++){
		//printf("%d\n", ids[i]);
		pthread_create(&(read_threads[i]), NULL, read_val, &(sensor_ids[i]));
	}

	//create new processes for every node
	for(int i=0; i < NUM_NODES; i++){
		node_child_id[i] = fork();
		node_ids[i] = i;
		if(node_child_id[i] == 0){
			node(node_ids[i]);
			return 0;
		}
		printf("%d, ", node_child_id[i]);
	}
	printf("NODE ids\n");
	fflush(stdout);

	for(int i = 0; i < NUM_NODES; i++){
		pthread_create(&(move_thread[i]), NULL, move_node, &(node_ids[i]));
	}
	pthread_t umap;
	//pthread_create(&umap, NULL, trigger_update, NULL);

	getchar();
	//pthread_kill(umap, SIGINT);
	printf("\t\tTERMINATING TEST\n\n\n");

	for (int i = 0; i < NUM_SENSORS; i++){
		printf("destruct sensor %d,%d\r", sensor_child_id[i], kill(sensor_child_id[i], SIGINT));
	}
	for (int i = 0; i < NUM_NODES; i++){
		printf("destruct node %d,%d\r", node_child_id[i], kill(node_child_id[i], SIGINT));
	}
	return 0;

}

void *trigger_update(void *i){
	while(1){
		update_map();
		sleep(1);
	}
}

void *read_val(void *r){
//	int fd = open("tmp/sensor_1", O_RDONLY);
	int *r_val = (int *) r;		
	char rfilename[50];
	sprintf(rfilename, "tmp/sensor_%d", *r_val);
	char wfilename [50];
	char buff[50];

	while(1){
		for(int i = 0; i < NUM_NODES; i++){
			while(!pthread_mutex_trylock(&sensor_node_map_lock));
			if(sensor_node_map[*r_val][i] == 0){
				pthread_mutex_unlock(&sensor_node_map_lock);
				continue;
			}
			pthread_mutex_unlock(&sensor_node_map_lock);
			sprintf(wfilename, "tmp/node_read_%d", i);
			int fdr = open(rfilename, O_RDONLY);
			int fdw = open(wfilename, O_WRONLY);
			read(fdr, buff, 50);
			if(strlen(buff)> 0)
				write(fdw, buff, 50);
				//printf("moved:%s", buff);
			close(fdr);
			close(fdw);
			
	//sleep(1);
		}
	}
}

void *move_node(void* r){
	int *r_val = (int*) r;
	char rfilename[50];
	sprintf(rfilename, "tmp/node_write_%d", *r_val);	//node writes, we read
	char wfilename[50];
	char buff[500];

	while(1){
		for(int i = 0; i < NUM_NODES; i++){
			while(!pthread_mutex_trylock(&node_node_map_lock));
			if(node_node_map[*r_val][i] == 0){
				pthread_mutex_unlock(&node_node_map_lock);
				continue;
			}
			pthread_mutex_unlock(&node_node_map_lock);
			
			sprintf(wfilename, "tmp/node_node_read_%d", i);
			int fdr = open(rfilename, O_RDONLY);
			int fdw = open(wfilename, O_WRONLY);
			read(fdr, buff, 500);
			if(strlen(buff)> 0)
				write(fdw, buff, 500);
				//printf("moved:%s", buff);
			close(fdr);
			close(fdw);
	//sleep(1);
		}
	}

}

void cleanup(int signum){
	printf("Just hit enter you dum dum\n");
	sensor_destruct();
	node_destruct();
	for(int i = 0; i < NUM_SENSORS; i++){
		kill(sensor_child_id[i], SIGINT);
	}
	for(int i = 0; i < NUM_NODES; i++){
		kill(node_child_id[i], SIGINT);
	}
	exit(1);
}



//TODO make the mapping random w/errors n stuff
void update_map(){
	time_t t;
	srand((unsigned) time(&t));

	//change node & sensor locations
	for(int i = 0; i < NUM_SENSORS; i++){
		sensor_x[i] = rand() % MAX_X;
		sensor_y[i] = rand() % MAX_Y;
	}
	for(int i = 0; i < NUM_NODES; i++){
		node_x[i] = rand() % MAX_X;
		node_y[i] = rand() % MAX_Y;
	}
	
	pthread_mutex_lock(&sensor_node_map_lock);
	for(int i = 0; i < NUM_SENSORS; i++){
		for(int j = 0; j < NUM_NODES; j++){
			if(distance(sensor_x[i], sensor_y[i], node_x[j], node_y[j]) < SENSOR_TRANSMIT){
				sensor_node_map[i][j] = 1;
			} else {
				sensor_node_map[i][j] = 0;
			}
		}
	}

	pthread_mutex_unlock(&sensor_node_map_lock);
	pthread_mutex_lock(&node_node_map_lock);
	for(int i = 0; i < NUM_NODES; i++){
		for(int j = i+1; j < NUM_NODES; j++){
			if(distance(node_x[i],node_y[i], node_x[j], node_y[j]) < NODE_TRANSMIT){
				node_node_map[i][j] = 1;
			} else {
				node_node_map[i][j] = 0;
			}
		}
	}
	pthread_mutex_unlock(&node_node_map_lock);
}

double distance(int x1, int y1, int x2, int y2){
	return sqrt(pow((x1-x2), 2) + pow((y1-y2), 2));
	
}
