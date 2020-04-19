#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


int main(void)
{
	int distance_buf;
	
	//open the device
	int fd = open("/dev/HCSR04_drv",O_RDWR);
	if(fd == -1)
	{
		printf("open error\n");
		return fd;
	}
	
	//read the distance every 3 seconds
	while(1)
	{
		
		read(fd,&distance_buf,sizeof(distance_buf));
		printf("distance = %d cm\n",distance_buf);
		sleep(3);
		
	}
	
	close(fd);
	
	return 0;
}
