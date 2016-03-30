#include <netinet/in.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <time.h>
#include "lldp_linux_framer.h"
#include "lldp_port.h"
#include "lldp_debug.h"
#include "common_func.h"

#define XENOSOCK 0

ssize_t lldp_write(struct lldp_port *lldp_port)
{
	return sendto(lldp_port->socket, lldp_port->tx.frame, lldp_port->tx.sendsize, 0, NULL, 0);
}

ssize_t lldp_read(struct lldp_port *lldp_port) {
    lldp_port->rx.recvsize = recvfrom(lldp_port->socket, lldp_port->rx.frame, lldp_port->mtu, 0, 0, 0);
    return (lldp_port->rx.recvsize);
}

/* get the MAC address of an interface */
static int _getmac(struct lldp_port *lldp_port)
{
	u8 *p;
	struct ifreq *ifr = calloc(1, sizeof(struct ifreq));
	int retval = 0;
	
	ifr->ifr_ifindex = lldp_port->if_index;
	
	strncpy(ifr->ifr_name, &lldp_port->if_name[0], strlen(lldp_port->if_name));
	
	retval = ioctl(lldp_port->socket, SIOCGIFHWADDR, ifr);
	
	if (retval < 0) {
		free(ifr);
		return retval;
	}
	
	memcpy(&lldp_port->source_mac[0], &ifr->ifr_hwaddr.sa_data[0], 6);

	p = lldp_port->source_mac;
	lldp_printf(MSG_INFO, "[%s %d] [INFO] interface %s's MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
				__FUNCTION__, __LINE__, lldp_port->if_name, p[0], p[1], p[2], p[3], p[4], p[5]);
	free(ifr);
	return 0;
}

/* get the IP address of an interface */
static int _getip(struct lldp_port *lldp_port)
{
	struct ifreq *ifr = calloc(1, sizeof(struct ifreq));
	int retval = 0;
	
	strncpy(ifr->ifr_name, &lldp_port->if_name[0], strlen(lldp_port->if_name));
	
	retval = ioctl(lldp_port->socket, SIOCGIFADDR, ifr);
	if (retval < 0) {
		/* set source ip address to 0.0.0.0 on error */
		memset(&lldp_port->source_ipaddr[0], 0x00, 4);
		free(ifr);
		return retval;
	} else {
		memcpy(&lldp_port->source_ipaddr[0], &ifr->ifr_addr.sa_data[2], 4);
	}
	
	free(ifr);
	
	return retval;
}

void lldp_refresh_if_data(struct lldp_port *lldp_port)
{
	_getmac(lldp_port);
	_getip(lldp_port);
}

