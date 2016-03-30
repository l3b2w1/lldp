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


char *lldp_neighbor_information(struct lldp_port *lldp_ports)
{
	struct lldp_port *lldp_port = lldp_ports;
	//struct lldp_msap *masp_cache = NULL;
	struct lldp_tlv_list *tlv_list = NULL;
	int neighbor_count = 0;
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

	return NULL;

}

