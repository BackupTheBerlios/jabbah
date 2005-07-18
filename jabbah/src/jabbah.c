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

#define BUFFSIZE 1

static XML_Parser          p = NULL;
static char                buff[BUFFSIZE];

static int                 sock;
//static SSL_CTX            *ssl_ctx = NULL;
static pthread_t           parse_thread;
static pthread_mutex_t     parse_mutex;
static int                 authorization = 0;
static int                 opened_session = 0;
static char               *session_id = NULL;
static jabbah_namespace_t *ns = NULL;
static char               *lang = NULL;
static char               *node_ns = NULL;
static void               *stream_callback = NULL;
static char               *server_address = NULL;

static void XMLCALL
start(void *data, const char *el, const char **attr)
{
  int i = 0;

  if (!strcmp("stream:stream", el))
  {
    opened_session = 1;
    // Getting session id
    for (i = 0; attr[i]; i += 2)
      if (!strcmp("id", attr[i]))
      {
              session_id = (char *)malloc(sizeof(char)*(strlen(attr[i+1])+1));
              strncpy(session_id, attr[i+1], strlen(attr[i+1]));
              session_id[strlen(attr[i+1])] = '\0';
              auth_init(session_id);
      } else if (!strcmp("xml:lang", attr[i])) {
              lang = (char *)malloc(sizeof(char)*(strlen(attr[i+1])+1));
              strncpy(lang, attr[i+1], strlen(attr[i+1]));
              lang[strlen(attr[i+1])] = '\0';
      } else if (!strcmp("xmlns", attr[i])) {
              node_ns = (char *)malloc(sizeof(char)*(strlen(attr[i+1])+1));
              strncpy(node_ns, attr[i+1], strlen(attr[i+1]));
              node_ns[strlen(attr[i+1])] = '\0';
      } else {
              ns = node_namespace_load(ns , attr[i], attr[i+1]);

      }
    node_set_env(sock, lang, node_ns, ns);
    if (stream_callback != NULL)
            CALL_HANDLER(stream_callback, 0);
  }
  else {
          if (opened_session == 1) {
                  pthread_mutex_lock(&parse_mutex);
                  node_create(el, attr);
                  pthread_mutex_unlock(&parse_mutex);
          }
  }
          
}



static void XMLCALL
data(void *data, const XML_Char *s, int len)
{
        if (opened_session == 1) {
                pthread_mutex_lock(&parse_mutex);
                node_append_value((const char *)s, len);
                pthread_mutex_unlock(&parse_mutex);
        }
}



static void XMLCALL
end(void *data, const char *el)
{
        
        if (!strcmp("stream:stream", el))
                {
                        opened_session = -1;
                }
        else {
                if (opened_session == 1) {
                        pthread_mutex_lock(&parse_mutex);
                        node_close(el);
                        pthread_mutex_unlock(&parse_mutex);
                }
        }

}


void
parseIt()
{
  int i = 0;

  p =   XML_ParserCreate(NULL);
  if (! p) {
    exit(-1);
  }

  XML_SetElementHandler(p, start, end);
  XML_SetCharacterDataHandler(p, data);
  
  for (;;) {
    int done;
    int len;
    if (opened_session == -1) break;
    
    len = recv(sock, buff, BUFFSIZE, 0);
            
    if (len == -1) {
            printf("ERROR: %s\n", strerror(errno));
            break;
    }
//    done = eof(sock);

    //printf("DEBUG: %s", buff);
    if (XML_Parse(p, buff, len, 0) == XML_STATUS_ERROR) {
            printf("Parse error at line %d:\n%s\n",
                   XML_GetCurrentLineNumber(p),
                   XML_ErrorString(XML_GetErrorCode(p)));
            exit(-1);
    }

//    if (done)
//      break;
  }
  // Free resources
  XML_ParserFree(p);
  p = NULL;
  close(sock);
  sock = -1;
  if (session_id != NULL)
    free(session_id);
}

void *
parsing_thread(void *p)
{

        // Registration of node callbacks
        node_callback_register("message", message_parse_node);
        node_callback_register("presence", presence_parse_node);
        node_callback_register("iq", iq_parse_node);

        iq_init("jabber:client", server_address);
        
        parseIt();

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
free_resources()
{
        if (p != NULL) {
                XML_ParserFree(p);
                p = NULL;
        }

        if (sock > -1) {
                close(sock);
                sock = -1;
        }
        
        if (session_id != NULL) {
                free(session_id);
                session_id = NULL;
        }

        iq_free_resources();
        node_free_resources();

        if (ns != NULL) {
                node_namespace_free(ns);
                ns = NULL;
        }
        
        if (lang != NULL) {
                free(lang);
                lang = NULL;
        }
        if (node_ns != NULL) {
                free(node_ns);
                node_ns = NULL;
        }
  
}

void
authorize(jabbah_auth_result_t *auth)
{
        if (auth->code == 500)
                authorization = 1;
        else
                authorization = -1;
}


///////////////////////////////////////////////////////////////////////////////////
//
// INTERFACE FUNCTIONS
//
int
jabbah_connect(char *login, char *passwd, char *resource, int prio, char *server_addr, int server_port, int ssl)
{
        struct hostent     *ht;
        struct in_addr      host_addr;
        struct sockaddr_in  st;
        socklen_t           st_len = sizeof (struct sockaddr_in);
        //SSL                *ssl_handler;
        char               *init_stream = NULL;
        int                 init_len = 0;
        int                 ret = 0;

        server_address = server_addr;

        pthread_mutex_init(&parse_mutex, NULL);
        
        presence_init(prio);
        
        // First - resolve host name
        ht = gethostbyname(server_addr);

        if (!ht)
                return -1;

        host_addr = * (struct in_addr *) ht->h_addr_list[0];

        // Now try to create socket
        sock = socket(PF_INET, SOCK_STREAM, 0);

        if (sock == -1)
                return -2;

        // Now - connect!
        st.sin_family = AF_INET;
        st.sin_addr   = host_addr;
        st.sin_port   = htons(server_port);
        
        ret = connect (sock, (struct sockaddr *) & st, sizeof (st));

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

        init_len = sprintf(init_stream, "<?xml version='1.0'?>\n<stream:stream xmlns:stream='http://etherx.jabber.org/streams' xmlns='jabber:client' to='%s'>\n\n", server_addr);
        write(sock, init_stream, init_len);
        free(init_stream);
        
        // Now start parsing thread
        if (pthread_create(&parse_thread, NULL, parsing_thread, NULL))
                return -5;

        // Wait until session will be opened
        while (!opened_session) sleep(1);

        //DEBUG MSG
        printf("Session opened! Lang: %s ID: %s\n", lang, session_id);

        // Now try to authorize
        auth_register(login, passwd, resource, authorize);

        // wait for authorization
        while (authorization == 0) sleep(1);

        if (authorization == -1) {
                return -6;
        }
        
        return 0;
}


void
jabbah_close(char *desc) {

        presence_set_status(STATE_OFFLINE, desc);

        pthread_mutex_lock(&parse_mutex);
        pthread_cancel(parse_thread);
        pthread_join(parse_thread, NULL);

        free_resources();
        pthread_mutex_destroy(&parse_mutex);
}
