#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "common.h"
#include "node.h"

// Macro for simplified running callbacks
#define CALL_HANDLER(h,x,y) (((void (*)(jabbah_context_t * , jabbah_node_t *))h)(x,y))

//
// LOCAL FUNCTION BLOCK
//


static int
is_empty(char *s)
{
        int len = 0;
        int i, c = 0;

        if (s != NULL) len = strlen(s);

        for (i = 0; i < len; i++)
                if (!isblank(s[i])) c++;

        return c;
}



static char *
node_sprintf(char *s, const char *fmt, ...) {
    /* Guess we need no more than 10 bytes. */
    int n, size = 100;
    char *p = NULL;
   
    va_list ap;
    if ((p = realloc(p, size)) == NULL)
       return NULL;
        
    while (1) {
       /* Try to print in the allocated space. */
       va_start(ap, fmt);
       n = vsnprintf (p, size, fmt, ap);
       va_end(ap);
       /* If that worked, return the string. */
       if (n > -1 && n < size)
               break;
       /* Else try again with more space. */
          if (n > -1)    
            size = n+1; 
          else           
            size += 100; 
          if ((p = realloc (p, size)) == NULL)
              return NULL;
    }

    if (s != NULL) {
            n = strlen(s) + strlen(p);
            s = (char *)realloc(s, sizeof(char)*(n+1));
            strncat(s, p, n);
            free(p);
            return s;
    } else {
            return p;
    }
}

static char *
prepare_string(char *s)
{
        char *r = NULL;
        int s_len = 0;
        int r_len = 0;
        int i;

        
        if (s == NULL)
                return NULL;

        s_len = strlen(s);
        r_len = 1;
        r = (char *)malloc(sizeof(char));
        r[0] = '\0';
        
        for (i = 0; i < s_len; i++) {
                r = (char *)realloc(r, ++r_len);
                switch (s[i]) {
                        case '"':
                                r_len += 5;
                                r = (char *)realloc(r, r_len);
                                strlcat(r, "&quot;", r_len);
                                break;
                        case '\'':
                                r_len += 5;
                                r = (char *)realloc(r, r_len);
                                strlcat(r, "&apos;", r_len);
                                break;  
                        case '<':
                                r_len += 3;
                                r = (char *)realloc(r, r_len);
                                strlcat(r, "&lt;", r_len);
                                break;  
                        case '&':
                                r_len += 4;
                                r = (char *)realloc(r, r_len);
                                strlcat(r, "&amp;", r_len);
                                break;
                        default:
                                r[r_len - 2] = s[i];
                }
                r[r_len - 1] = '\0';
        }

        return r;
}


static jabbah_node_t *
get_attributes(jabbah_node_t *node, const char **attr)
{
        int   i;
        int   delimiter = 0;
        char *ns_name   = NULL;
        char *ns        = NULL;
        char *attr_name = NULL;
        
        for (i = 0; attr[i]; i += 2)
        {
                // Do it only not for lang and xmlns declarations
                if (strcmp("xmlns", attr[i]) && strcmp("xml:lang", attr[i])) {
                        // Check if attribute is in the namespace
                        while (delimiter < strlen(attr[i])) {
                                if (attr[i][delimiter++] == ':')
                                        break;
                        }

                        if (delimiter < strlen(attr[i])) {
                                attr_name = (char *)(attr[i] + delimiter);
                                ns_name = (char *)malloc(sizeof(char)*delimiter);
                                strncpy(ns_name, attr[i], delimiter-1);
                                ns_name[delimiter-1] = '\0';
                                ns = node_namespace_get(node->registered_ns, ns_name);
                                free(ns_name);
                        } else {
                                attr_name = (char *)attr[i];
                                ns = NULL;
                        }

                        node = node_attribute_ns_add(node, attr_name, (char *)attr[i+1], ns);
                        
                        ns_name = NULL;
                        ns = NULL;
                        delimiter = 0;
                }
        }
        
        return node;
}

