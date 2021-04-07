#include "node.h"

#define UPDATE_THRESHOLD 2000 // number of ticks at which data is considered "out of date"

struct sensor_data {
	int data;
	int tick;		//from the sensor (guaranteed monotonic per sensor
	int node_tick;
};

struct sensor_data sensor_vals [NUM_SENSORS];
pthread_mutex_t data_lock;

char sensor_bitmap[(NUM_SENSORS%8) + 1];
char* node_bitmap[(NUM_NODES%8) + 1];
pthread_mutex_t bitmap_lock;


pthread_t write_thread;
pthread_t read_thread;
pthread_t node_read_thread;

int node_id;
int node_tick;
char node_fifo_write_name [50];
char node_fifo_read_name [50];
char node_fifo_node_read_name [50];

volatile int cont = 1;

//note: the max number of sensors/nodes in the network must be known for memory reasons
int node_init(int new_id){
	node_id = new_id;

	sprintf(node_fifo_read_name, "tmp/node_read_%d", node_id);	//where we read sensor values
	sprintf(node_fifo_write_name, "tmp/node_write_%d", node_id); //where we write our node update	
	sprintf(node_fifo_node_read_name, "tmp/node_node_read_%d", node_id); //where we read other node updates
	mknod(node_fifo_read_name, S_IFIFO | 0666, 0);
	mknod(node_fifo_write_name, S_IFIFO | 0666, 0);
	mknod(node_fifo_node_read_name, S_IFIFO | 0666, 0);
}

void node_destruct(){
	//printf("removing %s\n", node_fifo_read_name);
	//printf("removing %s\n", node_fifo_write_name);
	remove(node_fifo_read_name);
	remove(node_fifo_write_name);
	remove(node_fifo_node_read_name);
	cont=0;
	sleep(1);
	pthread_kill(write_thread, SIGINT);
	pthread_kill(read_thread, SIGINT);
	pthread_kill(node_read_thread, SIGINT);
	exit(1);
}


int node(int id){
	signal(SIGINT, node_destruct);
	node_init(id);

	printf("Node %d created\n", id);
	//TODO 
	//forward data to other nodes when significant changes
	//replicate data

	pthread_mutex_init(&data_lock, NULL);
	pthread_mutex_init(&bitmap_lock, NULL);

	pthread_create(&write_thread, NULL, node_write, NULL);
	pthread_create(&read_thread, NULL, node_read, NULL);
	pthread_create(&node_read_thread, NULL, other_node_read, NULL);
	
	while(cont);
}

void *update_bitmap(){
	pthread_mutex_lock(&bitmap_lock);
	for(int i = 0; i < NUM_SENSORS; i++){
		if(node_tick - sensor_vals[i].node_tick < UPDATE_THRESHOLD){
			sensor_bitmap[i/8] |= 1<<(i % 8);
		}else {
			sensor_bitmap[1/8] &= ~(1<<(i % 8));
		}
	}
	pthread_mutex_unlock(&bitmap_lock);
}


void *node_write(void* i){
	int fdw;
	char buff[500];
	
	while(cont){
		fdw = open(node_fifo_write_name, O_WRONLY);
		update_bitmap();
		for(int i = 0; i < NUM_SENSORS; i++){
			pthread_mutex_lock(&bitmap_lock);
			if(!sensor_bitmap[i/8]<<i%8){
				pthread_mutex_unlock(&bitmap_lock);
				continue;
			}
			pthread_mutex_unlock(&bitmap_lock);
			sprintf(buff, "%d,%d,%d,%d\n", i, sensor_vals[i].data, sensor_vals[i].tick, node_tick - sensor_vals[i].node_tick);
			write(fdw, buff, strlen(buff));
		}
		printf("NODE %d done with update broadcast\n", node_id);
		sleep(1);	//send update ~once per sec
	}

	close(fdw);
}


void *node_read(void* i){
	//printf("%s\n", node_fifo_read_name);
	int fdr = open(node_fifo_read_name, O_RDONLY);

	char buff[50];
	int id, tick, val;

	char* delim = ",";
	char* last_delim = "\n";
	char *raw_id, *raw_tick, *raw_val;

	while(cont){
		read(fdr, buff, 50);	//buffered reading doesnt play nice
		if(buff[0] == '\0'){
			printf("Wack\n");
			sleep(1);
			continue;
		}
		printf("NODE %d read %s\n", node_id, buff);
		raw_id  = strtok(buff, delim);
		raw_tick= strtok(NULL, delim);
		raw_val = strtok(NULL, last_delim);
		if(raw_id==NULL || raw_tick == NULL || raw_val == NULL)
			continue;
		id  = atoi(raw_id);
		tick= atoi(raw_tick);
		val = atoi(raw_val);
		//printf("|%s|\n", strtok(buff, delim));
		//printf("|%s|\n", strtok(NULL, delim));
		//printf("|%s|\n", strtok(NULL, last_delim));
		printf("NODE %d read s_id:%d, s_tick:%d, val:%d\n", node_id, id, tick, val);	
		pthread_mutex_lock(&data_lock);
		sensor_vals[id].data = val;
		sensor_vals[id].tick = tick;
		sensor_vals[id].node_tick = node_tick;
		node_tick++;
		pthread_mutex_unlock(&data_lock);
		
	}
	printf("closing FDR at %d\n", node_id);
	close(fdr);
}

void *other_node_read(void* i){
	
	int fdr = open(node_fifo_node_read_name, O_RDONLY);
	char buff [500];
	int id, data, tick, offset;
	char* delim = ",";
	char* last_delim = "\n";
	char *raw_id, *raw_data, *raw_tick, *raw_offset;
	while(cont){
		read(fdr, buff, 500);
		if(buff[0] == '\0'){
			sleep(1);	//no more updates, wait
			continue;
		}
		printf("NODE RAW read: %s\n", buff);
		raw_id = strtok(buff, delim);
		raw_data = strtok(NULL, delim);
		raw_tick = strtok(NULL, delim);
		raw_offset = strtok(NULL, last_delim);

		if(raw_id == NULL || raw_data == NULL || raw_tick==NULL || raw_offset == NULL)
			continue;
		id = atoi(raw_id);
		data = atoi(raw_data);
		tick = atoi(raw_tick);
		offset = atoi(raw_offset);
		printf("NODE %d read from other node data for sensor %d\n\n", node_id, id);	
		printf("%d, %d, %d, %d\n" ,id, data, tick, offset);
		pthread_mutex_lock(&data_lock);
		if(sensor_vals[id].tick < tick){	//if the recieved data from the sensor through the node is more recent than this nodes copy of the data from that sensor
			sensor_vals[id].data = data;
			sensor_vals[id].tick = tick;
			sensor_vals[id].node_tick = node_tick - offset;	//copy over the other nodes offset, ensures that stale data does not infinitely loop through node communication. Not super accurate, but ensures that stale data stays stale.
		}
		pthread_mutex_unlock(&data_lock);
	}
	close(fdr);
}




