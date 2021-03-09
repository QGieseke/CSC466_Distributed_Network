#include "sensor.h"

int my_id;
char fifo_name [50];
int sense;

int sensor_init(int new_id){
	my_id = new_id;
	sense = 1;

	sprintf(fifo_name, "tmp/sensor_%d", my_id);

	return mknod(fifo_name, S_IFIFO | 0666, 0);

}

int sensor_destruct(){
	sense = 0;
	printf("removing %s\n", fifo_name);
	return remove(fifo_name);
}



int sensor(int id_val){
	//printf("sensor init %d\n", id_val);
	sensor_init(id_val);
	//printf("My id %d\n", my_id);
	
	//printf("sense val: %d\n", sense);
	int sense_val = 1;
	char val[50];
	//int fd = open(fifo_name, O_WRONLY);
	while(sense){
		int fd = open(fifo_name, O_WRONLY);
		
		sprintf(val, "%d,%d", my_id, sense_val);
		write(fd, val, strlen(val));
//		printf("senseval %d\n", sense_val);
		sense_val++;
		close(fd);
		sleep(1);
		//printf("Am sensing\n");
	}
	return 0;

}
