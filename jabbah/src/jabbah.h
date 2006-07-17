#ifndef _JABBAH_H_
#define _JABBAH_H_


#include <pthread.h>
#include <expat.h>

//
// Node structures
//

// Oh, how lovely, 
// a list that contains all the attributes in a node
typedef struct _jabbah_attr_list_t {
	char			*name; 				// The name of the attribute
    char            *namespace; 		// The namespace
	char			*value; 			// The value of the attribute
	struct _jabbah_attr_list_t *next; 	// A pointer to the next attribute of the parent node
} jabbah_attr_list_t;

// A linked list that hold the namespaces used
typedef struct _jabbah_namespace_t {
        char                    *id; 			// TODO: Id, dont know...
        char                    *value; 		// TODO: value, whats in here?
        struct _jabbah_namespace_t *next; 		// A pointer to the next namespace
} jabbah_namespace_t;

/* A Linked list that hold all the params that xml contains
	VERY VERY Powerfull :D
Ex: 
	<mynode foo='bar' hello='world'>Hallo</mynode>
	<test>
		<mail>frangossauro@gmail.com</mail>
	</test>
*/
typedef struct _jabbah_node_t {
    struct _jabbah_node_t   *parent; 			// Who is the parent node? mynode isn't subnode of anyone \o/ 
	char 					*name; 				// Whats the name? mynode!
	char					*value; 			// The value? Hallo!
    char                    *lang; 				// The current language
    char                    *namespace; 		// The current namespace
    jabbah_namespace_t      *registered_ns;  	// We have others namespaces? I didn't understand this namespaces lists in everything ¬¬
	jabbah_attr_list_t		*attributes; 		// The attributes for this node Ex: foo, and foo.next = hello
	struct _jabbah_node_t	*subnodes; 			// We have subnodes inside the xml? No, but <mail> is subnode of <test>! 
	struct _jabbah_node_t   *next; 				// Ok, who is the next node in the same level of us? test!
} jabbah_node_t;

// A linked list that hold an name of the callback, and his respective pointer
typedef struct _node_callback_t {
        char *node_name; 				// The name of the node that associated the callback (message, presence or iq)
        void *callback; 				// The pointer for the function
        struct _node_callback_t *next; 	// The next callback
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

// A double linked list that hold's ???
typedef struct _jabbah_req_list_t {
        char *id; 							// A id of something :(
        void *fun; 							// A callback to anything :(
        struct _jabbah_req_list_t *prev; 	// The previous req
        struct _jabbah_req_list_t *next; 	// The next 	req
} jabbah_req_list_t;

//
// AUTH structures
//
typedef struct _jabbah_auth_result_t {
        int code; 							// LOOK, we have an authentication struct just for us!!
        char *message; 						// Thats nice, really nice!
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
        XML_Parser          p; 					// The expat parser(for xml)
        char                buff; 				// recv_buff
        int                 sock; 				// The socket
// 		SSL_CTX            *ssl_ctx = NULL;
        int                 continue_parse; 	// continue_parse=1 while everything ok

        pthread_t           parse_thread; 		// A thread who parse the input
        pthread_mutex_t     con_mutex; 			// Mutex for connection
        pthread_mutex_t     parse_mutex; 		// Mutex for parsing
        pthread_mutex_t     write_mutex; 		// Mutex for writing
        pthread_mutex_t     iq_mutex; 			// Mutex for iq

        int                 authorization; 		// Is authorized?
        int                 opened_session; 	// Session is opened with the server?
        char               *session_id; 		// Whats the current session id?
        jabbah_namespace_t *ns; 				// A linked list with all namespaces 
        char               *lang; 				// The default language
        char               *node_ns; 			// The current namespace name(for finding in
        void               *stream_callback; 	// TODO:???
        char               *server_address; 	// The dns name for the jabber server
        u_int               server_port; 		// The port for the jabber server
        int                 ssl; 				// Should we use ssl?

        jabbah_node_t      *curr_node; 			// A list countaining the current node in use (doesn't see correct to me <frangossauro@gmail.com >) 
        node_callback_t    *callbacks; 			// TODO: All the callbacks we should use(sound stupid, but this allow infinites contexts)
        char               *post_lang; 			// What language we should send?

        void               *message_cb; 		// The message callback. 	Read XMPP protocol
        void               *presence_cb; 		// Presence callback. 		Read XMPP protocol
        void               *auth_cb; 			// Athentication callbacl. 	Read XMPP protocol

        char               *login; 				// What's your login to this jabber server?
        char               *passwd; 			// And your password please?
        char               *resource; 			// Your resource? The /part, dumbass!
        
        int                 prio; 				//TODO: ????

        jabbah_req_list_t  *req_list; 			//TODO: ????
        int                 req_id; 			//TODO: ????

        jabbah_roster_t    *roster; 			//TODO: ????
        
} jabbah_context_t;

// The prototypes for the functions in jabbah.c
jabbah_context_t * jabbah_context_create(char *server_addr, u_int server_port, int ssl);
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
