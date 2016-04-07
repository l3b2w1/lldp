#ifndef _COMMON_FUNC_H__
#define _COMMON_FUNC_H__

#include <stdint.h>
#include "datatype.h"
void show_lldp_pdu(uint8_t *buf, uint32_t size);
void lldp_hex_dump(uint8_t *buf, uint32_t size);


#endif
