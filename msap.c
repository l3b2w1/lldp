#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "lldp_dunchong.h"
#include "lldp_neighbor.h"
#include "msap.h"
#include "tlv_common.h"
#include "common_func.h"
#include "lldp_debug.h"
#include "tx_sm.h"
#include "rx_sm.h"

struct lldp_msap *create_msap(struct lldp_tlv *tlv1, struct lldp_tlv *tlv2) {
	return NULL;
}

int gratuitous_arp_packet(struct lldp_port *lldp_port, 
			uint8_t *buf, uint32_t ipaddr)
{
#define MAC_BCAST_ADDR      (unsigned char *) "\xff\xff\xff\xff\xff\xff"
	uint8_t *pbuf = NULL;
	struct eth_hdr *eh;
	struct arp_hdr *arph;
	uint8_t *parp;
	uint32_t srcip;
	int32_t sendsize = 60;

	eh = (struct eth_hdr *)buf;
	memcpy(eh->src, lldp_port->source_mac, 6);
	memcpy(eh->dst, MAC_BCAST_ADDR, 6);
	eh->ethertype = htons(ETH_P_ARP);

	arph = (struct arp_hdr *)(eh + 1);

	arph->ar_hrd = htons(0x0001);
	arph->ar_pro = htons(ETH_P_IP);
	arph->ar_hln = 6;
	arph->ar_pln = 4;
	arph->ar_op = htons(0x0001);//ARPOP_REQUEST; // ARPOP_REPLY;

	parp = (uint8_t *)( arph + 1);
	memcpy(parp, lldp_port->source_mac, 6);
	parp += ETH_ALEN;

	//srcip = *((uint32_t*)lldp_port->source_ipaddr);
	//srcip = *((uint32_t*)ip);
	srcip = htonl(ipaddr);

	*((uint32_t*)parp) = srcip;
	parp += 4;

	memset(parp, 0, ETH_ALEN);
	parp += ETH_ALEN;

	*((uint32_t*)parp) = srcip;
	parp += 4;

	return sendsize;
}

int32_t lldp_send_gratuitous_arp(struct lldp_port *lldp_port, uint32_t ip)
{
#define IP_AVAILABLE		1
#define IP_DETECTED			0
	fd_set readfds;
	int32_t result;
	int32_t sendsize, recvsize;
	int32_t sock, retval;
	uint8_t arpbuf[128] = {0};
	struct timeval tv;
	struct sockaddr_ll sll;
	struct arp_hdr *arphdr;
	struct eth_hdr *ethhdr;
	int32_t equalmac = 1;
	uint8_t *p;
	int32_t i = 0;
    struct timeval current_time;
    struct timeval future_time;
	int32_t used = 0;

	memset(&sll, 0x0, sizeof(struct sockaddr_ll));
	sll.sll_family = PF_PACKET;
	sll.sll_ifindex = lldp_port->if_index;
	sll.sll_protocol = htons(ETH_P_ARP);

	sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
	if (sock < 0) {
		lldp_printf(MSG_ERROR, "[%s %d]Couldn't initialize raw socket for interface %s!\n", __FUNCTION__, __LINE__, lldp_port->if_name);
		return -1;
	}

	retval = bind(sock, (struct sockaddr*)&sll, sizeof(struct sockaddr_ll));
	if (retval < 0) { 
		lldp_printf(MSG_ERROR, "[%s %d] [ERROR] binding raw socket to interface %s failed\n", __FUNCTION__, __LINE__, lldp_port->if_name);
		return -1;
	}

#if 1	
	tv.tv_sec = 0;
	tv.tv_usec = 500;
	/* timeout after xxx second, get nothing. 
	 * namely, this IP address is available */
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)))
	  lldp_printf(MSG_ERROR, "[%s %d] set SO_RCVTIMEO failed\n",
				  __FUNCTION__, __LINE__);
#endif

	memset(arpbuf, 0x0, 128);
	sendsize = gratuitous_arp_packet(lldp_port, arpbuf, ip);
	sendto(sock, arpbuf, sendsize, 0, NULL, 0);
	//lldp_hex_dump(arpbuf, sendsize);

	result = recvfrom(sock, arpbuf, 128, 0, 0, 0);
	if (result > 0) {
		ethhdr = (struct eth_hdr*)arpbuf;
		arphdr = (struct arp_hdr *)(ethhdr + 1);
		equalmac = !memcmp(ethhdr->dst, lldp_port->source_mac, 6);

		//lldp_hex_dump(arpbuf, result);
#if 0
		printf("ip address %x\n", ip - 1);
		printf("ethertype %04x\n", ethhdr->ethertype);
		printf("ETH_P_APR %04x\n", htons(ETH_P_ARP));

		printf("arphdr op %04x\n", arphdr->ar_op);
		printf("arppro op %04x\n", htons(0x0002));

		printf("equalmac %d\n", equalmac);
#endif

		if ((ethhdr->ethertype == htons(ETH_P_ARP)) 
					&& (arphdr->ar_op == htons(0x0002)) && equalmac) {
			p = (uint8_t*)&ip;
			printf("detected, %d.%d.%d.%d\n", p[0], p[1], p[2], p[3]);
			lldp_hex_dump(arpbuf, result);
			used = 1;
			return IP_DETECTED;
		}
	}
	if (!used) {
		p = (uint8_t*)&ip;
		printf("available, %d.%d.%d.%d\n", p[0], p[1], p[2], p[3]);
		used = 0;
	}
	/*shutdown read and write action */
	shutdown(sock, 0);
}

