/* This file is part of rfBareMbed.                        
 *									 
 * rfBareMbed is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by	 
 * the Free Software Foundation, either version 3 of the License, or	 
 * (at your option) any later version.					 
 * 									 
 * rfBareMbed is distributed in the hope that it will be useful,		 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of	 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	 
 * GNU General Public License for more details.				 
 * 									 
 * You should have received a copy of the GNU General Public License	 
 * along with rfBareMbed.  If not, see <http://www.gnu.org/licenses/>.	 
 */

#include <debug.h>
#include <drivers/leds.h>
#include <drivers/uart.h>
#include <drivers/ethernet.h>
#include <drivers/rit.h> /* timer for link status */
#include <printf.h>
#include <interrupt.h>
#include <drivers/eth_const.h>

#include "protocols.h"


/* a good old duff's device for memcpy */
void memcpy(void *dst, const void *src, unsigned int bytes)
{
   unsigned char *to = (unsigned char *)dst;
   const unsigned char *from = (const unsigned char *)src;
   unsigned int n = (bytes + 7) / 8;
   switch (bytes % 8) {
      case 0: do { *to++ = *from++;
         case 7:      *to++ = *from++;
         case 6:      *to++ = *from++;
         case 5:      *to++ = *from++;
         case 4:      *to++ = *from++;
         case 3:      *to++ = *from++;
         case 2:      *to++ = *from++;
         case 1:      *to++ = *from++;
      } while (--n > 0);
   }
}


/* For printf */
int putchar(int c)
{
    rflpc_uart0_putchar(c);
    return c;
}

void dump_packet(rfEthDescriptor *d, rfEthRxStatus *s)
{
    printf("= %p %p %p",d, s, d->packet);
    if (s->status_info & (1 << 18))
	printf("cf ");
    else
	printf("   ");
    if (s->status_info & (1 << 21))
	printf("mf ");
    else
	printf("   ");
    if (s->status_info & (1 << 22))
	printf("bf ");
    else
	printf("   ");
    if (s->status_info & (1 << 23))
	printf("crc ");
    else
	printf("    ");
    if (s->status_info & (1 << 24))
	printf("se ");
    else
	printf("   ");
    if (s->status_info & (1 << 25))
	printf("le ");
    else
	printf("   ");
    if (s->status_info & (1 << 27))
	printf("ae ");
    else
	printf("   ");
    if (s->status_info & (1 << 28))
	printf("ov ");
    else
	printf("   ");
    if (s->status_info & (1 << 29))
	printf("nd ");
    else
	printf("   ");
    if (s->status_info & (1 << 30))
	printf("lf ");
    else
	printf("   ");

    printf("size:%d ", (s->status_info & 0x7FF) + 1);
/*    for (i = 0 ; i < (s->status_info & 0x7FF) + 1 ; ++i)
    {
	if (i % 16 == 0)
	    printf("\r\n");
	printf("%x ", d->packet[i]);
	}*/
    printf("\r\n");

}

volatile static int request_autoneg = 0;
volatile static int autoneg_mode = RFLPC_ETH_LINK_MODE_100FD;

RFLPC_IRQ_HANDLER _serial_handler()
{
    char c = rflpc_uart0_getchar();

    switch (c)
    {
	case 's':
	    request_autoneg = 1;
	    autoneg_mode = autoneg_mode ^ RFLPC_ETH_LINK_MODE_SPEED_BIT;
	    break;
	case 'd':
	    request_autoneg = 1;
	    autoneg_mode = autoneg_mode ^ RFLPC_ETH_LINK_MODE_DUPLEX_BIT;

	break;
    }
}

RFLPC_IRQ_HANDLER _rit_handler()
{
    rflpc_rit_clear_pending_interrupt();
    if (rflpc_eth_link_state())
	rflpc_led_set(LED1);
    else
	rflpc_led_clr(LED1);
}

#define RX_BUFFER_SIZE  RFLPC_ETH_MAX_FRAME_LENGTH
#define RX_BUFFER_COUNT 8

#define TX_BUFFER_SIZE  RFLPC_ETH_MAX_FRAME_LENGTH
#define TX_BUFFER_COUNT 4

rfEthDescriptor _rx_desc[RX_BUFFER_COUNT];
rfEthRxStatus _rx_status[RX_BUFFER_COUNT];

rfEthDescriptor _tx_desc[TX_BUFFER_COUNT];
rfEthTxStatus _tx_status[TX_BUFFER_COUNT];

uint8_t rxbuffers[RX_BUFFER_SIZE][RX_BUFFER_COUNT];
uint8_t txbuffers[TX_BUFFER_SIZE][TX_BUFFER_COUNT];

#define TYPE_OFFSET    12
#define PAYLOAD_OFFSET 14

#define TYPE_ARP               0x0806
#define ARP_OPCODE_OFFSET           6
#define ARP_OPCODE_REQUEST     0x0001
#define ARP_SENDER_MAC_OFFSET       8
#define ARP_SENDER_IP_OFFSET       14
#define ARP_TARGET_MAC_OFFSET      18
#define ARP_TARGET_IP_OFFSET       24

#define TYPE_IP 0x0800


EthAddr mac_addr;

