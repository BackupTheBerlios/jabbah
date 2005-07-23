/*
 *  main.c
 *  jabbah
 *
 *  Created by Bazyli Zygan on Sun Jun 12 2005.
 *  Copyright (c) 2005 __MyCompanyName__. All rights reserved.
 *
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <expat.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <unistd.h>

#include "message.h"
#include "presence.h"
#include "node.h"
#include "iq.h"
#include "auth.h"
#include "jabbah.h"

#define CALL_HANDLER(h,x) (((void (*)(int *))h)(x))

#define BUFFSIZE


static void XMLCALL
start(void *data, const char *el, const char **attr)
{
  int i = 0;
  jabbah_context_t *cnx = (jabbah_context_t *)data;
  
  if (!strcmp("stream:stream", el))
  {
    cnx->opened_session = 1;
    // Getting session id
    for (i = 0; attr[i]; i += 2)
      if (!strcmp("id", attr[i]))
      {
              cnx->session_id = (char *)malloc(sizeof(char)*(strlen(attr[i+1])+1));
              strncpy(cnx->session_id, attr[i+1], strlen(attr[i+1]));
              cnx->session_id[strlen(attr[i+1])] = '\0';
      } else if (!strcmp("xml:lang", attr[i])) {
              cnx->lang = (char *)malloc(sizeof(char)*(strlen(attr[i+1])+1));
              strncpy(cnx->lang, attr[i+1], strlen(attr[i+1]));
              cnx->lang[strlen(attr[i+1])] = '\0';
      } else if (!strcmp("xmlns", attr[i])) {
              cnx->node_ns = (char *)malloc(sizeof(char)*(strlen(attr[i+1])+1));
              strncpy(cnx->node_ns, attr[i+1], strlen(attr[i+1]));
              cnx->node_ns[strlen(attr[i+1])] = '\0';
      } else {
              node_namespace_load(cnx->ns , attr[i], attr[i+1]);

      }
    if (cnx->stream_callback != NULL)
            CALL_HANDLER(cnx->stream_callback, 0);
  }
  else {
          if (cnx->opened_session == 1) {
                  pthread_mutex_lock(&(cnx->parse_mutex));
                  node_create(cnx, el, attr);
                  pthread_mutex_unlock(&(cnx->parse_mutex));
          }
  }
          
}



static void XMLCALL
data(void *data, const XML_Char *s, int len)
{
        jabbah_context_t *cnx = (jabbah_context_t *)data;
        
        if (cnx->opened_session == 1) {
                pthread_mutex_lock(&(cnx->parse_mutex));
                node_append_value(cnx, (const char *)s, len);
                pthread_mutex_unlock(&(cnx->parse_mutex));
        }
}



static void XMLCALL
end(void *data, const char *el)
{
        jabbah_context_t *cnx = (jabbah_context_t *)data;
        
        if (!strcmp("stream:stream", el))
                {
                        cnx->opened_session = -1;
                }
        else {
                if (cnx->opened_session == 1) {
                        pthread_mutex_lock(&(cnx->parse_mutex));
                        node_close(cnx, el);
                        pthread_mutex_unlock(&(cnx->parse_mutex));
                }
        }

}


void
parseIt(jabbah_context_t *cnx)
{
  int i = 0;
  int len;
  
  XML_SetUserData(cnx->p, cnx);
  XML_SetElementHandler(cnx->p, start, end);
  XML_SetCharacterDataHandler(cnx->p, data);
  
  for (;;) {
    if (cnx->opened_session == -1) break;
    
    len = recv(cnx->sock, &(cnx->buff), 1, 0);
            
    if (len == -1) {
            printf("ERROR: %s\n", strerror(errno));
            break;
    }
//    done = eof(sock);

    //printf("DEBUG: %s", buff);
    if (XML_Parse(cnx->p, &(cnx->buff), len, 0) == XML_STATUS_ERROR) {
            printf("Parse error at line %d:\n%s\n",
                   XML_GetCurrentLineNumber(cnx->p),
                   XML_ErrorString(XML_GetErrorCode(cnx->p)));
            close(cnx->sock);
            cnx->sock = -1;
            return;
    }

//    if (done)
//      break;
  }
  // Free resources

  close(cnx->sock);
  cnx->sock = -1;

}

void *
parsing_thread(void *p)
{
        jabbah_context_t *cnx = (jabbah_context_t *)p;

        // Registration of node callbacks
        node_callback_register(cnx, "message", message_parse_node);
        node_callback_register(cnx, "presence", presence_parse_node);
        node_callback_register(cnx, "iq", iq_parse_node);

//        iq_init("jabber:client", cnx->server_address);
        
        parseIt(cnx);

        // Flushing everything
//        iq_free_resources();
//        node_callback_flush();

//        if (ns != NULL) {
//                node_namespace_free(ns);
//                ns = NULL;
//        }
        
//        if (lang != NULL) {
//                free(lang);
//                lang = NULL;
//        }
//        if (node_ns != NULL) {
//                free(node_ns);
//               node_ns = NULL;
//               } 
}


void
authorize(jabbah_context_t *cnx, jabbah_auth_result_t *auth)
{
        if (auth->code == 500)
                cnx->authorization = 1;
        else
                cnx->authorization = -1;
}


///////////////////////////////////////////////////////////////////////////////////
//
// INTERFACE FUNCTIONS
//


jabbah_context_t *
jabbah_context_create(char *server_addr, int server_port, int ssl)
{
        jabbah_context_t *cnx = NULL;

        cnx = (jabbah_context_t *)malloc(sizeof(jabbah_context_t));

        if (cnx == NULL)
                return NULL;

        cnx->p = XML_ParserCreate(NULL);

        cnx->authorization = 0;
        cnx->opened_session = 0;
        cnx->session_id = NULL;
        cnx->ns = NULL;
        cnx->lang = NULL;
        cnx->node_ns = NULL;
        cnx->stream_callback = NULL;

        
        cnx->server_address = (char *)malloc(sizeof(char)*(strlen(server_addr)+1));
        if (cnx->server_address == NULL) {
                free(cnx);
                return NULL;
        }
        strncpy(cnx->server_address, server_addr, strlen(server_addr));
        cnx->server_address[strlen(server_addr)] = '\0';
                
        
        cnx->server_port = server_port;
        cnx->ssl = ssl;

        pthread_mutex_init(&(cnx->parse_mutex), NULL);
        pthread_mutex_init(&(cnx->iq_mutex), NULL);

        //
        // Node part
        //
        cnx->curr_node = NULL;
        cnx->callbacks = NULL;
        cnx->post_lang = NULL;

        cnx->message_cb = NULL;
        cnx->presence_cb = NULL;
        cnx->auth_cb = NULL;

        cnx->login = NULL;
        cnx->passwd = NULL;
        cnx->resource = NULL;

        cnx->prio = 0;

        cnx->req_id = 0;
        cnx->req_list = NULL;
        
        return cnx;
}

void
jabbah_context_destroy(jabbah_context_t *cnx)
{
        node_callback_t *cb = cnx->callbacks;
        jabbah_req_list_t *reqs  = cnx->req_list;
        jabbah_req_list_t *reqs2 = NULL;

        cnx->req_list = NULL;
        if (cnx == NULL)
                return;

        if (cnx->p != NULL)
                XML_ParserFree(cnx->p);
                

        if (cnx->session_id != NULL)
                free(cnx->session_id);

        if (cnx->ns != NULL)
                node_namespace_free(cnx->ns);

        if (cnx->lang != NULL)
                free(cnx->lang);

        if (cnx->node_ns != NULL)
                free(cnx->node_ns);

        if (cnx->server_address != NULL)
                free(cnx->server_address);

        if (cnx->curr_node != NULL)
                node_free(cnx->curr_node);

        if (cnx->post_lang != NULL)
                free(cnx->post_lang);

        if (cnx->login != NULL)
                free(cnx->login);

        if (cnx->passwd != NULL)
                free(cnx->passwd);

        if (cnx->resource != NULL)
                free(cnx->resource);

        while (cb != NULL) {
                cb = cnx->callbacks->next;
                if (cnx->callbacks->node_name != NULL)
                        free(cnx->callbacks->node_name);
                free(cnx->callbacks);                     
                cnx->callbacks = cb;
        }

        while (reqs != NULL) {
                reqs2 = reqs->next;
                
                if (reqs->id != NULL)
                        free(reqs->id);
                free(reqs);
                reqs = reqs2;
        }

        pthread_mutex_destroy(&(cnx->parse_mutex));
        pthread_mutex_destroy(&(cnx->iq_mutex));

        free(cnx);
}

int
jabbah_connect(jabbah_context_t *cnx, char *login, char *passwd, char *resource, int prio)
{
        struct hostent     *ht;
        struct in_addr      host_addr;
        struct sockaddr_in  st;
        socklen_t           st_len = sizeof (struct sockaddr_in);
        //SSL                *ssl_handler;
        char               *init_stream = NULL;
        int                 init_len = 0;
        int                 ret = 0;


        pthread_mutex_init(&(cnx->parse_mutex), NULL);
        
        presence_init(cnx, prio);
        
        // First - resolve host name
        ht = gethostbyname(cnx->server_address);

        if (!ht)
                return -1;

        host_addr = * (struct in_addr *) ht->h_addr_list[0];

        // Now try to create socket
        cnx->sock = socket(PF_INET, SOCK_STREAM, 0);

        if (cnx->sock == -1)
                return -2;

        // Now - connect!
        st.sin_family = AF_INET;
        st.sin_addr   = host_addr;
        st.sin_port   = htons(cnx->server_port);
        
        ret = connect (cnx->sock, (struct sockaddr *) & st, sizeof (st));

        if (ret ==  -1) {                
                return -3;
        }
        
        //TODO: SSL SUPPORT
        /*
        if (ssl) {
                ssl_ctx = SSL_CTX_new (SSLv23_method ());
                ssl_handler = SSL_new(ssl_ctx);
                SSL_set_fd(ssl_handler, sock);
                if(!SSL_connect(sock))
                        return -4;
        }
        REMEMBER TO FREE ctx_resource! SSL_CTX_free (ssl_ctx);*/

        // Send welcome massage to the server
        init_stream = (char *)malloc(sizeof(char)*1024);
        if (init_stream == NULL)
                return -10;

        init_len = sprintf(init_stream, "<?xml version='1.0'?>\n<stream:stream xmlns:stream='http://etherx.jabber.org/streams' xmlns='jabber:client' to='%s'>\n\n", cnx->server_address);
        write(cnx->sock, init_stream, init_len);
        free(init_stream);
        
        // Now start parsing thread
        if (pthread_create(&(cnx->parse_thread), NULL, parsing_thread, (void *)cnx))
                return -5;

        // Wait until session will be opened
        while (!(cnx->opened_session)) sleep(1);

        //DEBUG MSG
        printf("Session opened! Lang: %s ID: %s\n", cnx->lang, cnx->session_id);

        // Now try to authorize
        auth_register(cnx, login, passwd, resource, authorize);

        // wait for authorization
        while (cnx->authorization == 0) sleep(1);

        if (cnx->authorization == -1) {
                return -6;
        }
        
        return 0;
}


