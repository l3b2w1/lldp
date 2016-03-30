#include <arpa/inet.h>
#include <stdint.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include "tlv.h"
#include "tlv_common.h"
#include "lldp_port.h"
#include "lldp_debug.h"
#include "lldp_neighbor.h"
#include "lldp_dunchong.h"

u8 (*validate_tlv[128])(struct lldp_tlv *tlv) = {
	validate_end_of_lldp_pdu_tlv,		/* 0 End of LLDP PDU TLV */
	validate_chassis_id_tlv,			/* 1 Chassis ID TLV */
	validate_port_id_tlv,				/* 2 Port ID TLV */
	validate_ttl_tlv,					/* 3 Time To Live TLV */
	validate_port_description_tlv,		/* 4 Port Description TLV */
	validate_system_name_tlv,			/* 5 System Name TLV */
	validate_system_description_tlv,	/* 6 System Description TLV */
	validate_system_capabilities_tlv,	/* 7 System Capabilities TLV */
	validate_management_address_tlv,	/* 8 Management Address TLV */
	/* 9 - 126 are reservecd and set to NULL in lldp_tlv_validator_init()
	 * 127 is populated for validate_organizationally_specific_tlv in lldp_tlv_validator_init() */
};


char *decode_tlv_subtype(struct lldp_tlv *tlv)
{
	return NULL;
}

char *decode_organizationally_specific_tlv(struct lldp_tlv *tlv)
{
	return NULL;
}

char *interface_subtype_name(uint8_t subtype) 
{
	switch(subtype) {
		case 1: 
			return strdup("Unknown");
			break;
		case 2:
			return strdup("ifIndex");
			break;
		case 3:
			return strdup("System Port Number");
			break;
		default:
			return strdup("Invalid Subtype");
	}
}

// http://www.iana.org/assignments/address-family-numbers
char *decode_iana_address_family(uint8_t family) {
	switch(family)
	{
		case IANA_RESERVED_LOW:
/*
 * #warning - IANA_RESERVED_HIGH is defined as 65536, 
 * which is too big for uint8_t, so am I using the wrong type, 
 * or should it be 255?
 */
		case IANA_RESERVED_HIGH:
			return strdup("Reserved");
		case IANA_IP:
			return strdup("IPv4");
		case IANA_IP6:
			return strdup("IPv6");
		case IANA_NSAP:
			return strdup("NSAP");
		case IANA_HDLC:
			return strdup("HDLC (8-bit multidrop)");
		case IANA_BBN_1822:
			return strdup("BBN 1822");
		case IANA_E_163:
			return strdup("E.163");
		case IANA_E_164_ATM:
			return strdup("E.164 (SMDS, Frame Relay, ATM)");
		case IANA_F_69:
			return strdup("F.69 (Telex)");
		case IANA_X_121:
			return strdup("X.121 (X.25, Frame Relay)");
		case IANA_IPX:
			return strdup("IPX");
		case IANA_APPLETALK:
			return strdup("Appletalk");
		case IANA_DECNET_IV:
			return strdup("Decnet IV");
		case IANA_BANYAN_VINES:
			return strdup("Banyan Vines");
		case IANA_E_164_NSAP:
			return strdup("E.164 with NSAP format subaddress");
		case IANA_DNS:
			return strdup("DNS (Domain Name System)");
		case IANA_DISTINGUISHED:
			return strdup("Distinguished Name");
		case IANA_AS_NUMBER:
			return strdup("AS Number");
		case IANA_XTP_IPV4:
			return strdup("XTP over IP version 4");
		case IANA_XTP_IPV6:
			return strdup("XTP over IP version 6");
		case IANA_FIBRE_PORT_NAME:
			return strdup("Fibre Channel World-Wide Port Name");
		case IANA_FIBRE_NODE_NAME:
			return strdup("Fibre Channel World-Wide Node name");
		case IANA_GWID:
			return strdup("GWID");
		case IANA_AFI_L2VPN:
			return strdup("AFI for L2VPN information");
		default:
			return strdup("Unassigned");
	}
}

