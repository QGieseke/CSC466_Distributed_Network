#include "sensor.h"

int my_id;
char fifo_name [50];
volatile int sense;

int sensor_init(int new_id){
	my_id = new_id;
	sense = 1;

	sprintf(fifo_name, "tmp/sensor_%d", my_id);
	return mknod(fifo_name, S_IFIFO | 0666, 0);

}

void sensor_destruct(){
	sense = 0;
	printf("removing %s\n", fifo_name);
	fflush(stdout);
	remove(fifo_name);
	exit(1);
}



int sensor(int id_val){
	signal(SIGINT, sensor_destruct);
	//printf("sensor init %d\n", id_val);
	sensor_init(id_val);
	//printf("My id %d\n", my_id);
	
	//printf("sense val: %d\n", sense);
	int sense_val = 1;
	int tick = 0;				//monotonic per sensor
	char val[50];
	int fd = open(fifo_name, O_WRONLY);
	ssize_t write_ammount;
	while(sense){
		//int fd = open(fifo_name, O_WRONLY);
		
		sprintf(val, "%d,%d,%d\n", my_id, tick, sense_val);
		write_ammount = write(fd, val, strlen(val));
		if(write_ammount < strlen(val)){
			printf("\t\tBUFFER FULL!!!!\n\n");
			sleep(1);
			continue;
		}
//		printf("senseval %d\n", sense_val);
		sense_val++;
		tick++;
		printf("SENSOR %s\n", val);
		usleep(1000);
		//printf("Am sensing\n");
	}
	close(fd);
	return 0;

}
