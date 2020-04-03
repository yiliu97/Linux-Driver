#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
	
	unsigned char buf[2];
	int sign, value, temperature;

	int fd = open("/dev/ds18b20_drv",O_RDWR);
	if(fd == -1)
	{
		perror("openerror\n");
		return fd;
	}
	
	//read data every 5 secs
	while(1)
	{	
		read(fd,&buf,sizeof(buf));
		
		sign=buf[0];
		value=buf[1];
		
		if(1==sign)
		{
        	temperature= value; 
		}
		else
		{
        	temperature= (-1)*value; 
		}
		
		printf("temperature = %d Celsius\n",temperature);
		sleep(5);	
	}
	
	close(fd);
	
	return 0;
}
