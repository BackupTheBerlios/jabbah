#ifndef _JABBAH_H_
#define _JABBAH_H_

#include "node.h"
#include "iq.h"
#include "message.h"
#include "presence.h"
#include "auth.h"

int  jabbah_connect(char *login, char *passwd, char *resource, int prio, char *server_addr, int server_port, int ssl);
void jabbah_close();
#endif