static void
free_attributes(jabbah_attr_list_t *attrs)
{
        jabbah_attr_list_t *c = NULL;
        jabbah_attr_list_t *n = NULL;
        
        if (attrs == NULL)
                return;

        c = attrs;

        while (c != NULL) {
                if (c->name != NULL)
                        free(c->name);

                if (c->value != NULL)
                        free(c->value);

                if (c->namespace != NULL)
                        free(c->namespace);
                
                n = c;
                c = c->next;
                free(n);
        }
}

/*! \brief  Check if the callback already exists in the context
 * 	check_callback verifies if the callback name already exits in the given context.
 *  \param cnx is a context
 *  \param name is the name of the callback
 *  \return -1 if the case of error, if not found , 1  if found
 */
static int
check_callback(jabbah_context_t *cnx, char *name)
{
        node_callback_t *cb_node = NULL;
        
        if (cnx->callbacks == NULL)
               return -1;

        cb_node = cnx->callbacks;

        while (cb_node != NULL) {
                if (!strcmp(cb_node->node_name, name))
                        return 1;
                cb_node = cb_node->next;
        }

        return 0;
}



static void
add_callback(jabbah_context_t *cnx, char *name, void *cb)
{
        node_callback_t *cb_node = (node_callback_t *)malloc(sizeof(node_callback_t));
        node_callback_t *cbs = cnx->callbacks;
        
        cb_node->next = NULL;

        // setting name
        cb_node->node_name = (char *)malloc(sizeof(char)*(strlen(name)+1));
        strlcpy(cb_node->node_name, name, strlen(name)+1);

        // adding pointer to function
        cb_node->callback = cb;

        if (cnx->callbacks ==  NULL) cnx->callbacks = cb_node;
        else {
                while (cbs->next != NULL) cbs = cbs->next;
                cbs->next = cb_node;
        }
        
}



static void
del_callback_node(node_callback_t *node)
{
        if (node == NULL) return;

        if (node->node_name != NULL)
                free(node->node_name);

        free(node);                     
}



static void
del_callback(jabbah_context_t *cnx, char *name)
{
        node_callback_t *prev = cnx->callbacks;
        node_callback_t *curr = cnx->callbacks;

        if (!strcmp(cnx->callbacks->node_name, name)) {
                cnx->callbacks = cnx->callbacks->next;
                del_callback_node(curr);
        }

        curr = prev->next;
        
        while (strcmp(curr->node_name, name)) {
                prev = curr;
                curr = curr->next;                
        }

        prev->next = curr->next;
        del_callback_node(curr);
}



static void
run_callback(jabbah_context_t *cnx, jabbah_node_t *node)
{
        node_callback_t *cb = cnx->callbacks;

//        node_print2(node);
        
        while (cb != NULL) {
                if (!strcmp(cb->node_name, node->name)) {
                        CALL_HANDLER(cb->callback, cnx, node);
                        return;
                }
                cb = cb->next;
        }
}


//
// INTERFACE FUNCTIONS
//

//
// CALLBACKS
//
int
node_callback_register(jabbah_context_t *cnx, char *node_name, void (*cb)(jabbah_context_t * , jabbah_node_t *))
{
        if (node_name == NULL || cb == NULL)
                return -1;

        if (check_callback(cnx, node_name))
                return -2;

        add_callback(cnx, node_name, (void *)cb);

        return 0;
}
        
int
node_callback_unregister(jabbah_context_t *cnx, char *node_name)
{
        if (node_name == NULL)
                return -1;

        if (!check_callback(cnx, node_name))
                return -2;

        del_callback(cnx, node_name);

        return 0;
}


void
node_callback_flush(jabbah_context_t *cnx)
{
        node_callback_t *cb = cnx->callbacks;

        while (cb != NULL) {
                cb = cnx->callbacks->next;
                del_callback_node(cnx->callbacks);
                cnx->callbacks = cb;
        }

        cnx->callbacks = NULL;
}


