#ifndef _AUTH_H_
#define _AUTH_H_


#include "jabbah.h"

int  auth_register(jabbah_context_t *cnx, char *login, char *pass, char *resource,
                   void (*cb)(jabbah_context_t *, jabbah_auth_result_t *));

void auth_manage_type_response(jabbah_context_t *cnx, jabbah_node_t *node);
void auth_manage_auth_response(jabbah_context_t *cnx, jabbah_node_t *node);
        

#endif
