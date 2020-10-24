/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Peter Lawrence
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdbool.h>
#include <stdalign.h>
#include <string.h>
#include <stddef.h>
#include "utils.h"
#include "usb.h"
#include "usb_std.h"
#include "usb_rndis.h"
#include "rndis.h"

static alignas(4) uint8_t received[RNDIS_BUFFER_SIZE];
static alignas(4) uint8_t transmitted[RNDIS_BUFFER_SIZE];

static bool can_xmit;

/* provide window between usb_rndis.c and rndis.c */
extern const uint8_t *const rndis_class_buffer;
extern uint32_t *const rndis_class_msg_size;
const int usb_rndis_xmit_size = RNDIS_BUFFER_SIZE;

static void usb_rndis_ep_send_callback(int size);
static void usb_rndis_ep_recv_callback(int size);

void usb_rndis_report(const uint8_t *data, int size)
{
  usb_send(USB_RNDIS_EP_COMM, (uint8_t *)data, size);
}

void usb_rndis_init(void)
{
  usb_set_callback(USB_RNDIS_EP_SEND, usb_rndis_ep_send_callback);
  usb_set_callback(USB_RNDIS_EP_RECV, usb_rndis_ep_recv_callback);
}

static void usb_rndis_send(uint8_t *data, int size)
{
  usb_send(USB_RNDIS_EP_SEND, data, size);
}

void usb_rndis_recv_renew(void)
{
  usb_recv(USB_RNDIS_EP_RECV, received, sizeof(received));
}

void usb_configuration_callback(int config)
{
  (void)config;

  usb_rndis_recv_renew();
  can_xmit = true;
}


static void usb_rndis_ep_send_callback(int size)
{
  (void)size;

  can_xmit = true;
}

static void usb_rndis_ep_recv_callback(int size)
{
  rndis_recv_callback(received, size);
}

bool usb_class_handle_request(usb_request_t *request)
{
  int length = request->wLength;

  switch (request->bmRequestType & 0x60)
  {
  case 0x20: /* CLASS */
    if (length)
    {
      if (request->bmRequestType & 0x80)
      {
        length = LIMIT(length, *rndis_class_msg_size);
        /* Device-to-Host */
        usb_control_send((uint8_t *)rndis_class_buffer, length);
      }
      else
      {
        /* Host-to-Device */
        usb_control_recv(rndis_class_set_handler);
      }
      return true;
    }
  }

  return false;
}

bool usb_rndis_can_xmit(void)
{
  return can_xmit;
}

void usb_rndis_xmit_packet(uint8_t *header, int header_size, struct pbuf *p, int packet_size)
{
  struct pbuf *q;
  uint8_t *data;

  if (!can_xmit)
    return;

  memcpy(transmitted, header, header_size);

  data = transmitted + header_size;
  for(q = p; q != NULL; q = q->next)
  {
    memcpy(data, (char *)q->payload, q->len);
    data += q->len;
    if (q->tot_len == q->len) break;
  }

  can_xmit = false;
  usb_rndis_send(transmitted, header_size + packet_size);
}
