#include <stdio.h>
#include <string.h>

#include "node.h"
#include "iq.h"
#include "sha1.h"
#include "auth.h"

#define CALL_HANDLER(h,x) (((void (*)(jabbah_auth_result_t *))h)(x))

typedef enum {
        AUTH_NONE,
        AUTH_GET_TYPE,
        AUTH_GOT_TYPE,
        AUTH_SENT_PASS,
        AUTH_GOT_RESULT
} auth_t;


// Local variables
static char   *server_id     = NULL;

static char   *auth_login    = NULL;
static char   *auth_pass     = NULL;
static char   *auth_resource = NULL;
static void   *auth_callback = NULL;
static auth_t  auth_state    = AUTH_NONE;

static void
auth_send_type_req()
{
        jabbah_node_t *user  = NULL;

        auth_state = AUTH_GET_TYPE;
        user = node_init("username");
        user = node_value_set(user, auth_login);
        
        iq_send_query(IQ_GET, "jabber:iq:auth", user, auth_manage_type_response);
}



static char *
digets_pass()
{

        char       *pass_string = (char *)malloc(sizeof(char)*(strlen(server_id)+strlen(auth_pass)+1));
        SHA1Context con;
        uint8_t     result[SHA1HashSize];
        char       *hash = (char *)malloc(sizeof(char)*(2*SHA1HashSize+1));
        int         i = 0;
        
        strncpy(pass_string, server_id, strlen(server_id));
        pass_string[strlen(server_id)] = '\0';
        strncat(pass_string, auth_pass, strlen(server_id) + strlen(auth_pass));
        
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
auth_send_passwd(int resource)
{
        jabbah_node_t *req     = NULL;
        jabbah_node_t *passwd  = NULL;
        jabbah_node_t *res     = NULL;

        req = node_init("username");
        req = node_value_set(req, auth_login);

        passwd = node_init("password");
        passwd = node_value_set(passwd, auth_pass);
        req->next = passwd;

        if (resource) {
                res = node_init("resource");
                res = node_value_set(res, auth_resource);
                passwd->next = res;
        }

        iq_send_query(IQ_SET, "jabber:iq:auth", req, auth_manage_auth_response);
                
}



static void
auth_send_digets_passwd(int resource)
{
        jabbah_node_t *req     = NULL;
        jabbah_node_t *passwd  = NULL;
        jabbah_node_t *res     = NULL;
        char          *digets  = digets_pass();

        req = node_init("username");
        req = node_value_set(req, auth_login);

        passwd = node_init("digest");
        passwd = node_value_set(passwd, digets);
        req->next = passwd;

        if (resource) {
                res = node_init("resource");
                res = node_value_set(res, auth_resource);
                passwd->next = res;
        }

        free(digets);
        
        iq_send_query(IQ_SET, "jabber:iq:auth", req, auth_manage_auth_response);
}



void
auth_init(char *id)
{
        server_id = id;
}

void
auth_manage_auth_response(jabbah_node_t *node)
{
        jabbah_attr_list_t   *attr = NULL;
        jabbah_attr_list_t   *error = NULL;
        jabbah_auth_result_t  result;

        auth_state = AUTH_GOT_RESULT;

        attr = node->attributes;
        
        while (attr != NULL && strcmp(attr->name, "type")) attr = attr->next;

        if (attr != NULL && !strcmp(attr->value, "result")) {
                result.code    = 500;
                result.message = "User authorized"; 

                CALL_HANDLER(auth_callback, &result);

        } else {
                if (attr == NULL) {
                        result.code = 403;
                        result.message = "Server internal error";

                        CALL_HANDLER(auth_callback, &result);
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

                        CALL_HANDLER(auth_callback, &result);                        
                                
                }
        }

        auth_free_resources();
}

void
auth_manage_type_response(jabbah_node_t *node)
{
        jabbah_attr_list_t *attr = node->attributes;
        jabbah_auth_result_t result;
        jabbah_node_t *query   = NULL;
        jabbah_node_t *el      = NULL;
        int           pass     = 0;
        int           digets   = 0;
        int           resource = 0;

        auth_state = AUTH_GOT_TYPE;

        
        // First of all, find if the response is clear
        while (attr != NULL && strcmp(attr->name, "type")) attr = attr->next;

        if (attr == NULL || strcmp(attr->value, "result")) {
                auth_free_resources();
                result.code = 404;
                result.message = "Authorization of jabber:iq:auth unsupported by this server";

                CALL_HANDLER(auth_callback, &result);
                
                return;
        }


        // Now find the query subnode
        query = node->subnodes;

        while (query != NULL && strcmp(query->name, "query")) query = query->next;

        if (query == NULL) {
                auth_free_resources();
                result.code = 404;
                result.message = "Authorization of jabber:iq:auth unsupported by this server";

                CALL_HANDLER(auth_callback, &result);
                
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

        auth_state = AUTH_SENT_PASS;
        if (digets) {
                auth_send_digets_passwd(resource);
        } else if (pass) {
                auth_send_passwd(resource);
        } else {
                auth_free_resources();
                result.code = 404;
                result.message = "Authorization of jabber:iq:auth unsupported by this server";

                CALL_HANDLER(auth_callback, &result);
        }
        
}

int
auth_register(char *login, char *pass, char *resource, void (*cb)(jabbah_auth_result_t *))
{
        if (auth_state != AUTH_NONE)
                return -1;

        auth_login    = (char *)malloc(sizeof(char)*(strlen(login)+1));
        strncpy(auth_login, login, strlen(login));
        auth_login[strlen(login)] = '\0';
        
        auth_pass     = (char *)malloc(sizeof(char)*(strlen(pass)+1));
        strncpy(auth_pass, pass, strlen(pass));
        auth_pass[strlen(pass)] = '\0';
        
        auth_resource = (char *)malloc(sizeof(char)*(strlen(resource)+1));
        strncpy(auth_resource, resource, strlen(resource));
        auth_resource[strlen(resource)] = '\0';

        auth_callback = cb;

        auth_send_type_req();

        return 0;
}


void
auth_free_resources()
{
        if (auth_login != NULL)
                free(auth_login);
        
        if (auth_pass != NULL)
                free(auth_pass);

        if (auth_resource != NULL)
                free(auth_resource);

        auth_callback = NULL;
        auth_state = AUTH_NONE;
}
