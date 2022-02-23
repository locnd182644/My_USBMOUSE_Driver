#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/fs.h>

/*
 * Version Information
 */
#define DRIVER_VERSION "v1.10"
#define DRIVER_AUTHOR "Nguyen Duy Loc - 20182644"
#define DRIVER_DESC "My USB Mouse Func Driver"
#define DRIVER_LICENSE "GPL"

/*
 * Vendor, Product ID
 */
#define MY_USB_VEND_ID 0x0458
#define MY_USB_PROD_ID 0x003a

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE(DRIVER_LICENSE);

static struct usb_device *device;
static struct usb_class_driver class;
static char current_data = 0;

// device file ops
static int my_open(struct inode *i, struct file *f)
{
    printk(KERN_INFO "Driver: open()\n");
    return 0;
}
static int my_close(struct inode *i, struct file *f)
{
    printk(KERN_INFO "Driver: close()\n");
    return 0;
}
static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
    printk(KERN_INFO "Driver: read()\n");

    return copy_to_user(buf, &current_data, 1) ? -EFAULT : 0; // Copy current click data to buffer
    current_data = 0;                                         // Clear current data
    return 0;
}
static ssize_t my_write(struct file *f, const char __user *buf,
                        size_t len, loff_t *off)
{
    printk(KERN_INFO "Driver: write()\n");
    return len;
}
static struct file_operations pugs_fops =
    {
        .owner = THIS_MODULE,
        .open = my_open,
        .release = my_close,
        .read = my_read,
        .write = my_write};

static int my_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    /* Initialize any local structures */
    struct usb_host_interface *iface_desc = interface->cur_altsetting;
    struct usb_endpoint_descriptor *endpoint;

    printk(KERN_INFO "My USB Device i/f %d now probed: (%04X:%04X)\n",
           iface_desc->desc.bInterfaceNumber, id->idVendor, id->idProduct);
    printk(KERN_INFO "ID->bNumEndpoints: %02X\n",
           iface_desc->desc.bNumEndpoints);
    printk(KERN_INFO "ID->bInterfaceClass: %02X\n",
           iface_desc->desc.bInterfaceClass);

    /* Truy nhập các thông tin về Endpoint */
    int i;
    for (i = 0; i < iface_desc->desc.bNumEndpoints; i++)
    {
        endpoint = &iface_desc->endpoint[i].desc;
        printk(KERN_INFO "ED[%d]->bDescriptorType: 0x%02X\n",
               i, endpoint->bDescriptorType);
        printk(KERN_INFO "ED[%d]->bInterval: 0x%02X\n",
               i, endpoint->bInterval);
        printk(KERN_INFO "ED[%d]->bEndpointAddress: 0x%02X\n",
               i, endpoint->bEndpointAddress);
        printk(KERN_INFO "ED[%d]->bmAttributes: 0x%02X\n",
               i, endpoint->bmAttributes);
        printk(KERN_INFO "ED[%d]->wMaxPacketSize: 0x%04X (%d)\n",
               i, endpoint->wMaxPacketSize, endpoint->wMaxPacketSize);
    }

    /* save our data pointer in this interface device */
    usb_set_intfdata(interface, device);

    class.name = "mymouse_class";
    class.fops = &pugs_fops;
    class.minor_base = 91;

    /* Register Device class */
    usb_register_dev(interface, &class);

    return 0;
}

static void my_disconnect(struct usb_interface *interface)
{
    printk(KERN_INFO "My USB Device i/f %d now disconnected\n",
           interface->cur_altsetting->desc.bInterfaceNumber);

    /* Hủy các dữ liệu về thiết bị đã lưu trữ từ hàm thăm dò,
để làm điều này ta sẽ thiết lập dữ liệu NULL cho interface intf: */
    usb_set_intfdata(interface, NULL);

    /* Hủy đăng kí lớp thiết bị: */
    usb_deregister_dev(interface, &class);
}

static struct usb_device_id my_usbmouse_table[] = {
    {USB_DEVICE(MY_USB_VEND_ID, MY_USB_PROD_ID)},
    {}};

MODULE_DEVICE_TABLE(usb, my_usbmouse_table);

static struct usb_driver my_driver = {
    .name = "mymouse_func",
    .id_table = my_usbmouse_table,
    .probe = my_probe,
    .disconnect = my_disconnect,
};

static int __init my_init(void)
{
    printk("Khoi tao My_usbmouse_Driver\n");
    int result;
    result = usb_register(&my_driver);
    if (result)
        printk("usb_register failed. Error number %d", result);
    return result;
}

static void __exit my_exit(void)
{
    printk("Ket thuc My_usbmouse_Driver\n");
    usb_deregister(&my_driver);
}

module_init(my_init);

module_exit(my_exit);