#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "version.h"
#include "node.h"
#include "iq.h"

#define CALL_HANDLER(h,x,y) (((void (*)(jabbah_context_t * , jabbah_node_t *))h)(x,y))

static void iq_send_agent(jabbah_context_t *cnx, jabbah_node_t *node);

void
iq_send_response(jabbah_context_t *cnx, char *to, char *id, char *xmlns, jabbah_node_t *content)
{
        jabbah_node_t *node  = NULL;
        jabbah_node_t *query = NULL;
        
        pthread_mutex_lock(&(cnx->iq_mutex));
        
        node  = node_init("iq");
        query = node_init("query");
        
        node = node_attribute_add(node, "type", "result");
        node = node_attribute_add(node, "id", id);
        node = node_attribute_add(node, "to", to);
        
        query->namespace = (char *)malloc(sizeof(char)*(strlen(xmlns)+1));
        strncpy(query->namespace, xmlns, strlen(xmlns));
        query->namespace[strlen(xmlns)] = '\0';
        
        query = node_subnode_add(query, content);
        node  = node_subnode_add(node, query);
        
        node_print(cnx, node);

        pthread_mutex_unlock(&(cnx->iq_mutex));
}



void
iq_send_query(jabbah_context_t *cnx, jabbah_iq_type_t type, char *xmlns, jabbah_node_t *request,
              void (*cb)(jabbah_context_t * , jabbah_node_t *))
{
        // Request vars
        char *id = (char *)malloc(sizeof(char)*20);
        jabbah_req_list_t *req = (jabbah_req_list_t *)malloc(sizeof(jabbah_req_list_t));
        jabbah_req_list_t *r_list = NULL;

        pthread_mutex_lock(&(cnx->iq_mutex)); 

        
        // Node vars
        jabbah_node_t *node  = node_init("iq");
        jabbah_node_t *query = node_init("query");
        
        snprintf(id, 20, "req_%d", ++(cnx->req_id));
        req->next = NULL;
        req->prev = NULL;
        req->id   = id;
        req->fun  = (void *)cb;

        if (cnx->req_list == NULL) cnx->req_list = req;
        else {
                r_list = cnx->req_list;
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
        node = node_attribute_add(node, "to", cnx->server_address);

        query->namespace = (char *)malloc(sizeof(char)*(strlen(xmlns)+1));
        strncpy(query->namespace, xmlns, strlen(xmlns));
        query->namespace[strlen(xmlns)] = '\0';

        if (request != NULL)
                query = node_subnode_add(query, request);
        
        node = node_subnode_add(node, query);

        // TODO: Wyslij to na socket
        node_print(cnx, node);
        pthread_mutex_unlock(&(cnx->iq_mutex));
}


void
iq_parse_node(jabbah_context_t *cnx, jabbah_node_t *node)
{
        char *id = NULL;
        jabbah_attr_list_t *attr = node->attributes;
        jabbah_req_list_t  *req  = cnx->req_list;
        void               *fun  = NULL;
        
        pthread_mutex_lock(&(cnx->iq_mutex));
        
        while (strcmp(attr->name, "id") && attr != NULL) attr = attr->next;

        if (attr == NULL || attr->value == NULL)
                return;
        
        id = attr->value;

        while (req != NULL && req->id != NULL && strcmp(req->id, id)) req = req->next;

        if (req == NULL) {
                pthread_mutex_unlock(&(cnx->iq_mutex));
                iq_send_agent(cnx, node);
                return;
        }

        if (req->fun != NULL)
                fun = req->fun;

        if (req->prev == NULL)
                cnx->req_list = req->next;
        else
                req->prev->next = req->next;

        free(req->id);
        free(req);
        pthread_mutex_unlock(&(cnx->iq_mutex));
        CALL_HANDLER(fun, cnx, node);
}

static void
iq_send_agent(jabbah_context_t *cnx, jabbah_node_t *node)
{
        jabbah_node_t *query = NULL;

        query = node->subnodes;
        
        while (query != NULL && strcmp(query->name, "query"))
                query = query->next;

        if (query == NULL)
                return;

        if (!strcmp(query->namespace, "jabber:iq:version"))
                version_send(cnx, node);
        
}
