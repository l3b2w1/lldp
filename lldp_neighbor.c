#include <stdint.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include "lldp_port.h"
#include "lldp_debug.h"
#include "lldp_neighbor.h"
#include "tlv_common.h"
#include "tlv.h"
#include "msap.h"
#include "common_func.h"
#include "lldp_dunchong.h"

int32_t dev_role;

/* This wifi working-mode is 2G or 5G ? */
int32_t get_wifi_mode(struct lldp_port* wifi_port)
{
	int8_t cmd[128] = {0};
    int32_t wifimode;
    sprintf(cmd, "iwprive %s get_mode", wifi_port->if_name);
}

int32_t get_dev_role()
{
	printf("[%s %d]device role %s\n", 
            __FUNCTION__, __LINE__, dev_role ? "slave" : "master");
    if (IS_MASTER())
        system("ifconfig br1[should be br1 for master] 169.254.254.254 netmask 255.255.0.0");
	return dev_role;
	return LLDP_DUNCHONG_ROLE_MASTER;
}

int get_sys_desc()
{
	int retval;

	struct utsname sysinfo;

	bzero(&lldp_systemdesc[0], 512);

	retval = uname(&sysinfo);

	if (retval < 0) {
        lldp_printf(MSG_ERROR, "[%s %d][ERROR] Call to uname failed!\n",
						__FUNCTION__, __LINE__);
        exit(0);    
    }
    
	lldp_printf(MSG_INFO, "[%s %d][INFO] sysinfo.machine: %s\n", 
				__FUNCTION__, __LINE__, sysinfo.machine);  
    lldp_printf(MSG_INFO, "[%s %d][INFO] sysinfo.sysname: %s\n", 
				__FUNCTION__, __LINE__, sysinfo.sysname);
    lldp_printf(MSG_INFO, "[%s %d][INFO] sysinfo.release: %s\n", 
				__FUNCTION__, __LINE__, sysinfo.release);

    strcpy(lldp_systemdesc, sysinfo.machine);
    strcat(lldp_systemdesc, "/");
    strcat(lldp_systemdesc, sysinfo.sysname);  
    strcat(lldp_systemdesc, " ");
    strcat(lldp_systemdesc, sysinfo.release);

    lldp_printf(MSG_INFO, "[%s %d][INFO] lldp_systemdesc: %s\n", 
				__FUNCTION__, __LINE__, lldp_systemdesc);

    return(0);
}

int get_sys_fqdn()
{
	int retval;

	bzero(&lldp_systemname[0], 512);

	retval = gethostname(lldp_systemname, 255);

	if (retval < 0) {
        lldp_printf(MSG_ERROR, "[%s %d][INFO] gethostname() failed! retval = %d.\n", 
					__FUNCTION__, __LINE__, retval);
	}

	strcat(lldp_systemname, ".");

	retval = getdomainname(&lldp_systemname[strlen(lldp_systemname)], 255 - strlen(lldp_systemname));

	if (retval < 0)
        lldp_printf(MSG_ERROR, "[%s %d][INFO] getdomainname() failed! retval = %d.\n", 
					__FUNCTION__, __LINE__, retval);

    lldp_printf(MSG_INFO, "[%s %d][INFO] lldp_systemname: %s\n",
				__FUNCTION__, __LINE__, lldp_systemname);
		
	return 0;
}

