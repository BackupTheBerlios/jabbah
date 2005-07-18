
#include <stdio.h>

#include "node.h"
#include "presence.h"


#define CALL_HANDLER(h,x) (((void (*)(jabbah_presence_t *))h)(x))
                           
static void *callback = NULL;
static int   priority = -1;


void
presence_init(int prio)
{
        if (prio > -1)
                priority = prio;
}


void                      
presence_register_callback(void (*cb)(jabbah_presence_t *))
{
		callback = cb;
}


jabbah_node_t * 
presence_create_node(jabbah_presence_t *presence)
{
        jabbah_node_t *pres_node = NULL;
        jabbah_node_t *show      = NULL; 
        jabbah_node_t *status    = NULL;
        jabbah_node_t *prio      = NULL;
        char          *c_prio    = NULL;
        int            l_prio    = 0;
        
        // Init main node
        pres_node = node_init("presence");

        // If we going offline, set type of node for unaviable
        if (presence->state == STATE_OFFLINE)
                pres_node = node_attribute_add(pres_node, "type", "unavailable");
        else {
                
                // Now let's check presence if we're not offline
                // If we are not online, we need "show" node
                if (presence->state != STATE_ONLINE) {
                        show = node_init("show");
                        switch (presence->state) {
                                case STATE_CHAT:
                                        show = node_value_set(show, "chat");
                                        break;
                                case STATE_AWAY:
                                        show = node_value_set(show, "away");
                                        break;
                                case STATE_XA:
                                        show = node_value_set(show, "xa");
                                        break;
                                case STATE_DND:
                                        show = node_value_set(show, "dnd");
                                        break;
                        }
                        pres_node = node_subnode_add(pres_node, show);
                }
        }

        // Now, if we have status text, we should add it :)
        if (presence->status != NULL) {
                status = node_init("status");
                status = node_value_set(status, presence->status);
                pres_node = node_subnode_add(pres_node, status);
        }

        if (presence->priority > -1) {
                prio = node_init("priority");
                c_prio = (char *)malloc(sizeof(char)*10);
                l_prio = sprintf(c_prio, "%d", presence->priority);
                prio = node_value_set(prio, c_prio);
                free(c_prio);
                pres_node = node_subnode_add(pres_node, prio);
        }
        
        // Now we can return our main node
        return pres_node;
}


void                      
presence_set_state(jabbah_state_t state)
{
        jabbah_presence_t p;
        
        p.jid    = NULL;
        p.status = NULL;
        p.priority = priority;
        p.state  = state;
        
        presence_set(&p);
}


void                      
presence_set_status(jabbah_state_t state, char *status)
{
        jabbah_presence_t p;
        
        p.jid    = NULL;
        p.status = status;
        p.priority = priority;
        p.state  = state;
        
        presence_set(&p);
        
}

void 
presence_set(jabbah_presence_t *presence)
{
        jabbah_node_t *pres_node = presence_create_node(presence);

        // TODO: Tutaj powinno nastapic wyslanie
        node_print(pres_node);
        
        node_free(pres_node);
}


void                      
presence_parse_node(jabbah_node_t *node)
{
			 jabbah_presence_t pres;
       jabbah_attr_list_t  *attr = NULL;
       jabbah_node_t       *snode = NULL;
       int                              i = 0;

				if (callback == NULL) return;

			 attr = node->attributes;
       
       pres.jid       = NULL;
       pres.state   = STATE_ONLINE;
       pres.status = NULL;

       while(attr != NULL) {
               if (!strcmp("from", attr->name))
                       pres.jid = attr->value;
               else if (!strcmp("type", attr->name) 
                        && !strcmp("unavailable", attr->value))
                       pres.state = STATE_OFFLINE;
               
               attr = attr->next;        
       }
       
       snode = node->subnodes;

       while(snode != NULL) {
               if (!strcmp("show", snode->name)) {
                       if (!strcmp("away", snode->value))
                               pres.state = STATE_AWAY;
                       else if (!strcmp("xa", snode->value))
                               pres.state = STATE_XA;
                       else if (!strcmp("dnd", snode->value))
                               pres.state = STATE_DND;
                       else if (!strcmp("chat", snode->value))
                               pres.state = STATE_CHAT;
               } else if (!strcmp("status", snode->name)) {
                       pres.status = snode->value;
               }
               snode = snode->next;
       }
      
      CALL_HANDLER(callback, &pres);       
}
