#ifndef _AUTH_H_
#define _AUTH_H_


typedef struct _jabbah_auth_result_t {
        int code;
        char *message;
} jabbah_auth_result_t;

void auth_init(char *id);

int  auth_register(char *login, char *pass, char *resource, void (*cb)(jabbah_auth_result_t *));

void auth_manage_type_response(jabbah_node_t *node);
void auth_manage_auth_response(jabbah_node_t *node);
        
void auth_free_resources(void);

#endif
