#include <linux/module.h> //module_init 
#include <linux/printk.h>
#include <linux/cdev.h> //cdev
#include <linux/fs.h> //file_operations
#include <linux/device.h> //class_create
#include <linux/err.h> // -EBUSY
#include <linux/io.h> //ioremap
#include <linux/delay.h>   //delay
#include <linux/uaccess.h> //copy_from_user
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/gpio.h> //gpio
#include <linux/moduleparam.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/kernel_stat.h>
#include <asm/cputime.h>
#include <cfg_type.h> 
#include <linux/miscdevice.h> 

#define DEVICE_NAME "HCSR04_drv" //device name
#define GPIO_TRIG (PAD_GPIO_C+7) //the gpio connected with the TRIG pin
#define GPIO_ECHO (PAD_GPIO_E+13) //the gpio connected with the ECHO pin

//open device
int HCSR04_open(struct inode *node, struct file *file)
{
	int ret;
	printk(KERN_INFO"module open\n");
	
	//request gpio
	ret = gpio_request(GPIO_TRIG,DEVICE_NAME);
	if(ret < 0)
	{
		printk(KERN_INFO"request trig error\n");
		return -EBUSY;		  
	}  
	//set the TRIG pin to output
	gpio_direction_output(GPIO_TRIG,0);
	  
	//request gpio
	ret = gpio_request(GPIO_ECHO,DEVICE_NAME);
	if(ret < 0)
	{
		printk(KERN_INFO"request echo error\n");
		return -EBUSY;	  
	}
	//set the ECHO pin to input
	gpio_direction_input(GPIO_ECHO);
	gpio_set_value(GPIO_ECHO, 0);
	return 0;
}

//release device
int HCSR04_release(struct inode *node, struct file *file)
{
	printk(KERN_INFO"module release\n");
	//free gpios
	gpio_free(GPIO_ECHO); 
	gpio_free(GPIO_TRIG); 
	return 0;
}

//read distance
ssize_t HCSR04_read(struct file *file, char __user *user_buff, size_t size, loff_t *offset)
{
	int ret;
	unsigned long distance = 0;
	long long start_time=0, end_time=0;
	unsigned int time_cnt = 0;

	printk(KERN_INFO"start measuring\n");
	
	//send a 10us pulse to trigger
	gpio_set_value(GPIO_TRIG, 1);
	udelay(10);
	
	gpio_set_value(GPIO_TRIG, 0);
	
	//wait for the echo signal
	while(gpio_get_value(GPIO_ECHO) == 0 ) 
	{
	}
	
	//get the start time
	start_time = ktime_to_us(ktime_get()); 
	
	//wait while the ECHO signal keeps high
	while(gpio_get_value(GPIO_ECHO) == 1)
	{	
	}
	
	//get the eng time
	end_time = ktime_to_us(ktime_get());

	//calculate the total time of ECHO signal
	time_cnt =  end_time - start_time;
	
	//calculate the distance
	//time in us / 58 = distance in cm
	distance = (int)(time_cnt/58);
	
	////copy to user space
	ret = copy_to_user(user_buff,&distance,sizeof(distance));

	return size;
}

//define file operations
static struct file_operations HCSR04_ops = {
	.owner = THIS_MODULE,
	.open = HCSR04_open,
	.read = HCSR04_read,
	.release = HCSR04_release,
};


//create a misc device
static struct miscdevice HC_misc = {
		.minor = MISC_DYNAMIC_MINOR,
		.name = DEVICE_NAME,
		.fops = &HCSR04_ops,
};

//initialization
static int __init HCSR04_init(void) 
{
	int ret;
	 
	printk(KERN_INFO"HCSR04 init\n");
	
	
	
	//register the misc dev
	 ret = misc_register(&HC_misc);
	  if(ret < 0)
	  {
		  printk(KERN_INFO"misc dev register error\n");
		  return ret;
		  
	  }

	return 0;

}

//exit
static void __exit HCSR04_exit(void)
{
	printk(KERN_INFO"HCSR04 exit\n");
	//free gpios
	gpio_free(GPIO_TRIG); 
	gpio_free(GPIO_ECHO); 
	//unregister the device
	misc_deregister(&HC_misc);
}


module_init(HCSR04_init);

module_exit(HCSR04_exit);

MODULE_AUTHOR("Yi Liu");
MODULE_DESCRIPTION("HC-SR04 driver on 6818");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL");
