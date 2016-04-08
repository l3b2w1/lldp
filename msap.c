#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "msap.h"
#include "tlv_common.h"
#include "common_func.h"
#include "lldp_debug.h"

struct lldp_msap *create_msap(struct lldp_tlv *tlv1, struct lldp_tlv *tlv2) {
	return NULL;
}


void destroy_list(struct lldp_tlv_list **tlv_list) 
{
	struct lldp_tlv_list *current = *tlv_list;
	
	if (!current)
		;/* warning, to delete empty list */
	
	while (current != NULL) {
		current = current->next;
		
		free((*tlv_list)->tlv->value);
		free((*tlv_list)->tlv);
		
		/* free tlv list node */
		free(*tlv_list);	
		
		(*tlv_list) = current;
	}
}

/* cleanup neighbors info */
void cleanupMsap(struct lldp_port *lldp_port)
{
	struct lldp_msap *curr, *tmp; 

	curr = lldp_port->msap_cache;

	if (!curr)
		return;

	while (curr) {
		free(curr->id);
		tmp = curr;
		curr = curr->next;
		free(tmp);
	}
}


void iterate_msap_cache(struct lldp_msap *msap_cache){
  while(msap_cache != NULL) {
    lldp_printf(MSG_DEBUG, "MSAP cache: %X\n", msap_cache);

    lldp_printf(MSG_DEBUG, "MSAP ID: ");
    //debug_hex_printf(MSG_DEBUG, msap_cache->id, msap_cache->length);
    lldp_hex_dump(msap_cache->id, msap_cache->length);
    
    lldp_printf(MSG_DEBUG, "MSAP Length: %X\n", msap_cache->length);

    lldp_printf(MSG_DEBUG, "MSAP Next: %X\n", msap_cache->next);

	msap_cache = msap_cache->next;
  }
}


void update_msap_cache(struct lldp_port *lldp_port, struct lldp_msap* msap_cache) {
	struct lldp_msap *old_cache = lldp_port->msap_cache;
	struct lldp_msap *new_cache = msap_cache;

	while(old_cache != NULL) {

		if(old_cache->length == new_cache->length) {
			lldp_printf(MSG_DEBUG, "MSAP Length: %X\n", old_cache->length);

			if(memcmp(old_cache->id, new_cache->id, new_cache->length) == 0) {
				lldp_printf(MSG_DEBUG, "MSAP Cache Hit on %s\n", lldp_port->if_name);

				iterate_msap_cache(msap_cache);

				/* warning This is leaking, but I can't figure out why it's crashing when uncommented. */

				destroy_tlv_list(&old_cache->tlv_list);

				old_cache->tlv_list = new_cache->tlv_list;

				// Now free the rest of the MSAP cache
				free(new_cache->id);
				free(new_cache);

				return;
			}

		}

		lldp_printf(MSG_DEBUG, "Checking next MSAP entry...\n");

		old_cache = old_cache->next;
	}

	lldp_printf(MSG_DEBUG, "MSAP Cache Miss on %s\n", lldp_port->if_name);

	new_cache->next = lldp_port->msap_cache;
	lldp_port->msap_cache = new_cache;

	/*warning We are leaking memory... need to dispose of the msap_cache under certain circumstances */
}