//
// MAIN NODE FUNCTIONS
//

/* Node create start a new node
 * and make cnx->curr_node point to it
 */
jabbah_node_t *
node_create(jabbah_context_t *cnx, const char *name, const char **attr)
{
        jabbah_node_t      *node        = NULL;
        char               *node_lang   = NULL;
        char               *node_name   = NULL;
        jabbah_namespace_t *node_reg_ns = NULL;
        char               *node_ns     = NULL;
        char               *ns_name     = NULL;
        int                 i;

        // Take some info from parent
        if (cnx->curr_node != NULL) {
                if (cnx->curr_node->lang != NULL) {
                        node_lang = (char *)malloc(sizeof(char)*(strlen(cnx->curr_node->lang)+1));
                        strlcpy(node_lang, cnx->curr_node->lang, strlen(cnx->curr_node->lang)+1);
                }
                node_reg_ns = node_namespace_copy(cnx->curr_node->registered_ns);
        } else {
                if (cnx->lang != NULL) {
                        node_lang = (char *)malloc(sizeof(char)*(strlen(cnx->lang)+1));
                        strlcpy(node_lang, cnx->lang, strlen(cnx->lang)+1);
                }
                node_reg_ns = node_namespace_copy(cnx->ns);
        }

        // Update language (if changed) and namespace (if needed)
        for(i = 0; attr[i]; i+=2) {
                if (!strcmp("xml:lang", attr[i])) {
                        if (node_lang != NULL) free(node_lang);
                        node_lang = (char *)malloc(sizeof(char)*(strlen(attr[i+1])+1));
                        strlcpy(node_lang, attr[i+1], strlen(attr[i+1])+1);
                } else if (!strcmp("xmlns", attr[i])) {
                        node_ns = (char *)malloc(sizeof(char)*(strlen(attr[i+1])+1));
                        strlcpy(node_ns, attr[i+1], strlen(attr[i+1])+1);
                }
        }

        // Now update info about registered namespaces
        for (i = 0; attr[i]; i+=2) 
                node_reg_ns = node_namespace_load(node_reg_ns, attr[i], attr[i+1]);

        // Clear name (if it is needed)
        int delimiter = 0;
        while (delimiter < strlen(name)) 
                if (name[delimiter++] == ':') break;

        if (delimiter < strlen(name))
                node_name = (char *)(name + delimiter);
        else {
                delimiter = 0;
                node_name = (char *)name;
        }
                        
        // At last define namespace of this element
        if (node_ns == NULL && delimiter > 0) {
                ns_name = (char *)malloc(sizeof(char)*delimiter);
                for (i = 0; i < delimiter - 1; i++)
                        ns_name[i] = name[i];
                ns_name[delimiter - 1] = '\0';
                node_ns = node_namespace_get(node_reg_ns, ns_name);
                free(ns_name);
        }

        // At last, if other methods fail, get the stream namespace
        if (node_ns == NULL && cnx->node_ns != NULL) {
                node_ns = (char *)malloc(sizeof(char)*strlen(cnx->node_ns)+1);
                strlcpy(node_ns, cnx->node_ns, strlen(cnx->node_ns)+1);
        }

        node = node_full_init(node_name, node_reg_ns, node_ns, node_lang);
        node = get_attributes(node, attr);

		// cnx->curr_node doesnt point to a tree node structure, but to the "current node"(strange... strange...)
        cnx->curr_node = node_subnode_add(cnx->curr_node, node);                        
        cnx->curr_node = node;

        if (node_lang != NULL)
                free(node_lang);

        if (node_ns != NULL)
                free(node_ns);

        if (node_reg_ns != NULL)
                node_namespace_free(node_reg_ns);
        
        return node;
}



jabbah_node_t *
node_append_value(jabbah_context_t *cnx, const char *value, int len)
{
        char *val = (char *)malloc(sizeof(char)*(len+1));

        strlcpy(val, value, len+1);
        cnx->curr_node = node_value_append(cnx->curr_node, val);
        free(val);

        return cnx->curr_node;
}



