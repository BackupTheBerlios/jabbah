#include <stdio.h>

#include "common.h"
#include "node.h"

static int
is_xmlns(const char *name)
{
        int pos = 0;
        int len = strlen(name);

        if (len < 7)
                return 0;

        if (name[0] != 'x' ||
            name[1] != 'm' ||
            name[2] != 'l' ||
            name[3] != 'n' ||
            name[4] != 's' ||
            name[5] != ':')
                return 0;

        return 6;
}

jabbah_namespace_t *
node_namespace_create(char *id, char *value)
{
        jabbah_namespace_t *ns = NULL;

        if (id == NULL || value == NULL)
                return NULL;
        
        ns = (jabbah_namespace_t *)malloc(sizeof(jabbah_namespace_t));

        ns->id = (char *)malloc(sizeof(char)*(strlen(id)+1));
        strlcpy(ns->id, id, strlen(id)+1);

        ns->value = (char *)malloc(sizeof(char)*(strlen(value)+1));
        strlcpy(ns->value, value, strlen(value)+1);

        ns->next = NULL;
        return ns;
}


jabbah_namespace_t *
node_namespace_add(jabbah_namespace_t *ns, char *id, char *value)
{
        jabbah_namespace_t *n = ns;

        if (ns == NULL)
                return node_namespace_create(id, value);

        while(n->next != NULL) n = n->next;

        n->next = node_namespace_create(id, value);

        return n;
}


jabbah_namespace_t *
node_namespace_copy(jabbah_namespace_t *ns)
{
        jabbah_namespace_t *prev = NULL;
        jabbah_namespace_t *new  = NULL;
        jabbah_namespace_t *old  = ns;
        jabbah_namespace_t *ret  = NULL;

        if (ns == NULL)
                return NULL;

        prev = node_namespace_create(ns->id, ns->value);
        ret = prev;
        
        old = old->next;
        
        while (old != NULL) {
                new = node_namespace_create(old->id, old->value);
                prev->next = new;
                prev = prev->next;
                old = old->next;
        }

        return ret;
}


jabbah_namespace_t *
node_namespace_load(jabbah_namespace_t *ns, const char *attr_name, const char *attr_value)
{
        jabbah_namespace_t *new_ns = NULL;
        int pos = is_xmlns(attr_name);

        if (!pos)
                return ns;

        return node_namespace_add(ns, (char *)(attr_name + pos), (char *)attr_value);
}



char *
node_namespace_get(jabbah_namespace_t *ns, char *id)
{
        jabbah_namespace_t *n = ns;

        if (id == NULL)
                return NULL;


        while (n != NULL) {
                if (!strcmp(n->id, id))
                        return n->value;
                n = n->next;
        }
        
        return NULL;
}


void
node_namespace_free(jabbah_namespace_t *ns)
{
        jabbah_namespace_t *n = ns;
        jabbah_namespace_t *p = NULL;

        if (ns == NULL) return;

        while (n != NULL) {
                p = n->next;
                if (n->id != NULL)
                        free(n->id);
                if (n->value != NULL)
                        free(n->value);
                free(n);
                n = p;
        }
}

