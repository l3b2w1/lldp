#include <arpa/inet.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "rx_sm.h"
#include "lldp_port.h"
#include "tlv.h"
#include "tlv_common.h"
#include "common_func.h"
#include "msap.h"
#include "lldp_debug.h"
#include "lldp_dunchong.h"

uint8_t badFrame;

extern void tx_do_tx_setip_frame(struct lldp_port *lldp_port);

#define IS_DC_OUI(tlv) \
	((tlv->value[0] == 0xA4) && (tlv->value[1] == 0xFB) \
	 && (tlv->value[2] == 0x8D))

/* Defined by the IEEE 802.1AB standard */
uint8_t rxInitializeLLDP(struct lldp_port *lldp_port) {
    /* As per IEEE 802.1AB section 10.5.5.3 */
    lldp_port->rx.rcvFrame        = 0;

    /* As per IEEE 802.1AB section 10.1.2 */
    lldp_port->rx.tooManyNeighbors = 0;

    lldp_port->rx.rxInfoAge = 0;

    mibDeleteObjects(lldp_port);

    return 0;
}

extern int32_t dev_role;
  
void rxBadFrameInfo(uint8_t frameErrors) 
{
	lldp_printf(MSG_WARNING, "[WARNING] This frame had %d errors!\n", frameErrors);
}

