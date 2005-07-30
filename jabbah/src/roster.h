#ifndef _ROSTER_H_
#define _ROSTER_H_

#include "jabbah.h"

jabbah_roster_item_t * roster_find(jabbah_context_t *cnx, char *jid);
void                   roster_get(jabbah_context_t *cnx);
jabbah_roster_t *      roster_wait(jabbah_context_t *cnx);
void                   roster_free(jabbah_context_t *cnx);

void                   roster_parse_node (jabbah_context_t *cnx, jabbah_node_t *node);
void                   roster_parse_pres_node (jabbah_context_t *cnx, jabbah_node_t *node);
void                   roster_parse_presence(jabbah_context_t *cnx, jabbah_presence_t *pres);
#endif
