/*******************************************************************
 *
 * OpenLLDP TX Statemachine
 *
 * See LICENSE file for more info.
 * 
 * File: lldp_sm_tx.c
 * 
 * Authors: Terry Simons (terry.simons@gmail.com)
 *
 *******************************************************************/

#include <stdint.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <malloc.h>
#include <stdlib.h>
#include "tx_sm.h"
#include "lldp_port.h"
#include "lldp_debug.h"
#include "tlv_common.h"
#include "tlv.h"
#include "common_func.h"

#include "lldp_linux_framer.h"

void mibConstrInfoLLDPDU(struct lldp_port *lldp_port)
{
	struct eth_hdr tx_hdr;
	struct lldp_tlv_list *tlv_list = NULL;
	struct lldp_flat_tlv *tlv = NULL;
	struct lldp_tlv_list *tmp = NULL;
	u32 frame_offset = 0;
#if 0
	tx_hdr.dst[0] = 0xff;
	tx_hdr.dst[1] = 0xff;
	tx_hdr.dst[2] = 0xff;
	tx_hdr.dst[3] = 0xff;
	tx_hdr.dst[4] = 0xff;
	tx_hdr.dst[5] = 0xff;

#else
	tx_hdr.dst[0] = 0x01;
	tx_hdr.dst[1] = 0x80;
	tx_hdr.dst[2] = 0xc2;
	tx_hdr.dst[3] = 0x00;
	tx_hdr.dst[4] = 0x00;
	tx_hdr.dst[5] = 0x0e;
#endif
	tx_hdr.src[0] = lldp_port->source_mac[0];
	tx_hdr.src[1] = lldp_port->source_mac[1];
	tx_hdr.src[2] = lldp_port->source_mac[2];
	tx_hdr.src[3] = lldp_port->source_mac[3];
	tx_hdr.src[4] = lldp_port->source_mac[4];
	tx_hdr.src[5] = lldp_port->source_mac[5];

	tx_hdr.ethertype = htons(ETH_P_DUNCHONG);

	frame_offset = 0;

	if (lldp_port->tx.frame == NULL) {
		lldp_printf(MSG_ERROR, "[%s %d] lldp_port->tx.frame is NULL\n", __FUNCTION__, __LINE__);
		return;
	}


	if (lldp_port->tx.frame != NULL)
	  memcpy(&lldp_port->tx.frame[frame_offset], &tx_hdr, sizeof(tx_hdr));

	frame_offset += sizeof(struct eth_hdr);

	/* This TLV *MUST* be first */
	add_tlv(create_chassis_id_tlv(lldp_port), &tlv_list);

	/* This TLV *MUST* be second */
	add_tlv(create_port_id_tlv(lldp_port), &tlv_list);

	/* This TLV *MUST* be third */
	add_tlv(create_ttl_tlv(lldp_port), &tlv_list);


	/* add the optional tlv */
	add_tlv(create_port_description_tlv(lldp_port), &tlv_list);

	add_tlv(create_system_name_tlv(lldp_port), &tlv_list);

	add_tlv(create_system_description_tlv(lldp_port), &tlv_list);

    add_tlv(create_system_capabilities_tlv(lldp_port), &tlv_list);

    add_tlv(create_management_address_tlv(lldp_port), &tlv_list);

	/* Vendor-specific TLVs */
	add_tlv(create_dunchong_tlv(lldp_port), &tlv_list);
	add_tlv(create_dunchong_ipaddr_tlv(lldp_port), &tlv_list);


	/* This TLV "MUST" be last */
	add_tlv(create_end_of_lldp_pdu_tlv(lldp_port), &tlv_list);

	tmp = tlv_list;

	while (tmp != NULL) {
		tlv = flatten_tlv(tmp->tlv);

		if (lldp_port->tx.frame != NULL)
		  memcpy(&lldp_port->tx.frame[frame_offset], tlv->tlv, tlv->size);

		frame_offset += tlv->size;

		destroy_flattened_tlv(&tlv);

		tmp = tmp->next;
	}

	destroy_tlv_list(&tlv_list);

	if (frame_offset < 64)
	  lldp_port->tx.sendsize = 64;
	else
	  lldp_port->tx.sendsize = frame_offset;
	//lldp_printf(MSG_DEBUG, "[%s %d] store info into lldp_port tx.frame buffer\n", __FUNCTION__, __LINE__);
}

void mibConstructShutdownLLDPPDU(struct lldp_port *lldp_port)
{
	struct lldp_tlv * end_tlv;

	end_tlv = create_end_of_lldp_pdu_tlv(lldp_port);

	lldp_printf(MSG_INFO, "[%s %d][INFO] Would send shutdown!");

	if (validate_end_of_lldp_pdu_tlv(end_tlv))
		memcpy(&lldp_port->tx.frame[0], end_tlv, 3);
	else 
		lldp_printf(MSG_INFO, "[%s %d]TLV End of LLDPDU validation failure !\n", __FUNCTION__, __LINE__);

	free(end_tlv);
}