int32_t alloc_ip_for_slave(struct lldp_msap *msap_cache)
{

}
/* Defined by the IEEE 802.1AB standard */
int rxProcessFrame(struct lldp_port *lldp_port)
{
	uint8_t badFrame = 0;

    uint16_t num_tlvs      = 0;
    uint8_t tlv_type       = 0;
    uint16_t tlv_length    = 0;
    uint16_t tlv_offset    = 0;

    struct eth_hdr *ether_hdr;
    struct eth_hdr expect_hdr;

	// The current TLV and respective helper variables
	struct lldp_tlv *tlv     = NULL;
	uint16_t *tlv_hdr        = NULL;
	uint16_t debug_offset    = 0;
	uint8_t *tlv_value = NULL;

    /* The IEEE 802.1AB MSAP is made up of the TLV information string from */
    /* TLV 1 and TLV 2. */
    uint8_t *msap_id           = NULL;
    uint32_t msap_length       = 0;
    uint8_t have_msap          = 0;
    uint8_t have_dc_msap       = 0;
    struct lldp_tlv *msap_tlv1 = NULL;
    struct lldp_tlv *msap_tlv2 = NULL;
    struct lldp_tlv msap_dctlv;

    int8_t cmd[256] = {0};

	static uint8_t ips[] = {169, 254, 30, 30};
	/* temp variables */
	uint8_t *p;

    /*
	 * wifi module this device have (2G/5G)
	 * This list will be added to the MSAP cache
	  */
	struct wifi_mod wifimods[DEVICE_WIFI_MODULES_NUM] = {0};
	int32_t wifinum = 0;
	//struct lldp_tlv_list *wifi_list = NULL;

    /* The MSAP cache for this frame */
    struct lldp_msap *msap_cache = NULL;

    struct lldp_msap *msap = NULL;

	/* dunchong TLV vairiables */
	int32_t role = 0;
	int32_t subtype;
	uint8_t ipaddr[4];
    int32_t setipflag = 0;

	/* wifi module working mode */

#if 0
	expect_hdr.dst[0] = 0x01;
    expect_hdr.dst[1] = 0x80;
    expect_hdr.dst[2] = 0xc2;
    expect_hdr.dst[3] = 0x00;
    expect_hdr.dst[4] = 0x00;
    expect_hdr.dst[5] = 0x0e;
#endif
    expect_hdr.ethertype = htons(0x88cc);

    ether_hdr = (struct eth_hdr *)&lldp_port->rx.frame[0];

	//show_lldp_pdu(lldp_port->rx.frame, lldp_port->rx.recvsize);

	if(ether_hdr->ethertype != expect_hdr.ethertype) {
		lldp_printf(MSG_DEBUG, "[ERROR] This frame has an incorrect ethertype of: '%x'.\n", htons(ether_hdr->ethertype));
		badFrame++;
	}

	do {
		num_tlvs++;

		if (tlv_offset > lldp_port->rx.recvsize) {
			badFrame++;
			break;
		}

		tlv_hdr = (uint16_t *)&lldp_port->rx.frame[sizeof(struct eth_hdr) + tlv_offset];

		/* grab the first 9 bits */
		tlv_length = htons(*tlv_hdr) & 0x01ff;

		/* then shift to get the last 7 bits */
		tlv_type = htons(*tlv_hdr) >> 9;

		//lldp_printf(MSG_DEBUG, "[%s %d]tlv hdr %04x, length %d, type %d\n", 
		//		   __FUNCTION__, __LINE__, *tlv_hdr, tlv_length, tlv_type);
		
		if (tlv_type == END_OF_LLDPDU_TLV)
			break;

		tlv_value = (uint8_t *)&lldp_port->rx.frame[sizeof(struct eth_hdr) + sizeof(*tlv_hdr) + tlv_offset];

		tlv = initialize_tlv();

		if(!tlv) {
			lldp_printf(MSG_DEBUG, "[ERROR] Unable to malloc buffer in %s() at line: %d!\n", __FUNCTION__, __LINE__);
			break;
		}

		tlv->type        = tlv_type;
		tlv->length      = tlv_length;
		if(tlv->length > 0)	  
			tlv->value = calloc(1, tlv_length);

		if(tlv_type == TIME_TO_LIVE_TLV) {
			if(tlv_length != 2) {
				lldp_printf(MSG_DEBUG, "[ERROR] TTL TLV has an invalid length!  Should be '2', but is '%d'\n", tlv_length); 
				/*warning We should actually discard this frame and print out an error...
				  warning Write a unit test to stress this*/
			} else {
				lldp_port->rx.timers.rxTTL = htons(*(uint16_t *)&tlv_value[0]);
				//lldp_printf(MSG_DEBUG, "rxTTL is: %d\n", lldp_port->rx.timers.rxTTL);
			}
		}

		if(tlv->value) {
			memset(tlv->value, 0x0, tlv_length);
			memcpy(tlv->value, tlv_value, tlv_length);
		} 

        /* Begin, Here to Validate the TLV */

        /* End,   Here to Validate the TLV */
		
		/* Store the MSAP elements */
		if(tlv_type == CHASSIS_ID_TLV) {
			msap_tlv1 = initialize_tlv();
			tlvcpy(msap_tlv1, tlv);
		} else if(tlv_type == PORT_ID_TLV) {
			msap_tlv2 = initialize_tlv();
			tlvcpy(msap_tlv2, tlv);

			/* Minus 2, for the chassis id subtype and port id subtype... 
			 * IEEE 802.1AB specifies that the MSAP shall be composed of 
			 * The value of the subtypes. 
			 */
			msap_length = (msap_tlv1->length - 1) + (msap_tlv2->length - 1);

			msap_id = calloc(1, msap_length);
			if(msap_id == NULL)
			{
				lldp_printf(MSG_DEBUG, "[ERROR] Unable to malloc buffer in %s() at line: %d!\n", __FUNCTION__, __LINE__);
			}

			/* Copy the first part of the MSAP  */
			memcpy(msap_id, &msap_tlv1->value[1], msap_tlv1->length - 1);

			/* Copy the second part of the MSAP */ 
			memcpy(&msap_id[msap_tlv1->length - 1], &msap_tlv2->value[1], msap_tlv2->length - 1);

#if 0
			lldp_printf(MSG_DEBUG, "MSAP TLV1 Length: %d\n", msap_tlv1->length);
			lldp_printf(MSG_DEBUG, "MSAP TLV2 Length: %d\n", msap_tlv2->length);

			lldp_printf(MSG_DEBUG, "MSAP is %d bytes: ", msap_length);
#endif
			//debug_hex_printf(MSG_DEBUG, msap_id, msap_length);
			//prefix_hex_dump("msap_id ", msap_id, msap_length);

			// Free the MSAP pieces
			destroy_tlv(&msap_tlv1);
			destroy_tlv(&msap_tlv2);

			msap_tlv1 = NULL;
			msap_tlv2 = NULL;

			have_msap = 1;
		} else if (tlv_type == ORG_SPECIFIC_TLV) {
			if (!IS_DC_OUI(tlv)) {
				destroy_tlv(&tlv);
				continue;
			}

			memset(&msap_dctlv, 0x0, sizeof(struct lldp_tlv));
			tlvcpy(&msap_dctlv, tlv);

			subtype = msap_dctlv.value[3];

			switch (subtype) {
				case LLDP_DUNCHONG_DEVICE_IPADDR:
					memcpy(ipaddr, &msap_dctlv.value[4], sizeof(ipaddr));
					break;
				case LLDP_DUNCHONG_DEVICE_SET_IP:
					memcpy(ipaddr, &msap_dctlv.value[4], sizeof(ipaddr));
                    if (dev_role == LLDP_DUNCHONG_ROLE_SLAVE) {
                        setipflag = 1;
       					p = ipaddr;
    					lldp_printf(MSG_INFO, "[%s %d]allocated ip address %d.%d.%d.%d\n", 
                                __FUNCTION__, __LINE__, p[0], p[1], p[2], p[3]);
                        memset(cmd, 0x0, sizeof(cmd));
                        sprintf(cmd, "ifconfig br0:1 %d.%d.%d.%d netmask 255.255.0.0", p[0], p[1], p[2], p[3]);
                        printf("[Should be br0 for slave] %s\n", cmd);
                        system(cmd);
                    }
					break;
				case LLDP_DUNCHONG_DEVICE_ROLE:
					role = msap_dctlv.value[4];
					break;
				case LLDP_DUNCHONG_DEVICE_WIFI:
                    if (wifinum >= DEVICE_WIFI_MODULES_NUM) {
                        lldp_printf(MSG_ERROR, "have no enough space to store more wifi "
                                                "moudle info coming from the neighbor, just skip\n");
                        break;
                    }
                        
					printf("------------ begin helloworld------\n");
					p = &msap_dctlv.value[4];
					printf("wifi module: %02x:%02x:%02x:%02x:%02x:%02x\n", p[0], p[1], p[2], p[3], p[4], p[5]);
					memcpy(wifimods[wifinum].mac, &msap_dctlv.value[4], 6);
					wifimods[wifinum].mode = msap_dctlv.value[10];
					printf("------------ wifimode %d -----------\n", wifimods[wifinum].mode);
					wifinum++;

					break;
				default:
					break;
			}

			if (dev_role == LLDP_DUNCHONG_ROLE_MASTER && role == LLDP_DUNCHONG_ROLE_SLAVE)
			    have_dc_msap = 1;
			else if (dev_role == LLDP_DUNCHONG_ROLE_SLAVE && role == LLDP_DUNCHONG_ROLE_MASTER)
			    have_dc_msap = 1;
			else 
			  have_dc_msap = 0;
			  
			free(msap_dctlv.value);
		}

		tlv_offset += sizeof(*tlv_hdr) + tlv_length;

		//decode_tlv_subtype(tlv);

		destroy_tlv(&tlv);

	} while (tlv_type != 0);

	lldp_printf(MSG_DEBUG, "[%s %d]local_role %s, remote_role %s, ipaddr %d.%d.%d.%d, have_dc_msap %s\n", 
				__FUNCTION__, __LINE__,
				dev_role ? "slave":"master", role ? "slave":"master", ipaddr[0], 
				ipaddr[1], ipaddr[2], ipaddr[3],
				have_dc_msap ? "yes" : "no");

	if (have_msap && have_dc_msap) {
		/* warning We need to verify whether this is actually the case. */
		lldp_port->rxChanges = TRUE;

		//lldp_printf(MSG_DEBUG, "We have a(n) %d byte MSAP!\n", msap_length);
		//printf("[DC MSAP] role %d, ipaddr %x\n", role, ipaddr);

		msap_cache = calloc(1, sizeof(struct lldp_msap));

		msap_cache->id = msap_id;
		msap_cache->length = msap_length;
		msap_cache->role = role;
		msap_cache->next = NULL;
		msap_cache->rxInfoTTL = lldp_port->rx.timers.rxTTL;
		memcpy(&msap_cache->wifimods, &wifimods, sizeof(wifimods));
		memcpy(msap_cache->ipaddr, ipaddr, 4);

#if 0
		lldp_printf(MSG_DEBUG, "Iterating MSAP Cache...\n");

		iterate_msap_cache(msap_cache);

		lldp_printf(MSG_DEBUG, "Updating MSAP Cache...\n");

		lldp_printf(MSG_DEBUG, "Setting rxInfoTTL to: %d\n", lldp_port->rx.timers.rxTTL);

#endif
        msap = update_msap_cache(lldp_port, msap_cache);

		/* alloc ip address for slave here */
        
        if ((dev_role == LLDP_DUNCHONG_ROLE_MASTER) && (role == LLDP_DUNCHONG_ROLE_SLAVE)) {
            if (msap->allocated == 0) {
                memcpy(msap->ipaddr, ips, 4);
                memcpy(lldp_port->slaveip, ips, 4);	
                memcpy(lldp_port->slavemac, msap->id, 6);
                msap->allocated = 1;

    			tx_do_tx_setip_frame(lldp_port);
                lldp_printf(MSG_INFO, "[%s %d] send out setip lldpdu, "
                                        "mac %02x:%02x:%02x:%02x:%02x:%02x,"
                                        " %d.%d.%d.%d, msap->allocated %d\n ",
                            __FUNCTION__, __LINE__, msap->id[0], msap->id[1], msap->id[2], 
                            msap->id[3], msap->id[4], msap->id[5], 
                            msap->ipaddr[0],msap->ipaddr[1],msap->ipaddr[2],msap->ipaddr[3], msap->allocated);
    			ips[3] += 1;
                if (ips[3] == 255 || ips[3] == 0)
                    ips[3] += 1;
            } else {
                lldp_printf(MSG_INFO, "[%s %d]already msap->allocated %d "
                                        "mac %02x:%02x:%02x:%02x:%02x:%02x,"
                                        " %d.%d.%d.%d\n ", 
                                        __FUNCTION__, __LINE__, 
                                        msap->allocated, msap->id[0], msap->id[1], msap->id[2], 
                                        msap->id[3], msap->id[4], msap->id[5], 
                                        msap->ipaddr[0], msap->ipaddr[1], msap->ipaddr[2], msap->ipaddr[3]);
            }
        }

		if(msap_tlv1 != NULL) {
			lldp_printf(MSG_ERROR, "Error: msap_tlv1 is still allocated!\n");
			free(msap_tlv1);
			msap_tlv1 = NULL;
		}

		if(msap_tlv2 != NULL) {
			lldp_printf(MSG_ERROR, "Error: msap_tlv2 is still allocated!\n");
			free(msap_tlv2);
			msap_tlv2 = NULL;
		}

	}
	else if(have_msap)
	{
		free(msap_id);
		lldp_printf(MSG_ERROR, "[ERROR] No MSAP for TLVs in Frame!\n");
	}

	/* Report frame errors */
	if(badFrame) {
		rxBadFrameInfo(badFrame);
	}

	return badFrame;
}

