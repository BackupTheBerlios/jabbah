#ifndef _IQ_H_
#define _IQ_H_

#include "jabbah.h"

void iq_send_response(jabbah_context_t *cnx, char *to, char *id, char *xmlns, jabbah_node_t *content);
void iq_send_query(jabbah_context_t *cnx, jabbah_iq_type_t type, char *xmlns, jabbah_node_t *request,
                   void (*cb)(jabbah_context_t * , jabbah_node_t *));

void iq_parse_node(jabbah_context_t *cnx, jabbah_node_t *node);

#endif
