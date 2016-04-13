/** @file lldp_linux_framer.h
 * 
 * See LICENSE file for more info.
 *
 * Authors: Terry Simons (terry.simons@gmail.com)
 * 
 **/

#ifndef __LLDP_LINUX_FRAMER_H__
#define __LLDP_LINUX_FRAMER_H__

#include "lldp_port.h"
static int _getmac(struct lldp_port *lldp_port);
static int _getip(struct lldp_port *lldp_port);

ssize_t lldp_read(struct lldp_port *lldp_port);
ssize_t lldp_write(struct lldp_port *lldp_port);

int socketInitializeLLDP(struct lldp_port *lldp_port);
void socketCleanupLLDP(struct lldp_port *lldp_port);

void refreshInterfaceData(struct lldp_port *lldp_port);
int lldp_init_socket(struct lldp_port *lldp_port);
int get_wifi_interface();


	

#endif /* __LLDP_LINUX_FRAMER_H__ */