uint8_t mibDeleteObjects(struct lldp_port *lldp_port)
{
	struct lldp_msap *curr = lldp_port->msap_cache;
	struct lldp_msap *tail = NULL;
	struct lldp_msap *tmp  = NULL;

	while (curr != NULL) {
		if (curr->rxInfoTTL <= 0) {

			/* if the top list is expired, then ajust the list
			 * before we delete the node */
			if (curr == lldp_port->msap_cache)
				lldp_port->msap_cache = curr->next;
			else
				tail->next = curr->next;

			tmp = curr;
			curr = curr->next;

			if (tmp->id != NULL)
				free(tmp->id);

			free(tmp);
		} else {
			tail = curr;
			curr = curr->next;
		}
	}
}

/* Just a stub */
uint8_t mibUpdateObjects(struct lldp_port *lldp_port) {
#if 0
	struct lldp_msap *curr = lldp_port->msap_cache;

	while (curr != NULL) {
		curr = curr->next;
	}

    return 0;
#endif
}

void rx_decrement_timer(uint16_t *timer) 
{
    if ((*timer) > 0)
        (*timer)--;
}

void rx_display_timers(struct lldp_port *lldp_port) 
{
	struct lldp_msap *msap_cache = lldp_port->msap_cache;

	while(msap_cache != NULL) {
		lldp_printf(MSG_DEBUG, "[TIMER] (%s with MSAP: ", lldp_port->if_name);
		//prefix_hex_printf(NULL, msap_cache->id, msap_cache->length);
		lldp_printf(MSG_DEBUG, ") rxInfoTTL: %d\n", msap_cache->rxInfoTTL);

		msap_cache = msap_cache->next;
	}

	lldp_printf(MSG_DEBUG, "[TIMER] (%s) tooManyNeighborsTimer: %d\n", lldp_port->if_name, lldp_port->rx.timers.tooManyNeighborsTimer);
}


