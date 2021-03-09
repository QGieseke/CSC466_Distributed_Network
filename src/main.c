#include "main.h"
#include "sensor.h"

#define NUM_SENSORS 20

pid_t child_id[NUM_SENSORS];
pthread_t read_thread[NUM_SENSORS];
int ids[NUM_SENSORS];

int main(){
	signal(SIGINT, cleanup);
	//sensor_init(1);
	

	for(int i = 0; i < NUM_SENSORS; i++){
		child_id[i] = fork();
		printf("ids: %d\n", child_id[i]);
		ids[i] = i;						//linear assignment for now
		if(child_id[i] == 0){
			sensor(ids[i]);
			return 0;
		}
	}
	
	for(int i = 0; i < NUM_SENSORS; i++){
		//printf("%d\n", ids[i]);
		pthread_create(&(read_thread[i]), NULL, read_val, &(ids[i]));
	}
	
	getchar();
	for (int i = 0; i < NUM_SENSORS; i++){
		printf("destruct %d,%d\n", child_id[i], kill(child_id[i], SIGINT));
	}
	return 0;

}


void *read_val(void *i){
//	int fd = open("tmp/sensor_1", O_RDONLY);
	int *i_val = (int *) i;	
	//printf("%p, %d\n", i_val, *i_val);
	char filename[50];
	sprintf(filename, "tmp/sensor_%d", *i_val);
	//printf("%s\n", filename);
	char buff[50];
	while(1){
		int fd = open(filename, O_RDONLY);

		read(fd, buff, 50);	
		if(buff[0] != '\0')
			printf("read:%s\n", buff);
		close(fd);
	//sleep(1);
	}
}

void cleanup(int signum){
	//printf("Just hit enter you dum dum\n");
	sensor_destruct();
	for(int i = 0; i < NUM_SENSORS; i++){
		kill(child_id[i], SIGINT);
	}
	exit(1);
}
