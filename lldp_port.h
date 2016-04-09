#ifndef _LLDP_PORT_H_
#define _LLDP_PORT_H_

#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <linux/if_ether.h>

#define MIN_INTERFACES   1
#define MAX_INTERFACES  16


#define ETH_P_LLDP			0x88cc

//#define ETH_P_DUNCHONG		ETH_P_LLDP
#define ETH_P_DUNCHONG		ETH_P_ALL

#define TRUE	1
#define FALSE	0

struct eth_hdr {
	uint8_t dst[6];
	uint8_t src[6];
	uint16_t ethertype;
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
	uint64_t statsFramesOutTotal; /**< Defined by IEEE802.1AB section 10.5.2.1 > */
};

/**
  LLDP interface transmission timers.
  Part of lldp_tx_port
  These timers are per-interface, and have a resolution of:
  0 < n < 6553 as per the IEEE802.1AB specification.
  */
struct lldp_tx_port_timers {
	uint16_t reinitDelay;	/**< IEEE 802.1AB 10.5.3 */
	uint16_t msgTxHold;		/**< IEEE 802.1AB 10.5.3 */
	uint16_t msgTxInterval;		/**< IEEE 802.1AB 10.5.3 */
	uint16_t txDelay;		 /**< IEEE 802.1AB 10.5.3 */

	uint16_t txTTR;	/**< IEEE 802.1AB 10.5.3 - transmit on expire. */

	/* not sure what this is for */
	uint16_t txShutdownWhile;
	uint16_t txDelayWhile;
};

struct lldp_tx_port {
	uint8_t *frame;    /**< The tx frame buffer */
	uint64_t sendsize; /**< The size of our tx frame */
	uint8_t state;     /**< The tx state for this interface */
	uint8_t somethingChangedLocal; /**< IEEE 802.1AB var (from where?) */
	uint16_t txTTL;/**< IEEE 802.1AB var (from where?) */
	struct lldp_tx_port_timers timers; /**< The lldp tx state machine timers for this interface */
	struct lldp_tx_port_statistics statistics; /**< The lldp tx statistics for this interface */
};

struct lldp_rx_port_statistics {
	uint64_t statsAgeoutsTotal;
	uint64_t statsFramesDiscardedTotal;
	uint64_t statsFramesInErrorsTotal;
	uint64_t statsFramesInTotal;
	uint64_t statsTLVsDiscardedTotal;
	uint64_t statsTLVsUnrecognizedTotal;
};

struct lldp_rx_port_timers {
	uint16_t tooManyNeighborsTimer;
	uint16_t rxTTL;
};

struct lldp_rx_port {
	uint8_t *frame;
	ssize_t recvsize;
	uint8_t state;
	uint8_t badFrame;
	uint8_t rcvFrame;
	uint8_t rxInfoAge;
	uint8_t somethingChangedRemote;
	uint8_t tooManyNeighbors;
	struct lldp_rx_port_timers timers;
	struct lldp_rx_port_statistics statistics;
	//    struct lldp_msap_cache *msap;
};

struct lldp_port {
	struct lldp_port *next;
	int socket;        // The socket descriptor for this interface.
	char *if_name;     // The interface name.
	uint32_t if_index; // The interface index.
	uint32_t mtu;      // The interface MTU.
	uint8_t source_mac[6];
	uint8_t source_ipaddr[4];
	struct lldp_rx_port rx;
	struct lldp_tx_port tx;
	uint8_t portEnabled;
	uint8_t adminStatus;

	/*
	 * slave mac address, the unicast frame sended to slave
	 * fill it before calling config_XXX_for_slave() 
	 */
	int32_t wifimode; /* wifi network interface working-mode 2G/5G ? */
	int32_t role;	/* master ap or slave ap */

	/* used for txFrame */
	uint8_t slaveip[4];
	uint8_t slavemac[6]; 

	uint8_t masterip[4];


	/* I'm not sure where this goes... the state machine indicates it's per-port */
	uint8_t rxChanges;

	// I'm really unsure about the best way to handle this... 
	uint8_t tick;
	time_t last_tick;

	struct lldp_msap *msap_cache;


	// 802.1AB Appendix G flag variables.
	uint8_t  auto_neg_status;
	uint8_t auto_neg_advertized_capabilities;
	uint8_t operational_mau_type;
};

struct lldp_msap {
	struct lldp_msap *next;
	uint8_t *id;
	uint8_t length;
	struct lldp_tlv_list *tlv_list;
	//uint32_t ipaddr; /* if not allocated by master-ap, NULL */
	uint32_t role;	/* master or slave */

	uint8_t ipaddr[4];
	// XXX Revisit this
	// A pointer to the TTL TLV
	// This is a hack to decrement
	// the timer properly for 
	// lldpneighbors output
	struct lldp_tlv *ttl_tlv;

	/* IEEE 802.1AB MSAP-specific counters */
	uint16_t rxInfoTTL;
};



#endif
