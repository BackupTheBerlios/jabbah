#include <stdio.h>
#include <string.h>

#include "node.h"
#include "iq.h"
#include "sha1.h"
#include "auth.h"

#define CALL_HANDLER(h,x,y) (((void (*)(jabbah_context_t * , jabbah_auth_result_t *))h)(x, y))


static void
auth_send_type_req(jabbah_context_t *cnx)
{
        jabbah_node_t *user  = NULL;

        user = node_init("username");
        user = node_value_set(user, cnx->login);
        
        iq_send_query(cnx, IQ_GET, "jabber:iq:auth", user, auth_manage_type_response);
}



static char *
digets_pass(jabbah_context_t *cnx, char *auth_pass)
{

        char       *pass_string = (char *)malloc(sizeof(char)*(strlen(cnx->session_id)+strlen(auth_pass)+1));
        SHA1Context con;
        uint8_t     result[SHA1HashSize];
        char       *hash = (char *)malloc(sizeof(char)*(2*SHA1HashSize+1));
        int         i = 0;
        
        strncpy(pass_string, cnx->session_id, strlen(cnx->session_id));
        pass_string[strlen(cnx->session_id)] = '\0';
        strncat(pass_string, cnx->passwd, strlen(cnx->session_id) + strlen(cnx->passwd));
        
        SHA1Reset(&con);
        SHA1Input(&con,  (const uint8_t *)pass_string, strlen(pass_string));
        SHA1Result(&con, result);
        
        for (i = 0; i < SHA1HashSize; i++)
                sprintf(hash + (2*i), "%02x", result[i]);
        hash[2*SHA1HashSize] = '\0';
        free(pass_string);

        return hash;
}



static void
auth_send_passwd(jabbah_context_t *cnx)
{
        jabbah_node_t *req     = NULL;
        jabbah_node_t *passwd  = NULL;
        jabbah_node_t *res     = NULL;

        req = node_init("username");
        req = node_value_set(req, cnx->login);

        passwd = node_init("password");
        passwd = node_value_set(passwd, cnx->passwd);
        req->next = passwd;

        if (cnx->resource) {
                res = node_init("resource");
                res = node_value_set(res, cnx->resource);
                passwd->next = res;
        }

        iq_send_query(cnx, IQ_SET, "jabber:iq:auth", req, auth_manage_auth_response);
                
}



static void
auth_send_digets_passwd(jabbah_context_t *cnx)
{
        jabbah_node_t *req     = NULL;
        jabbah_node_t *passwd  = NULL;
        jabbah_node_t *res     = NULL;
        char          *digets  = digets_pass(cnx, cnx->passwd);

        req = node_init("username");
        req = node_value_set(req, cnx->login);

        passwd = node_init("digest");
        passwd = node_value_set(passwd, digets);
        req->next = passwd;

        if (cnx->resource) {
                res = node_init("resource");
                res = node_value_set(res, cnx->resource);
                passwd->next = res;
        }

        free(digets);
        
        iq_send_query(cnx, IQ_SET, "jabber:iq:auth", req, auth_manage_auth_response);
}


void
auth_manage_auth_response(jabbah_context_t *cnx, jabbah_node_t *node)
{
        jabbah_attr_list_t   *attr = NULL;
        jabbah_attr_list_t   *error = NULL;
        jabbah_auth_result_t  result;

        attr = node->attributes;
        
        while (attr != NULL && strcmp(attr->name, "type")) attr = attr->next;

        if (attr != NULL && !strcmp(attr->value, "result")) {
                result.code    = 500;
                result.message = "User authorized"; 

                CALL_HANDLER(cnx->auth_cb, cnx, &result);

        } else {
                if (attr == NULL) {
                        result.code = 403;
                        result.message = "Server internal error";

                        CALL_HANDLER(cnx->auth_cb, cnx, &result);
                } else {
                        if (!strcmp(attr->value, "error") &&
                            !strcmp(node->subnodes->name, "error")) {
                                error = node->subnodes->attributes;

                                while (error != NULL && strcmp(error->name, "code"))
                                        error = error->next;

                                if (error == NULL) {
                                        result.code = 403;
                                        result.message = "Server internal error";
                                } else {
                                        result.code = atoi(error->value);
                                        switch(result.code) {
                                                case 401:
                                                        result.message = "You are not authorized";
                                                        break;
                                                case 409:
                                                        result.message = "Resource conflict";
                                                        break;
                                                case 406:
                                                        result.message = "Required information not provided";
                                                        break;
                                                default:
                                                        result.message = "Unknown error";
                                                        break;
                                        }
                                }
                        } else {
                                result.code = 403;
                                result.message = "Server internal error";
                        }

                        CALL_HANDLER(cnx->auth_cb, cnx, &result);                        
                                
                }
        }
}

void
auth_manage_type_response(jabbah_context_t *cnx, jabbah_node_t *node)
{
        jabbah_attr_list_t *attr = node->attributes;
        jabbah_auth_result_t result;
        jabbah_node_t *query   = NULL;
        jabbah_node_t *el      = NULL;
        int           pass     = 0;
        int           digets   = 0;
        int           resource = 0;

        
        // First of all, find if the response is clear
        while (attr != NULL && strcmp(attr->name, "type")) attr = attr->next;

        if (attr == NULL || strcmp(attr->value, "result")) {
                result.code = 404;
                result.message = "Authorization of jabber:iq:auth unsupported by this server";

                CALL_HANDLER(cnx->auth_cb, cnx, &result);
                
                return;
        }


        // Now find the query subnode
        query = node->subnodes;

        while (query != NULL && strcmp(query->name, "query")) query = query->next;

        if (query == NULL) {
                result.code = 404;
                result.message = "Authorization of jabber:iq:auth unsupported by this server";

                CALL_HANDLER(cnx->auth_cb, cnx, &result);
                
                return;
        }

        // Check the authorization possibilities
        el = query->subnodes;

        while (el != NULL) {
                if (!strcmp(el->name, "password")) {
                        pass = 1;
                } else if (!strcmp(el->name, "digest")) {
                        digets = 1;
                } else if (!strcmp(el->name, "resource")) {
                        resource = 1;
                }

                el = el->next;
        }

        if (digets) {
                auth_send_digets_passwd(cnx);
        } else if (pass) {
                auth_send_passwd(cnx);
        } else {
                result.code = 404;
                result.message = "Authorization of jabber:iq:auth unsupported by this server";

                CALL_HANDLER(cnx->auth_cb, cnx, &result);
        }
        
}

int
auth_register(jabbah_context_t *cnx, char *login, char *pass, char *resource, void (*cb)(jabbah_context_t *, jabbah_auth_result_t *))
{

        cnx->login    = (char *)malloc(sizeof(char)*(strlen(login)+1));
        strncpy(cnx->login, login, strlen(login));
        cnx->login[strlen(login)] = '\0';
        
        cnx->passwd   = (char *)malloc(sizeof(char)*(strlen(pass)+1));
        strncpy(cnx->passwd, pass, strlen(pass));
        cnx->passwd[strlen(pass)] = '\0';
        
        cnx->resource = (char *)malloc(sizeof(char)*(strlen(resource)+1));
        strncpy(cnx->resource, resource, strlen(resource));
        cnx->resource[strlen(resource)] = '\0';

        cnx->auth_cb = cb;

        auth_send_type_req(cnx);

        return 0;
}


