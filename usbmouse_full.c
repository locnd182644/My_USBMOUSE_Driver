#include <linux/kernel.h>	 // Needed for KERN_INFO
#include <linux/slab.h>		 // Allocation/freeing memory in kernel space
#include <linux/module.h>	 // Needed by all modules
#include <linux/init.h>		 // Needed for the macros
#include <linux/usb/input.h> // Needed for device input
// #include <linux/hid.h> 		   // USB HID quirk support for Linux itdefines all USB_VENDOR_ID parameters
#include <linux/fs.h>		   // file_operations
#include <asm/segment.h>	   // Segment operation header file, which defines embedded assembly functions related to segment register operations.
#include <asm/uaccess.h>	   // Contains function definitions such as copy_to_user, copy_from_user and the kernel to access the memory address of the user process.
#include <linux/buffer_head.h> // holds all the information that the kernel needs to manipulate buffers.
#include <linux/device.h>	   // contains some sections that are device specific: interrupt numbers, features, data structures and the address mapping for device-specific peripherals of devices
#include <linux/cdev.h>		   // Utility Applications of the Control Device Interface
#include <linux/input.h> 

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
#define MY_USB_VEND_ID 0x0458
#define MY_USB_PROD_ID 0x003a

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE(DRIVER_LICENSE);

/* Serve the transfer of data to the device file */
// static char current_data = 0;

/* Mouse Description */
struct usb_mouse
{
	char name[128];
	char phys[64];
	struct usb_device *usbdev;
	struct input_dev *dev;
	struct urb *irq; // interrupt handler

	signed char *data;	 // interrupt buffer
	dma_addr_t data_dma; // dma address
};

/* Seve */
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

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = Mouse_open,
	.read = Mouse_read,
	.write = Mouse_write,
	.release = Mouse_close,
	.unlocked_ioctl = Mouse_ioclt,

};

/* When usb_submit_urb was successful return 0 & chuyen quyen kiem quat urb tu dirver -> USB core */
/* When the urb is completed in USB core-> set status of Urb -> Call complete fuction */
/* When complete fuction is called,
   the USB core is finished with the URB
   & control of it is now returned to the device driver. */

