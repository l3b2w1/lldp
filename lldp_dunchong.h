#pragma once


/* Dunchong OUI */
//uint8_t DCOUI[] = {0xA4, 0xFB, 0x8D};

#define LLDP_DUNCHONG_VENDOR			0 /* device vendor name */
#define LLDP_DUNCHONG_DEVICE_TYPE		1 /* board type */	
#define LLDP_DUNCHONG_DEVICE_SN			2 /* device serial number */	
#define LLDP_DUNCHONG_DEVICE_IPADDR		3 /* ip address, should allocated by master ap to slave ap */	
#define LLDP_DUNCHONG_DEVICE_VERSION	4 /* software version */	
#define LLDP_DUNCHONG_DEVICE_ROLE		5 /* master or slave */
#define LLDP_DUNCHONG_DEVICE_SET_IP		6 /* alloc ip addr for slave */
#define LLDP_DUNCHONG_DEVICE_WIFI		7 /* wifi interface working mode 2G or 5G */ 

#define LLDP_DUNCHONG_ROLE_MASTER		0
#define LLDP_DUNCHONG_ROLE_SLAVE		1	

#define LLDP_DUNCHONG_WIFI_2G			0x1
#define LLDP_DUNCHONG_WIFI_5G			0x2	
