#ifndef _NODE_H_
#define _NODE_H_


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



// Callback registration
int             node_callback_register(char *node_name, void (*cb)(jabbah_node_t *node));
int             node_callback_unregister(char *node_name);
void            node_callback_flush(void);

void            node_free_resources(void);
// Functions for expat
void            node_set_env(int socket, char *lang, char *node_ns, jabbah_namespace_t *ns);
jabbah_node_t * node_create(const char *name, const char **attr); 
jabbah_node_t * node_append_value(const char *value, int len);
jabbah_node_t * node_close(const char *name);

// Additional node operations
void            node_print(jabbah_node_t *node);
void            node_print2(jabbah_node_t *node);
char *          node_to_string(jabbah_node_t *node);
char *          node_to_string_ex(jabbah_node_t *node, char *curr_lang, char *curr_ns);
void            node_free(jabbah_node_t *node);

// Namespace struct manipulation
jabbah_namespace_t * node_namespace_create(char *id, char *value);
jabbah_namespace_t * node_namespace_add(jabbah_namespace_t *ns, char *id, char *value);
jabbah_namespace_t * node_namespace_copy(jabbah_namespace_t *ns);
jabbah_namespace_t * node_namespace_load(jabbah_namespace_t *ns, const char *attr_name, const char *attr_value); 
char               * node_namespace_get(jabbah_namespace_t *ns, char *id);
void                 node_namespace_free(jabbah_namespace_t *ns);

// Node manipulation functions
jabbah_node_t * node_init(char *name);
jabbah_node_t * node_full_init(char *name, jabbah_namespace_t *reg_ns, char *ns, char *lang);
jabbah_node_t * node_value_set(jabbah_node_t *node, char *value);
jabbah_node_t * node_value_append(jabbah_node_t *node, char *value);

jabbah_node_t * node_attribute_add(jabbah_node_t *node, char *attr_name, char *attr_value);
jabbah_node_t * node_attribute_adda(jabbah_node_t *node, char *attr_name, char *attr_value);
jabbah_node_t * node_attribute_addz(jabbah_node_t *node, char *attr_name, char *attr_value);

jabbah_node_t * node_attribute_ns_add(jabbah_node_t *node, char *attr_name, char *attr_value, char*ns);
jabbah_node_t * node_attribute_ns_adda(jabbah_node_t *node, char *attr_name, char *attr_value, char *ns);
jabbah_node_t * node_attribute_ns_addz(jabbah_node_t *node, char *attr_name, char *attr_value, char *ns);

jabbah_node_t * node_subnode_add(jabbah_node_t *parent, jabbah_node_t *child);
jabbah_node_t * node_subnode_adda(jabbah_node_t *parent, jabbah_node_t *child);
jabbah_node_t * node_subnode_addz(jabbah_node_t *parent, jabbah_node_t *child);

#endif