void process_packet(rfEthDescriptor *rxd, rfEthRxStatus *rxs)
{
    rfEthDescriptor *txd;
    rfEthTxStatus *txs;
    EthHead eth;
    EthHead eth_reply;
    

    /* demangle ethernet frame*/
    proto_eth_demangle(&eth, rxd->packet);
    if (eth.type == TYPE_ARP)
    {
	ArpHead arp_rcv;
	ArpHead arp_send;
	
	proto_arp_demangle(&arp_rcv, rxd->packet + PAYLOAD_OFFSET);

	printf("ARP: From: %x:%x:%x:%x:%x:%x. Who has %d.%d.%d.%d?, tell %d.%d.%d.%d\r\n", 
	       arp_rcv.sender_mac.addr[0],
	       arp_rcv.sender_mac.addr[1],
	       arp_rcv.sender_mac.addr[2],
	       arp_rcv.sender_mac.addr[3],
	       arp_rcv.sender_mac.addr[4],
	       arp_rcv.sender_mac.addr[5],
	       (arp_rcv.target_ip >> 24) & 0xff,
	       (arp_rcv.target_ip >> 16) & 0xff,
	       (arp_rcv.target_ip >> 8) & 0xff,
	       (arp_rcv.target_ip) & 0xff,
	       (arp_rcv.sender_ip >> 24) & 0xff,
	       (arp_rcv.sender_ip >> 16) & 0xff,
	       (arp_rcv.sender_ip >> 8) & 0xff,
	       (arp_rcv.sender_ip) & 0xff);
	/* generate reply */
	if (!rflpc_eth_get_current_tx_packet_descriptor(&txd, &txs))
	{
	    printf("No available TX buffers, can't generate answer\r\n");
	    return;
	}
	/* Ethernet Header */
	eth_reply.type = eth.type;
	eth_reply.src = mac_addr;
	eth_reply.dst = eth.src;
	/* ARP  */
	arp_send.hard_type = 1;
	arp_send.protocol_type = 0x0800;
	arp_send.hlen = 6;
	arp_send.plen = 4;
	arp_send.opcode = 2; /* reply */
	arp_send.sender_mac = mac_addr;
	arp_send.sender_ip = arp_rcv.target_ip;
	arp_send.target_mac = arp_rcv.sender_mac;
	arp_send.target_ip = arp_rcv.sender_ip;

	proto_eth_mangle(&eth_reply, txd->packet);
	proto_arp_mangle(&arp_send, txd->packet + PAYLOAD_OFFSET);
	
	/* Set control bits in descriptor with  size, enable padding, crc, and last fragment */
	txd->control = (ETH_HEADER_SIZE + ARP_HEADER_SIZE) | (1 << 18) | (1 << 29) | (1 << 30);
	/* send packet */
	rflpc_eth_done_process_tx_packet();
	return;
    }
    if (eth.type == TYPE_IP)
    {
	dump_packet(rxd, rxs);
	return;
    }
    dump_packet(rxd, rxs);
}

void ethernet()
{
    int i;
    mac_addr.addr[0] = 2;
    mac_addr.addr[1] = 3;
    mac_addr.addr[2] = 4;
    mac_addr.addr[3] = 5;
    mac_addr.addr[4] = 6;
    mac_addr.addr[5] = 7;
    rflpc_eth_init();
    rflpc_eth_set_mac_address(mac_addr.addr);
    rflpc_eth_get_mac_address(mac_addr.addr);
    printf("My mac: %x:%x:%x:%x:%x:%x\r\n", mac_addr.addr[0], mac_addr.addr[1], mac_addr.addr[2], mac_addr.addr[3], mac_addr.addr[4], mac_addr.addr[5]);
    rflpc_led_val(0);

    printf("Autonegociating to 100Mbps full duplex\r\n");
    rflpc_eth_link_auto_negociate(RFLPC_ETH_LINK_MODE_100FD);
    printf("Waiting for link to be up\r\n");
    while(!rflpc_eth_link_state());

    /* setting rit timer to periodicaly check for link state */
    rflpc_rit_enable();
    rflpc_rit_set_callback(0xFFFFF, 0, 1, _rit_handler);
    

    printf("Ok. Press:\r\n");
    printf("- 's' to toggle speed\r\n");
    printf("- 'd' to toggle duplex\r\n");
    rflpc_uart0_set_rx_callback(_serial_handler);

    /* initialize rx descriptors */
    for (i = 0 ; i < RX_BUFFER_COUNT ; ++i)
    {
	_rx_desc[i].packet = rxbuffers[i];
	_rx_desc[i].control = RX_BUFFER_SIZE - 1; /* -1 encoding */
    }
    rflpc_eth_set_rx_base_addresses(_rx_desc, _rx_status, RX_BUFFER_COUNT);

    /* initialize tx descriptors */
    for (i = 0 ; i < TX_BUFFER_COUNT ; ++i)
    {
	_tx_desc[i].packet = txbuffers[i];
    }
    rflpc_eth_set_tx_base_addresses(_tx_desc, _tx_status, TX_BUFFER_COUNT);

    while (1)
    {
	rfEthDescriptor *d;
	rfEthRxStatus *s;
	if (rflpc_eth_get_current_rx_packet_descriptor(&d, &s))
	{
	    /* packet received */
	    /*dump_packet(d, s);*/
	    process_packet(d,s);
	    /* done with it */
	    rflpc_eth_done_process_rx_packet();
	}

    }
}


int main()
{
    if (rflpc_uart0_init() == -1)
	RFLPC_STOP(LED1 | LED3, 1000000);
    printf("rfBareMbed ethernet sample\r\n");

    ethernet();

    RFLPC_STOP(LED2 | LED3, 1000000);

    return 0;
}