char *decode_network_address(u8 *network_address)
{
	char *result         = NULL;
	uint8_t addr_len     = 0;
	uint8_t subtype      = 0;
	char *addr_family    = NULL;
	uint8_t *addr        = NULL;
	uint8_t *tmp         = NULL;

	if(network_address == NULL)
		return NULL;

	addr_len    = network_address[0];
	subtype     = network_address[1];
	addr        = &network_address[2];
	addr_family = decode_iana_address_family(subtype);


	if(addr_family == NULL) {
		lldp_printf(MSG_WARNING, "[%s %d][WARNING]NULL Address Family\n",
					__FUNCTION__, __LINE__);
		return NULL;
	}
	
	result = calloc(1, 2048);
	sprintf(result, "%s - ", addr_family);

	free(addr_family);

	switch (subtype) {
		case IANA_IP:
			if (addr_len == 5) {
				tmp = calloc(1, 16);
				sprintf((char*)tmp, "%d.%d.%d.%d", addr[0], addr[1],
							addr[2], addr[3]);
				strncat(result, (char*)tmp, 2048);
				free(tmp);
			} else {
				lldp_printf(MSG_ERROR, "[%s %d][ERROR] Invalid IPv4 address length: %d\n", __FUNCTION__, __LINE__, addr_len);
				//lldp_hex_strcat((uint8_t *)result, addr, addr_len - 1);
			}
			break;
		default:
			break;
			//lldp_hex_strcat((u8*)result, addr, addr_len - 1);
	}

	return result;
}



struct lldp_tlv *initialize_tlv() 
{
  struct lldp_tlv *tlv = (struct lldp_tlv *)calloc(1, sizeof(struct lldp_tlv));
  return tlv;
}

struct lldp_tlv *create_end_of_lldp_pdu_tlv()
{
    struct lldp_tlv* tlv = initialize_tlv();

    tlv->type = END_OF_LLDPDU_TLV; // Constant defined in lldp_tlv.h
    tlv->length = 0;     // The End of LLDPDU TLV is length 0.

    tlv->value = NULL;

	lldp_printf(MSG_DEBUG, "[%s %d] add end of lldppdu tlv\n", __FUNCTION__, __LINE__);
    return tlv;
}

u8 validate_end_of_lldp_pdu_tlv(struct lldp_tlv *tlv)
{
	if (tlv->length != 0) {
        lldp_printf(MSG_ERROR, "[%s %d][ERROR] TLV type is 'End of LLDPDU' (0), but TLV length is %d when it should be 0!\n",
					__FUNCTION__, __LINE__, tlv->length);
		return XEINVALIDTLV;
	}

	return XVALIDTLV;
}

int validate_length_max_255(struct lldp_tlv *tlv)
{
    //Length will never be below 0 because the variable used is unsigned... 
    if(tlv->length > 255)
    {
        lldp_printf(MSG_ERROR, "[%s %d][ERROR] TLV has invalid length %d.\n\tIt should be between 0 and 255 inclusive!\n",
					__FUNCTION__, __LINE__, tlv->length);

        return XEINVALIDTLV;
    }

    return XVALIDTLV;
}

int validate_length_max_256(struct lldp_tlv *tlv)
{
    if(tlv->length < 2 || tlv->length > 256)
    {
        lldp_printf(MSG_ERROR, "[ERROR] TLV has invalid length '%d'.\n\tIt should be between 2 and 256 inclusive!\n",
					__FUNCTION__, __LINE__, tlv->length);

        return XEINVALIDTLV;
    }

    return XVALIDTLV;
}

struct lldp_tlv *create_chassis_id_tlv(struct lldp_port *lldp_port)
{
	char *p;
	struct lldp_tlv *tlv = initialize_tlv();
	
