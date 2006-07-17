
#include <stdio.h>

#include "common.h"
#include "node.h"
#include "message.h"

#define CALL_HANDLER(h,x,y) (((void (*)(jabbah_context_t * , jabbah_message_t *))h)(x, y))

                           

void
message_register_callback(jabbah_context_t *cnx, void (*cb)(jabbah_context_t * , jabbah_message_t *))
{
        cnx->message_cb = cb;
}


jabbah_node_t * 
message_create_node(jabbah_context_t *cnx, jabbah_message_t *msg)
{
        jabbah_node_t *msg_node  = NULL;
        jabbah_node_t *subject   = NULL;
        jabbah_node_t *body      = NULL;

        // Init of main node structure
        msg_node = node_init("message");

        // Adding receipent
        msg_node = node_attribute_add(msg_node, "to", msg->to);

        // Adding message type
        if (msg->type != MSG_NORMAL) 
                switch (msg->type) {
                        case MSG_CHAT:
                                msg_node = node_attribute_adda(msg_node, "type", "chat");
                                break;
                        case MSG_GROUPCHAT:
                                msg_node = node_attribute_adda(msg_node, "type", "groupchat");
                                break;
                        case MSG_HEADLINE:
                                msg_node = node_attribute_adda(msg_node, "type", "headline");
                                break;
                        case MSG_ERROR:
                                msg_node = node_attribute_adda(msg_node, "type", "error");
                                break;
                }

        // If we have subject - let's add one
        if (msg->subject != NULL) {
                subject  = node_init("subject");
                subject  = node_value_set(subject, msg->subject);
                msg_node = node_subnode_add(msg_node, subject);
        }

        // Now, let's add the body
        body = node_init("body");
        body = node_value_set(body, msg->body);
        msg_node = node_subnode_add(msg_node, body);

        return msg_node;
        
}



void                      
message_send(jabbah_context_t *cnx, jabbah_message_t *msg)
{
     jabbah_node_t *msg_node = message_create_node(cnx, msg);
     
     // TODO: To zmienic na funkcje, ktora to popchnie w strumien wyjsciowy      
     node_print(cnx, msg_node);

     node_free(msg_node);
}


void
message_send_normal(jabbah_context_t *cnx, char *jid, char *subject, char *msg)
{
        jabbah_message_t m;
        
        m.to = jid;
        m.type = MSG_NORMAL;
        m.from = NULL;
        m.subject = subject;
        m.body = msg;        
        m.message_node = NULL;

        message_send(cnx, &m);
}


void
message_send_chat(jabbah_context_t *cnx, char *jid, char *msg)
{
        jabbah_message_t m;

        m.to = jid;
        m.type = MSG_CHAT;
        m.from = NULL;
        m.subject = NULL;
        m.body = msg;
        m.message_node = NULL;

        message_send(cnx, &m);
}


void
message_parse_node(jabbah_context_t *cnx, jabbah_node_t *node)
{
        jabbah_message_t    msg;
        jabbah_attr_list_t *attr = NULL;
        jabbah_node_t      *snode = NULL;

        if (cnx->message_cb == NULL)
                return;
        
        attr = node->attributes;

        msg.type    = MSG_NORMAL;
        msg.from    = NULL;
        msg.to      = NULL;
        msg.subject = NULL;
        msg.body    = NULL;
                
        while (attr != NULL) {
                if (!strcmp("from", attr->name))
                        msg.from = attr->value;
                else if (!strcmp("type", attr->name)) {
                        if (!strcmp("normal", attr->value))
                                msg.type = MSG_NORMAL;
                        else if (!strcmp("chat", attr->value))
                                msg.type = MSG_CHAT;
                        else if (!strcmp("groupchat", attr->value))
                                msg.type = MSG_GROUPCHAT;
                        else if (!strcmp("error", attr->value))
                                msg.type = MSG_ERROR;
                        else if (!strcmp("headline", attr->value))
                                msg.type = MSG_HEADLINE;
                }
                attr = attr->next;
        }

        snode = node->subnodes;

        while (snode != NULL) {
                if (!strcmp("subject", snode->name))
                        msg.subject = snode->value;
                else if (!strcmp("body", snode->name))
                        msg.body = snode->value;
                snode = snode->next;
        }

        msg.message_node = node;

        CALL_HANDLER(cnx->message_cb, cnx, &msg);
}
        
