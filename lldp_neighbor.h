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
int32_t get_wifi_mode();
int32_t get_dev_role();

char *lldp_neighbor_info(struct lldp_port *lldp_ports);
char *lldp_neighbor_information(struct lldp_port *lldp_ports);

extern int32_t dev_role;

#define IS_MASTER() (dev_role == LLDP_DUNCHONG_ROLE_MASTER)

#endif /* _LLDP_NEIGHBOR_H_ */