	tlv->type = CHASSIS_ID_TLV; /* constant defined in tlv.h */
	tlv->length = 7; /* size of MAC + size of the subtype */
	
	tlv->value = calloc(1, tlv->length);
	tlv->value[0] = CHASSIS_ID_MAC_ADDRESS;
	
	memcpy(&tlv->value[1], lldp_port->source_mac, 6);

	p = lldp_port->source_mac;
	lldp_printf(MSG_DEBUG, "[%s %d][DEBUG] lldp_port->source_mac %02x:%02x:%02x:%02x:%02x:%02x\n",
				__FUNCTION__, __LINE__, lldp_port->if_name, p[0], p[1], p[2], p[3], p[4], p[5]);
	
	return tlv;	
}

uint8_t validate_chassis_id_tlv(struct lldp_tlv *tlv)
{
    // Several TLVs have this requirement.
    return validate_length_max_256(tlv);
}

struct lldp_tlv *create_port_id_tlv(struct lldp_port *lldp_port) 
{

    struct lldp_tlv* tlv = initialize_tlv();

    tlv->type = PORT_ID_TLV; // Constant defined in lldp_tlv.h
    tlv->length = 1 + strlen(lldp_port->if_name); //The length of the interface name + the size of the subtype (1 byte)

    tlv->value = calloc(1, tlv->length);

    // PORT_ID_INTERFACE_NAME is a 1-byte value - 5 in this case. Defined in lldp_tlv.h
    tlv->value[0] = PORT_ID_INTERFACE_NAME;


    // We need to start copying at the 2nd byte, so we use [1] here...
    // This reads "memory copy to the destination at the address of tlv->value[1] with the source lldp_port->if_name for strlen(lldp_port->if_name) bytes"
    memcpy(&tlv->value[1], lldp_port->if_name, strlen(lldp_port->if_name));

	lldp_printf(MSG_DEBUG, "[%s %d][DEBUG] add port id tlv, if_name %s\n", 
				__FUNCTION__, __LINE__, lldp_port->if_name);
    return tlv;
}

uint8_t validate_port_id_tlv(struct lldp_tlv *tlv)
{
    // Several TLVs have this requirement.
    return validate_length_max_256(tlv);
}

struct lldp_tlv *create_ttl_tlv(struct lldp_port *lldp_port)
{
	struct lldp_tlv *tlv = initialize_tlv();
	u16 ttl = htons(lldp_port->tx.txTTL);
	
	tlv->type = TIME_TO_LIVE_TLV;
	tlv->length = 2;
	
	tlv->value = calloc(1, tlv->length);
	memcpy(tlv->value, &ttl, tlv->length);
	lldp_printf(MSG_DEBUG, "[%s %d][DEBUG] add ttl tlv, ttl %d\n",
				__FUNCTION__, __LINE__, lldp_port->tx.txTTL);
	
	return tlv;	
}

uint8_t validate_ttl_tlv(struct lldp_tlv *tlv)
{
    if(tlv->length != 2)
    {
        lldp_printf(MSG_ERROR, "[%s %d][ERROR] TLV has invalid length '%d'.\n\tLength should be '2'.\n", 
					__FUNCTION__, __LINE__, tlv->length);

        return XEINVALIDTLV;
    }

    return XVALIDTLV;
}

struct lldp_tlv *create_port_description_tlv(struct lldp_port *lldp_port) 
{
	struct lldp_tlv *tlv = initialize_tlv();

	tlv->type = PORT_DESCRIPTION_TLV;
	tlv->length = strlen(lldp_port->if_name);

	tlv->value = calloc(1, tlv->length);

	memcpy(tlv->value, lldp_port->if_name, strlen(lldp_port->if_name));
	lldp_printf(MSG_DEBUG, "[%s %d][DEBUG] add port description tlv if_name %s\n", __FUNCTION__, __LINE__, lldp_port->if_name);