void destroy_list(struct lldp_tlv_list **tlv_list) 
{
	struct lldp_tlv_list *current = *tlv_list;
	
	if (!current)
		;/* warning, to delete empty list */
	
	while (current != NULL) {
		current = current->next;
		
		free((*tlv_list)->tlv->value);
		free((*tlv_list)->tlv);
		
		/* free tlv list node */
		free(*tlv_list);	
		
		(*tlv_list) = current;
	}
}

/* cleanup neighbors info */
void cleanupMsap(struct lldp_port *lldp_port)
{
	struct lldp_msap *curr, *tmp; 

	curr = lldp_port->msap_cache;

	if (!curr)
		return;

	while (curr) {
		free(curr->id);
		tmp = curr;
		curr = curr->next;
		free(tmp);
	}
}


void iterate_msap_cache(struct lldp_msap *msap_cache) 
{
	while(msap_cache != NULL) {
		lldp_printf(MSG_DEBUG, "MSAP cache: %X\n", msap_cache);

		lldp_printf(MSG_DEBUG, "MSAP ID: ");
		//debug_hex_printf(MSG_DEBUG, msap_cache->id, msap_cache->length);
		lldp_hex_dump(msap_cache->id, msap_cache->length);

		lldp_printf(MSG_DEBUG, "MSAP IpAddr: %X\n", msap_cache->ipaddr);

		lldp_printf(MSG_DEBUG, "MSAP rxInfoTTL: %X\n", msap_cache->rxInfoTTL);

		lldp_printf(MSG_DEBUG, "MSAP Length: %X\n", msap_cache->length);

		lldp_printf(MSG_DEBUG, "MSAP Next: %X\n", msap_cache->next);

		msap_cache = msap_cache->next;
	}
}

int gratuitous_arp_send(struct lldp_port *lldp_port)
{
#define MAC_BCAST_ADDR      (unsigned char *) "\xff\xff\xff\xff\xff\xff"
	uint8_t *buf = NULL;
	struct eth_hdr *eh;
	struct arp_hdr *arph;
	uint8_t *parp;
	uint32_t srcip;
	uint8_t ip[] = {0x0a, 0x00, 0x01, 0xfe};

	buf = &lldp_port->tx.frame[0];

	eh = (struct eth_hdr *)buf;
	memcpy(eh->src, lldp_port->source_mac, 6);
	memcpy(eh->dst, MAC_BCAST_ADDR, 6);
	eh->ethertype = htons(ETH_P_ARP);

	arph = (struct arp_hdr *)(eh + 1);

	arph->ar_hrd = htons(0x0001);
	arph->ar_pro = htons(ETH_P_IP);
	arph->ar_hln = 6;
	arph->ar_pln = 4;
	arph->ar_op = htons(0x0001);//ARPOP_REQUEST; // ARPOP_REPLY;

	parp = (uint8_t *)( arph + 1);
	memcpy(parp, lldp_port->source_mac, 6);
	parp += ETH_ALEN;

	//srcip = *((uint32_t*)lldp_port->source_ipaddr);
	srcip = *((uint32_t*)ip);
	//memcpy(&srcip, ip, 4);

	srcip = srcip;
	*((uint32_t*)parp) = srcip;
	printf("source ipaddr %x\n", srcip);
	printf("parp %x\n", *((uint32_t*)parp));
	parp += 4;

	memset(parp, 0, ETH_ALEN);
	parp += ETH_ALEN;

	*((uint32_t*)parp) = srcip;
	parp += 4;

	lldp_port->tx.sendsize = 60;

	return 0;
}

void update_msap_cache(struct lldp_port *lldp_port, struct lldp_msap* msap_cache) {
	struct lldp_msap *old_cache = lldp_port->msap_cache;
	struct lldp_msap *new_cache = msap_cache;

	while(old_cache != NULL) {

		if(old_cache->length == new_cache->length) {
			//lldp_printf(MSG_DEBUG, "MSAP Length: %X\n", old_cache->length);

			if(memcmp(old_cache->id, new_cache->id, new_cache->length) == 0) {
				lldp_printf(MSG_DEBUG, "MSAP Cache Hit on %s\n", lldp_port->if_name);
				iterate_msap_cache(msap_cache);

				if ((dev_role == LLDP_DUNCHONG_ROLE_MASTER) && (msap_cache->role == LLDP_DUNCHONG_ROLE_SLAVE) && (!msap_cache->ipaddr))
					alloc_ip_for_slave(msap_cache);

				old_cache->rxInfoTTL = new_cache->rxInfoTTL;

				// Now free the rest of the MSAP cache
				free(new_cache->id);
				free(new_cache);

				return;
			}

		}

		//lldp_printf(MSG_DEBUG, "Checking next MSAP entry...\n");

		old_cache = old_cache->next;
	}

	lldp_printf(MSG_DEBUG, "MSAP Cache Miss on %s\n", lldp_port->if_name);

	new_cache->next = lldp_port->msap_cache;
	lldp_port->msap_cache = new_cache;

	/*warning We are leaking memory... need to dispose of the msap_cache under certain circumstances */
}