void rx_do_update_timers(struct lldp_port* lldp_port)
{
	struct lldp_msap *msap_cache = lldp_port->msap_cache;

	while (msap_cache != NULL) {
		rx_decrement_timer(&msap_cache->rxInfoTTL);

		if (msap_cache->rxInfoTTL <= 0)
			lldp_port->rx.rxInfoAge = TRUE;

		msap_cache = msap_cache->next;
	}

	//rx_decrement_timer(&lldp_port->rx.timers.tooManyNeighborsTimer);

	//rx_display_timers(lldp_port);
}

char *rxStateFromID(uint8_t state)
{
    switch(state)
    {
        case LLDP_WAIT_PORT_OPERATIONAL:
            return "LLDP_WAIT_PORT_OPERATIONAL";
        case DELETE_AGED_INFO:
            return "DELETE_AGED_INFO";
        case RX_LLDP_INITIALIZE:
            return "RX_LLDP_INITIALIZE";
        case RX_WAIT_FOR_FRAME:
            return "RX_WAIT_FOR_FRAME";
        case RX_FRAME:
            return "RX_FRAME";
        case DELETE_INFO:
            return "DELETE_INFO";
        case UPDATE_INFO:
            return "UPDATE_INFO";
    };

    lldp_printf(MSG_ERROR, "[ERROR] Unknown RX State: '%d'\n", state);
    return "Unknown";
}
void rx_do_lldp_wait_port_operational(struct lldp_port *lldp_port) {
    /* As per IEEE 802.1AB 10.5.5.3 state diagram */
}

