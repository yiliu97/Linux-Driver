#include <linux/module.h> //module_init 
#include <linux/printk.h>
#include <linux/cdev.h> //cdev
#include <linux/fs.h> //file_operations
#include <linux/device.h> //class_create
#include <linux/err.h> // -EBUSY
#include <linux/io.h> //ioremap
#include <linux/delay.h>   //udelay
#include <linux/uaccess.h> //copy_from_user
#include <linux/gpio.h> //gpio
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel_stat.h>
#include <cfg_type.h>

#define DEVICE_NAME "ds18b20_drv" //device name

#define GPIO_DQ (PAD_GPIO_C+17) //the gpio connected with the DQ pin

//define the device number
static dev_t ds18b20_devt;
static unsigned int major = 0;
static unsigned int minor = 0;

static struct class *ds18b20_class = NULL;
static struct device *ds18b20_device = NULL;

//define a character device
static struct cdev ds18b20_cdev;

//initialization function
int DS18_module_init(void)
{
	int ret=1;
	//set a low pulse and lasts at least 480us 
	gpio_direction_output(GPIO_DQ,0);
	udelay(500);
	
	//pull up the line
	gpio_direction_output(GPIO_DQ,1);
	//delay 60us
	udelay(60);
	
	//read the input line and get value
	gpio_direction_input(GPIO_DQ);
	ret=gpio_get_value(GPIO_DQ);
	
	udelay(200);
	//pull up the line again
	gpio_direction_output(GPIO_DQ,1);
	return ret;
}

//reset function
int DS18_module_reset(void)
{
	int ret=1;
	//pull low
	gpio_direction_output(GPIO_DQ,0);
	udelay(500);
	
	//pull up
	gpio_direction_output(GPIO_DQ,1);

	udelay(60);
	//get value
	gpio_direction_input(GPIO_DQ);
	ret=gpio_get_value(GPIO_DQ);
	
	udelay(200);
	gpio_direction_output(GPIO_DQ,1);
	
	if(1== (ret&0x01))
	{
		printk(KERN_INFO"reset failed\n");
		return -1;
	}
	
	return ret;
}


//write one byte data
static void write_byte(unsigned char data)
 {
    char i;
	
	gpio_direction_output(GPIO_DQ,1);
	
	for(i=0; i<8;i++)

	{
        //pull low the line for 1us
		gpio_direction_output(GPIO_DQ,0);
		udelay(1);

		if(1 == (data & 0x01))
		{
			//pull up to write 1
			gpio_direction_output(GPIO_DQ,1);
		}
		else
		{
			//pull low to write 0
			gpio_direction_output(GPIO_DQ,0);
		}
		
		//each writing period is 60us
		udelay(60);

		gpio_set_value(GPIO_DQ,1);

		udelay(15);
		//move right to read the next bit
		data >>= 1;
	}

    gpio_set_value(GPIO_DQ,1);
}


//read one byte data
unsigned char read_byte(void)
{
	int i=0;
	unsigned char data_byt=0;
	
	for (i=0; i<8; i++)
	{
		unsigned char bit_buf=0;
		
		//pull the line down for 1us
		gpio_direction_output(GPIO_DQ, 0);
		udelay(1);
		gpio_set_value(GPIO_DQ, 1);
		
		//begin read value
		gpio_direction_input(GPIO_DQ);
		udelay(12);
		
		//read the value, each read period is around 60us
		bit_buf=gpio_get_value(GPIO_DQ);
		data_byt |= (bit_buf<<i);
			
		udelay(60);
		
		gpio_direction_output(GPIO_DQ, 1);
		udelay(2);
		
	}
	gpio_direction_output(GPIO_DQ, 1);
	return data_byt;	
}


//open device
int DS18B20_open(struct inode *node, struct file *file)
{
	int ret=-1;
	unsigned char flag;
	
	//request gpio
	ret = gpio_request(GPIO_DQ,DEVICE_NAME);
	if(ret < 0)
	{
		printk("request gpio error\n");
		return -ENODEV;
	}
	
	//initialize the module
	flag=DS18_module_init();
	
	if(flag)
	{
		printk(KERN_INFO"init failed\n");
		return -1;
	}

	printk(KERN_INFO"module_opened\n");

	return 0;
}

