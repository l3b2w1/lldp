#ifndef _LLDP_NEIGHBOR_H_
#define _LLDP_NEIGHBOR_H_


#include <sys/un.h>
#include "lldp_port.h"

char lldp_systemname[512];
char lldp_systemdesc[512];

int neighbor_local_sd;
int neighbor_remote_sd;

struct sockaddr_un local;
struct sockaddr_un remote;

int get_sys_desc(void);
int get_sys_fqdn(void);

char *lldp_neighbor_information(struct lldp_port *lldp_ports);


#endif /* _LLDP_NEIGHBOR_H_ */
