/*
    USB descriptor macros for CDC

    Copyright (C) 2015,2016,2018 Peter Lawrence

    Permission is hereby granted, free of charge, to any person obtaining a 
    copy of this software and associated documentation files (the "Software"), 
    to deal in the Software without restriction, including without limitation 
    the rights to use, copy, modify, merge, publish, distribute, sublicense, 
    and/or sell copies of the Software, and to permit persons to whom the 
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in 
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
    DEALINGS IN THE SOFTWARE.
*/

#ifndef __CDC_HELPER_H
#define __CDC_HELPER_H

#include <stdint.h>
#include "usbhelper.h"

#define CDC_CMD_PACKET_SIZE      8
#define USB_FS_MAX_PACKET_SIZE   64

struct cdc_interface
{
  struct interface_association_descriptor cdc_association;
  struct interface_descriptor             ctl_interface;
  struct cdc_functional_descriptor_header cdc_acm_header;
  struct cdc_cm_functional_descriptor     cdc_cm;
  struct cdc_acm_functional_descriptor    cdc_acm;
  struct cdc_union_functional_descriptor  cdc_union;
  struct endpoint_descriptor              ctl_ep;
  struct interface_descriptor             dat_interface;
  struct endpoint_descriptor              ep_in;
  struct endpoint_descriptor              ep_out;
};

/* macro to help generate CDC ACM USB descriptors */

#define CDC_DESCRIPTOR(COMMAND_ITF, DATA_ITF, COMMAND_EP, DATAOUT_EP, DATAIN_EP) \
    { \
      .cdc_association = { \
        /*Interface Association Descriptor */ \
        .bLength            = sizeof(struct interface_association_descriptor), /* Interface Association Descriptor size */ \
        .bDescriptorType    = USB_INTERFACE_ASSOCIATION_DESCRIPTOR,            /* Interface Association */ \
        .bFirstInterface    = COMMAND_ITF,                                     /* First Interface of Association */ \
        .bInterfaceCount    = 0x02,                                            /* quantity of interfaces in association */ \
        .bFunctionClass     = 0x02,                                            /* Communication Interface Class */ \
        .bFunctionSubClass  = 0x02,                                            /* Abstract Control Model */ \
        .bFunctionProtocol  = 0x01,                                            /* Common AT commands */ \
        .iFunction          = 0x00,                                            \
      }, \
 \
      .ctl_interface = { \
        /*Interface Descriptor */ \
        .bLength            = sizeof(struct interface_descriptor),             /* Interface Descriptor size */ \
        .bDescriptorType    = USB_INTERFACE_DESCRIPTOR,                        /* Interface */ \
        .bInterfaceNumber   = COMMAND_ITF,                                     /* Number of Interface */ \
        .bAlternateSetting  = 0x00,                                            /* Alternate setting */ \
        .bNumEndpoints      = 0x01,                                            /* One endpoints used */ \
        .bInterfaceClass    = 0x02,                                            /* Communication Interface Class */ \
        .bInterfaceSubclass = 0x02,                                            /* Abstract Control Model */ \
        .bInterfaceProtocol = 0x01,                                            /* Common AT commands */ \
        .iInterface         = 0x00,                                            \
      }, \
 \
      .cdc_acm_header = { \
        /*Header Functional Descriptor*/ \
        .bFunctionLength    = sizeof(struct cdc_functional_descriptor_header), /* Endpoint Descriptor size */ \
        .bDescriptorType    = 0x24,                                            /* CS_INTERFACE */ \
        .bDescriptorSubtype = 0x00,                                            /* Header Func Desc */ \
        .bcdCDC             = USB_UINT16(0x0110),                              /* spec release number */ \
      }, \
 \
      .cdc_cm = { \
        /*Call Management Functional Descriptor*/ \
        .bFunctionLength    = sizeof(struct cdc_cm_functional_descriptor),     \
        .bDescriptorType    = 0x24,                                            /* CS_INTERFACE */ \
        .bDescriptorSubtype = 0x01,                                            /* Call Management Func Desc */ \
        .bmCapabilities     = 0x00,                                            /* D0+D1 */ \
        .bDataInterface     = DATA_ITF,                                        \
      }, \
 \
      .cdc_acm = { \
        /*ACM Functional Descriptor*/ \
        .bFunctionLength    = sizeof(struct cdc_acm_functional_descriptor),    \
        .bDescriptorType    = 0x24,                                            /* CS_INTERFACE */ \
        .bDescriptorSubtype = 0x02,                                            /* Abstract Control Management desc */ \
        .bmCapabilities     = 0x02,                                            \
      }, \
 \
      .cdc_union = { \
        /*Union Functional Descriptor*/ \
        .bFunctionLength    = sizeof(struct cdc_union_functional_descriptor),  \
        .bDescriptorType    = 0x24,                                            /* CS_INTERFACE */ \
        .bDescriptorSubtype = 0x06,                                            /* Union func desc */ \
        .bMasterInterface   = COMMAND_ITF,                                     /* Communication class interface */ \
        .bSlaveInterface0   = DATA_ITF,                                        /* Data Class Interface */ \
      }, \
 \
      .ctl_ep = { \
        /* Command Endpoint Descriptor*/ \
        .bLength            = sizeof(struct endpoint_descriptor),              /* Endpoint Descriptor size */ \
        .bDescriptorType    = USB_ENDPOINT_DESCRIPTOR,                         /* Endpoint */ \
        .bEndpointAddress   = COMMAND_EP,                                      \
        .bmAttributes       = 0x03,                                            /* Interrupt */ \
        .wMaxPacketSize     = USB_UINT16(CDC_CMD_PACKET_SIZE),                 \
        .bInterval          = 0x10,                                            \
      }, \
 \
      { \
        /*Data class interface descriptor*/ \
        .bLength            = sizeof(struct interface_descriptor),             /* Endpoint Descriptor size */ \
        .bDescriptorType    = USB_INTERFACE_DESCRIPTOR,                        \
        .bInterfaceNumber   = DATA_ITF,                                        /* Number of Interface */ \
        .bAlternateSetting  = 0x00,                                            /* Alternate setting */ \
        .bNumEndpoints      = 0x02,                                            /* Two endpoints used */ \
        .bInterfaceClass    = 0x0A,                                            /* CDC */ \
        .bInterfaceSubclass = 0x00,                                            \
        .bInterfaceProtocol = 0x00,                                            \
        .iInterface         = 0x00,                                            \
      }, \
 \
      .ep_in = { \
        /* Data Endpoint OUT Descriptor */ \
        .bLength            = sizeof(struct endpoint_descriptor),              /* Endpoint Descriptor size */ \
        .bDescriptorType    = USB_ENDPOINT_DESCRIPTOR,                         /* Endpoint */ \
        .bEndpointAddress   = DATAOUT_EP,                                      \
        .bmAttributes       = 0x02,                                            /* Bulk */ \
        .wMaxPacketSize     = USB_UINT16(USB_FS_MAX_PACKET_SIZE),              \
        .bInterval          = 0x00,                                            /* ignore for Bulk transfer */ \
      }, \
 \
      .ep_out = { \
        /* Data Endpoint IN Descriptor*/ \
        .bLength            = sizeof(struct endpoint_descriptor),              /* Endpoint Descriptor size */ \
        .bDescriptorType    = USB_ENDPOINT_DESCRIPTOR,                         /* Endpoint */ \
        .bEndpointAddress   = DATAIN_EP,                                       \
        .bmAttributes       = 0x02,                                            /* Bulk */ \
        .wMaxPacketSize     = USB_UINT16(USB_FS_MAX_PACKET_SIZE),              \
        .bInterval          = 0x00                                             /* ignore for Bulk transfer */ \
      } \
    },