//read temperature
static int DS18B20_read(struct file *filp, char *buffer, size_t count, loff_t *ppos)
{
	unsigned char lsb_val, msb_val;
	unsigned char tmp_buf;
	unsigned char tmp_data[2] = {0, 0};
	int ret;
	
	//reset
	ret=DS18_module_reset();
	
	udelay(200);
	
	//send command 0xcc(Skip ROM)
	write_byte(0xcc);
	
	//send 0x44(start temperature conversion)
    write_byte(0x44);
	
	//wait at least 750ms
	mdelay(1000);
	
	//reset
	DS18_module_reset();
	
	udelay(200);
	
	//send 0xcc
	write_byte(0xcc);
	
	//send 0xbe(Read Scratchpad)
    write_byte(0xbe);
	
	//read data
	//least significant 8 bits comes first
    lsb_val = read_byte();

	//most significant 8 bits
	msb_val=read_byte();
	
	//convert the data
	//in this driver, only the interger part is displayed
	tmp_buf= (msb_val<<8) | lsb_val;

	//if the temperature is negative, the highest two digits are ff(e.g. for -1°C, tmp_buf= ffef)
	if(tmp_buf >= 65280)
	{
		tmp_data[0]= -1;
		//inverse the number and add 1, then times 0.0625
		tmp_data[1] = ((~tmp_buf)+ 1)* 0.0625;
	}
	else
	{
		tmp_data[0] = 1;
		//directly times 0.0625
		tmp_data[1] = tmp_buf * 0.0625;
	} 
	
	//copy the data to user space
	ret=copy_to_user(buffer, &tmp_data, sizeof(tmp_data)); 

	return 0;
}

int DS18B20_release(struct inode *node, struct file *file)
{
	gpio_free(GPIO_DQ);
	printk(KERN_INFO "module_released\n");
	return 0;
}

//file operations
 static struct file_operations DS18B20_ops = {
	.owner = THIS_MODULE,
	.open = DS18B20_open,
	.read = DS18B20_read,
	.release = DS18B20_release,
};


//init funcrion
static int __init DS18B20_init(void)

{
	int ret;
	printk(KERN_INFO"start_init\n");
	
	//allocate a device number
	if(major == 0)
	{ 
		ret = alloc_chrdev_region(&ds18b20_devt,minor,1,DEVICE_NAME);
	}
	else
	{ 
		ds18b20_devt = MKDEV(major,minor);
		//int register_chrdev_region(dev_t from, unsigned count, const char *name)
		ret = register_chrdev_region(ds18b20_devt,1,DEVICE_NAME);
	}

    if(ret < 0)
	{
		printk(KERN_INFO"chrdev register failed\n");
		return -ENODEV;
	}
	
	//initialize the char device
	cdev_init(&ds18b20_cdev, &DS18B20_ops);
	ds18b20_cdev.owner = THIS_MODULE;
    ds18b20_cdev.ops = &DS18B20_ops;
	
	//add the char device to system
	//int cdev_add(struct cdev *p, dev_t dev, unsigned count)
	ret = cdev_add(&ds18b20_cdev, ds18b20_devt, 1);
	if(ret < 0){
		
		printk("cdev_add error\n");
		return -ENODEV;
	}
	
	//create the struct class structure
	//struct class * class_create(struct module *owner, const char *name)
	ds18b20_class = class_create(THIS_MODULE,"ds18b20_class");
	if(ds18b20_class == NULL){
		printk("class_create error\n");
		ret = -ENOMEM;  //#include <linux/errno.h>
	}
	
	//create the device and register
	//struct device *device_create(struct class *class, struct device *parent, dev_t devt, void *drvdata, const char *fmt, ...)
	ds18b20_device = device_create(ds18b20_class,NULL,ds18b20_devt, DEVICE_NAME, DEVICE_NAME);
	if(ds18b20_device == NULL)
	{
		printk("device_create error\n");
		ret = -ENOMEM;
	}

	return 0;
}

//exit function
static void __exit DS18B20_exit(void)

{

	gpio_free(GPIO_DQ);

	//destroy device
	device_destroy(ds18b20_class, ds18b20_devt);
	ds18b20_device = NULL;

	//destroy class
	class_destroy(ds18b20_class);
	ds18b20_class = NULL;
	
	//delete char dev
	cdev_del(&ds18b20_cdev);
	unregister_chrdev_region(ds18b20_devt,1);
	
	printk(KERN_INFO"module_exited\n");

}

module_init(DS18B20_init);
module_exit(DS18B20_exit);

MODULE_DESCRIPTION("DS18B20 Driver on 6818");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
MODULE_AUTHOR("Yi Liu");
