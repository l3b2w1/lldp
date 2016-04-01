#ifndef _LLDP_PORT_H_
#define _LLDP_PORT_H_

#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <linux/if_ether.h>

#include "datatype.h"	// linux/types.h

#define MIN_INTERFACES   1
#define MAX_INTERFACES  16


#define ETH_P_LLDP			0x88cc

#define ETH_P_DUNCHONG		ETH_P_LLDP


struct eth_hdr {
	u8 dst[6];
	u8 src[6];
	u16 ethertype;
};

enum portAdminStatus {
    disabled,
    enabledTxOnly,
    enabledRxOnly,
    enabledRxTx,
};

/**
	LLDP interface transmission statstics
	Part of lldp_tx_port
	Tied to the tx state machine
	*/
struct lldp_tx_port_statistics {
	u64 statsFramesOutTotal; /**< Defined by IEEE802.1AB section 10.5.2.1 > */
};

/**
	LLDP interface transmission timers.
	Part of lldp_tx_port
	These timers are per-interface, and have a resolution of:
	0 < n < 6553 as per the IEEE802.1AB specification.
	*/
struct lldp_tx_port_timers {
	u16 reinitDelay;	/**< IEEE 802.1AB 10.5.3 */
	u16 msgTxHold;		/**< IEEE 802.1AB 10.5.3 */
	u16 msgTxInterval;		/**< IEEE 802.1AB 10.5.3 */
	u16 txDelay;		 /**< IEEE 802.1AB 10.5.3 */
	
	u16 txTTR;	/**< IEEE 802.1AB 10.5.3 - transmit on expire. */
	
	/* not sure what this is for */
	u16 txShutdownWhile;
	u16 txDelayWhile;
};

struct lldp_tx_port {
    u8 *frame;    /**< The tx frame buffer */
    u64 sendsize; /**< The size of our tx frame */
    u8 state;     /**< The tx state for this interface */
    u8 somethingChangedLocal; /**< IEEE 802.1AB var (from where?) */
    u16 txTTL;/**< IEEE 802.1AB var (from where?) */
    struct lldp_tx_port_timers timers; /**< The lldp tx state machine timers for this interface */
    struct lldp_tx_port_statistics statistics; /**< The lldp tx statistics for this interface */
};

struct lldp_rx_port_statistics {
    u64 statsAgeoutsTotal;
    u64 statsFramesDiscardedTotal;
    u64 statsFramesInErrorsTotal;
    u64 statsFramesInTotal;
    u64 statsTLVsDiscardedTotal;
    u64 statsTLVsUnrecognizedTotal;
};

struct lldp_rx_port_timers {
	u16 tooManyNeighborsTimer;
	u16 rxTTL;
};

struct lldp_rx_port {
	u8 *frame;
	ssize_t recvsize;
	u8 state;
	u8 badframe;
	u8 rcvFrame;
	u8 rxInfoAge;
	u8 somethingChangedRemote;
	u8 tooManyNeighbors;
	struct lldp_rx_port_timers timers;
	struct lldp_rx_port_statistics statistics;
	//    struct lldp_msap_cache *msap;
};

struct lldp_port {
  struct lldp_port *next;
  int socket;        // The socket descriptor for this interface.
  char *if_name;     // The interface name.
  u32 if_index; // The interface index.
  u32 mtu;      // The interface MTU.
  u8 source_mac[6];
  u8 source_ipaddr[4];
  struct lldp_rx_port rx;
  struct lldp_tx_port tx;
  u8 portEnabled;
  u8 adminStatus;
  
  /* I'm not sure where this goes... the state machine indicates it's per-port */
  u8 rxChanges;
  
  // I'm really unsure about the best way to handle this... 
  u8 tick;
  time_t last_tick;
  
  //struct lldp_msap *msap_cache;


  // 802.1AB Appendix G flag variables.
  u8  auto_neg_status;
  u8 auto_neg_advertized_capabilities;
  u8 operational_mau_type;
};


#endif