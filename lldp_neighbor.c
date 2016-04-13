#include <stdint.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
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
int32_t get_wifi_mode()
{
	
}

int32_t get_dev_role()
{
	printf("[%s %d]device role %d\n", __FUNCTION__, __LINE__, dev_role);
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

#define LLDP_NEIGHBOR_INFO_FILE_PATH	"/tmp/lldp_neighbors_info.log"
/* store neighbor information into file */
uint8_t *lldp_store_neighbor_info(uint8_t *buf)
{

}

char *lldp_neighbor_info(struct lldp_port *lldp_ports)
{
	struct lldp_port *lldp_port = lldp_ports;
	struct lldp_msap *msap_cache = NULL;
	int32_t neighbor_count = 0;
	uint8_t neighbors[1024] = {0};
	uint8_t *p, *pdata;
	int32_t size;

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
		}

		*(p-1) = 0;

		lldp_port = lldp_port->next;
	}
	printf("%s\n", neighbors);

	/* here to store neighbors info into files  */

	return NULL;
}
#if 0
char *lldp_neighbor_information(struct lldp_port *lldp_ports) {
	struct lldp_port *lldp_port      = lldp_ports;
	struct lldp_msap *msap_cache     = NULL;
	struct lldp_tlv_list *tlv_list   = NULL;
	int neighbor_count               = 0;
	char *result = malloc(2048);
	char *buffer = malloc(2048);
	char *info_buffer = malloc(2048);
	char *tmp_buffer = malloc(2048);
	char *tlv_name = NULL;
	char *tlv_subtype = NULL;

	memset(result, 0x0, 2048);
	memset(buffer, 0x0, 2048);
	memset(info_buffer, 0x0, 2048);
	memset(tmp_buffer, 0x0, 2048);

	sprintf(result, "\nOpenLLDP Neighbor Info: \n\n");

	while(lldp_port != NULL) {
		neighbor_count = 0;
		memset(buffer, 0x0, 2048);
		memset(info_buffer, 0x0, 2048);
		memset(tmp_buffer, 0x0, 2048);
		sprintf(buffer, "Interface '%s' has ", lldp_port->if_name);

		strncat(result, buffer, 2048);

		msap_cache = lldp_port->msap_cache;

		while(msap_cache != NULL) {

			neighbor_count++;

			tlv_list = msap_cache->tlv_list;

			sprintf(tmp_buffer, "Neighbor %d:\n", neighbor_count);

			strncat(info_buffer, tmp_buffer, 2048);

			memset(tmp_buffer, 0x0, 2048);


			while(tlv_list != NULL) {
				memset(tmp_buffer, 0x0, 2048);

				if(tlv_list->tlv != NULL) {

					tlv_name = tlv_typetoname(tlv_list->tlv->type);

					if(tlv_name != NULL) {
						sprintf(tmp_buffer, "\t%s: ", tlv_name);

						strncat(info_buffer, tmp_buffer, 2048);

						//free(tlv_name);    
						//tlv_name = NULL;

						memset(tmp_buffer, 0x0, 2048);
#if 0

						//tlv_subtype = decode_tlv_subtype(tlv_list->tlv);

						if(tlv_subtype != NULL) {	    
							sprintf(tmp_buffer, "\t%s\n", tlv_subtype);

							strncat(info_buffer, tmp_buffer, 2048);

							memset(tmp_buffer, 0x0, 2048);

							free(tlv_subtype);
							tlv_subtype = NULL;
						}
#endif
					} else {
						sprintf(tmp_buffer, "\t\tUnknown TLV Type (%d)\n", tlv_list->tlv->type);
						strncat(info_buffer, tmp_buffer, 2048);
					}

				} else {
					lldp_printf(MSG_DEBUG, "Yikes... NULL TLV in MSAP cache!\n");
				}

				tlv_list = tlv_list->next;
			}

			strncat(info_buffer, "\n", 2048);

			msap_cache = msap_cache->next;
		}

		memset(buffer, 0x0, 2048);

		sprintf(buffer, "%d LLDP Neighbors: \n\n", neighbor_count);
		strncat(result, buffer, 2048);
		strncat(result, info_buffer, 2048);

		lldp_port = lldp_port->next;
	}

	free(tmp_buffer);
	free(info_buffer);
	free(buffer);

	return(result);
}
#endif


