#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#include "lldp_debug.h"
#include "tlv.h"
#include "tlv_common.h"

struct lldp_flat_tlv *flatten_tlv(struct lldp_tlv *tlv)
{
	uint16_t type_and_length = 0;
	struct lldp_flat_tlv *flat_tlv = NULL;
	
	if (!tlv) {
		lldp_printf(MSG_WARNING, "[%s %d] tlv is NULL\n", __FUNCTION__, __LINE__);
		return NULL;
	}
	
	type_and_length = tlv->type;
	
	type_and_length = type_and_length << 9;
	type_and_length |= tlv->length;
	
	/* convert it to network byte order */
	type_and_length = htons(type_and_length);
	
	/* now cram all of bits into our flat TLV container. */
	flat_tlv = malloc(sizeof(struct lldp_flat_tlv));
	if (!flat_tlv)
		return NULL;
	
	/* malloc for the size of the entire TLV, plus the 2 bytes for type and length */
	flat_tlv->size = tlv->length + 2;
	
	flat_tlv->tlv = malloc(flat_tlv->size);
	memset(&flat_tlv->tlv[0], 0x0, flat_tlv->size);
	
	/* 1st, copy in the type and length */
	memcpy(&flat_tlv->tlv[0], &type_and_length, sizeof(type_and_length));
	
	//lldp_printf(MSG_WARNING, "[%s %d]\n", __FUNCTION__, __LINE__);
	/* 2nd, copy in the value, for the size of tlv->length */
	memcpy(&flat_tlv->tlv[sizeof(type_and_length)], tlv->value, tlv->length);
	
	return flat_tlv;
}

void destroy_tlv(struct lldp_tlv **tlv)
{
	if ((tlv != NULL && (*tlv) != NULL)) {
		if((*tlv)->value != NULL) {
			free((*tlv)->value);
			((*tlv)->value) = NULL;
		}
		
		free(*tlv);
		(*tlv) = NULL;
	}
}

void destroy_flattened_tlv(struct lldp_flat_tlv **tlv) 
{
	if((tlv != NULL && (*tlv) != NULL)) {
		if((*tlv)->tlv != NULL) {
			free((*tlv)->tlv);
			(*tlv)->tlv = NULL;
			
			free(*tlv);
			(*tlv) = NULL;
		}
	}
}
/** */
void destroy_tlv_list(struct lldp_tlv_list **tlv_list) {
    struct lldp_tlv_list *current  = *tlv_list;

    if(current == NULL) {
        lldp_printf(MSG_WARNING, "[%s %d][WARNING] Asked to delete empty list!\n",
					__FUNCTION__, __LINE__);
		return;
    }

    //lldp_printf(MSG_DEBUG, "[%s %d][DEBUG]Entering Destroy loop\n", __FUNCTION__, __LINE__);

    while(current != NULL) {   

//      lldp_printf(MSG_DEBUG, "current = %X\n", current);
   
 //     lldp_printf(MSG_DEBUG, "current->next = %X\n", current->next);

        current = current->next;

//	lldp_printf(MSG_DEBUG, "Freeing TLV Info String.\n");

        free((*tlv_list)->tlv->value);

//	lldp_printf(MSG_DEBUG, "Freeing TLV.\n");
        free((*tlv_list)->tlv);

//	lldp_printf(MSG_DEBUG, "Freeing TLV List Node.\n");
        free(*tlv_list);

        (*tlv_list) = current;
    }
}


void mdestroy_tlv_list(struct lldp_tlv_list **tlv_list) 
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


void add_tlv(struct lldp_tlv *tlv, struct lldp_tlv_list **tlv_list)
{
	struct lldp_tlv_list *tail = NULL;
	struct lldp_tlv_list *tmp = *tlv_list;
	
	if (!tlv) {
		lldp_printf(MSG_WARNING, "[%s %d] pointer tlv is NULL, no add and return \n", __FUNCTION__, __LINE__);
		return;
	}
	
	tail = malloc(sizeof(struct lldp_tlv_list));
	tail->tlv = tlv;
	tail->next = NULL;
	
	//lldp_printf(MSG_DEBUG, "[%s %d] Setting temp->next to %X\n", __FUNCTION__, __LINE__, tail);
	if ((*tlv_list) == NULL) {
		(*tlv_list) = tail;
	} else {
		while (tmp->next != NULL)
			tmp = tmp->next;
		tmp->next = tail;
	}
}

/* A helper function to explode a flattened TLV */
struct lldp_tlv *explode_tlv(struct lldp_flat_tlv *flat_tlv)
{
	uint16_t type_and_length = 0;
	struct lldp_tlv *tlv = NULL;

	tlv = calloc(1, sizeof(struct lldp_tlv));

	if (!tlv) {
		lldp_printf(MSG_ERROR, "[%s %d][ERROR] Calloc struct lldp_tlv failed !\n",
						__FUNCTION__, __LINE__);
		return NULL;
	}

	/* suck the type and length out... */
	type_and_length = *(uint16_t*)&tlv[0];

	tlv->length		 = type_and_length & 511;
	type_and_length	 = type_and_length >> 9;
	tlv->type		 = (uint8_t)type_and_length;

	tlv->value = calloc(1, tlv->length);

	if (!tlv->value) {
		free(tlv);
		lldp_printf(MSG_ERROR, "[%s %d][ERROR] Calloc tlv->value failed !\n",
						__FUNCTION__, __LINE__);
		return NULL;
	}

	/* Copy the info string into our tlv->value */
	memcpy(&tlv->value[0], &flat_tlv->tlv[sizeof(type_and_length)], 
				tlv->length);

	return tlv;
}
