#ifndef _COMMON_FUNC_H__
#define _COMMON_FUNC_H__

#include <stdint.h>
void show_lldp_pdu(uint8_t *buf, int32_t size);
void lldp_hex_dump(uint8_t *buf, int32_t size);
void prefix_hex_dump(char *info, uint8_t *buf, int32_t size);


#endif