void
jabbah_close(jabbah_context_t *cnx, char *desc) {

        presence_set_status(cnx, STATE_OFFLINE, desc);

        pthread_mutex_lock(&(cnx->parse_mutex));
        pthread_cancel(cnx->parse_thread);
        pthread_join(cnx->parse_thread, NULL);
        pthread_mutex_unlock(&(cnx->parse_mutex));
}


//
// PRESENCE
//
void
jabbah_presence_set_state(jabbah_context_t *cnx, jabbah_state_t state)
{
        presence_set_state(cnx, state);
}


void
jabbah_presence_set_status(jabbah_context_t *cnx, jabbah_state_t state, char *status)
{
        presence_set_status(cnx, state, status);
}


void
jabbah_presence_register_callback(jabbah_context_t *cnx, void (*cb)(jabbah_context_t * , jabbah_presence_t *))
{
        presence_register_callback(cnx, cb);
}

//
// MESSAGE
//
void
jabbah_message_register_callback(jabbah_context_t *cnx, void (*cb)(jabbah_context_t * , jabbah_message_t *))
{
        message_register_callback(cnx, cb);
}


void
jabbah_message_send_normal(jabbah_context_t *cnx, char *jid, char *subject, char *msg)
{
        message_send_normal(cnx, jid, subject, msg);
}


void
jabbah_message_send_chat(jabbah_context_t *cnx, char *jid, char *msg)
{
        message_send_chat(cnx, jid, msg);
}

