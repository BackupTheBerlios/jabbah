#ifndef _IQ_H_
#define _IQ_H_

#include "node.h"

typedef enum {
        IQ_GET,
        IQ_SET,
        IQ_RESULT
} jabbah_iq_type_t;

void iq_init(char *namespace, char *server_addr);
void iq_send_query(jabbah_iq_type_t type, char *xmlns, jabbah_node_t *request, void (*cb)(jabbah_node_t *));
void iq_free_resources(void);

void iq_parse_node(jabbah_node_t *node);

#endif
