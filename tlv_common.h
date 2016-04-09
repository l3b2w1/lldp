#ifndef _LLDP_TLV_COMMON_H_
#define _LLDP_TLV_COMMON_H_

#include <sys/types.h>
#include <stdint.h>

struct lldp_flat_tlv {
	uint16_t size;
	uint8_t *tlv;
};

typedef struct lldp_tlv {
	uint8_t type;
	uint16_t length;
	uint8_t *value;
}tlv_t;

struct lldp_tlv_list {
	struct lldp_tlv_list *next;
	struct lldp_tlv *tlv;
};

void add_tlv(struct lldp_tlv *tlv, struct lldp_tlv_list **tlv_list);

struct lldp_flat_tlv *lldp_flatten_tlv(struct lldp_tlv *tlv);

struct lldp_tlv *explode_tlv(struct lldp_flat_tlv *flat_tlv);

void destroy_tlv(struct lldp_tlv **tlv);

void destroy_flattened_tlv(struct lldp_flat_tlv **tlv);

void destroy_tlv_list(struct lldp_tlv_list **tlv_list);

struct lldp_flat_tlv *flatten_tlv(struct lldp_tlv *tlv);

struct lldp_tlv *explode_tlv(struct lldp_flat_tlv *flat_tlv);
#endif





