#ifndef _JABBAH_H_
#define _JABBAH_H_


#include <pthread.h>
#include <expat.h>

//
// Node structures
//
typedef struct _jabbah_attr_list_t {
	char			*name;
        char                    *namespace;
	char			*value;
	struct _jabbah_attr_list_t *next;
} jabbah_attr_list_t;

typedef struct _jabbah_namespace_t {
        char                    *id;
        char                    *value;
        struct _jabbah_namespace_t *next;
} jabbah_namespace_t;

typedef struct _jabbah_node_t {
        struct _jabbah_node_t   *parent;
	char 			*name;
	char			*value;
        char                    *lang;
        char                    *namespace;
        jabbah_namespace_t      *registered_ns;
	jabbah_attr_list_t	*attributes;
	struct _jabbah_node_t	*subnodes;
        struct _jabbah_node_t   *next;
} jabbah_node_t;

typedef struct _node_callback_t {
        char *node_name;
        void *callback;
        struct _node_callback_t *next;
} node_callback_t;

//
// Message structures
//
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

//
// Presence structures
//
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

//
// IQ Structures
//
typedef enum {
        IQ_GET,
        IQ_SET,
        IQ_RESULT
} jabbah_iq_type_t;

typedef struct _jabbah_req_list_t {
        char *id;
        void *fun;
        struct _jabbah_req_list_t *prev;
        struct _jabbah_req_list_t *next;
} jabbah_req_list_t;

//
// AUTH structures
//
typedef struct _jabbah_auth_result_t {
        int code;
        char *message;
} jabbah_auth_result_t;

//
// ROSTER structures
//

typedef enum {
        SUB_NONE,
        SUB_TO,
        SUB_FROM,
        SUB_BOTH
} jabbah_subscription_t;

typedef struct _jabbah_logged_res_t {
        char          *resource;
        int            prio;
        char          *status;
        jabbah_state_t state;
} jabbah_logged_res_t;


typedef struct _jabbah_roster_item_t {
        char *name;
        char *jid;
        int   res_count;
        jabbah_subscription_t subscription;
        jabbah_logged_res_t **res;
} jabbah_roster_item_t;

typedef struct _jabbah_roster_group_t {
        char *name;
        int   item_count;
        jabbah_roster_item_t **items;
} jabbah_roster_group_t;

typedef struct _jabbah_roster_t {
        jabbah_roster_item_t  **items;
        int                     item_count;
        jabbah_roster_group_t **groups;
        int                     group_count;
        jabbah_roster_item_t  **nogroups;
        int                     nogroup_count;
        jabbah_roster_item_t  **transports;
        int                     transport_count;
} jabbah_roster_t;


//
// CONTEXT structure
//
typedef struct _jabbah_context_t {
        XML_Parser          p;
        char                buff;
        int                 sock;
        //SSL_CTX            *ssl_ctx = NULL;
        int                 continue_parse;
        pthread_t           parse_thread;
        pthread_mutex_t     con_mutex;
        pthread_mutex_t     parse_mutex;
        pthread_mutex_t     write_mutex;
        pthread_mutex_t     iq_mutex;
        int                 authorization;
        int                 opened_session;
        char               *session_id;
        jabbah_namespace_t *ns;
        char               *lang;
        char               *node_ns;
        void               *stream_callback;
        char               *server_address;
        int                 server_port;
        int                 ssl;

        jabbah_node_t      *curr_node;
        node_callback_t    *callbacks;
        char               *post_lang;

        void               *message_cb;
        void               *presence_cb;
        void               *auth_cb;

        char               *login;
        char               *passwd;
        char               *resource;
        
        int                 prio;

        jabbah_req_list_t  *req_list;
        int                 req_id;

        jabbah_roster_t    *roster;
        
} jabbah_context_t;

jabbah_context_t * jabbah_context_create(char *server_addr, int server_port, int ssl);
void               jabbah_context_destroy(jabbah_context_t *cnx);

int                jabbah_connect(jabbah_context_t *cnx, char *login, char *passwd, char *resource, int prio);
void               jabbah_close(jabbah_context_t *cnx, char *desc);

void               jabbah_presence_set_state(jabbah_context_t *cnx, jabbah_state_t state);
void               jabbah_presence_set_status(jabbah_context_t *cnx, jabbah_state_t state, char *status);
void               jabbah_presence_register_callback(jabbah_context_t *cnx, void (*cb)(jabbah_context_t * , jabbah_presence_t *));

void               jabbah_message_register_callback(jabbah_context_t *cnx, void (*cb)(jabbah_context_t * , jabbah_message_t *));
void               jabbah_message_send_normal(jabbah_context_t *cnx, char *jid, char *subject, char *msg);
void               jabbah_message_send_chat(jabbah_context_t *cnx, char *jid, char *msg);

void               jabbah_roster_get(jabbah_context_t *cnx);
jabbah_roster_t *  jabbah_roster_wait(jabbah_context_t *cnx);
#endif
