/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 by Sergey Fetisov <fsenok@gmail.com>
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

#include <stdalign.h>
#include <string.h>
#include "rndis_protocol.h"
#include "usb_rndis.h"
#include "rndis.h"

static const uint8_t station_hwaddr[6] = { RNDIS_HWADDR };
static const uint8_t permanent_hwaddr[6] = { RNDIS_HWADDR };

static usb_eth_stat_t usb_eth_stat = { 0, 0, 0, 0 };
static uint32_t oid_packet_filter = 0x0000000;
static rndis_state_t rndis_state;

/* data handed to USB peripheral must be 32-bit aligned and in RAM */
static alignas(4) uint8_t ndis_report[8] = { 0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

static const uint32_t OIDSupportedList[] = 
{
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_ID,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_VENDOR_DRIVER_VERSION,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_PROTOCOL_OPTIONS,
    OID_GEN_MAC_OPTIONS,
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_GEN_MAXIMUM_SEND_PACKETS,
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,
    OID_802_3_MAC_OPTIONS
};

#define OID_LIST_LENGTH ARRAY_SIZE(OIDSupportedList)
#define ENC_BUF_SIZE    (OID_LIST_LENGTH * 4 + 32)

static uint8_t encapsulated_buffer[ENC_BUF_SIZE];

/* provide window between usb_rndis.c and rndis.c */
const uint8_t *const rndis_class_buffer = encapsulated_buffer;
uint32_t *const rndis_class_msg_size = &((rndis_generic_msg_t *)encapsulated_buffer)->MessageLength;
extern int usb_rndis_xmit_size;

static void rndis_report(void)
{
  usb_rndis_report(ndis_report, sizeof(ndis_report));
}

static void rndis_query_cmplt32(int status, uint32_t data)
{
  rndis_query_cmplt_t *c;
  c = (rndis_query_cmplt_t *)encapsulated_buffer;
  c->MessageType = REMOTE_NDIS_QUERY_CMPLT;
  c->MessageLength = sizeof(rndis_query_cmplt_t) + 4;
  c->InformationBufferLength = 4;
  c->InformationBufferOffset = 16;
  c->Status = status;
  memcpy(c + 1, &data, sizeof(data));
  rndis_report();
}

static void rndis_query_cmplt(int status, const void *data, int size)
{
  rndis_query_cmplt_t *c;
  c = (rndis_query_cmplt_t *)encapsulated_buffer;
  c->MessageType = REMOTE_NDIS_QUERY_CMPLT;
  c->MessageLength = sizeof(rndis_query_cmplt_t) + size;
  c->InformationBufferLength = size;
  c->InformationBufferOffset = 16;
  c->Status = status;
  memcpy(c + 1, data, size);
  rndis_report();
}

#define MAC_OPT NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA | \
      NDIS_MAC_OPTION_RECEIVE_SERIALIZED  | \
      NDIS_MAC_OPTION_TRANSFERS_NOT_PEND  | \
      NDIS_MAC_OPTION_NO_LOOPBACK

static const char *rndis_vendor = RNDIS_VENDOR;

static void rndis_query(void)
{
  switch (((rndis_query_msg_t *)encapsulated_buffer)->Oid)
  {
    case OID_GEN_SUPPORTED_LIST:         rndis_query_cmplt(RNDIS_STATUS_SUCCESS, OIDSupportedList, 4 * OID_LIST_LENGTH); return;
    case OID_GEN_VENDOR_DRIVER_VERSION:  rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, 0x00001000);  return;
    case OID_802_3_CURRENT_ADDRESS:      rndis_query_cmplt(RNDIS_STATUS_SUCCESS, &station_hwaddr, 6); return;
    case OID_802_3_PERMANENT_ADDRESS:    rndis_query_cmplt(RNDIS_STATUS_SUCCESS, &permanent_hwaddr, 6); return;
    case OID_GEN_MEDIA_SUPPORTED:        rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, NDIS_MEDIUM_802_3); return;
    case OID_GEN_MEDIA_IN_USE:           rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, NDIS_MEDIUM_802_3); return;
    case OID_GEN_PHYSICAL_MEDIUM:        rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, NDIS_MEDIUM_802_3); return;
    case OID_GEN_HARDWARE_STATUS:        rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, 0); return;
    case OID_GEN_LINK_SPEED:             rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, RNDIS_LINK_SPEED / 100); return;
    case OID_GEN_VENDOR_ID:              rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, 0x00FFFFFF); return;
    case OID_GEN_VENDOR_DESCRIPTION:     rndis_query_cmplt(RNDIS_STATUS_SUCCESS, rndis_vendor, strlen(rndis_vendor) + 1); return;
    case OID_GEN_CURRENT_PACKET_FILTER:  rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, oid_packet_filter); return;
    case OID_GEN_MAXIMUM_FRAME_SIZE:     rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, ETH_MAX_PACKET_SIZE - ETH_HEADER_SIZE); return;
    case OID_GEN_MAXIMUM_TOTAL_SIZE:     rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, ETH_MAX_PACKET_SIZE); return;
    case OID_GEN_TRANSMIT_BLOCK_SIZE:    rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, ETH_MAX_PACKET_SIZE); return;
    case OID_GEN_RECEIVE_BLOCK_SIZE:     rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, ETH_MAX_PACKET_SIZE); return;
    case OID_GEN_MEDIA_CONNECT_STATUS:   rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, NDIS_MEDIA_STATE_CONNECTED); return;
    case OID_GEN_RNDIS_CONFIG_PARAMETER: rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, 0); return;
    case OID_802_3_MAXIMUM_LIST_SIZE:    rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, 1); return;
    case OID_802_3_MULTICAST_LIST:       rndis_query_cmplt32(RNDIS_STATUS_NOT_SUPPORTED, 0); return;
    case OID_802_3_MAC_OPTIONS:          rndis_query_cmplt32(RNDIS_STATUS_NOT_SUPPORTED, 0); return;
    case OID_GEN_MAC_OPTIONS:            rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, /*MAC_OPT*/ 0); return;
    case OID_802_3_RCV_ERROR_ALIGNMENT:  rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, 0); return;
    case OID_802_3_XMIT_ONE_COLLISION:   rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, 0); return;
    case OID_802_3_XMIT_MORE_COLLISIONS: rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, 0); return;
    case OID_GEN_XMIT_OK:                rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, usb_eth_stat.txok); return;
    case OID_GEN_RCV_OK:                 rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, usb_eth_stat.rxok); return;
    case OID_GEN_RCV_ERROR:              rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, usb_eth_stat.rxbad); return;
    case OID_GEN_XMIT_ERROR:             rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, usb_eth_stat.txbad); return;
    case OID_GEN_RCV_NO_BUFFER:          rndis_query_cmplt32(RNDIS_STATUS_SUCCESS, 0); return;
    default:                             rndis_query_cmplt(RNDIS_STATUS_FAILURE, NULL, 0); return;
  }
}

