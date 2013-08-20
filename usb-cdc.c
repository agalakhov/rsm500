#include "usb-cdc.h"

#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>

#include <stddef.h>
#include <stdint.h>
#include <errno.h>

#include "common.h"

#include "debug.h"

/*
 * Communication endpoints and interface
 */

static const struct {
	struct usb_cdc_header_descriptor header;
	struct usb_cdc_call_management_descriptor call_mgmt;
	struct usb_cdc_acm_descriptor acm;
	struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors = {
	.header = {
		.bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_HEADER,
		.bcdCDC = 0x0110,
	},
	.call_mgmt = {
		.bFunctionLength =
			sizeof(struct usb_cdc_call_management_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
		.bmCapabilities = 0,
		.bDataInterface = 1,
	},
	.acm = {
		.bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_ACM,
		.bmCapabilities = 0,
	},
	.cdc_union = {
		.bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_UNION,
		.bControlInterface = 0,
		.bSubordinateInterface0 = 1,
	 },
};

static const struct usb_endpoint_descriptor cdcacm_comm_endpoints[] = {
	{
		.bLength = sizeof(struct usb_endpoint_descriptor),
		.bDescriptorType = USB_DT_ENDPOINT,
		.bEndpointAddress = 0x83,
		.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
		.wMaxPacketSize = 16,
		.bInterval = 255,
	}
};

const struct usb_interface_descriptor cdcacm_comm_iface = {
	.bLength = sizeof(struct usb_interface_descriptor),
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bInterfaceClass = USB_CLASS_CDC,
	.bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
	.bInterfaceProtocol = USB_CDC_PROTOCOL_NONE,
	.iInterface = 0,

	.bNumEndpoints = ARRAY_SIZE(cdcacm_comm_endpoints),
	.endpoint = cdcacm_comm_endpoints,

	.extra = &cdcacm_functional_descriptors,
	.extralen = sizeof(cdcacm_functional_descriptors),
};

/*
 * Data endpoints and interface
 */

static const struct usb_endpoint_descriptor cdcacm_data_endpoints[] = {
	{
		.bLength = sizeof(struct usb_endpoint_descriptor),
		.bDescriptorType = USB_DT_ENDPOINT,
		.bEndpointAddress = 0x01,
		.bmAttributes = USB_ENDPOINT_ATTR_BULK,
		.wMaxPacketSize = 64,
		.bInterval = 1,
	}, {
		.bLength = sizeof(struct usb_endpoint_descriptor),
		.bDescriptorType = USB_DT_ENDPOINT,
		.bEndpointAddress = 0x82,
		.bmAttributes = USB_ENDPOINT_ATTR_BULK,
		.wMaxPacketSize = 64,
		.bInterval = 1,
	}
};

const struct usb_interface_descriptor cdcacm_data_iface = {
	.bLength = sizeof(struct usb_interface_descriptor),
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 1,
	.bAlternateSetting = 0,
	.bInterfaceClass = USB_CLASS_DATA,
	.bInterfaceSubClass = 0,
	.bInterfaceProtocol = 0,
	.iInterface = 0,

	.bNumEndpoints = ARRAY_SIZE(cdcacm_data_endpoints),
	.endpoint = cdcacm_data_endpoints,
};

/*
 * RX/TX handling
 */

struct cdcacm_io_rec {
	void *buf;
	size_t size;
	io_cb_t cb;
	intptr_t parm;
};

static usbd_device *cdcacm_dev = NULL;
bool cdcacm_ready = false;

static struct cdcacm_io_rec cdcacm_rx_rec = {
	.cb = NULL,
};

static struct cdcacm_io_rec cdcacm_tx_rec = {
	.cb = NULL,
};

static void cdcacm_data_rx_cb(usbd_device *dev, uint8_t ep)
{
	if (! cdcacm_rx_rec.cb)
		return;
	cdcacm_rx_rec.size = usbd_ep_read_packet(dev, ep,
			cdcacm_rx_rec.buf, cdcacm_rx_rec.size);
	cdcacm_rx_rec.cb(cdcacm_rx_rec.buf, cdcacm_rx_rec.size, cdcacm_rx_rec.parm);
	cdcacm_rx_rec.cb = NULL;
}

static void cdcacm_data_tx_cb(usbd_device *dev, uint8_t ep)
{
	(void) dev;
	(void) ep;
	if (! cdcacm_tx_rec.cb)
		return;
	cdcacm_tx_rec.cb(cdcacm_tx_rec.buf, cdcacm_tx_rec.size, cdcacm_tx_rec.parm);
	cdcacm_tx_rec.cb = NULL;
}

/*
 * Configuration
 */

static int cdcacm_control_request(usbd_device *dev, struct usb_setup_data *req,
		uint8_t **buf, uint16_t *len,
		void (**complete)(usbd_device *dev, struct usb_setup_data *req))
{
	(void)complete;
	(void)buf;
	(void)dev;

	gpio_toggle(DBGO, DBG_B);

	switch (req->bRequest) {
	case USB_CDC_REQ_SET_CONTROL_LINE_STATE:
		return 1;
	case USB_CDC_REQ_SET_LINE_CODING:
		if (*len < sizeof(struct usb_cdc_line_coding))
			return 0;
		return 1;
	default:
		return 0;
	}
}

static void cdcacm_set_config(usbd_device *dev, uint16_t wValue)
{
	(void)wValue;

	gpio_toggle(DBGO, DBG_R);

	usbd_ep_setup(dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_rx_cb);
	usbd_ep_setup(dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_tx_cb);
	usbd_ep_setup(dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

	usbd_register_control_callback(dev,
				USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
				cdcacm_control_request);

	cdcacm_ready = true;
}

/*
 * Public API
 */

void cdcacm_set_dev(usbd_device *dev)
{
	cdcacm_dev = dev;
	usbd_register_set_config_callback(dev, cdcacm_set_config);
}

static inline int cdcacm_setup_io(struct cdcacm_io_rec *io,
		void *buf, size_t size, io_cb_t cb, intptr_t p)
{
	if (! cdcacm_dev)
		return -ENODEV;
	if (! cdcacm_ready)
		return -EAGAIN;
	if (io->cb)
		return -EAGAIN;
	io->cb = cb;
	io->parm = p;
	io->buf = buf;
	io->size = size;
	return 0;
}

int cdcacm_read(void *buf, size_t size, io_cb_t cb, intptr_t p)
{
	int ret;
	ret = cdcacm_setup_io(&cdcacm_rx_rec, buf, size, cb, p);
	if (ret)
		return ret;
	return 0;
}

int cdcacm_write(const void *buf, size_t size, io_cb_t cb, intptr_t p)
{
	int ret;
	ret = cdcacm_setup_io(&cdcacm_tx_rec, (void *)buf, size, cb, p);
	if (ret)
		return ret;
	cdcacm_tx_rec.size = usbd_ep_write_packet(cdcacm_dev, 0x82, buf, size);
	return 0;
}

static void cdcacm_sync_cb(void *buf, size_t size, intptr_t p)
{
	(void) buf;
	*((int *)p) = size;
}

int cdcacm_read_sync(void *buf, size_t size)
{
	int ret;
	int cbret = -1;
	while (! cdcacm_ready)
		cdcacm_poll_usb();
	ret = cdcacm_read(buf, size, cdcacm_sync_cb, (intptr_t) &cbret);
	if (ret)
		return ret;
	while (cbret < 0)
		cdcacm_poll_usb();
	return cbret;
}

int cdcacm_write_sync(const void *buf, size_t size)
{
	int ret;
	int cbret = -1;
	while (! cdcacm_ready)
		cdcacm_poll_usb();
	ret = cdcacm_write(buf, size, cdcacm_sync_cb, (intptr_t) &cbret);
	if (ret)
		return ret;
	while (cbret < 0)
		cdcacm_poll_usb();
	return cbret;
}

void cdcacm_poll_usb(void)
{
	if (cdcacm_dev)
		usbd_poll(cdcacm_dev);
}
