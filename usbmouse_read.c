/*
 *  Solution: Device have 2 interface   (intr 0: 1 EP, intf 1: 0EP)
 */

#include <linux/kernel.h>    // Needed for KERN_INFO
#include <linux/slab.h>      // Allocation/freeing memory in kernel space
#include <linux/module.h>    // Needed by all modules
#include <linux/init.h>      // Needed for the macros
#include <linux/usb/input.h> // Needed for device input
// #include <linux/hid.h> 		   // USB HID quirk support for Linux itdefines all USB_VENDOR_ID parameters
#include <linux/fs.h>          // file_operations
#include <asm/segment.h>       // Segment operation header file, which defines embedded assembly functions related to segment register operations.
#include <asm/uaccess.h>       // Contains function definitions such as copy_to_user, copy_from_user and the kernel to access the memory address of the user process.
#include <linux/buffer_head.h> // holds all the information that the kernel needs to manipulate buffers.
#include <linux/device.h>      // contains some sections that are device specific: interrupt numbers, features, data structures and the address mapping for device-specific peripherals of devices
#include <linux/cdev.h>        // Utility Applications of the Control Device Interface
#include <linux/string.h>      // Handler String
#include <linux/delay.h>

/*
 * Version Information
 */
#define DRIVER_VERSION "v1.10"
#define DRIVER_AUTHOR "Nguyen Duy Loc - 20182644"
#define DRIVER_DESC "My USB Mouse Driver"
#define DRIVER_LICENSE "GPL"

/*
 * Vendor, Product ID
 */
/* Mouse 1 */
// #define MY_USB_VEND_ID 0x0458
// #define MY_USB_PROD_ID 0x003a

/* Mouse 2 */
#define MY_USB_VEND_ID 0x0458
#define MY_USB_PROD_ID 0x0186

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE(DRIVER_LICENSE);

/* Mouse Description */
struct usb_mouse
{
    // char name[128];
    // char phys[64];
    struct usb_device *usbdev;
    struct urb *irq; // interrupt handler

    signed char *data;   // interrupt buffer
    dma_addr_t data_dma; // dma address
};

/*======================================================================*/
static char buff_r[100] = "Mouse World\n"; // Buffer store data from driver -> user
static char buff_w[100];                   // Buffer store data from user -> driver
static int retval;                         // return value
static char *data;                         // event data of mouse
/*======================================================================*/

/*======================================================================*/
static ssize_t Mouse_read(struct file *, char *, size_t, loff_t *);
static ssize_t Mouse_write(struct file *, const char __user *, size_t, loff_t *offset);
static int Mouse_open(struct inode *, struct file *filep);
static int Mouse_close(struct inode *, struct file *);
static long Mouse_ioclt(struct file *, unsigned int, unsigned long);
static int usb_mouse_probe(struct usb_interface *, const struct usb_device_id *);
static void usb_mouse_disconnect(struct usb_interface *);
static void usb_mouse_irq(struct urb *);
/*======================================================================*/

/*========================= Add class struct ===========================*/
static struct file_operations mouse_fop = {
    .owner = THIS_MODULE,
    .open = Mouse_open,
    .read = Mouse_read,
    .write = Mouse_write,
    .release = Mouse_close,
    .unlocked_ioctl = Mouse_ioclt,
};

static struct usb_class_driver mouse_class = {
    .name = "my_usbmouse",
    .fops = &mouse_fop,
};
/*===================================================================*/

/*======================= Set id_table variable - (VENDOR ID, PRODUC ID) =====================*/
// /* Automic */
// static struct usb_device_id usb_mouse_id_table [] = {
// 	{ USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,
// 		USB_INTERFACE_PROTOCOL_MOUSE) },
// 	{ }	/* Terminating entry */
// };

/* 	If id_table variable is not set, the
    probe function callback in the USB driver is never called. */
/* Specify VENDOR, PRODUC ID */
static struct usb_device_id usb_mouse_id_table[] = {
    {USB_DEVICE(MY_USB_VEND_ID, MY_USB_PROD_ID)},
    {} /* Terminating entry */
};
MODULE_DEVICE_TABLE(usb, usb_mouse_id_table);
/*===================================================================*/