#define RNDIS_DESCRIPTOR(COMMAND_ITF, DATA_ITF, COMMAND_EP, DATAOUT_EP, DATAIN_EP) \
    { \
      .cdc_association = { \
        /*Interface Association Descriptor */ \
        .bLength            = sizeof(struct interface_association_descriptor), /* Interface Association Descriptor size */ \
        .bDescriptorType    = USB_INTERFACE_ASSOCIATION_DESCRIPTOR,            /* Interface Association */ \
        .bFirstInterface    = COMMAND_ITF,                                     /* First Interface of Association */ \
        .bInterfaceCount    = 0x02,                                            /* quantity of interfaces in association */ \
        .bFunctionClass     = 224,                                             /* Wireless */ \
        .bFunctionSubClass  = 0x01,                                            /* Radio Frequency */ \
        .bFunctionProtocol  = 0x03,                                            /* RNDIS */ \
        .iFunction          = 0x00,                                            \
      }, \
 \
      .ctl_interface = { \
        /*Interface Descriptor */ \
        .bLength            = sizeof(struct interface_descriptor),             /* Interface Descriptor size */ \
        .bDescriptorType    = USB_INTERFACE_DESCRIPTOR,                        /* Interface */ \
        .bInterfaceNumber   = COMMAND_ITF,                                     /* Number of Interface */ \
        .bAlternateSetting  = 0x00,                                            /* Alternate setting */ \
        .bNumEndpoints      = 0x01,                                            /* One endpoints used */ \
        .bInterfaceClass    = 224,                                             /* Wireless */ \
        .bInterfaceSubclass = 0x01,                                            /* Radio Frequency */ \
        .bInterfaceProtocol = 0x03,                                            /* RNDIS */ \
        .iInterface         = 0x00,                                            \
      }, \
 \
      .cdc_acm_header = { \
        /*Header Functional Descriptor*/ \
        .bFunctionLength    = sizeof(struct cdc_functional_descriptor_header), /* Endpoint Descriptor size */ \
        .bDescriptorType    = 0x24,                                            /* CS_INTERFACE */ \
        .bDescriptorSubtype = 0x00,                                            /* Header Func Desc */ \
        .bcdCDC             = USB_UINT16(0x0110),                              /* spec release number */ \
      }, \
 \
      .cdc_cm = { \
        /*Call Management Functional Descriptor*/ \
        .bFunctionLength    = sizeof(struct cdc_cm_functional_descriptor),     \
        .bDescriptorType    = 0x24,                                            /* CS_INTERFACE */ \
        .bDescriptorSubtype = 0x01,                                            /* Call Management Func Desc */ \
        .bmCapabilities     = 0x00,                                            /* D0+D1 */ \
        .bDataInterface     = DATA_ITF,                                        \
      }, \
 \
      .cdc_acm = { \
        /*ACM Functional Descriptor*/ \
        .bFunctionLength    = sizeof(struct cdc_acm_functional_descriptor),    \
        .bDescriptorType    = 0x24,                                            /* CS_INTERFACE */ \
        .bDescriptorSubtype = 0x02,                                            /* Abstract Control Management desc */ \
        .bmCapabilities     = 0x00,                                            \
      }, \
 \
      .cdc_union = { \
        /*Union Functional Descriptor*/ \
        .bFunctionLength    = sizeof(struct cdc_union_functional_descriptor),  \
        .bDescriptorType    = 0x24,                                            /* CS_INTERFACE */ \
        .bDescriptorSubtype = 0x06,                                            /* Union func desc */ \
        .bMasterInterface   = COMMAND_ITF,                                     /* Communication class interface */ \
        .bSlaveInterface0   = DATA_ITF,                                        /* Data Class Interface */ \
      }, \
 \
      .ctl_ep = { \
        /* Command Endpoint Descriptor*/ \
        .bLength            = sizeof(struct endpoint_descriptor),              /* Endpoint Descriptor size */ \
        .bDescriptorType    = USB_ENDPOINT_DESCRIPTOR,                         /* Endpoint */ \
        .bEndpointAddress   = COMMAND_EP,                                      \
        .bmAttributes       = 0x03,                                            /* Interrupt */ \
        .wMaxPacketSize     = USB_UINT16(CDC_CMD_PACKET_SIZE),                 \
        .bInterval          = 0x01,                                            \
      }, \
 \
      { \
        /*Data class interface descriptor*/ \
        .bLength            = sizeof(struct interface_descriptor),             /* Endpoint Descriptor size */ \
        .bDescriptorType    = USB_INTERFACE_DESCRIPTOR,                        \
        .bInterfaceNumber   = DATA_ITF,                                        /* Number of Interface */ \
        .bAlternateSetting  = 0x00,                                            /* Alternate setting */ \
        .bNumEndpoints      = 0x02,                                            /* Two endpoints used */ \
        .bInterfaceClass    = 0x0A,                                            /* CDC */ \
        .bInterfaceSubclass = 0x00,                                            \
        .bInterfaceProtocol = 0x00,                                            \
        .iInterface         = 0x00,                                            \
      }, \
 \
      .ep_in = { \
        /* Data Endpoint OUT Descriptor */ \
        .bLength            = sizeof(struct endpoint_descriptor),              /* Endpoint Descriptor size */ \
        .bDescriptorType    = USB_ENDPOINT_DESCRIPTOR,                         /* Endpoint */ \
        .bEndpointAddress   = DATAOUT_EP,                                      \
        .bmAttributes       = 0x02,                                            /* Bulk */ \
        .wMaxPacketSize     = USB_UINT16(USB_FS_MAX_PACKET_SIZE),              \
        .bInterval          = 0x00,                                            /* ignore for Bulk transfer */ \
      }, \
 \
      .ep_out = { \
        /* Data Endpoint IN Descriptor*/ \
        .bLength            = sizeof(struct endpoint_descriptor),              /* Endpoint Descriptor size */ \
        .bDescriptorType    = USB_ENDPOINT_DESCRIPTOR,                         /* Endpoint */ \
        .bEndpointAddress   = DATAIN_EP,                                       \
        .bmAttributes       = 0x02,                                            /* Bulk */ \
        .wMaxPacketSize     = USB_UINT16(USB_FS_MAX_PACKET_SIZE),              \
        .bInterval          = 0x00                                             /* ignore for Bulk transfer */ \
      } \
    },

#endif /* __CDC_HELPER_H */