	return tlv;
}

u8 validate_port_description_tlv(struct lldp_tlv *tlv)
{
	return validate_length_max_255(tlv);
}

struct lldp_tlv *create_system_name_tlv(struct lldp_port *lldp_port)
{
	struct lldp_tlv *tlv = initialize_tlv();

	tlv->type = SYSTEM_NAME_TLV;
	tlv->length = strlen(lldp_systemname);

	tlv->value = calloc(1, tlv->length);

	memcpy(tlv->value, lldp_systemname, tlv->length);
	lldp_printf(MSG_DEBUG, "[%s %d][DEBUG] add sytem name tlv, sysname %s\n", 
				__FUNCTION__, __LINE__, lldp_systemname);

	return tlv;
}

uint8_t validate_system_name_tlv(struct lldp_tlv *tlv)
{
    // Several TLVs have this requirement.
    return validate_length_max_255(tlv);
}

struct lldp_tlv *create_system_description_tlv(struct lldp_port *lldp_port)
{

    struct lldp_tlv* tlv = initialize_tlv();

	tlv->type = SYSTEM_DESCRIPTION_TLV;
	tlv->length = strlen(lldp_systemdesc);

	tlv->value = calloc(1, tlv->length);

	memcpy(tlv->value, lldp_systemdesc, tlv->length);

	lldp_printf(MSG_DEBUG, "[%s %d][DEBUG] add sytem description tlv, systemdesc %s\n",
				__FUNCTION__, __LINE__, lldp_systemdesc);
    return tlv;

}

uint8_t validate_system_description_tlv(struct lldp_tlv *tlv)
{
    // Several TLVs have this requirement.
    return validate_length_max_255(tlv);
}

struct lldp_tlv *create_system_capabilities_tlv(struct lldp_port *lldp_port)
{
    struct lldp_tlv* tlv = initialize_tlv();
    uint16_t capabilities = htons(0x001c);
    uint16_t enabled_caps = htons(0x0008);

    tlv->type = SYSTEM_CAPABILITIES_TLV; // Constant defined in lldp_tlv.h

    tlv->length = 4;

    tlv->value = calloc(1, tlv->length);

    memcpy(&tlv->value[0], &capabilities, sizeof(uint16_t));
    memcpy(&tlv->value[2], &enabled_caps, sizeof(uint16_t));

    return tlv;
}

uint8_t validate_system_capabilities_tlv(struct lldp_tlv *tlv)
{
    if(tlv->length != 4)
    {
        lldp_printf(MSG_ERROR, "[%s %d][ERROR] TLV has invalid length '%d'.\n\tLength should be '4'.\n", __FUNCTION__, __LINE__, tlv->length);

        return XEINVALIDTLV;
    }

    return XVALIDTLV;
}

struct lldp_tlv *create_management_address_tlv(struct lldp_port *lldp_port)
{
    struct lldp_tlv* tlv = initialize_tlv();
	uint32_t if_index = lldp_port->if_index;

	tlv->type = MANAGEMENT_ADDRESS_TLV; // Constant defined in lldp_tlv.h

#define MGMT_ADDR_STR_LEN 1
#define MGMT_ADDR_SUBTYPE 1
#define IPV4_LEN 4
#define IF_NUM_SUBTYPE 1
#define IF_NUM 4
#define OID 1
#define OBJ_IDENTIFIER 0

	// management address string length (1 octet)
	// management address subtype (1 octet)
    // management address (4 bytes for IPv4)
    // interface numbering subtype (1 octet)
    // interface number (4 bytes)
    // OID string length (1 byte)
    // object identifier (0 to 128 octets)
    tlv->length = MGMT_ADDR_STR_LEN + MGMT_ADDR_SUBTYPE + IPV4_LEN + IF_NUM_SUBTYPE + IF_NUM + OID + OBJ_IDENTIFIER ;

    //uint64_t tlv_offset = 0;