void rx_do_delete_aged_info(struct lldp_port *lldp_port)
{
	lldp_port->rx.somethingChangedRemote = FALSE;
	mibDeleteObjects(lldp_port);
	lldp_port->rx.rxInfoAge = FALSE;
	lldp_port->rx.somethingChangedRemote = TRUE;
}	

void rx_do_lldp_initialize(struct lldp_port *lldp_port)
{
	rxInitializeLLDP(lldp_port);
	lldp_port->rx.rcvFrame = FALSE;
}

void rx_do_wait_for_frame(struct lldp_port *lldp_port)
{
	lldp_port->rx.badFrame = FALSE;
	lldp_port->rx.rxInfoAge = FALSE;
	lldp_port->rx.somethingChangedRemote = FALSE;
}

void rx_do_rx_frame(struct lldp_port *lldp_port)
{
	lldp_port->rxChanges = FALSE;
	lldp_port->rx.rcvFrame = FALSE;
	//show_lldp_pdu(lldp_port->rx.frame, lldp_port->rx.recvsize);
	rxProcessFrame(lldp_port);

	memset(&lldp_port->rx.frame[0], 0x0, lldp_port->rx.recvsize);
}

void rx_do_delete_info(struct lldp_port *lldp_port)
{
	mibDeleteObjects(lldp_port);
	lldp_port->rx.somethingChangedRemote = TRUE;
}


