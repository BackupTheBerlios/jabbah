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
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"
#include "message.h"
#include "presence.h"
#include "node.h"
#include "iq.h"
#include "auth.h"
#include "roster.h"
#include "jabbah.h"

#define CALL_HANDLER(h,x) (((void (*)(int *))h)(x))

#define BUFFSIZE


/* Xml Functions */

// We parse a xml sended by the server
// save the session attributes or append the node(for later specific parsing).
static void XMLCALL
start(void *data, const char *el, const char **attr)
{
  int i = 0;
  jabbah_context_t *cnx = (jabbah_context_t *)data;
  
  if (!strcmp("stream:stream", el)) {
    cnx->opened_session = 1;

    // Getting the params
	// Because attr is a double pointer, i = attribute_name, i+1 = attribute_value
    for (i = 0; attr[i]; i += 2){
      if (!strcmp("id", attr[i])){
              cnx->session_id = (char *)malloc(sizeof(char)*(strlen(attr[i+1])+1));
              strlcpy(cnx->session_id, attr[i+1], strlen(attr[i+1])+1);
      } else if (!strcmp("xml:lang", attr[i])) {
              cnx->lang = (char *)malloc(sizeof(char)*(strlen(attr[i+1])+1));
              strlcpy(cnx->lang, attr[i+1], strlen(attr[i+1])+1);
      } else if (!strcmp("xmlns", attr[i])) {
              cnx->node_ns = (char *)malloc(sizeof(char)*(strlen(attr[i+1])+1));
              strlcpy(cnx->node_ns, attr[i+1], strlen(attr[i+1])+1);
      } else {
              node_namespace_load(cnx->ns , attr[i], attr[i+1]);
      }
	}
    if (cnx->stream_callback != NULL)
            CALL_HANDLER(cnx->stream_callback, 0);

  } else {
          if (cnx->opened_session == 1) {
                  pthread_mutex_lock(&(cnx->parse_mutex));
                  node_create(cnx, el, attr);
                  pthread_mutex_unlock(&(cnx->parse_mutex));
          }
  }
          
}


// We parse the inner xml (where it is value)
// <xml>Value</xml>
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


// Here the end tags are parsed
// And we call the apropriated callback for the tag
// in node_close
static void XMLCALL
end(void *data, const char *el)
{
        jabbah_context_t *cnx = (jabbah_context_t *)data;
       
	   	// The connection was closed? 	
        if (!strcmp("stream:stream", el)){
                cnx->opened_session = -1;
		} else {
                if (cnx->opened_session == 1) {
                        pthread_mutex_lock(&(cnx->parse_mutex));
                        node_close(cnx, el); 		// Please note that node_close will call the apropriated callback( if is closing iq, message or presence ) 
                        pthread_mutex_unlock(&(cnx->parse_mutex));
                }
        }
}

// This function will expect for any input
// and will survive forever, in parse_thread
void
parseIt(jabbah_context_t *cnx)
{
  int i = 0;
  int len;
  
  XML_SetUserData(cnx->p, cnx);
  XML_SetElementHandler(cnx->p, start, end);
  XML_SetCharacterDataHandler(cnx->p, data);

  pthread_mutex_lock(&(cnx->con_mutex));
  while (cnx->continue_parse == 1) {
    pthread_mutex_unlock(&(cnx->con_mutex));      
    if (cnx->opened_session == -1) break;
    
    len = recv(cnx->sock, &(cnx->buff), 1, 0);
    if (len == -1) {
            fprintf(stderr,"ERROR: %s\n", strerror(errno));
            break;
    }
//    done = eof(sock);

    //printf("DEBUG: %s", buff);
    if (XML_Parse(cnx->p, &(cnx->buff), len, 0) == XML_STATUS_ERROR) {
            fprintf(stderr,"Parse error at line %d:\n%s\n",
                   XML_GetCurrentLineNumber(cnx->p),
                   XML_ErrorString(XML_GetErrorCode(cnx->p)));
            close(cnx->sock);
            cnx->sock = -1;
            return;
    }
    pthread_mutex_lock(&(cnx->con_mutex));      
//    if (done)
//      break;
  }
  // Free resources
  pthread_mutex_unlock(&(cnx->con_mutex));      

}