/* Complete Fuction */
static void usb_mouse_irq(struct urb *urb) // to build urb callback functions
{
	struct usb_mouse *mouse = urb->context;
	signed char *data = mouse->data;
	struct input_dev *dev = mouse->dev;
	int status;

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

	/* Chuyen data len tang tren de xu ly */
	input_report_key(dev, BTN_LEFT, data[0] & 0x01);   // Left mouse
	input_report_key(dev, BTN_RIGHT, data[0] & 0x02);  // Right mouse
	input_report_key(dev, BTN_MIDDLE, data[0] & 0x04); // Middle mouse

	input_report_rel(dev, REL_X, data[1]);	   // horizontal axis
	input_report_rel(dev, REL_Y, data[2]);	   // vertical axis
	input_report_rel(dev, REL_WHEEL, data[3]); // knob coordinates

	input_sync(dev); // sync data with input device

resubmit:
	/* Maintain the submoit urb -> USB_core to get data */
	status = usb_submit_urb(urb, GFP_ATOMIC);
	if (status)
		dev_err(&mouse->usbdev->dev,
				"can't resubmit intr, %s-%s/input0, status %d\n",
				mouse->usbdev->bus->bus_name,
				mouse->usbdev->devpath, status);

	// current_data = data[0];
	if (!(data[0] & 0x01) && !(data[0] & 0x02))
	{
		pr_info("No button pressed!\n");
		return; // Neither button pressed
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
}
// This function gets device data and send with urb
static int usb_mouse_open(struct input_dev *dev) // opens mouse devices.and submit its status to build urb
{
	struct usb_mouse *mouse = input_get_drvdata(dev);

	mouse->irq->dev = mouse->usbdev;
	if (usb_submit_urb(mouse->irq, GFP_KERNEL))
		return -EIO;

	return 0;
}

// This function gets device's data and cancel urb transmission
static void usb_mouse_close(struct input_dev *dev) // turns of usb device at the end of urb cycle
{
	struct usb_mouse *mouse = input_get_drvdata(dev);

	usb_kill_urb(mouse->irq);
}

/* Function  */
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

// // device file ops
// static int my_open(struct inode *i, struct file *f)
// {
// 	printk(KERN_INFO "Driver: open()\n");
// 	return 0;
// }
// static int my_close(struct inode *i, struct file *f)
// {
// 	printk(KERN_INFO "Driver: close()\n");
// 	return 0;
// }
// static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
// {
// 	printk(KERN_INFO "Driver: read()\n");

// 	return copy_to_user(buf, &current_data, 1) ? -EFAULT : 0; // Copy current click data to buffer
// 	current_data = 0;										  // Clear current data
// 	return 0;
// }
// static ssize_t my_write(struct file *f, const char __user *buf,
// 						size_t len, loff_t *off)
// {
// 	printk(KERN_INFO "Driver: write()\n");
// 	return len;
// }

static int usb_mouse_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(intf); // usb device type pointer
	struct usb_host_interface *interface;
	struct usb_endpoint_descriptor *endpoint; // contains infomations of endpoints
	struct usb_mouse *mouse;
	struct input_dev *input_dev;
	int pipe, maxp;
	int error = -ENOMEM; // memory overflow error

	interface = intf->cur_altsetting; // pointer to its current setting

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

	pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress); // create a pipeline to receive commands
	maxp = usb_maxpacket(dev, pipe, usb_pipeout(pipe));		// set the maximum packet for endpoint

	mouse = kzalloc(sizeof(struct usb_mouse), GFP_KERNEL); // device memory allocation
	input_dev = input_allocate_device();

	if (!mouse || !input_dev)
		goto fail1;

	mouse->data = usb_alloc_coherent(dev, 8, GFP_ATOMIC, &mouse->data_dma);
	if (!mouse->data)
		goto fail1;

	mouse->irq = usb_alloc_urb(0, GFP_KERNEL);
	if (!mouse->irq)
		goto fail2;

	mouse->usbdev = dev;
	mouse->dev = input_dev;

	if (dev->manufacturer) // get infomations of mouse's manufacturer
		strlcpy(mouse->name, dev->manufacturer, sizeof(mouse->name));

	if (dev->product)
	{ // get infomations of product
		if (dev->manufacturer)
			strlcat(mouse->name, " ", sizeof(mouse->name));
		strlcat(mouse->name, dev->product, sizeof(mouse->name));
	}

	if (!strlen(mouse->name)) // get productID, vendorID
		snprintf(mouse->name, sizeof(mouse->name),
				 "USB HIDBP Mouse %04x:%04x",
				 le16_to_cpu(dev->descriptor.idVendor),
				 le16_to_cpu(dev->descriptor.idProduct));

	// create path to sysfs and complete initialization of mouse input device
	usb_make_path(dev, mouse->phys, sizeof(mouse->phys));
	strlcat(mouse->phys, "/input0", sizeof(mouse->phys));

	input_dev->name = mouse->name;
	input_dev->phys = mouse->phys;
	usb_to_input_id(dev, &input_dev->id);

	input_dev->dev.parent = &intf->dev;
	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
	input_dev->keybit[BIT_WORD(BTN_MOUSE)] = BIT_MASK(BTN_LEFT) |
											 BIT_MASK(BTN_RIGHT) | BIT_MASK(BTN_MIDDLE);
	input_dev->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y);
	input_dev->keybit[BIT_WORD(BTN_MOUSE)] |= BIT_MASK(BTN_SIDE) |
											  BIT_MASK(BTN_EXTRA);
	input_dev->relbit[0] |= BIT_MASK(REL_WHEEL);

	input_set_drvdata(input_dev, mouse);

	input_dev->open = usb_mouse_open;
	input_dev->close = usb_mouse_close;

	/* Properly initialize a urb 	to be sent to a interrupt endpoint of a USB device */
	usb_fill_int_urb(mouse->irq, dev, pipe, mouse->data,
					 (maxp > 8 ? 8 : maxp),
					 usb_mouse_irq, mouse, endpoint->bInterval);

	mouse->irq->transfer_dma = mouse->data_dma;
	mouse->irq->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	error = input_register_device(mouse->dev);
	if (error)
		goto fail3;

	/* Save mouse data pointer -> this interface device */
	usb_set_intfdata(intf, mouse);

	/*==================================================/
	/* Register the character device driver */
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

	/*==================================================*/

/* jump to fail3 to free memory which is allocated for urb */
fail3:
	usb_free_urb(mouse->irq);

/* jump to fail2 in order to free buffer memory which is allocated by usb_alloc_coherent function */
fail2:
	usb_free_coherent(dev, 8, mouse->data, mouse->data_dma);

/* jump to fail1 if cannot allocate memory or buffer memory for mouse device, fail1 free memory for input device and mouse */
fail1:
	input_free_device(input_dev);
	kfree(mouse);
	return error;
}

// This function is called when the mouse is disconnected from the computer
static void usb_mouse_disconnect(struct usb_interface *intf)
{
	struct usb_mouse *mouse = usb_get_intfdata(intf);

	usb_set_intfdata(intf, NULL);
	if (mouse)
	{
		usb_kill_urb(mouse->irq);
		input_unregister_device(mouse->dev);
		usb_free_urb(mouse->irq);
		usb_free_coherent(interface_to_usbdev(intf), 8, mouse->data, mouse->data_dma);
		kfree(mouse);
	}

	/* if registered, unregister device */
	printk("Inside the __%s__ function\n", __FUNCTION__);
	cdev_del(Mouse_chrdev.dev_cdev);
	device_destroy(Mouse_chrdev.dev_class, Mouse_chrdev.dev_num);
	class_destroy(Mouse_chrdev.dev_class);
	/* Unregister the character device driver */
	unregister_chrdev_region(Mouse_chrdev.dev_num, 1);

	printk("Ket thuc thanh cong");
}

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

static struct usb_driver usb_mouse_driver = {
	.name = "usbmouse",
	.id_table = usb_mouse_id_table,
	.probe = usb_mouse_probe,
	.disconnect = usb_mouse_disconnect,
};

/* Register the struct usb_driver with the USB core */
module_usb_driver(usb_mouse_driver);