void rx_do_update_info(struct lldp_port *lldp_port)
{
	mibUpdateObjects(lldp_port);
	lldp_port->rx.somethingChangedRemote = TRUE;
}

void rxChangeToState(struct lldp_port *lldp_port, uint8_t state)
{
    lldp_printf(MSG_INFO, "[%s %d][%s] %s -> %s\n", 
				__FUNCTION__, __LINE__, lldp_port->if_name,
				rxStateFromID(lldp_port->rx.state), rxStateFromID(state));

    switch(state) {
        case LLDP_WAIT_PORT_OPERATIONAL:
			break;    /* do nothing */
        case RX_LLDP_INITIALIZE:
			if (lldp_port->rx.state != LLDP_WAIT_PORT_OPERATIONAL)
				lldp_printf(MSG_ERROR, "[ERROR] Illegal Transition: [%s] %s -> %s\n", lldp_port->if_name, rxStateFromID(lldp_port->rx.state), rxStateFromID(state));
			break;
		case DELETE_AGED_INFO:
			if (lldp_port->rx.state != LLDP_WAIT_PORT_OPERATIONAL)
				lldp_printf(MSG_DEBUG, "[ERROR] Illegal Transition: [%s] %s -> %s\n", lldp_port->if_name, rxStateFromID(lldp_port->rx.state), rxStateFromID(state));      
				break;
		case RX_WAIT_FOR_FRAME:
				if(!(lldp_port->rx.state == RX_LLDP_INITIALIZE ||
								lldp_port->rx.state == DELETE_INFO ||
								lldp_port->rx.state == UPDATE_INFO ||
								lldp_port->rx.state == RX_FRAME))
					lldp_printf(MSG_DEBUG, "[ERROR] Illegal Transition: [%s] %s -> %s\n", lldp_port->if_name, rxStateFromID(lldp_port->rx.state), rxStateFromID(state));      
				break;
		case RX_FRAME:
				if(lldp_port->rx.state != RX_WAIT_FOR_FRAME)
					lldp_printf(MSG_DEBUG, "[ERROR] Illegal Transition: [%s] %s -> %s\n", lldp_port->if_name, rxStateFromID(lldp_port->rx.state), rxStateFromID(state));      
				break;
        case DELETE_INFO:
				if(!(lldp_port->rx.state == RX_WAIT_FOR_FRAME ||
								lldp_port->rx.state == RX_FRAME))
					lldp_printf(MSG_DEBUG, "[ERROR] Illegal Transition: [%s] %s -> %s\n", lldp_port->if_name, rxStateFromID(lldp_port->rx.state), rxStateFromID(state));      
				break;
		case UPDATE_INFO:
				if(lldp_port->rx.state != RX_FRAME)
					lldp_printf(MSG_DEBUG, "[ERROR] Illegal Transition: [%s] %s -> %s\n", lldp_port->if_name, rxStateFromID(lldp_port->rx.state), rxStateFromID(state));      
				break;
        default:
				lldp_printf(MSG_DEBUG, "[ERROR] Illegal Transition: [%s] %s -> %s\n", lldp_port->if_name, rxStateFromID(lldp_port->rx.state), rxStateFromID(state));      
	}

    /* Now update the interface state */
    lldp_port->rx.state = state;
}

