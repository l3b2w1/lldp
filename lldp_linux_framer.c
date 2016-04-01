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
		lldp_printf(MSG_ERROR, "[%s %d] [ERROR] Got NULL interface\n", __FUNCTION__, __LINE__);
		return XENOSOCK;
	}
	
	if (lldp_port->if_index <= 0) {
		lldp_printf(MSG_ERROR, "[%s %d] [ERROR] if_index <= 0, does not appear b to be a valid interface\n", __FUNCTION__, __LINE__);
		return XENOSOCK;
	}
	
    lldp_printf(MSG_INFO, "[%s %d]'%s' is index %d\n", 
				__FUNCTION__, __LINE__,
				lldp_port->if_name, lldp_port->if_index);

	/* Set up the raw socket for the LLDP ethertype */
	lldp_port->socket = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_DUNCHONG));
	if (lldp_port->socket < 0) {
        lldp_printf(MSG_ERROR, "[%s %d]Couldn't initialize raw socket for interface %s!\n", __FUNCTION__, __LINE__, lldp_port->if_name);
		return XENOSOCK;
	}
	

	/* Set up the interface for binding */
	sll->sll_family = PF_PACKET;
	sll->sll_ifindex = lldp_port->if_index;
	sll->sll_protocol = htons(ETH_P_DUNCHONG);
	
	retval = bind(lldp_port->socket, (struct sockaddr*)sll, sizeof(struct sockaddr_ll));
	if (retval < 0) { 
		lldp_printf(MSG_ERROR, "[%s %d] [ERROR] binding raw socket to interface %s failed\n", __FUNCTION__, __LINE__, lldp_port->if_name);
		return XENOSOCK;
	}
	
	ifr->ifr_ifindex = lldp_port->if_index;
	
	strncpy(ifr->ifr_name, &lldp_port->if_name[0], strlen(lldp_port->if_name));
	if (strlen(ifr->ifr_name) == 0) {
		lldp_printf(MSG_ERROR, "[%s %d] [ERROR]Invalid interface name\n", __FUNCTION__, __LINE__);
		return XENOSOCK;
	}
	
	lldp_refresh_if_data(lldp_port);
	
	retval = ioctl(lldp_port->socket, SIOCGIFFLAGS, ifr);
	if (retval == -1)
		lldp_printf(MSG_WARNING, "[%s %d] [WARNING] can not get flags for interface %s\n", __FUNCTION__, __LINE__, lldp_port->if_name);
	
	if ((ifr->ifr_flags & IFF_UP) == 0) {
		lldp_printf(MSG_WARNING, "[%s %d] [WARNING] Interface %s is DOWN,portEnabled = 0\n", lldp_port->if_name, __FUNCTION__, __LINE__);
		lldp_port->portEnabled = 0;
	}
	

	 /* set allmulti on interface 
	 * need to devise a grateful way to turn off allmulti otherwise
	 * it is left on for the interface when problem is terminated.
	 *
	 * ifr.ifr_flags &= ~IFF_ALLMULTI;  allmulti off
	 */
	retval = ioctl(lldp_port->socket, SIOCGIFFLAGS, ifr);
	if (retval == -1)
		lldp_printf(MSG_WARNING, "[%s %d] [WARNING] Can not get flags for interface %s\n", 
							__FUNCTION__, __LINE__, lldp_port->if_name);

	/* allmulti on (verified via ifconfig) */
	ifr->ifr_flags  |= IFF_ALLMULTI; 

	retval = ioctl(lldp_port->socket, SIOCSIFFLAGS, ifr);
    if (retval == -1) {
		lldp_printf(MSG_ERROR, "[%s %d]Can't set flags for interface %s\n", __FUNCTION__, __LINE__, lldp_port->if_name);
    }

	
	/* discover MTU for this interface */
	retval = ioctl(lldp_port->socket, SIOCGIFMTU, ifr);
	if (retval < 0) {
		lldp_printf(MSG_ERROR, "[%s %d] [ERROR] Can't determin MTU for interface %s\n", __FUNCTION__, __LINE__, lldp_port->if_name);
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
