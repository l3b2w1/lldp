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

#define IS_DC_OUI(tlv) \
	((tlv->value[0] == 0xA4) && (tlv->value[1] == 0xFB) \
	 && (tlv->value[2] == 0x8D))

/* Defined by the IEEE 802.1AB standard */
u8 rxInitializeLLDP(struct lldp_port *lldp_port) {
    /* As per IEEE 802.1AB section 10.5.5.3 */
    lldp_port->rx.rcvFrame        = 0;

    /* As per IEEE 802.1AB section 10.1.2 */
    lldp_port->rx.tooManyNeighbors = 0;

    lldp_port->rx.rxInfoAge = 0;

    //mibDeleteObjects(lldp_port);

    return 0;
}

int rxProcessFrame(struct lldp_port *lldp_port)
{
	uint8_t badFrame = 0;

	uint16_t last_tlv_type = 0;
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

	// The TLV to be cached for this frame's MSAP
	struct lldp_tlv *cached_tlv = NULL;

    /* The IEEE 802.1AB MSAP is made up of the TLV information string from */
    /* TLV 1 and TLV 2. */
    uint8_t *msap_id           = NULL;
    uint32_t msap_length       = 0;
    uint8_t have_msap          = 0;
    uint8_t have_dc_msap          = 0;
    struct lldp_tlv *msap_tlv1 = NULL;
    struct lldp_tlv *msap_tlv2 = NULL;
    struct lldp_tlv *msap_dctlv = NULL;
    struct lldp_tlv *msap_ttl_tlv = NULL;

    /* The TLV list for this frame */
    /* This list will be added to the MSAP cache */
    struct lldp_tlv_list *tlv_list = NULL;

    /* The MSAP cache for this frame */
    struct lldp_msap *msap_cache = NULL;

	// Variables for location based LLDP-MED TLV
	char *elin   = NULL;
	int calength = 0;
	int catype   = 0;
	
	/* dunchong TLV vairiables */
	int32_t role = 0;
	int32_t subtype;
	uint32_t ipaddr;

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

	printf(" ------------- ifname %s ------\n", lldp_port->if_name);

	show_lldp_pdu(lldp_port->rx.frame, lldp_port->rx.recvsize);

	do {
		num_tlvs++;

		if (tlv_offset > lldp_port->rx.recvsize) {
			badFrame++;
			break;
		}

		tlv_hdr = (uint16_t *)&lldp_port->rx.frame[sizeof(struct eth_hdr) + tlv_offset];

		tlv_length = htons(*tlv_hdr) & 0x01ff;

		tlv_type = htons(*tlv_hdr) >> 9;

		lldp_printf(MSG_DEBUG, "tlv hdr %04x, length %d, type %d\n", 
					*tlv_hdr, tlv_length, tlv_type);
		
		if (tlv_type == 0)
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
				msap_ttl_tlv = tlv;
				lldp_printf(MSG_DEBUG, "rxTTL is: %d\n", lldp_port->rx.timers.rxTTL);
			}
		}

		if(tlv->value) {
			memset(tlv->value, 0x0, tlv_length);
			memcpy(tlv->value, tlv_value, tlv_length);
		} 

        /* Begin, Here to Validate the TLV */

        /* End,   Here to Validate the TLV */
#if 0
		cached_tlv = initialize_tlv();
		if(tlvcpy(cached_tlv, tlv) != 0)
			lldp_printf(MSG_DEBUG, "Error copying TLV for MSAP cache!\n");

		//lldp_printf(MSG_DEBUG, "Adding exploded TLV to MSAP TLV list.\n");
		// Now we can start stuffing the msap data... ;)
		add_tlv(cached_tlv, &tlv_list);