#define LLDP_NEIGHBOR_INFO_FILE_PATH	"/tmp/lldp_neighbors_info.txt"
/* store neighbor information into file */
char *lldp_neighbor_info(struct lldp_port *lldp_ports)
{
	struct lldp_port *lldp_port = lldp_ports;
	struct lldp_msap *msap_cache = NULL;
	int32_t neighbor_count = 0;
	uint8_t neighbors[1024] = {0};
	uint8_t *p, *pdata;
	int32_t size;
	int32_t fd;
	static int32_t find = 0;
	int32_t i;
	struct wifi_mod *pwm;

	p = neighbors;

	while (lldp_port != NULL) {
		neighbor_count = 0;

		msap_cache = lldp_port->msap_cache;

       // iterate_msap_cache(msap_cache);
        
		while (msap_cache != NULL) {
			neighbor_count++;
            
			for (i = 0; i < DEVICE_WIFI_MODULES_NUM; ++i) {
				pwm = &msap_cache->wifimods[i];
              
				/* wifi module do not exist */
				if (pwm->mode == 0)
					continue;
                
				pdata = pwm->mac;
				size = sprintf(p, "%02x:%02x:%02x:%02x:%02x:%02x;", 
							pdata[0],pdata[1],pdata[2], pdata[3], pdata[4], pdata[5]);
				p += size;

				pdata = (uint8_t*)&msap_cache->ipaddr;
				size = sprintf(p, "%d.%d.%d.%d;%d", pdata[0],pdata[1],
							pdata[2], pdata[3], pwm->mode);
				p += size;

				*p++ = '\r';
				*p++ = '\n';
			}

			pdata = msap_cache->id;
			//prefix_hex_dump("neighbors info msap id", msap_cache->id, msap_cache->length);
		
			lldp_printf(MSG_DEBUG, "[%s %d]msap rxInfoTTL %d, msap ip %d.%d.%d.%d\n", 
						__FUNCTION__, __LINE__, msap_cache->rxInfoTTL, msap_cache->ipaddr[0],
						msap_cache->ipaddr[1],msap_cache->ipaddr[2],msap_cache->ipaddr[3]);
			msap_cache = msap_cache->next;
			find++;
		}

		//*(p-1) = 0;

		lldp_port = lldp_port->next;
	}
	lldp_printf(MSG_DEBUG, "<--------- neighbors ------->\n%s\n", neighbors);
    
#if 1
	/* here to store neighbors info into files  */
	if ((fd = open(LLDP_NEIGHBOR_INFO_FILE_PATH, O_CREAT | O_TRUNC | O_WRONLY, 0666)) < 0) {
		lldp_printf(MSG_ERROR, "[%s %d]Open file %s failed\n", 
					__FUNCTION__, __LINE__, LLDP_NEIGHBOR_INFO_FILE_PATH);
		return NULL;
	}

	if (write(fd, neighbors, strlen(neighbors)) < 0)
	  lldp_printf(MSG_ERROR, "[%s %d]write file %s failed\n", 
				  __FUNCTION__, __LINE__, LLDP_NEIGHBOR_INFO_FILE_PATH);

	close(fd);   

	if (dev_role == LLDP_DUNCHONG_ROLE_SLAVE && find) {
		lldp_printf(MSG_INFO, "[%s %d] we find the Master AP, now exit!\n", 
					__FUNCTION__, __LINE__);
		//exit(0);
	}
	else if (dev_role == LLDP_DUNCHONG_ROLE_MASTER && find >= 2) {
		// exit(0);
	}
#endif
	return NULL;

}

#if 0

char *lldp_neighbor_info(struct lldp_port *lldp_ports)
{
	struct lldp_port *lldp_port = lldp_ports;
	struct lldp_msap *msap_cache = NULL;
	int32_t neighbor_count = 0;
	uint8_t neighbors[1024] = {0};
	uint8_t *p, *pdata;
	int32_t size;
    int32_t fd;
    static int32_t find = 0;

	p = neighbors;
	
	while (lldp_port != NULL) {
		neighbor_count = 0;

		msap_cache = lldp_port->msap_cache;
		
		while (msap_cache != NULL) {
			neighbor_count++;
			pdata = msap_cache->id;
			prefix_hex_dump("neighbors info msap id", msap_cache->id, msap_cache->length);
			size = sprintf(p, "%02x:%02x:%02x:%02x:%02x:%02x;", 
						pdata[0],pdata[1],pdata[2], pdata[3], pdata[4], pdata[5]);
			p += size;

			pdata = (uint8_t*)&msap_cache->ipaddr;
			size = sprintf(p, "%d.%d.%d.%d;", pdata[0],pdata[1],pdata[2], pdata[3]);
			p += size;

			*p++ = '2';// 2G module
			*p++ = '\r';
			*p++ = '\n';

			lldp_printf(MSG_DEBUG, "[%s %d]msap rxInfoTTL %d\n", 
						__FUNCTION__, __LINE__, msap_cache->rxInfoTTL);
			msap_cache = msap_cache->next;
            find++;
		}

		//*(p-1) = 0;

		lldp_port = lldp_port->next;
	}
	printf("%s\n", neighbors);
#if 1
	/* here to store neighbors info into files  */
    if ((fd = open(LLDP_NEIGHBOR_INFO_FILE_PATH, O_CREAT | O_TRUNC | O_WRONLY, 0666)) < 0) {
        lldp_printf(MSG_ERROR, "[%s %d]Open file %s failed\n", 
						__FUNCTION__, __LINE__, LLDP_NEIGHBOR_INFO_FILE_PATH);
        return NULL;
    }

    if (write(fd, neighbors, strlen(neighbors)) < 0)
        lldp_printf(MSG_ERROR, "[%s %d]write file %s failed\n", 
						__FUNCTION__, __LINE__, LLDP_NEIGHBOR_INFO_FILE_PATH);

    close(fd);   
    
    if (dev_role == LLDP_DUNCHONG_ROLE_SLAVE && find) {
        lldp_printf(MSG_INFO, "[%s %d] we find the Master AP, now exit!\n", 
						__FUNCTION__, __LINE__);
        //exit(0);
    }
    else if (dev_role == LLDP_DUNCHONG_ROLE_MASTER && find >= 2) {
       // exit(0);
    }
#endif
	return NULL;
}
#endif