uint8_t rxGlobalStatemachineRun(struct lldp_port *lldp_port)
{
	lldp_printf(MSG_INFO, "[%s %d]rxInfoAge %d, portEnabled %d, rcvFrame %d\n", 
				__FUNCTION__, __LINE__, 
				lldp_port->rx.rxInfoAge, lldp_port->portEnabled,
				lldp_port->rx.rcvFrame);
	if ((lldp_port->rx.rxInfoAge == FALSE) && (lldp_port->portEnabled == FALSE))
		rxChangeToState(lldp_port, LLDP_WAIT_PORT_OPERATIONAL);

	show_lldp_pdu(lldp_port->rx.frame, lldp_port->rx.recvsize);
	switch (lldp_port->rx.state) {
		case LLDP_WAIT_PORT_OPERATIONAL:
			if(lldp_port->rx.rxInfoAge == TRUE)
				rxChangeToState(lldp_port, DELETE_AGED_INFO);
			else if(lldp_port->portEnabled == TRUE) 
				rxChangeToState(lldp_port, RX_LLDP_INITIALIZE);
			break;
		case DELETE_AGED_INFO:
			rxChangeToState(lldp_port, LLDP_WAIT_PORT_OPERATIONAL);
			break;
		case RX_LLDP_INITIALIZE:
			if((lldp_port->adminStatus == enabledRxTx) || (lldp_port->adminStatus == enabledRxOnly))
				rxChangeToState(lldp_port, RX_WAIT_FOR_FRAME);
			break;
		case RX_WAIT_FOR_FRAME:
			if(lldp_port->rx.rxInfoAge == TRUE)
				rxChangeToState(lldp_port, DELETE_INFO);
			else if(lldp_port->rx.rcvFrame == TRUE)
				rxChangeToState(lldp_port, RX_FRAME);
			break;
		case DELETE_INFO:
			rxChangeToState(lldp_port, RX_WAIT_FOR_FRAME);
			break;
		case RX_FRAME:
			if(lldp_port->rx.timers.rxTTL == 0)
				rxChangeToState(lldp_port, DELETE_INFO);
			else if((lldp_port->rx.timers.rxTTL != 0) && (lldp_port->rxChanges == TRUE))
				rxChangeToState(lldp_port, UPDATE_INFO);
			else
				rxChangeToState(lldp_port, RX_WAIT_FOR_FRAME);
			break;
		case UPDATE_INFO:
			rxChangeToState(lldp_port, RX_WAIT_FOR_FRAME);
			break;
		default:
			lldp_printf(MSG_DEBUG, "[ERROR] The RX Global State Machine is broken!\n");
	}

	return 0;
}

void rxStatemachineRun(struct lldp_port *lldp_port)
{
    rxGlobalStatemachineRun(lldp_port);

	switch (lldp_port->rx.state) {
		case LLDP_WAIT_PORT_OPERATIONAL:
			rx_do_lldp_wait_port_operational(lldp_port);
	 		break;
		case DELETE_AGED_INFO:
			rx_do_delete_aged_info(lldp_port);
	 		break;
		case RX_LLDP_INITIALIZE:
			rx_do_lldp_initialize(lldp_port);
	 		break;
		case RX_WAIT_FOR_FRAME:
			rx_do_wait_for_frame(lldp_port);
	 		break;
		case RX_FRAME:
			rx_do_rx_frame(lldp_port);
	 		break;
		case DELETE_INFO:
			rx_do_delete_info(lldp_port);
	 		break;
		case UPDATE_INFO:
			rx_do_update_info(lldp_port);
	 		break;
		default:
			lldp_printf(MSG_DEBUG, "[ERROR] The RX State Machine is broken!\n");      
	}

	rx_do_update_timers(lldp_port);
}

