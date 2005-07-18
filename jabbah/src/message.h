#ifndef _MESSAGE_H_
#define _MESSAGE_H_


#include "node.h"

typedef enum {
        MSG_NORMAL,
        MSG_CHAT,
        MSG_GROUPCHAT,
        MSG_HEADLINE,
        MSG_ERROR
} jabbah_message_type;

typedef struct _jabbah_message_t {
        char *from;
        char *to;
        char *subject;
        char *body;
        jabbah_message_type type;
        jabbah_node_t *message_node;
} jabbah_message_t;


void            message_register_callback(void (*cb)(jabbah_message_t *));
jabbah_node_t * message_create_node(jabbah_message_t *msg);
void            message_send(jabbah_message_t *msg);
void            message_send_normal(char *jid, char *subject, char *msg);
void            message_send_chat(char *jid, char *msg);

void            message_parse_node(jabbah_node_t *node);

#endif