#endif
		/* Store the MSAP elements */
		if(tlv_type == CHASSIS_ID_TLV) {
			lldp_printf(MSG_DEBUG, "Copying TLV1 for MSAP Processing...\n");
			msap_tlv1 = initialize_tlv();
			tlvcpy(msap_tlv1, tlv);
		}

		if(tlv_type == PORT_ID_TLV) {
			lldp_printf(MSG_DEBUG, "Copying TLV2 for MSAP Processing...\n");
			msap_tlv2 = initialize_tlv();
			tlvcpy(msap_tlv2, tlv);

			msap_id = calloc(1, msap_tlv1->length - 1  + msap_tlv2->length - 1);
			if(msap_id == NULL)
			{
				lldp_printf(MSG_DEBUG, "[ERROR] Unable to malloc buffer in %s() at line: %d!\n", __FUNCTION__, __LINE__);
			}
			// Copy the first part of the MSAP 
			memcpy(msap_id, &msap_tlv1->value[1], msap_tlv1->length - 1);

			// Copy the second part of the MSAP 
			memcpy(&msap_id[msap_tlv1->length - 1], &msap_tlv2->value[1], msap_tlv2->length - 1);

			msap_length = (msap_tlv1->length - 1) + (msap_tlv2->length - 1);

#if 0
			lldp_printf(MSG_DEBUG, "MSAP TLV1 Length: %d\n", msap_tlv1->length);
			lldp_printf(MSG_DEBUG, "MSAP TLV2 Length: %d\n", msap_tlv2->length);

			lldp_printf(MSG_DEBUG, "MSAP is %d bytes: ", msap_length);
#endif
			//debug_hex_printf(MSG_DEBUG, msap_id, msap_length);
			lldp_hex_dump(msap_id, msap_length);

			// Free the MSAP pieces
			destroy_tlv(&msap_tlv1);
			destroy_tlv(&msap_tlv2);

			msap_tlv1 = NULL;
			msap_tlv2 = NULL;

			have_msap = 1;
		}

		if (tlv_type == ORG_SPECIFIC_TLV)
		{
			if (!IS_DC_OUI(tlv))
				continue;
#if 1
			have_dc_msap = 1;
			msap_dctlv = initialize_tlv();
			tlvcpy(msap_dctlv, tlv);

			subtype = msap_dctlv->value[3];

			if (subtype == LLDP_DUNCHONG_DEVICE_IPADDR) {
				memcpy(&ipaddr, &msap_dctlv->value[4], sizeof(ipaddr));
				ipaddr = ntohl(ipaddr);
			}
#if 0
			if (subtype == LLDP_DUNCHONG_DEVICE_ROLE)
				role = msap_dctlv->value[4];
#endif

			destroy_tlv(&msap_dctlv);

			msap_dctlv = NULL;
#endif
			
		}

		tlv_offset += sizeof(*tlv_hdr) + tlv_length;

		last_tlv_type = tlv_type;

		//decode_tlv_subtype(tlv);

		destroy_tlv(&tlv);



	} while (tlv_type != 0);

#if 0
	if (num_tlvs <= 3)
	  /* Create a compound offset */
	  debug_offset = tlv_length + sizeof(struct eth_hdr) + tlv_offset + sizeof(*tlv_hdr);

	/* The TLV is telling us to index off the end of the frame... tisk tisk */
	if(debug_offset > lldp_port->rx.recvsize) {
		lldp_printf(MSG_DEBUG, "[ERROR] Received a bad TLV:  %d bytes too long!  Frame will be skipped.\n", debug_offset - lldp_port->rx.recvsize);
		badFrame++;
		break;
	} else {
		/* Temporary Debug to validate above... */
		lldp_printf(MSG_DEBUG, "TLV would read to: %d, Frame ends at: %d\n", debug_offset, lldp_port->rx.recvsize);
	}

#endif


	/* We're done processing the frame and all TLVs... now we can cache it */
	/* Only cache this TLV list if we have an MSAP for it */
	if(have_msap && have_dc_msap)
	{
		/* warning We need to verify whether this is actually the case. */
		lldp_port->rxChanges = 1;

		lldp_printf(MSG_DEBUG, "We have a(n) %d byte MSAP!\n", msap_length);
		printf("DC TLV, role %d, ipaddr %x\n", role, ipaddr);

		msap_cache = calloc(1, sizeof(struct lldp_msap));

		msap_cache->id = msap_id;
		msap_cache->length = msap_length;
	//	msap_cache->tlv_list = tlv_list;
		msap_cache->next = NULL;
		msap_cache->ipaddr = ipaddr;

		msap_cache->ttl_tlv = msap_ttl_tlv;
		msap_ttl_tlv = NULL;

		//lldp_printf(MSG_DEBUG, "Iterating MSAP Cache...\n");

		//iterate_msap_cache(msap_cache);

		lldp_printf(MSG_DEBUG, "Updating MSAP Cache...\n");

		lldp_printf(MSG_DEBUG, "Setting rxInfoTTL to: %d\n", lldp_port->rx.timers.rxTTL);

		msap_cache->rxInfoTTL = lldp_port->rx.timers.rxTTL;

		update_msap_cache(lldp_port, msap_cache);

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
	else
	{
		lldp_printf(MSG_ERROR, "[ERROR] No MSAP for TLVs in Frame!\n");
	}

	/* Report frame errors */
	if(badFrame) {
		//rxBadFrameInfo(badFrame);
	}

	//free(tlv);

	return badFrame;

}