#define INFBUF ((uint32_t *)((uint8_t *)&(m->RequestId) + m->InformationBufferOffset))

static void rndis_handle_config_parm(const char *data, int keyoffset, int valoffset, int keylen, int vallen)
{
    (void)data;
    (void)keyoffset;
    (void)valoffset;
    (void)keylen;
    (void)vallen;
}

static void rndis_packetFilter(uint32_t newfilter)
{
    (void)newfilter;
}

void rndis_handle_set_msg(void)
{
  rndis_set_cmplt_t *c;
  rndis_set_msg_t *m;
  rndis_Oid_t oid;

  c = (rndis_set_cmplt_t *)encapsulated_buffer;
  m = (rndis_set_msg_t *)encapsulated_buffer;

  oid = m->Oid;
  c->MessageType = REMOTE_NDIS_SET_CMPLT;
  c->MessageLength = sizeof(rndis_set_cmplt_t);
  c->Status = RNDIS_STATUS_SUCCESS;

  switch (oid)
  {
    /* Parameters set up in 'Advanced' tab */
    case OID_GEN_RNDIS_CONFIG_PARAMETER:
      {
        rndis_config_parameter_t *p;
        char *ptr = (char *)m;
        ptr += sizeof(rndis_generic_msg_t);
        ptr += m->InformationBufferOffset;
        p = (rndis_config_parameter_t *)ptr;
        rndis_handle_config_parm(ptr, p->ParameterNameOffset, p->ParameterValueOffset, p->ParameterNameLength, p->ParameterValueLength);
      }
      break;

    /* Mandatory general OIDs */
    case OID_GEN_CURRENT_PACKET_FILTER:
      oid_packet_filter = *INFBUF;
      if (oid_packet_filter)
      {
        rndis_packetFilter(oid_packet_filter);
        rndis_state = rndis_data_initialized;
      } 
      else 
      {
        rndis_state = rndis_initialized;
      }
      break;

    case OID_GEN_CURRENT_LOOKAHEAD:
      break;

    case OID_GEN_PROTOCOL_OPTIONS:
      break;

    /* Mandatory 802_3 OIDs */
    case OID_802_3_MULTICAST_LIST:
      break;

    /* Power Managment: fails for now */
    case OID_PNP_ADD_WAKE_UP_PATTERN:
    case OID_PNP_REMOVE_WAKE_UP_PATTERN:
    case OID_PNP_ENABLE_WAKE_UP:
    default:
      c->Status = RNDIS_STATUS_FAILURE;
      break;
  }

  /* c->MessageID is same as before */
  rndis_report();
  return;
}

