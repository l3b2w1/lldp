#include <arpa/inet.h>
#include <strings.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "rx_sm.h"
#include "lldp_port.h"
#include "tlv.h"
#include "tlv_common.h"
//#include "msap.h"
//#include "lldp_debug.h"

/* Defined by the IEEE 802.1AB standard */
u8 rxInitializeLLDP(struct lldp_port *lldp_port) {
    /* As per IEEE 802.1AB section 10.5.5.3 */
    lldp_port->rx.rcvFrame        = 0;

    /* As per IEEE 802.1AB section 10.1.2 */
    lldp_port->rx.tooManyNeighbors = 0;

    lldp_port->rx.rxInfoAge = 0;

    //mibDeleteObjects(lldp_port);

    return 0;
}

