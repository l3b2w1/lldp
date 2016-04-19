#ifndef _LLDP_TX_SM_H_
#define _LLDP_TX_SM_H_

#include <stdint.h>
#include "lldp_port.h"

// States from the transmit state machine diagram (IEEE 802.1AB 10.5.4.3)
#define TX_LLDP_INITIALIZE 0 /**< Initialize state from IEEE 802.1AB 10.5.4.3 */
#define TX_IDLE            1 /**< Idle state from IEEE 802.1AB 10.5.4.3 */
#define TX_SHUTDOWN_FRAME  2 /**< Shutdown state from IEEE 802.1AB 10.5.4.3 */
#define TX_INFO_FRAME      3 /**< Transmit state from IEEE 802.1AB 10.5.4.3 */
// End states from the trnsmit state machine diagram


/* Defined by the 802.1AB specification */
uint8_t txInitializeLLDP(struct lldp_port *lldp_port);
void mibConstrInfoLLDPDU(struct lldp_port *lldp_port);
void mibConstrShutdownLLDPDU();
uint8_t txFrame();
void txStatemachineRun(struct lldp_port *lldp_port);
void txChangeToState(struct lldp_port *lldp_port, uint8_t state);
void tx_do_tx_lldp_initialize(struct lldp_port *lldp_port);
void tx_do_tx_setip_frame(struct lldp_port *lldp_port);
/* End Defined by the 802.1AB specification */

#endif