    tlv->value = calloc(1, tlv->length);

    // Management address string length
    // subtype of 1 byte + management address length, so 5 for IPv4
    tlv->value[0] = 5;

    // 1 for IPv4 as per http://www.iana.org/assignments/address-family-numbers
    tlv->value[1] = 1;

    // Copy in our IP
    memcpy(&tlv->value[2], lldp_port->source_ipaddr, 4);

    // Interface numbering subtype... system port number in our case.
    tlv->value[6] = 3;

    // Interface number... 4 bytes long, or uint32_t
    memcpy(&tlv->value[7], &lldp_port->if_index, sizeof(uint32_t));

    //lldp_printf(MSG_INFO, "Would stuff interface #: %d\n", if_index);

    // OID - 0 for us
    tlv->value[11] = 0;

	return tlv;
}
 
uint8_t validate_management_address_tlv(struct lldp_tlv *tlv)
{
    return XVALIDTLV;
}

struct lldp_tlv *create_dunchong_tlv(struct lldp_port *lldp_port)
{
    struct lldp_tlv* tlv = initialize_tlv();
	u8 *vendor = "dunchong";
	u8 subtype = LLDP_DUNCHONG_VENDOR;

    tlv->type = ORG_SPECIFIC_TLV; // Constant defined in lldp_tlv.h

    tlv->length = strlen(vendor) + 3 + 1;

    tlv->value = calloc(1, tlv->length);

	memcpy(tlv->value, DCOUI, 3);
	memcpy(&tlv->value[3], &subtype, 1);
    memcpy(&tlv->value[4], vendor, strlen(vendor));

    return tlv;
}

struct lldp_tlv *create_dunchong_ipaddr_tlv(struct lldp_port *lldp_port)
{
    struct lldp_tlv* tlv = initialize_tlv();
	u8 ipaddr[] = {0x0a, 0x00, 0x01, 0xc3};
	u8 subtype = LLDP_DUNCHONG_DEVICE_IPADDR;

    tlv->type = ORG_SPECIFIC_TLV; // Constant defined in lldp_tlv.h

    tlv->length = sizeof(ipaddr) + 3 + 1;

    tlv->value = calloc(1, tlv->length);

	memcpy(tlv->value, DCOUI, 3);
	memcpy(&tlv->value[3], &subtype, 1);
    memcpy(&tlv->value[4], ipaddr, sizeof(ipaddr));

    return tlv;
}

u8 validate_organizationally_specific_tlv(struct lldp_tlv *tlv)
{
    if(tlv->length < 4 || tlv->length > 511)
    {
        lldp_printf(MSG_DEBUG, "[%s %d][ERROR] TLV has invalid length '%d'.\n\tIt should be between 4 and 511 inclusive!\n", 
						__FUNCTION__, __LINE__, tlv->length);

        return XEINVALIDTLV;
    }

    return XVALIDTLV;
}

int validate_generic_tlv(struct lldp_tlv *tlv)
{
#if 0
    lldp_printf(DEBUG_TLV, "Generic TLV Validation for TLV type: %d.\n", tlv->type);
    lldp_printf(DEBUG_TLV, "TLV Info String Length: %d\n", tlv->length);
    lldp_printf(DEBUG_TLV, "TLV Info String: ");
    lldp_hex_dump(DEBUG_TLV, tlv->value, tlv->length);
#endif
    // Length will never fall below 0 because it's an unsigned variable
    if(tlv->length > 511)
    {
        lldp_printf(MSG_ERROR, "[%s %d][ERROR] TLV has invalid length '%d'.\n\tIt should be between 0 and 511 inclusive!\n",
						__FUNCTION__, __LINE__, tlv->length);

        return XEINVALIDTLV;
    }

    return XVALIDTLV;
}


u8 tlvInitializeLLDP(struct lldp_port *lldp_port)
{
    return 0;
}


void tlvCleanupLLDP()
{
}

