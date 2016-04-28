#include <stdio.h>
#include <string.h>
#include "common_func.h"
#include "lldp_debug.h"

void prefix_hex_dump(char *info, uint8_t *buf, int32_t size)
{
	int i;
	printf("%s	", info);
	for (i = 0; i < size; ++i)
		printf("%02x ", buf[i]);
	printf("\n");
}


void lldp_hex_dump(uint8_t *buf, int32_t size)
{
	int i, j, line, col;
	line = size / 16;
	col = size % 16;

	uint8_t lastline[16] = {0};
	uint8_t *p;

	if (!buf) {
		lldp_printf(MSG_WARNING, "[%s %d][WARNING]send buffer is NULL, now return\n",
					__FUNCTION__, __LINE__);
		return;
	}
	p = buf;
	
//	lldp_printf(MSG_DEBUG, "[%s %d]>>>>> size %d <<<<<\n", 
//				__FUNCTION__, __LINE__, size);

	for (i = 0; i < line; ++i) {
		lldp_printf(MSG_DEBUG, "%02x %02x %02x %02x %02x %02x %02x %02x    %02x %02x %02x %02x %02x %02x %02x %02x\n",
					p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
					p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
		p += 16;
	}

	if (col != 0) {
		memcpy(lastline, p, col);
		lldp_printf(MSG_DEBUG, "%02x %02x %02x %02x %02x %02x %02x %02x    %02x %02x %02x %02x %02x %02x %02x %02x\n",
					p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
					p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
	}
}

void show_lldp_pdu(uint8_t *buf, int32_t size)
{
	int i, j, line, col;
	line = size / 16;
	col = size % 16;
	uint8_t lastline[16] = {0};
	uint8_t *p;
	
	if (!buf) {
		lldp_printf(MSG_WARNING, "[%s %d][WARNING]send buffer is NULL, now return\n",
					__FUNCTION__, __LINE__);
		return;
	}
	p = buf;
	lldp_printf(MSG_INFO, "--------------- lldp pdu %d bytes-------------\n", size);
	for (i = 0; i < line; ++i) {
		lldp_printf(MSG_INFO, "%02x %02x %02x %02x %02x %02x %02x %02x    %02x %02x %02x %02x %02x %02x %02x %02x\n",
					p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
					p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
		p += 16;
	}

	if (col != 0) {
		memcpy(lastline, p, col);
		p = lastline;
		lldp_printf(MSG_INFO, "%02x %02x %02x %02x %02x %02x %02x %02x    %02x %02x %02x %02x %02x %02x %02x %02x\n",
					p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
					p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
	}

	
}