u8 txInitializeLLDP(struct lldp_port *lldp_port)
{
	/* As per IEEE802.1AB section 10.1.1 */
	lldp_port->tx.somethingChangedLocal = 0;

	/* defined in 10.5.2.1 */
	lldp_port->tx.statistics.statsFramesOutTotal = 0;

	/* recommended minimum by 802.1ab 10.5.3.3 */
	lldp_port->tx.timers.reinitDelay = 2; 
	lldp_port->tx.timers.msgTxHold = 4;
	lldp_port->tx.timers.msgTxInterval = 5; // 15000 ~= 3 seconds
	lldp_port->tx.timers.txDelay = 2;

	/* unsure what to set these to ... */
	lldp_port->tx.timers.txShutdownWhile = 0;

	lldp_printf(MSG_DEBUG, "[%s %d] Initialize the TX timers and reset statistics\n", __FUNCTION__, __LINE__);
	/* collect all of the system specific information here */
	return 0;
}

u8 txFrame(struct lldp_port *lldp_port)
{
	lldp_write(lldp_port);

	show_lldp_pdu(lldp_port->tx.frame, lldp_port->tx.sendsize);

	if (lldp_port->tx.frame != NULL)
	  memset(&lldp_port->tx.frame[0], 0x0, lldp_port->tx.sendsize);

	lldp_printf(MSG_DEBUG, "[%s %d] TX frame through interface %s, size %d\n",
				__FUNCTION__, __LINE__, lldp_port->if_name, lldp_port->tx.sendsize);
	return 0;
}

void tx_do_tx_shutdown_frame(struct lldp_port *lldp_port)
{
	mibConstructShutdownLLDPPDU(lldp_port);
	txFrame(lldp_port);
	lldp_port->tx.timers.txShutdownWhile = lldp_port->tx.timers.reinitDelay;
}

void tx_do_tx_idle(struct lldp_port *lldp_port)
{
	/* As per 802.1AB 10.5.4.3 */
	/* I think these belong in the change to state block...
	   lldp_port->tx.txTTL = min(65535, (lldp_port->tx.timers.msgTxInterval * lldp_port->tx.timers.msgTxHold));
	   lldp_port->tx.timers.txTTR = lldp_port->tx.timers.msgTxInterval;
	   lldp_port->tx.somethingChangedLocal = FALSE;
	   lldp_port->tx.timers.txDelayWhile = lldp_port->tx.timers.txDelay;
	   */
}

void tx_do_tx_info_frame(struct lldp_port *lldp_port) {
	/* As per 802.1AB 10.5.4.3 */
	mibConstrInfoLLDPDU(lldp_port);
	txFrame(lldp_port);
}

void tx_do_tx_lldp_initialize(struct lldp_port *lldp_port) {
	/* As per 802.1AB 10.5.4.3 */
	txInitializeLLDP(lldp_port);
}

void txGlobalStatemachineRun(struct lldp_port *lldp_port)
{
	if (lldp_port->portEnabled == 0) {
		lldp_port->portEnabled = 1;
		txChangeToState(lldp_port, TX_LLDP_INITIALIZE);
	}

	switch (lldp_port->tx.state) {
		case TX_LLDP_INITIALIZE:
			if ((lldp_port->adminStatus == enabledRxTx) || (lldp_port->adminStatus == enabledTxOnly))
				txChangeToState(lldp_port, TX_IDLE);
			break;
		case TX_IDLE:
			/* it is time to send a shutdown frame ... */
			if ((lldp_port->adminStatus == disabled) || (lldp_port->adminStatus == enabledRxOnly)) {
				txChangeToState(lldp_port, TX_SHUTDOWN_FRAME);
				break;
			}

			/* it is time to send a frame ... */
			if ((lldp_port->tx.timers.txDelayWhile == 0) && ((lldp_port->tx.timers.txTTR == 0) || (lldp_port->tx.somethingChangedLocal)))
				txChangeToState(lldp_port, TX_INFO_FRAME);
			break;
		case TX_SHUTDOWN_FRAME:
			if (lldp_port->tx.timers.txShutdownWhile == 0)
			  txChangeToState(lldp_port, TX_LLDP_INITIALIZE);
			break;
		case TX_INFO_FRAME:
			txChangeToState(lldp_port, TX_IDLE);
			break;
		default:/*Error, the tx state machine is broken */
			lldp_printf(MSG_ERROR, "[%s %d][ERROR] The TX State Machine is broken!\n", __FUNCTION__, __LINE__);
			break;

	};
}