void *
parsing_thread(void *p)
{
        jabbah_context_t *cnx = (jabbah_context_t *)p;

        // Registration of node callbacks
        node_callback_register(cnx, "message", message_parse_node);
        node_callback_register(cnx, "presence", presence_parse_node);
        node_callback_register(cnx, "iq", iq_parse_node);

        parseIt(cnx);
        pthread_exit(NULL);
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
jabbah_context_create(char *server_addr, u_int server_port, int ssl)
{
        jabbah_context_t *cnx = NULL;
        cnx = (jabbah_context_t *)malloc(sizeof(jabbah_context_t));

        if (cnx == NULL)
                return NULL;

        cnx->p = XML_ParserCreate(NULL);

        cnx->continue_parse = 1;
        cnx->authorization = 0;
        cnx->opened_session = 0;
        cnx->session_id = NULL;
        cnx->ns = NULL;
        cnx->lang = NULL;
        cnx->post_lang = NULL;
        cnx->node_ns = NULL;
        cnx->stream_callback = NULL;

       
	    // XMPP compliant
		if(strlen(server_addr) > 1023 || strlen(server_addr) < 0)
			return NULL;	

        cnx->server_address = (char *)malloc(sizeof(char)*(strlen(server_addr)+1));
        if (cnx->server_address == NULL) {
                free(cnx);
                return NULL;
        }
        strlcpy(cnx->server_address, server_addr, strlen(server_addr)+1);
       
        cnx->server_port = server_port;
        cnx->ssl = ssl;

        pthread_mutex_init(&(cnx->parse_mutex), NULL);
        pthread_mutex_init(&(cnx->write_mutex), NULL);
        pthread_mutex_init(&(cnx->iq_mutex), NULL);
        pthread_mutex_init(&(cnx->con_mutex), NULL);

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

        cnx->roster = NULL;
        
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

        if (cnx->roster != NULL) 
                roster_free(cnx);

        pthread_mutex_destroy(&(cnx->parse_mutex));
        pthread_mutex_destroy(&(cnx->write_mutex));
        pthread_mutex_destroy(&(cnx->iq_mutex));
        pthread_mutex_destroy(&(cnx->con_mutex));

        free(cnx);
}

#define DNS_ERROR 	 			-1
#define SOCK_CREAT_ERROR 		-2
#define CONNECT_FAIL 			-3
#define THR_PARSE_CREAT_ERROR 	-5
#define AUTHORIZATION_ERROR 	-6
#define LOGIN_TOO_BIG 			-7
#define RESOURCE_TOO_BIG 		-8
#define NO_MEM 					-10
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

		// We check for node and resource is too big(xmpp reference)
		if(strlen(login) > 1023)  	return LOGIN_TOO_BIG;
		if(strlen(resource) > 1023) return RESOURCE_TOO_BIG;

        pthread_mutex_init(&(cnx->parse_mutex), NULL);
        presence_init(cnx, prio); 				// Sounds stupid, but actually it only set cnx->prio = prio
        
        // First - resolve host name
        ht = gethostbyname(cnx->server_address);
        if (!ht)
                return DNS_ERROR;

        host_addr = * (struct in_addr *) ht->h_addr_list[0];

        // Try to create the socket
        cnx->sock = socket(PF_INET, SOCK_STREAM, 0);
        if (cnx->sock == -1)
                return SOCK_CREAT_ERROR;

        // Now connect!
        st.sin_family = AF_INET;
        st.sin_addr   = host_addr;
        st.sin_port   = htons(cnx->server_port);
        
        ret = connect (cnx->sock, (struct sockaddr *) & st, sizeof (st));
        if (ret ==  -1) {                
                return CONNECT_FAIL;
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
		// Possible buffer overflow fixed by <frangossauro@gmail.com> at 10/07/06  
		init_stream = (char *) malloc(sizeof(char) * (116 + strlen(cnx->server_address) + 1));
        if (init_stream == NULL)
                return NO_MEM;

		// TODO:  Use the node API, instead of handcoded xml
        init_len = sprintf(init_stream, "<?xml version='1.0'?>\n"
										"<stream:stream xmlns:stream='http://etherx.jabber.org/streams' xmlns='jabber:client' to='%s'>\n\n",
									   	cnx->server_address);
        write(cnx->sock, init_stream, init_len);
        free(init_stream);
        
        // Now start parsing thread
        if (pthread_create(&(cnx->parse_thread), NULL, parsing_thread, (void *)cnx))
                return THR_PARSE_CREAT_ERROR;

        // Wait until session will be opened (400 microseconds sounds more appropriate...)
        while (!(cnx->opened_session)) 
			usleep( 400 );

        // Now try to authorize
        auth_register(cnx, login, passwd, resource, authorize);

        // wait for authorization
        while (cnx->authorization == 0) sleep(1);
        if (cnx->authorization == -1) {
                return AUTHORIZATION_ERROR;
        }
        
        return 0;
}


void
jabbah_close(jabbah_context_t *cnx, char *desc) {

        presence_set_status(cnx, STATE_OFFLINE, desc);

        pthread_mutex_lock(&(cnx->con_mutex));
        cnx->continue_parse = 0;
        pthread_mutex_unlock(&(cnx->con_mutex));
        pthread_mutex_lock(&(cnx->write_mutex));
        write(cnx->sock, "</stream:stream>\n", 17);
        pthread_mutex_unlock(&(cnx->write_mutex));
        pthread_join(cnx->parse_thread, NULL);
        close(cnx->sock);
        cnx->sock = -1;
        cnx->opened_session = 0;
        cnx->authorization = 0;

        if (cnx->lang != NULL) {
                free(cnx->lang);
                cnx->lang = NULL;
        }

        if (cnx->node_ns != NULL) {
                free(cnx->node_ns);
                cnx->node_ns = NULL;
        }

        if (cnx->ns != NULL) {
                node_namespace_free(cnx->ns);
                cnx->ns = NULL;
        }

        if (cnx->session_id != NULL) {
                free(cnx->session_id);
                cnx->session_id = NULL;
        }

        if (cnx->login != NULL) {
                free(cnx->login);
                cnx->login = NULL;
        }

        if (cnx->passwd != NULL) {
                free(cnx->passwd);
                cnx->passwd = NULL;
        }

        if (cnx->resource != NULL) {
                free(cnx->resource);
                cnx->resource = NULL;
        }

        cnx->prio = 0;
        
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


void
jabbah_roster_get(jabbah_context_t *cnx)
{
        roster_get(cnx);
}


jabbah_roster_t *
jabbah_roster_wait(jabbah_context_t *cnx)
{
        return roster_wait(cnx);
}

