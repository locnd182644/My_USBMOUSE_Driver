#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>

/*
 * Version Information
 */
#define DRIVER_VERSION "v1.10"
#define DRIVER_AUTHOR "Nguyen Duy Loc - 20182644"
#define DRIVER_DESC "My USB Mouse Info Driver"
#define DRIVER_LICENSE "GPL"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE(DRIVER_LICENSE);

static struct usb_device *device;

static int my_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    struct usb_host_interface *iface_desc;
    struct usb_endpoint_descriptor *endpoint;

    iface_desc = interface->cur_altsetting;
    printk(KERN_INFO "My USB Device i/f %d now probed: (%04X:%04X)\n",
            iface_desc->desc.bInterfaceNumber, id->idVendor, id->idProduct);
    printk(KERN_INFO "ID->bNumEndpoints: %02X\n",
            iface_desc->desc.bNumEndpoints);
    printk(KERN_INFO "ID->bInterfaceClass: %02X\n",
            iface_desc->desc.bInterfaceClass);
    int i;
    for (i = 0; i < iface_desc->desc.bNumEndpoints; i++)
    {
        endpoint = &iface_desc->endpoint[i].desc;
        printk(KERN_INFO "ED[%d]->bEndpointAddress: 0x%02X\n",
                i, endpoint->bEndpointAddress);
        printk(KERN_INFO "ED[%d]->bmAttributes: 0x%02X\n",
                i, endpoint->bmAttributes);
        printk(KERN_INFO "ED[%d]->wMaxPacketSize: 0x%04X (%d)\n",
                i, endpoint->wMaxPacketSize, endpoint->wMaxPacketSize);

    }
    device = interface_to_usbdev(interface);
    return 0;
}

static void my_disconnect(struct usb_interface *interface)
{
    /* Hủy các dữ liệu về thiết bị đã lưu trữ từ hàm thăm dò,
để làm điều này ta sẽ thiết lập dữ liệu NULL cho interface intf: */
    // usb_set_intfdata(intf, NULL);

    /* Hủy đăng kí lớp thiết bị: */
    // usb_deregister_dev(struct usb_interface* , struct usb_class_driver* );
    printk(KERN_INFO "My USB Device i/f %d now disconnected\n",
            interface->cur_altsetting->desc.bInterfaceNumber);
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