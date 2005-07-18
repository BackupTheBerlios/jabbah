#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "node.h"
#include "iq.h"

#define CALL_HANDLER(h,x) (((void (*)(jabbah_node_t *))h)(x))

static char *ns     = NULL;
static char *server = NULL;
static pthread_mutex_t iq_mutex;

typedef struct _jabbah_req_list_t {
        char *id;
        void *fun;
        struct _jabbah_req_list_t *prev;
        struct _jabbah_req_list_t *next;
} jabbah_req_list_t;

int curr_id = 0;

jabbah_req_list_t *req_list = NULL;

void
iq_init(char *namespace, char *server_addr)
{
        ns = namespace;
        server = server_addr;
        pthread_mutex_init(&iq_mutex, NULL);
}


void
iq_send_query(jabbah_iq_type_t type, char *xmlns, jabbah_node_t *request, void (*cb)(jabbah_node_t *))
{
        // Request vars
        char *id = (char *)malloc(sizeof(char)*20);
        jabbah_req_list_t *req = (jabbah_req_list_t *)malloc(sizeof(jabbah_req_list_t));
        jabbah_req_list_t *r_list = NULL;

        if (pthread_mutex_lock(&iq_mutex) != 0) {
                pthread_mutex_unlock(&iq_mutex);
                free(req);
                free(id);
                return;
        }
        
        // Node vars
        jabbah_node_t *node  = node_init("iq");
        jabbah_node_t *query = node_init("query");
        
        snprintf(id, 20, "req_%d", ++curr_id);
        req->next = NULL;
        req->prev = NULL;
        req->id   = id;
        req->fun  = (void *)cb;

        if (req_list == NULL) req_list = req;
        else {
                r_list = req_list;
                while (r_list->next != NULL) r_list = r_list->next;

                req->prev = r_list;
                r_list->next = req;
        }

        switch (type) {
          case IQ_GET:
                node = node_attribute_add(node, "type", "get");
                break;
          case IQ_SET:
                node = node_attribute_add(node, "type", "set");
                break;
          default:
                node = node_attribute_add(node, "type", "result");
                break;
        }
                        
        node = node_attribute_add(node, "id", id);
        node = node_attribute_add(node, "to", server);

        query->namespace = (char *)malloc(sizeof(char)*(strlen(xmlns)+1));
        strncpy(query->namespace, xmlns, strlen(xmlns));
        query->namespace[strlen(xmlns)] = '\0';

        if (request != NULL)
                query = node_subnode_add(query, request);
        
        node = node_subnode_add(node, query);

        // TODO: Wyslij to na socket
        node_print(node);
        pthread_mutex_unlock(&iq_mutex);
}

void
iq_free_resources()
{
        jabbah_req_list_t *reqs  = req_list;
        jabbah_req_list_t *reqs2 = NULL;

        pthread_mutex_lock(&iq_mutex);

        req_list = NULL;
        
        while (reqs != NULL) {
                reqs2 = reqs->next;
                
                if (reqs->id != NULL)
                        free(reqs->id);
                free(reqs);
                reqs = reqs2;
        }
        
        pthread_mutex_unlock(&iq_mutex);
        pthread_mutex_destroy(&iq_mutex);

}

void
iq_parse_node(jabbah_node_t *node)
{
        char *id = NULL;
        jabbah_attr_list_t *attr = node->attributes;
        jabbah_req_list_t  *req  = req_list;
        void               *fun  = NULL;
        
        if(pthread_mutex_lock(&iq_mutex) != 0) {
                pthread_mutex_unlock(&iq_mutex);
                return;
        }
        
        while (strcmp(attr->name, "id") && attr != NULL) attr = attr->next;

        if (attr == NULL || attr->value == NULL)
                return;
        
        id = attr->value;

        while (req != NULL && req->id != NULL && strcmp(req->id, id)) req = req->next;

        if (req == NULL) {
                pthread_mutex_unlock(&iq_mutex);
                return;
        }

        if (req->fun != NULL)
                fun = req->fun;

        if (req->prev == NULL)
                req_list = req->next;
        else
                req->prev->next = req->next;

        free(req->id);
        free(req);
        pthread_mutex_unlock(&iq_mutex);
        CALL_HANDLER(fun, node);
}
