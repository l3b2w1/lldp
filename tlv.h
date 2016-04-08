#ifndef _LLDP_TLV_H_
#define _LLDP_TLV_H_

#include "lldp_port.h"
#include "datatype.h"

/* TLV Types from section 9.4.1 of IEEE 802.1AB */
#define END_OF_LLDPDU_TLV       0    /* MANDATORY */
#define CHASSIS_ID_TLV          1    /* MANDATORY */
#define PORT_ID_TLV             2    /* MANDATORY */
#define TIME_TO_LIVE_TLV        3    /* MANDATORY */
#define PORT_DESCRIPTION_TLV    4    /* OPTIONAL  */
#define SYSTEM_NAME_TLV         5    /* OPTIONAL  */
#define SYSTEM_DESCRIPTION_TLV  6    /* OPTIONAL  */
#define SYSTEM_CAPABILITIES_TLV 7    /* OPTIONAL  */
#define MANAGEMENT_ADDRESS_TLV  8    /* OPTIONAL  */
/* 9 - 126 are reserved */
#define ORG_SPECIFIC_TLV        127  /* OPTIONAL */

/* Chassis ID TLV Subtypes */
/* 0 is reserved */
#define CHASSIS_ID_CHASSIS_COMPONENT 1
#define CHASSIS_ID_INTERFACE_ALIAS   2
#define CHASSIS_ID_PORT_COMPONENT    3
#define CHASSIS_ID_MAC_ADDRESS       4
#define CHASSIS_ID_NETWORK_ADDRESS   5
#define CHASSIS_ID_INTERFACE_NAME    6
#define CHASSIS_ID_LOCALLY_ASSIGNED  7 
/* 8-255 are reserved */
/* End Chassis ID TLV Subtypes */

/* Port ID TLV Subtypes */
/* 0 is reserved */
#define PORT_ID_INTERFACE_ALIAS  1
#define PORT_ID_PORT_COMPONENT   2
#define PORT_ID_MAC_ADDRESS      3
#define PORT_ID_NETWORK_ADDRESS  4
#define PORT_ID_INTERFACE_NAME   5
#define PORT_ID_AGENT_CIRCUIT_ID 6
#define PORT_ID_LOCALLY_ASSIGNED 7
/* 8-255 are reserved */ 
/* End Port ID TLV Subtypes */

/* System Capabilities TLV Subtypes */
#define SYSTEM_CAPABILITY_OTHER     1
#define SYSTEM_CAPABILITY_REPEATER  2
#define SYSTEM_CAPABILITY_BRIDGE    4
#define SYSTEM_CAPABILITY_WLAN      8
#define SYSTEM_CAPABILITY_ROUTER    16
#define SYSTEM_CAPABILITY_TELEPHONE 32
#define SYSTEM_CAPABILITY_DOCSIS    64
#define SYSTEM_CAPABILITY_STATION   128
/* 8 - 15 reserved */
/* End System Capabilities TLV Subtypes */

/* IANA Family Number Assignments */
/* http://www.iana.org/assignments/address-family-numbers */
#define IANA_RESERVED_LOW     0
#define IANA_IP               1
#define IANA_IP6              2
#define IANA_NSAP             3
#define IANA_HDLC             4
#define IANA_BBN_1822         5
#define IANA_802              6
#define IANA_E_163            7
#define IANA_E_164_ATM        8
#define IANA_F_69             9 
#define IANA_X_121           10
#define IANA_IPX             11
#define IANA_APPLETALK       12
#define IANA_DECNET_IV       13
#define IANA_BANYAN_VINES    14
#define IANA_E_164_NSAP      15
#define IANA_DNS             16
#define IANA_DISTINGUISHED   17
#define IANA_AS_NUMBER       18
#define IANA_XTP_IPV4        19
#define IANA_XTP_IPV6        20
#define IANA_XTP_XTP         21
#define IANA_FIBRE_PORT_NAME 22
#define IANA_FIBRE_NODE_NAME 23
#define IANA_GWID            24
#define IANA_AFI_L2VPN       25
// Everything from 26 to 65534 is Unassigned
#define IANA_RESERVED_HIGH   65535
/* End IANA Family Number Assignments */

#define XVALIDTLV     	0
#define XEINVALIDTLV 	-1

//extern u8 (*validate_tlv[])(struct lldp_tlv*);
u8 intializeTLVFunctionValidators();

struct lldp_tlv *create_chassis_id_tlv(struct lldp_port *lldp_port);
uint8_t validate_chassis_id_tlv(struct lldp_tlv *tlv);

struct lldp_tlv *create_port_id_tlv(struct lldp_port *lldp_port);
uint8_t validate_port_id_tlv(struct lldp_tlv *tlv);

struct lldp_tlv *create_ttl_tlv(struct lldp_port *lldp_port);
uint8_t validate_ttl_tlv(struct lldp_tlv *tlv);

struct lldp_tlv *create_end_of_lldp_pdu_tlv();
uint8_t validate_end_of_lldp_pdu_tlv(struct lldp_tlv *tlv);

struct lldp_tlv *create_port_description_tlv(struct lldp_port *lldp_port);
uint8_t validate_port_description_tlv(struct lldp_tlv *tlv);

struct lldp_tlv *create_system_name_tlv(struct lldp_port *lldp_port);
uint8_t validate_system_name_tlv(struct lldp_tlv *tlv);

struct lldp_tlv *create_system_description_tlv(struct lldp_port *lldp_port);
uint8_t validate_system_description_tlv(struct lldp_tlv *tlv);

struct lldp_tlv *create_system_capabilities_tlv(struct lldp_port *lldp_port);
uint8_t validate_system_capabilities_tlv(struct lldp_tlv *tlv);

struct lldp_tlv *create_management_address_tlv(struct lldp_port *lldp_port);
uint8_t validate_management_address_tlv(struct lldp_tlv *tlv);

struct lldp_tlv *create_lldpmed_location_identification_tlv(struct lldp_port *lldp_port);
struct lldp_tlv *validate_lldpmed_location_identification_tlv(struct lldp_tlv *tlv);  // NB todo

// Should probably allow this create function to specify the OUI in question?
struct lldp_tlv *create_organizationally_specific_tlv(struct lldp_port *lldp_port, uint8_t *oui);
uint8_t validate_organizationally_specific_tlv(struct lldp_tlv *tlv);



#define LLDP_BEGIN_RESERVED_TLV 9
#define LLDP_END_RESERVED_TLV 126

char *decode_organizationally_specific_tlv(struct lldp_tlv *tlv);

char *decode_network_address(uint8_t *network_address);
char *decode_tlv_subtype(struct lldp_tlv *tlv);

void tlvCleanupLLDP();
u8 tlvInitializeLLDP(struct lldp_port *lldp_port);

struct lldp_tlv *initialize_tlv();
uint8_t tlvcpy(struct lldp_tlv *dst, struct lldp_tlv *src);

struct lldp_tlv *create_dunchong_tlv(struct lldp_port *lldp_port);
struct lldp_tlv *create_dunchong_ipaddr_tlv(struct lldp_port *lldp_port);
struct lldp_tlv *create_role_tlv(struct lldp_port *lldp_port, int8_t role);
struct lldp_tlv *create_ip_tlv(struct lldp_port *lldp_port, uint32_t ipaddr);

char *tlv_typetoname(uint8_t tlv_type);

#endif