jabbah_node_t *
node_close(jabbah_context_t *cnx, const char *name)
{
        jabbah_node_t *n = NULL;
        
        if (cnx->curr_node->parent != NULL) {
                cnx->curr_node = cnx->curr_node->parent;
        } else {
#ifdef DEBUG_PRINT
                node_print_local(cnx, cnx->curr_node);
#endif
			run_callback(cnx, cnx->curr_node);
            node_free(cnx->curr_node);
            cnx->curr_node = NULL;
        }

        return cnx->curr_node;
}


void
node_print(jabbah_context_t *cnx, jabbah_node_t *node)
{
    if (node == NULL || cnx->sock < 0)
    	return;

    char *nodeBuff = node_to_string(cnx, node);

    pthread_mutex_lock(&(cnx->write_mutex));
#ifdef DEBUG_PRINT
	fprintf(stderr,"SEND NODE: %s\n", nodeBuff);
#endif
	write(cnx->sock, nodeBuff, strlen(nodeBuff));
    pthread_mutex_unlock(&(cnx->write_mutex));

    free(nodeBuff);
}

void
node_print_local(jabbah_context_t *cnx, jabbah_node_t *node)
{
        char *nodeBuff;

        if (node == NULL || cnx->sock < 0)
                return;
        
        nodeBuff = node_to_string(cnx, node);
        printf("SEND NODE: \n%s\n", nodeBuff);
        free(nodeBuff);
}


char *
node_to_string(jabbah_context_t *cnx, jabbah_node_t *node)
{
        return node_to_string_ex(node, cnx->post_lang, "jabber:client");
}

char *
node_to_string_ex(jabbah_node_t *node, char *curr_lang, char *curr_ns)
{
        char               *s         = NULL;
        char               *s2        = NULL;
        jabbah_attr_list_t *attr      = NULL;
        jabbah_node_t      *snode     = NULL;
        char               *prep      = NULL;
        char               *prep2     = NULL;
        char               *node_lang = NULL;
        char               *node_ns   = NULL;

        // First - check if we can do it
        if (node == NULL) return NULL;
        
        // First of all begin the tag
        s = node_sprintf(s, "<%s", node->name);
        
        
        // Add the attributes
        attr = node->attributes;
        
        while (attr != NULL) {
                prep  = prepare_string(attr->name);
                prep2 = prepare_string(attr->value);
                s = node_sprintf(s, " %s='%s'",
                                 prep, prep2);
                if (prep != NULL)
                        free(prep);
                if (prep2 != NULL)
                        free(prep2);
                attr = attr->next;
        }

        // Manage language of the node
        if ((node->lang != NULL && curr_lang == NULL)
            || (node->lang != NULL && curr_lang != NULL && strcmp(node->lang, curr_lang))) {
                s = node_sprintf(s, " xml:lang='%s'", node->lang);
                node_lang = node->lang;
        } else {
                node_lang = curr_lang;
        }

        // Manage namespace of the node
        if ((node->namespace != NULL && curr_ns == NULL) ||
            (node->namespace != NULL && curr_ns != NULL && strcmp(node->namespace, curr_ns))) {
                s = node_sprintf(s, " xmlns='%s'", node->namespace);
                node_ns = node->namespace;
        } else {
                node_ns = curr_ns;
        }
        
        // Close beggining tag
        if (node->subnodes == NULL)
                if (!is_empty(node->value)) {
                        s = node_sprintf(s, " />\n");
                        return s;
                } else {
                        s = node_sprintf(s, ">");
                }
        else
                s = node_sprintf(s, ">\n");
        // Check if there is any subnodes. If there are, get them!
        snode = node->subnodes;

            
        if (snode != NULL)
                while (snode != NULL) {               
                        s2 = node_to_string_ex(snode, node_lang, node_ns);
                        if (s2 != NULL) {
                                s = (char *)realloc(s, sizeof(char *)*
                                                    (strlen(s) + strlen(s2) + 1));
                                if (s != NULL) {
                                        strncat(s, s2, strlen(s) + strlen(s2));
                                        free(s2);
                                }
                        }
                        snode = snode->next;
                }
        else {
                prep = prepare_string(node->value);
                s = node_sprintf(s, "%s", prep);
                if (prep != NULL)
                        free(prep);
        }

        s = node_sprintf(s, "</%s>\n", node->name);

        return s;       
}

