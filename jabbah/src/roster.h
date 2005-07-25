#ifndef _ROSTER_H_
#define _ROSTER_H_

#include "jabbah.h"

void              roster_get(jabbah_context_t *cnx);
jabbah_roster_t * roster_wait(jabbah_context_t *cnx);
void              roster_free(jabbah_context_t *cnx);

void              roster_parse_node (jabbah_context_t *cnx, jabbah_node_t *node);
#endif