u16 min(u16 value1, u16 value2)
{
	if(value1 < value2)
		return value1;

	return value2;
}

void tx_decrement_timer(u16 *timer) 
{
	if((*timer) > 0)
	  (*timer)--;
}

void tx_display_timers(struct lldp_port *lldp_port) 
{
#if 1
	lldp_printf(MSG_ERROR, "[IP] (%s) IP: %d.%d.%d.%d\n", lldp_port->if_name, lldp_port->source_ipaddr[0], lldp_port->source_ipaddr[1], lldp_port->source_ipaddr[2], lldp_port->source_ipaddr[3]);

	lldp_printf(MSG_ERROR, "[TIMER] (%s) txTTL: %d\n", lldp_port->if_name, lldp_port->tx.txTTL);
	lldp_printf(MSG_ERROR, "[TIMER] (%s) txTTR: %d\n", lldp_port->if_name, lldp_port->tx.timers.txTTR);
	lldp_printf(MSG_ERROR, "[TIMER] (%s) txDelayWhile: %d\n", lldp_port->if_name, lldp_port->tx.timers.txDelayWhile);
	lldp_printf(MSG_ERROR, "[TIMER] (%s) txShutdownWhile: %d\n", lldp_port->if_name, lldp_port->tx.timers.txShutdownWhile);
#endif
}

void tx_do_update_timers(struct lldp_port *lldp_port) {
	tx_decrement_timer(&lldp_port->tx.timers.txShutdownWhile);
	tx_decrement_timer(&lldp_port->tx.timers.txDelayWhile);
	tx_decrement_timer(&lldp_port->tx.timers.txTTR);

	tx_display_timers(lldp_port);
}

char *txStateFromID(u8 state) {
	switch(state) {
		case TX_LLDP_INITIALIZE:
			return "TX_LLDP_INITIALIZE";
		case TX_IDLE:
			return "TX_IDLE";
		case TX_SHUTDOWN_FRAME:
			return "TX_SHUTDOWN_FRAME";
		case TX_INFO_FRAME:
			return "TX_INFO_FRAME";
	};

	lldp_printf(MSG_ERROR, "[%s %d][ERROR] Unknown TX State: '%d'\n",
				__FUNCTION__, __LINE__, state);
	return "Unknown";
}



void txStatemachineRun(struct lldp_port *lldp_port)
{
	lldp_printf(MSG_DEBUG, "[%s %d][DEBUG] %s -> %s\n", 
					__FUNCTION__, __LINE__, lldp_port->if_name, txStateFromID(lldp_port->tx.state));
	txGlobalStatemachineRun(lldp_port);

	switch (lldp_port->tx.state) {
		case TX_LLDP_INITIALIZE:
			tx_do_tx_lldp_initialize(lldp_port);
			break;
		case TX_IDLE:
			tx_do_tx_idle(lldp_port);
			break;
		case TX_SHUTDOWN_FRAME:
			tx_do_tx_shutdown_frame(lldp_port);
			break;
		case TX_INFO_FRAME:
			tx_do_tx_info_frame(lldp_port);
			break;
		default:
			lldp_printf(MSG_ERROR, "[%s %d][ERROR] The TX State Machine is broken!\n",
						__FUNCTION__, __LINE__);
			break;
	}

	tx_do_update_timers(lldp_port);
}



void txChangeToState(struct lldp_port *lldp_port, u8 state)
{
	switch (state) {
		case TX_LLDP_INITIALIZE:
			if ((lldp_port->tx.state != TX_SHUTDOWN_FRAME) && lldp_port->portEnabled)
			  ; /* Error, Illegal transition */
			break;
		case TX_IDLE:
			if (!(lldp_port->tx.state == TX_LLDP_INITIALIZE || lldp_port->tx.state == TX_INFO_FRAME))
				lldp_printf(MSG_ERROR, "[%s %d] Error, illegal transition\n"); /* Error, Illegal transition */
			lldp_port->tx.txTTL = min(65535, (lldp_port->tx.timers.msgTxInterval * lldp_port->tx.timers.msgTxHold));
			lldp_port->tx.timers.txTTR = lldp_port->tx.timers.msgTxInterval;
			lldp_port->tx.somethingChangedLocal = 0;
			lldp_port->tx.timers.txDelayWhile = lldp_port->tx.timers.txDelay;
			break;
		case TX_SHUTDOWN_FRAME:
			if (lldp_port->tx.state != TX_IDLE)
			  ; /* Error, Illegal transition */
			break;
		default:
			break;/* Error, Illegal transition */
	}

	lldp_port->tx.state = state;
}
