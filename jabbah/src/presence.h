#ifndef _PRESENCE_H_
#define _PRESENCE_H_

#include "node.h"

typedef enum {
        STATE_CHAT,
        STATE_ONLINE,
        STATE_AWAY,
        STATE_XA,
        STATE_DND,
        STATE_OFFLINE
} jabbah_state_t;

typedef struct _jabbah_presence_t {
	char           *jid;
        jabbah_state_t  state;
        char           *status;
        int             priority;
} jabbah_presence_t;

void                      presence_init(int prio);
void                      presence_register_callback(void (*cb)(jabbah_presence_t *));
jabbah_node_t * presence_create_node(jabbah_presence_t *presence);
void                      presence_set_state(jabbah_state_t state);
void                      presence_set_status(jabbah_state_t state, char *status);
void                      presence_set(jabbah_presence_t *presence);

void                      presence_parse_node(jabbah_node_t *node);

#endif
