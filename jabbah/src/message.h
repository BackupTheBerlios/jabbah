#ifndef _MESSAGE_H_
#define _MESSAGE_H_


#include "jabbah.h"



void            message_register_callback(jabbah_context_t *cnx, void (*cb)(jabbah_context_t * , jabbah_message_t *));
jabbah_node_t * message_create_node(jabbah_context_t *cnx, jabbah_message_t *msg);
void            message_send(jabbah_context_t *cnx, jabbah_message_t *msg);
void            message_send_normal(jabbah_context_t *cnx, char *jid, char *subject, char *msg);
void            message_send_chat(jabbah_context_t *cnx, char *jid, char *msg);

void            message_parse_node(jabbah_context_t *cnx, jabbah_node_t *node);

#endif
