#include <sys/utsname.h>
#include <string.h>

#include "../config.h"
#include "node.h"
#include "iq.h"
#include "version.h"

void
version_send(jabbah_context_t *cnx, jabbah_node_t *node)
{
        jabbah_node_t      *name    = NULL;
        jabbah_node_t      *version = NULL;
        jabbah_node_t      *os      = NULL;
        jabbah_attr_list_t *attr = node->attributes;
        char               *id   = NULL;
        char               *to   = NULL;
        char               *os_txt   = NULL;
        struct utsname     sysop;

        //
        // Checking type
        //
        while (attr != NULL && strcmp(attr->name, "type"))
                attr = attr->next;

        if (attr == NULL)
                return;

        if (strcmp(attr->value, "get"))
                return;

        //
        // Getting query ID
        //
        attr = node->attributes;

        while (attr != NULL && strcmp(attr->name, "id"))
                attr = attr->next;

        if (attr == NULL)
                return;

        id = attr->value;

        //
        // Geting sender
        //
        attr = node->attributes;

        while (attr != NULL && strcmp(attr->name, "from"))
                attr = attr->next;

        if (attr == NULL)
                return;

        to = attr->value;

        //
        // Getting system type
        //
        uname(&sysop);

        os_txt = (char *)malloc(sizeof(char)*(strlen(sysop.sysname) + strlen(sysop.release) + 2));
        strncpy(os_txt, sysop.sysname, strlen(sysop.sysname));
        os_txt[strlen(sysop.sysname)] = ' ';
        os_txt[strlen(sysop.sysname) + 1] = '\0';
        strcat(os_txt, sysop.release);
        
        name = node_init("name");
        name = node_value_set(name, "Jabbah");
        
        version = node_init("version");
        version = node_value_set(version, VERSION);

        os = node_init("os");
        os = node_value_set(os, os_txt);
        free(os_txt);

        version->next = os;
        name->next = version;
        
        iq_send_response(cnx, to, id, "jabber:iq:version", name);
}
