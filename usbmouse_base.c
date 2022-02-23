#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>

/*
 * Version Information
 */
#define DRIVER_VERSION "v1.10"
#define DRIVER_AUTHOR "Nguyen Duy Loc - 20182644"
#define DRIVER_DESC "My USB Mouse Driver"
#define DRIVER_LICENSE "GPL"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE(DRIVER_LICENSE);

/*  */
static int my_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
    printk("My USB Device <%04X:%04X> plugged \n", id->idVendor, id->idProduct);
    return 0;
}

static void my_disconnect(struct usb_interface *interface)
{
    printk("My USB Device unplugged\n");
}

static struct usb_device_id my_usbmouse_table[] = {
    {USB_DEVICE(0x0458, 0x0186)},
    {}};

MODULE_DEVICE_TABLE(usb, my_usbmouse_table);

static struct usb_driver my_driver = {
    .name = "My_USB_MOUSE",
    .id_table = my_usbmouse_table,
    .probe = my_probe,
    .disconnect = my_disconnect,
};

static int __init my_init(void)
{
    printk("Khoi tao My_usbmouse_Driver\n");
    return usb_register(&my_driver);
}

static void __exit my_exit(void)
{
    printk("Ket thuc My_usbmouse_Driver\n");
    usb_deregister(&my_driver);
}

module_init(my_init);

module_exit(my_exit);