void
node_free(jabbah_node_t *node)
{
        jabbah_node_t *snode = NULL;
        
        if (node == NULL)
                return;

        if (node->name != NULL)
                free(node->name);

        if (node->value != NULL)
                free(node->value);

        if (node->lang != NULL)
                free(node->lang);

        if (node->namespace != NULL)
                free(node->namespace);


        node_namespace_free(node->registered_ns);
      
        free_attributes(node->attributes);

        while (node->subnodes != NULL) {
                snode = node->subnodes->next;
                node_free(node->subnodes);
                node->subnodes = snode;
        }

        free(node);
                
}


//
// FUNCTIONS FOR NODE MANIPULATION
//


/*
 * node_full_init just create a default node, cnx->curr_node mantains unchanged!
 */
jabbah_node_t *
node_full_init(char *name, jabbah_namespace_t *reg_ns, char *ns, char *lang)
{
        jabbah_node_t *node = NULL;

        if (name == NULL)
                return NULL;
        
        node = (jabbah_node_t *)malloc(sizeof(jabbah_node_t));
        if (node == NULL)
                return NULL;

        node->name = (char *)malloc(sizeof(char)*(strlen(name)+1));
        strlcpy(node->name, name, strlen(name)+1);

        node->value         = NULL;
        node->parent        = NULL;
        node->attributes    = NULL;
        node->next          = NULL;
        node->subnodes      = NULL;

        if (reg_ns != NULL) {
                node->registered_ns = node_namespace_copy(reg_ns);
        }else
                node->registered_ns = NULL;
        
        if (ns != NULL) {
                node->namespace = (char *)malloc(sizeof(char)*(strlen(ns)+1));
                strlcpy(node->namespace, ns, strlen(ns)+1);
        } else
                node->namespace     = NULL;


        if (lang != NULL) {
                node->lang = (char *)malloc(sizeof(char)*(strlen(lang)+1));
                strlcpy(node->lang, lang, strlen(lang)+1);
        } else
                node->lang          = NULL;

        return node;
}

/*
 * node_init is a binding to node_full_init
 * \sa node_full_init, node_create just create a default node, cnx->curr_node mantains unchanged!
 */
jabbah_node_t *
node_init(char *name)
{
        return node_full_init(name, NULL, NULL, NULL);
}


/* Insert the value in the node
 * <node> value </node>
 */
jabbah_node_t *
node_value_set(jabbah_node_t *node, char *value)
{
        if (node == NULL || value == NULL)
                return NULL;

        if (node->value != NULL)
                free(node->value);
        
        node->value = (char *)malloc(sizeof(char)*(strlen(value)+1));
        strlcpy(node->value, value, strlen(value)+1);

        return node;
}

jabbah_node_t *
node_value_append(jabbah_node_t *node, char *value)
{
        char *val = NULL;
        int l, len;


        if (node == NULL || value == NULL) {
                return NULL;
        }

        len = strlen(value);
        val = (char *)malloc(sizeof(char)*(len+1));
        strlcpy(val, value, len);
        
        if (node->value == NULL) {
                node->value = val;
        } else {
                l = len + strlen(node->value) + 1;
                node->value = (char *)realloc(node->value, sizeof(char)*l);
                strlcat(node->value, val, l);
                free(val);
        }

        return node;
}


jabbah_node_t *
node_attribute_add(jabbah_node_t *node, char *attr_name, char *attr_value)
{
        return node_attribute_ns_addz(node, attr_name, attr_value, NULL);
}