int lldp_init_socket(struct lldp_port *lldp_port)
{
	struct ifreq *ifr = calloc(1, sizeof(struct ifreq));
	struct sockaddr_ll *sll = calloc(1, sizeof(struct sockaddr_ll));
	int retval = 0;
	
	if (lldp_port->if_name == NULL) {
		lldp_printf(MSG_ERROR, "[%s %d] [ERROR] port if_name is NULL\n", __FUNCTION__, __LINE__);
		return XENOSOCK;
	}
	
	if (lldp_port->if_index <= 0) {
		lldp_printf(MSG_ERROR, "[%s %d] [ERROR] port if_index <= 0\n", __FUNCTION__, __LINE__);
		return XENOSOCK;
	}
	
	lldp_port->socket = socket(PF_PACKET, SOCK_RAW, htons(0x88cc));
	if (lldp_port->socket < 0) {
		lldp_printf(MSG_ERROR, "[%s %d] [ERROR] port create raw socket failed, socket < 0\n", __FUNCTION__, __LINE__);
		return XENOSOCK;
	}
	
	sll->sll_family = PF_PACKET;
	sll->sll_ifindex = lldp_port->if_index;
	sll->sll_protocol = htons(0x88cc);
	
	retval = bind(lldp_port->socket, (struct sockaddr*)sll, sizeof(struct sockaddr_ll));
	if (retval < 0) { 
		lldp_printf(MSG_ERROR, "[%s %d] [ERROR] socket bind failed\n", __FUNCTION__, __LINE__);
		return XENOSOCK;
	}
	
	ifr->ifr_ifindex = lldp_port->if_index;
	
	strncpy(ifr->ifr_name, &lldp_port->if_name[0], strlen(lldp_port->if_name));
	if (strlen(ifr->ifr_name) == 0) {
		lldp_printf(MSG_ERROR, "[%s %d] [ERROR] ifr_name length is 0\n", __FUNCTION__, __LINE__);
		return XENOSOCK;
	}
	
	lldp_refresh_if_data(lldp_port);
	
	retval = ioctl(lldp_port->socket, SIOCGIFFLAGS, ifr);
	if (retval == -1)
		lldp_printf(MSG_WARNING, "[%s %d] [WARNING] Interface %s, can not get flags for \n", __FUNCTION__, __LINE__, lldp_port->if_name);
	
	if ((ifr->ifr_flags & IFF_UP) == 0) {
		/* interface is down. set portEnabled = 0 */
		lldp_printf(MSG_WARNING, "[%s %d] [WARNING] Interface %s is NOT UP, now set portEnable to 0\n", lldp_port->if_name, __FUNCTION__, __LINE__);
		lldp_port->portEnabled = 0;
	}
	

	 /* set allmulti on interface 
	 * need to devise a grateful way to turn off allmulti otherwise
	 * it is left on for the interface when problem is terminated.
	 *
	 * ifr.ifr_flags &= ~IFF_ALLMULTI;  allmulti off
	 */
	ifr->ifr_flags |= IFF_ALLMULTI; /* allmulti on (verified via ifconfig) */
	retval = ioctl(lldp_port->socket, SIOCSIFFLAGS, ifr);
	if (retval == -1)
		lldp_printf(MSG_WARNING, "[%s %d] [WARNING] Interface %s, cannot set flags IFF_ALLMULTI\n", 
							lldp_port->if_name, __FUNCTION__, __LINE__);
	
	/* discover MTU for this interface */
	retval = ioctl(lldp_port->socket, SIOCGIFMTU, ifr);
	if (retval <0) {
		lldp_printf(MSG_ERROR, "[%s %d] [ERROR] Interface %s, cannot discover its MTU\n", 
							lldp_port->if_name, __FUNCTION__, __LINE__);
		return retval;
	}
	

    lldp_port->mtu = ifr->ifr_ifru.ifru_mtu;
    lldp_printf(MSG_INFO, "[%s %d][INFO] Interface %s's MTU is %d\n",
				__FUNCTION__, __LINE__, lldp_port->if_name, lldp_port->mtu);

	lldp_port->rx.frame = calloc(1, lldp_port->mtu - 4);
	lldp_port->tx.frame = calloc(1, lldp_port->mtu - 2);
	
	if (!lldp_port->tx.frame)
        lldp_printf(MSG_ERROR, "[%s %d] [ERROR] Unable to malloc buffer for tx.frame\n", __FUNCTION__, __LINE__);
	
	if (!lldp_port->rx.frame)
        lldp_printf(MSG_ERROR, "[%s %d] [ERROR] Unable to malloc buffer for rx.frame\n", __FUNCTION__, __LINE__);
	
	free(ifr);
	free(sll);
	
	return 0;
}

void socketCleanupLLDP(struct lldp_port *lldp_port)
{
	if (!lldp_port) {
		/* lldp_port was NULL ! */
		return;
	}
	
	if (lldp_port->if_name != NULL) {
		free(lldp_port->rx.frame);
		lldp_port->rx.frame = NULL;
	} else {
		/*lldp_port->rx.frame was NULL ! */
	}
	
	if (lldp_port->tx.frame != NULL) {
		free(lldp_port->tx.frame);
		lldp_port->tx.frame = NULL;
	} else {
		/*lldp_port->tx.frame was NULL ! */
	}
	
	if (lldp_port->if_name != NULL) {
		free(lldp_port->if_name);
		lldp_port->if_name = NULL;
	} else {
		/* Error, lldp_port->if_name was NULL */
	}
}
