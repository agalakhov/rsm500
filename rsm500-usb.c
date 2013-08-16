#include "rsm500-usb.h"

#include "usb-cdc.h"

#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/stm32/f1/gpio.h>

#include "common.h"
#include <stddef.h>
#include <stdint.h>

/*
 * Device identification
 */

static const char *rsm500_usb_strings[] = {
	"Institute of Metal Physics",
	"RSM-500 X-ray Spectrometer",
	"0001",
};

enum {
	VID = 0x1eaf,
	PID = 0x0500,
	REV = 0x0001,
	MAX_CURRENT = 100, /* mA */
};

/*
 * General USB descriptors
 */

static const struct usb_interface_descriptor rsm500_comm_iface;
static const struct usb_interface_descriptor rsm500_data_iface;

static const struct usb_device_descriptor rsm500_usb_dev = {
	.bLength = sizeof(struct usb_device_descriptor),
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,	/* USB 2.0 */

	.bDeviceClass = USB_CLASS_CDC,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,

	.idVendor = VID,
	.idProduct = PID,
	.bcdDevice = REV,

	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,

	.bNumConfigurations = 1,
};

static const struct usb_interface rsm500_usb_ifaces[] = {
	{
		.num_altsetting = 1,
		.altsetting = &cdcacm_comm_iface,
	}, {
		.num_altsetting = 1,
		.altsetting = &cdcacm_data_iface,
	}
};

static const struct usb_config_descriptor rsm500_usb_config = {
	.bLength = sizeof(struct usb_config_descriptor),
	.bDescriptorType = USB_DT_CONFIGURATION,

	.wTotalLength = 0,

	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bNumInterfaces = ARRAY_SIZE(rsm500_usb_ifaces),
	.interface = rsm500_usb_ifaces,

	.bmAttributes = 0x80,	/* Nothing */
	.bMaxPower = MAX_CURRENT / 2,
};

/*
 * Public interface
 */

static uint8_t usbd_control_buffer[128];

usbd_device *rsm500_usb_init(void)
{
	usbd_device *dev;

	/* Enable the pull-up transistor at D+ line */
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ,
		      GPIO_CNF_OUTPUT_OPENDRAIN, GPIO9);
	/* active low */
	gpio_clear(GPIOB, GPIO9);

	dev = usbd_init(&stm32f103_usb_driver,
			&rsm500_usb_dev, &rsm500_usb_config,
			rsm500_usb_strings, ARRAY_SIZE(rsm500_usb_strings),
			usbd_control_buffer, sizeof(usbd_control_buffer));

	cdcacm_set_dev(dev);

	return dev;
}
