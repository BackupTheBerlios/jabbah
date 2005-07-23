#ifndef _NODE_H_
#define _NODE_H_


#include "jabbah.h"


// Callback registration
int             node_callback_register(jabbah_context_t *cnx, char *node_name, void (*cb)(jabbah_context_t *, jabbah_node_t *));
int             node_callback_unregister(jabbah_context_t *cnx, char *node_name);
void            node_callback_flush(jabbah_context_t *cnx);


// Functions for expat
jabbah_node_t * node_create(jabbah_context_t *cnx, const char *name, const char **attr); 
jabbah_node_t * node_append_value(jabbah_context_t *cnx, const char *value, int len);
jabbah_node_t * node_close(jabbah_context_t *cnx, const char *name);

// Additional node operations
void            node_print(jabbah_context_t *cnx, jabbah_node_t *node);
char *          node_to_string(jabbah_context_t *cnx, jabbah_node_t *node);
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