void rndis_recv_callback(uint8_t *data, int size)
{
  rndis_data_packet_t *p;
  p = (rndis_data_packet_t *)data;

  if (size >= sizeof(rndis_data_packet_t))
    if ( (p->MessageType == REMOTE_NDIS_PACKET_MSG) && (p->MessageLength <= size))
      if ( (p->DataOffset + offsetof(rndis_data_packet_t, DataOffset) + p->DataLength) <= size)
      {
        usb_eth_stat.rxok++;
        usb_rndis_recv_callback(&data[p->DataOffset + offsetof(rndis_data_packet_t, DataOffset)], p->DataLength);
        return;
    }

  usb_eth_stat.rxbad++;
  usb_rndis_recv_renew();
}

void rndis_class_set_handler(uint8_t *data, int size)
{
  memcpy(encapsulated_buffer, data, size);
  switch (((rndis_generic_msg_t *)data)->MessageType)
  {
    case REMOTE_NDIS_INITIALIZE_MSG:
      {
        rndis_initialize_cmplt_t *m;
        m = ((rndis_initialize_cmplt_t *)encapsulated_buffer);
        /* m->MessageID is same as before */
        m->MessageType = REMOTE_NDIS_INITIALIZE_CMPLT;
        m->MessageLength = sizeof(rndis_initialize_cmplt_t);
        m->MajorVersion = RNDIS_MAJOR_VERSION;
        m->MinorVersion = RNDIS_MINOR_VERSION;
        m->Status = RNDIS_STATUS_SUCCESS;
        m->DeviceFlags = RNDIS_DF_CONNECTIONLESS;
        m->Medium = RNDIS_MEDIUM_802_3;
        m->MaxPacketsPerTransfer = 1;
        m->MaxTransferSize = RNDIS_BUFFER_SIZE;
        m->PacketAlignmentFactor = 0;
        m->AfListOffset = 0;
        m->AfListSize = 0;
        rndis_state = rndis_initialized;
        rndis_report();
      }
      break;

    case REMOTE_NDIS_QUERY_MSG:
      rndis_query();
      break;
      
    case REMOTE_NDIS_SET_MSG:
      rndis_handle_set_msg();
      break;

    case REMOTE_NDIS_RESET_MSG:
      {
        rndis_reset_cmplt_t * m;
        m = ((rndis_reset_cmplt_t *)encapsulated_buffer);
        rndis_state = rndis_uninitialized;
        m->MessageType = REMOTE_NDIS_RESET_CMPLT;
        m->MessageLength = sizeof(rndis_reset_cmplt_t);
        m->Status = RNDIS_STATUS_SUCCESS;
        m->AddressingReset = 1; /* Make it look like we did something */
          /* m->AddressingReset = 0; - Windows halts if set to 1 for some reason */
        rndis_report();
      }
      break;

    case REMOTE_NDIS_KEEPALIVE_MSG:
      {
        rndis_keepalive_cmplt_t * m;
        m = (rndis_keepalive_cmplt_t *)encapsulated_buffer;
        m->MessageType = REMOTE_NDIS_KEEPALIVE_CMPLT;
        m->MessageLength = sizeof(rndis_keepalive_cmplt_t);
        m->Status = RNDIS_STATUS_SUCCESS;
      }
      /* We have data to send back */
      rndis_report();
      break;

    default:
      break;
  }
}

void rndis_send(struct pbuf *p)
{
  rndis_data_packet_t hdr;
  int max_size;
  struct pbuf *q;
  int size;

  for(q = p, size = 0; q != NULL; q = q->next)
    size += q->len;

  memset(&hdr, 0, sizeof(hdr));
  hdr.MessageType = REMOTE_NDIS_PACKET_MSG;
  hdr.MessageLength = sizeof(rndis_data_packet_t) + size;
  hdr.DataOffset = sizeof(rndis_data_packet_t) - offsetof(rndis_data_packet_t, DataOffset);

  max_size = usb_rndis_xmit_size - sizeof(rndis_data_packet_t);
  if (size > max_size)
    size = max_size;

  hdr.DataLength = size;

  usb_rndis_xmit_packet((uint8_t *)&hdr, sizeof(hdr), p, size);
}