/*============ Register the struct usb_driver with the USB core ==============*/
static struct usb_driver usb_mouse_driver = {
    .name = "my_usbmouse",
    .id_table = usb_mouse_id_table,
    .probe = usb_mouse_probe,
    .disconnect = usb_mouse_disconnect,
};
module_usb_driver(usb_mouse_driver);
/*===================================================================*/

/*============ Build urb callback functions - Complete function ==============*/
/* When usb_submit_urb was successful return 0 & chuyen quyen kiem quat urb tu dirver -> USB core */
/* When the urb is completed in USB core-> set status of Urb -> Call complete fuction */
/* When complete fuction is called,
   the USB core is finished with the URB
   & control of it is now returned to the device driver. */

/* Complete Fuction */
static void usb_mouse_irq(struct urb *urb) // to build urb callback functions
{
    struct usb_mouse *mouse = urb->context;
    data = mouse->data;
    int status;
    int i;
    switch (urb->status)
    {
    case 0: /* success */
        break;
    case -ECONNRESET: /* unlink */
    case -ENOENT:
    case -ESHUTDOWN:
        return;
    /* -EPIPE:  should clear the halt */
    default: /* error */
        goto resubmit;
    }

resubmit:
    /* Maintain the submoit urb -> USB_core to get data */
    status = usb_submit_urb(urb, GFP_ATOMIC);
    if (status)
        dev_err(&mouse->usbdev->dev,
                "can't resubmit intr, %s-%s/input0, status %d\n",
                mouse->usbdev->bus->bus_name,
                mouse->usbdev->devpath, status);

    /* Handler event in kernel -> dmesg */
    if (!(data[0] & 0x01) && !(data[0] & 0x02) && !(data[0] & 0x04))
    {
        pr_info("No button pressed!\n");
    }

    // check which button pressed
    if (data[0] & 0x01)
    {
        pr_info("Left mouse button clicked!\n");
    }
    else if (data[0] & 0x02)
    {
        pr_info("Right mouse button clicked!\n");
    }
    else if (data[0] & 0x04)
    {
        pr_info("Middle mouse button clicked!\n");
    }

    if (data[3] == 1)
    {
        pr_info("Roll Up!\n");
    }
    else if (data[3] == -1)
    {
        pr_info("Roll Down!\n");
    }

    for (i = 0; i < 8; i++)
    {
        pr_info("Byte[%d]: %d", i, data[i]);
    }
}

// Bonus: This function send event -> User_process -> Display
/*===================================================================*/

/* Function  */
static int Mouse_open(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "\n----------------------> Open Mouse\n");
    struct usb_mouse *mouse;
    struct usb_interface *interface;

    /* Get minor number by inode */
    int subminor = iminor(inodep);
    /*...........................*/

    /* Get interface by minor number */
    interface = usb_find_interface(&usb_mouse_driver, subminor);
    if (!interface)
    {
        printk(KERN_ERR "%s: error, can't find device for minor %d",
               __FUNCTION__, subminor);
        return -ENODEV;
    }
    mouse = usb_get_intfdata(interface); // get struct mouse from interface
    mouse->irq->dev = mouse->usbdev;     // ????????????????
    if (usb_submit_urb(mouse->irq, GFP_KERNEL))
        return -EIO;
    printk(KERN_INFO "----------------------> Open Mouse Complete \n\n");
    return 0;
}

static int Mouse_close(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "----------------------> Close Mouse\n");
    struct usb_mouse *mouse;
    struct usb_interface *interface;

    /* Get minor number by inode */
    int subminor = iminor(inodep);
    /*...........................*/

    /* Get minor number by class */
    interface = usb_find_interface(&usb_mouse_driver, subminor);
    if (!interface)
    {
        printk(KERN_ERR "%s: error, can't find device for minor %d",
               __FUNCTION__, subminor);
        return -ENODEV;
    }
    mouse = usb_get_intfdata(interface); // get device from interface
    usb_kill_urb(mouse->irq);
    printk(KERN_INFO "----------------------> Close Mouse Complete \n\n");
    return 0;
}

