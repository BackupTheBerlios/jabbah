#ifndef _PRESENCE_H_
#define _PRESENCE_H_

#include "jabbah.h"


void            presence_init(jabbah_context_t *cnx, int prio);
void            presence_register_callback(jabbah_context_t *cnx, void (*cb)(jabbah_context_t * , jabbah_presence_t *));
jabbah_node_t * presence_create_node(jabbah_presence_t *presence);
void            presence_set_state(jabbah_context_t *cnx, jabbah_state_t state);
void            presence_set_status(jabbah_context_t *cnx, jabbah_state_t state, char *status);
void            presence_set(jabbah_context_t *cnx, jabbah_presence_t *presence);

void            presence_parse_node(jabbah_context_t *cnx, jabbah_node_t *node);

#endif