jabbah_node_t *
node_attribute_ns_add(jabbah_node_t *node, char *attr_name, char *attr_value, char *ns)
{
        return node_attribute_ns_addz(node, attr_name, attr_value, ns);
}


jabbah_node_t *
node_attribute_adda(jabbah_node_t *node, char *attr_name, char *attr_value)
{
        return node_attribute_ns_adda(node, attr_name, attr_value, NULL);
}


jabbah_node_t *
node_attribute_ns_adda(jabbah_node_t *node, char *attr_name, char *attr_value, char *ns)
{
        jabbah_attr_list_t *attr = NULL;

        if (node == NULL || attr_name == NULL || attr_value == NULL)
                return NULL;

        attr = (jabbah_attr_list_t *)malloc(sizeof(jabbah_attr_list_t));

        attr->name = (char *)malloc(sizeof(char)*(strlen(attr_name)+1));
        strncpy(attr->name, attr_name, strlen(attr_name));
        attr->name[strlen(attr_name)] = '\0';
        
        attr->value = (char *)malloc(sizeof(char)*(strlen(attr_value)+1));
        strncpy(attr->value, attr_value, strlen(attr_value));
        attr->value[strlen(attr_value)] = '\0';


        if (ns != NULL) {
                attr->namespace = (char *)malloc(sizeof(char)*(strlen(ns)+1));
                strncpy(attr->namespace, ns, strlen(ns));
                attr->namespace[strlen(ns)] = '\0';
        } else
                attr->namespace = NULL;
        
        attr->next = node->attributes;
        node->attributes = attr;

        return node;
}


jabbah_node_t *
node_attribute_addz(jabbah_node_t *node, char *attr_name, char *attr_value)
{
        return node_attribute_ns_addz(node, attr_name, attr_value, NULL);
}


jabbah_node_t *
node_attribute_ns_addz(jabbah_node_t *node, char *attr_name, char *attr_value, char *ns)
{
        jabbah_attr_list_t *attr = NULL;
        jabbah_attr_list_t *na   = NULL;

        if (node == NULL || attr_name == NULL || attr_value == NULL)
                return NULL;

        attr = (jabbah_attr_list_t *)malloc(sizeof(jabbah_attr_list_t));

        attr->name = (char *)malloc(sizeof(char)*(strlen(attr_name)+1));
        strlcpy(attr->name, attr_name, strlen(attr_name)+1);
        
        attr->value = (char *)malloc(sizeof(char)*(strlen(attr_value)+1));
        strlcpy(attr->value, attr_value, strlen(attr_value)+1);

        if (ns != NULL) {
                attr->namespace = (char *)malloc(sizeof(char)*(strlen(ns)+1));
                strlcpy(attr->namespace, ns, strlen(ns)+1);
        } else
                attr->namespace = NULL;

        
        attr->next = NULL;

        na = node->attributes;

        if (na == NULL)
                node->attributes = attr;
        else {
                while (na->next != NULL) na = na->next;
                na->next = attr;
        }
                

        return node;
}


jabbah_node_t *
node_subnode_add(jabbah_node_t *parent, jabbah_node_t *child)
{
        return node_subnode_addz(parent, child);
}



jabbah_node_t *
node_subnode_adda(jabbah_node_t *parent, jabbah_node_t *child)
{
        
        if (parent == NULL || child == NULL)
                return NULL;

        child->parent = parent;
        child->next   = parent->subnodes;
        parent->subnodes = child;

        return parent;
}



jabbah_node_t *
node_subnode_addz(jabbah_node_t *parent, jabbah_node_t *child)
{
        jabbah_node_t *node = NULL;
        
        if (parent == NULL || child == NULL)
                return NULL;

        child->parent = parent;

        if (parent->subnodes == NULL)
                parent->subnodes = child;
        else {
                node = parent->subnodes;
                while (node->next != NULL) node = node->next;
                node->next = child;
        }
        
        return parent;
}

