#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>

/* Buoc de tao driver */

/*   B1: Register the character device driver | Number
     B2: Tao 1 lop cac thiet bi 
     B3: Tao thiet bi trong lop | Device file
     B4: Register entry point with kernel | Cdev  
*/


/* Creat file in dev */
// sudo  mknod  -m  666  /dev/device_name  c  Major  Minor

#define DRIVER_AUTHOR "Nguyen Duy Loc - 20182644"
#define DRIVER_DESC "Mouse character device driver"

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);    /* Who wrote this module? */
MODULE_DESCRIPTION(DRIVER_DESC); /* What does this module do */

struct Dev_info
{
     dev_t dev_num;
     struct class *dev_class;
     struct device *dev;
     struct cdev *dev_cdev;
} Mouse_chrdev;

static char buff_r[100] = "Mouse World\n";
static char buff_w[100];

static ssize_t Mouse_read(struct file *filp, char *buffer, size_t length, loff_t *offset);
static ssize_t Mouse_write(struct file *filp, const char __user *buffer, size_t length, loff_t *offset);
static int Mouse_open(struct inode *inodep, struct file *filep);
static int Mouse_close(struct inode *inodep, struct file *filep);
static long Mouse_ioclt(struct file *filp, unsigned int cmd, unsigned long arg);

// Cấu trúc để đăng ký các hàm qua driver interface được được qui định theo chuẩn

static struct file_operations fops = {

    .owner = THIS_MODULE,
    .open = Mouse_open,
    .read = Mouse_read,
    .write = Mouse_write,
    .release = Mouse_close,
    .unlocked_ioctl = Mouse_ioclt,

};

static int __init Mouse_init(void)
{
     printk("Inside the __%s__ function\n", __FUNCTION__);

     /* Register the character device driver */
     /* Static */
     // register_chrdev(240 /* Major Number */,
     //                 "Mouse chrdev module" /* Name of driver */,
     //                 &fops /* File operations */);

     /* Dynamic */
     if (alloc_chrdev_region(&Mouse_chrdev.dev_num, 0, 1, "Mouse chrdev module") < 0)
     {
          return -1;
     }
     printk(KERN_INFO "Khoi tao thanh cong__<Major, Minor>: <%d, %d>\n",
            MAJOR(Mouse_chrdev.dev_num), MINOR(Mouse_chrdev.dev_num));

     /* Creat device file */
     // Creat device class
     Mouse_chrdev.dev_class = class_create(THIS_MODULE, "class Mouse dev");
     if (Mouse_chrdev.dev_class == NULL)
     {
          printk("Failed to creat a device class\n");
          goto failed_creat_class;
     }

     // Creat device file
     Mouse_chrdev.dev = device_create(Mouse_chrdev.dev_class, NULL, Mouse_chrdev.dev_num, NULL, "Mouse_chrdev");
     if (Mouse_chrdev.dev == NULL)
     {
          printk("Failed to creat a device\n");
          goto failed_creat_device;
     }

     /* Register entry point with kernel */
     Mouse_chrdev.dev_cdev = cdev_alloc();
     if (Mouse_chrdev.dev_cdev == NULL)
     {
          printk("Failed to allocate cdev structure\n");
          goto failed_creat_struct_cdev;
     }
     cdev_init(Mouse_chrdev.dev_cdev, &fops);

     /* Once the cdev structure is set up, the final step is to tell the kernel about it with a call to: */
     if (cdev_add(Mouse_chrdev.dev_cdev, Mouse_chrdev.dev_num, 1) < 0)
     {
          printk("Fail to add charactor device to the system \n");
          goto faild_add_cdev;
     }

     printk("Khoi tao thanh cong :) Mouse_chrdev driver\n");
     return 0;

faild_add_cdev:
     cdev_del(Mouse_chrdev.dev_cdev);
failed_creat_struct_cdev:
     device_destroy(Mouse_chrdev.dev_class, Mouse_chrdev.dev_num);
failed_creat_device:
     class_destroy(Mouse_chrdev.dev_class);
failed_creat_class:
     unregister_chrdev_region(Mouse_chrdev.dev_num, 1);

     printk("Khoi tao that bai :( \n");
     return -1;
}

static void __exit Mouse_exit(void)
{
     printk("Inside the __%s__ function\n", __FUNCTION__);

     cdev_del(Mouse_chrdev.dev_cdev);
     device_destroy(Mouse_chrdev.dev_class, Mouse_chrdev.dev_num);
     class_destroy(Mouse_chrdev.dev_class);
     /* Unregister the character device driver */
     unregister_chrdev_region(Mouse_chrdev.dev_num, 1);

     printk("Ket thuc thanh cong");
}

static int Mouse_open(struct inode *inodep, struct file *filep)
{
     printk(KERN_INFO "Open Mouse\n");
     return 0;
}

static int Mouse_close(struct inode *inodep, struct file *filep)
{
     printk(KERN_INFO "Close Mouse\n");
     return 0;
}

// Hàm read được cung cấp qua driver interface  
static ssize_t Mouse_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset)
{
     printk(KERN_INFO "Call read\n");
     printk(KERN_INFO "Data to User: %s\n", buff_r);
     __copy_to_user(buffer, buff_r, sizeof(buff_r));
     return sizeof(buff_r);
}

static ssize_t Mouse_write(struct file *filp, const char __user *buffer, size_t length, loff_t *offset)
{
     printk(KERN_INFO "Call Mouse_write\n");
     __copy_from_user(buff_w, buffer, sizeof(buff_w));
     printk(KERN_INFO "Data from User: %s\n", buff_w);
     return sizeof(buff_w);
}

static long Mouse_ioclt(struct file *filp, unsigned int cmd, unsigned long arg)
{
     if (cmd == 100)
          printk(KERN_INFO "Success send cmd: %lu\n", arg);
     return 0;
}

module_init(Mouse_init);

module_exit(Mouse_exit);