// Hàm read được cung cấp qua driver interface
static ssize_t Mouse_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset)
{
    int i;
    for ( i = 0; i < 8; i++)
    {
        buff_r[i] = *(data + i) + 48;
    }

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

static int usb_mouse_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
    struct usb_device *dev = interface_to_usbdev(intf); // usb device type pointer
    struct usb_mouse *mouse;
    struct usb_host_interface *interface = intf->cur_altsetting; // pointer to its current setting
    struct usb_endpoint_descriptor *endpoint;                    // contains infomations of endpoints
    int pipe, maxp;

    /* Information Device */
    printk(KERN_INFO "My USB Device i/f %d now probed: (%04X:%04X)\n",
           interface->desc.bInterfaceNumber, id->idVendor, id->idProduct);
    printk(KERN_INFO "ID->bNumEndpoints: %02X\n",
           interface->desc.bNumEndpoints);
    printk(KERN_INFO "ID->bInterfaceClass: %02X\n",
           interface->desc.bInterfaceClass);

    /* Truy nhập các thông tin về Endpoint - Only one Endpoint */
    int i;
    for (i = 0; i < interface->desc.bNumEndpoints; i++)
    {
        endpoint = &interface->endpoint[0].desc;
        if (!usb_endpoint_is_int_in(endpoint))
            return -ENODEV;
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
    if (i == 0)
    {
        printk(KERN_INFO "Interface haven't Endpoint \n");
        return -1;
    }

    /********************* Creat device file with name in class ************************/
    /* we can register the device now, as it is ready */
    /* Register minor number with major number avalible - 180 usb  */
    retval = usb_register_dev(intf, &mouse_class);
    if (retval)
    {
        /* something prevented us from registering this driver */
        printk(KERN_ERR "%s: error. Not able to get a minor for this device.", __FUNCTION__);
        usb_set_intfdata(intf, NULL);
        return -1;
    }
    else
    {
        printk(KERN_INFO "Minor obtained: %d\n", intf->minor);
    }
    /*************************************************************************************/

    pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress); // create a pipeline to receive commands
    maxp = usb_maxpacket(dev, pipe, usb_pipeout(pipe));     // set the maximum packet for endpoint

    mouse = kzalloc(sizeof(struct usb_mouse), GFP_KERNEL); // device memory allocation

    if (!mouse)
        goto fail1;

    mouse->data = usb_alloc_coherent(dev, 8, GFP_ATOMIC, &mouse->data_dma);
    if (!mouse->data)
        goto fail1;

    mouse->irq = usb_alloc_urb(0, GFP_KERNEL);
    if (!mouse->irq)
        goto fail2;

    mouse->usbdev = dev;

    /* Properly initialize a urb 	to be sent to a interrupt endpoint of a USB device */
    usb_fill_int_urb(mouse->irq, dev, pipe, mouse->data,
                     (maxp > 8 ? 8 : maxp),
                     usb_mouse_irq, mouse, endpoint->bInterval);

    mouse->irq->transfer_dma = mouse->data_dma;
    mouse->irq->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

    /* Save mouse data pointer -> this interface device */
    usb_set_intfdata(intf, mouse);

    printk("Khoi tao thanh cong :) Mouse_chrdev driver\n");
    return 0;

    /*==================================================*/

/* jump to fail2 in order to free buffer memory which is allocated by usb_alloc_coherent function */
fail2:
    usb_free_coherent(dev, 8, mouse->data, mouse->data_dma);

/* jump to fail1 if cannot allocate memory or buffer memory for mouse device, fail1 free memory for input device and mouse */
fail1:
    kfree(mouse);
    return -ENOMEM; // memory overflow error;
}

// This function is called when the mouse is disconnected from the computer
static void usb_mouse_disconnect(struct usb_interface *intf)
{
    struct usb_mouse *mouse = usb_get_intfdata(intf);

    usb_set_intfdata(intf, NULL);

    /* give back our minor */
    /* Clean device file */
    usb_deregister_dev(intf, &mouse_class);
    if (mouse)
    {
        usb_kill_urb(mouse->irq);
        usb_free_urb(mouse->irq);
        usb_free_coherent(interface_to_usbdev(intf), 8, mouse->data, mouse->data_dma);
        kfree(mouse);
    }

    printk("Ket thuc thanh cong");
}
