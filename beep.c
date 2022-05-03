#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#define BEEP_CNT            1
#define BEEP_NAME           "beep"
#define BEEP_ON             1
#define BEEP_OFF            0 
struct beep_dev_t
{
    dev_t devid;/*设备号*/
    int major;/*主设备号*/
    int minor;/*次设备号*/
    struct cdev cdev;/*字符设备*/
    struct class *class;/*类*/
    struct device *device;/*设备*/
    struct device_node *nd;/*设备树节点*/
    int beep_gpio;/*beep节点所使用的GPIO编号*/
};
struct beep_dev_t beep_dev;
static int beep_open(struct inode *nd, struct file *filp)
{
    filp->private_data = &beep_dev;
    return 0;
}
static int beep_release(struct inode *nd, struct file *filp)
{
    filp->private_data = NULL;
    return 0;
}
static ssize_t beep_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{   
    struct beep_dev_t *dev = filp->private_data;
    int ret = 0;
    unsigned char data_buf[1];
    ret = copy_from_user(data_buf,buf,count);
    if(ret<0)
    {
        ret = -EFAULT;
        return ret;
    }
    switch (data_buf[0])
    {
    case BEEP_ON:
        gpio_set_value(dev->beep_gpio,0);
        break;
    case BEEP_OFF:
        gpio_set_value(dev->beep_gpio,1);
        break;
    default:
        break;
    }
    return 0;
}
static struct file_operations beep_fops = {
    .owner = THIS_MODULE,
    .open = beep_open,
    .release = beep_release,
    .write = beep_write,
};
/*驱动加载*/
static int __init beep_init(void)
{
    int ret = 0;
    /*注册字符设备*/
    /*创建设备号*/
    beep_dev.major = 0;/*给定设备号*/
    if(beep_dev.major)/*如果定义了设备号*/
    {
        beep_dev.devid = MKDEV(beep_dev.major,0);
        ret = register_chrdev_region(beep_dev.devid,BEEP_CNT,BEEP_NAME);
    }
    else/*没有定义设备号*/
    {
        ret = alloc_chrdev_region(&beep_dev.devid,0,BEEP_CNT,BEEP_NAME);
    }
    if(ret<0)
    {
        goto fail_register_dev;
    }
    else
    {
        beep_dev.major = MAJOR(beep_dev.devid);
        beep_dev.minor = MINOR(beep_dev.devid);
        printk("beep major=%d,minor = %d\r\n",beep_dev.major,beep_dev.minor);
    }
    /*创建字符设备cdev*/
    beep_dev.cdev.owner = THIS_MODULE;
    cdev_init(&beep_dev.cdev,&beep_fops);

    ret = cdev_add(&beep_dev.cdev,beep_dev.devid,BEEP_CNT);
	if(ret){
        goto fail_add_cdev;
	}
    /*创建类*/
    beep_dev.class = class_create(THIS_MODULE,BEEP_NAME);
    if(IS_ERR(beep_dev.class))
    {
        ret = PTR_ERR(beep_dev.class);
        goto fail_create_class;
    }
    /*创建设备*/
    beep_dev.device = device_create(beep_dev.class, NULL,beep_dev.devid,NULL,BEEP_NAME);
    if(IS_ERR(beep_dev.device))
    {
        ret = PTR_ERR(beep_dev.device);
        goto fail_create_device;
    }
    /*设置beep所使用的GPIO*/
    /*获取设备树节点:beep*/
    beep_dev.nd = of_find_node_by_path("/beep");
    if(beep_dev.nd == NULL)
    {
        ret = -EINVAL;
        goto fail_find_node;
    }
    /*获取设备树中的gpio属性，得到beep节点所使用的GPIO编号*/
    beep_dev.beep_gpio = of_get_named_gpio(beep_dev.nd,"beep-gpios",0);
    if(beep_dev.beep_gpio<0)
    {
        ret = -EINVAL;
        goto fail_find_gpio_num;
    }
    /*请求gpio*/
    ret = gpio_request(beep_dev.beep_gpio, "beep");
    if(ret)
    {
        goto fail_request_gpio;
    }
    /*设置GPIO5_IO01为输出，并且输出高电平，默认关闭beep*/
    ret = gpio_direction_output(beep_dev.beep_gpio,1);
    if(ret<0)
    {
        goto fail_set_gpio_dir;
    }
    return 0;
fail_set_gpio_dir:
    gpio_free(beep_dev.beep_gpio);
fail_request_gpio:

fail_find_gpio_num:

fail_find_node:
    device_destroy(beep_dev.class, beep_dev.devid);
fail_create_device:
    class_destroy(beep_dev.class);
fail_create_class:
    cdev_del(&beep_dev.cdev);
fail_add_cdev:
   unregister_chrdev_region(beep_dev.devid,BEEP_CNT); 
fail_register_dev:
    return ret;
}
/*驱动卸载*/
static void __exit beep_exit(void)
{
    /*关闭beep*/
    gpio_set_value(beep_dev.beep_gpio,1);
    /*释放gpio*/
    gpio_free(beep_dev.beep_gpio);
    /*注销设备号*/
    unregister_chrdev_region(beep_dev.devid,BEEP_CNT);
    /*删除cdev*/
    cdev_del(&beep_dev.cdev);
    /*摧毁设备*/
    device_destroy(beep_dev.class,beep_dev.devid);
    /*摧毁类*/
    class_destroy(beep_dev.class);
}
module_init(beep_init);
module_exit(beep_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("tanmingahng@goodix.com");

