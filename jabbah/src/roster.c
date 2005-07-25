#include <stdio.h>

#include "node.h"
#include "iq.h"
#include "roster.h"


static void
add_to_group(jabbah_roster_t *roster, jabbah_roster_item_t *item, char *group)
{
        jabbah_roster_group_t **gr  = roster->groups;
        jabbah_roster_group_t  *ngr = NULL;
        int                     i   = 0;
        
        //
        // Check if this groups is already added
        //
        for (i = 0; i < roster->group_count; i++) {
                if (!strcmp(gr[i]->name, group)) {
                        ngr = gr[i];
                        break;
                }
        }

        //
        // If group does not exist - create it and add to the list
        //
        if (ngr == NULL) {
                ngr = (jabbah_roster_group_t *)malloc(sizeof(jabbah_roster_group_t));

                gr = (jabbah_roster_group_t **)realloc(gr, sizeof(jabbah_roster_group_t *)*(++(roster->group_count)));
                roster->groups = gr;
                
                ngr->name = (char *)malloc(sizeof(char)*(strlen(group)+1));
                strncpy(ngr->name, group, strlen(group));
                ngr->name[strlen(group)] = '\0';
                ngr->items = NULL;
                ngr->item_count = 0;

                roster->groups[roster->group_count - 1] = ngr;
        }
        
        ngr->items = (jabbah_roster_item_t **)realloc(ngr->items,
                                                      sizeof(jabbah_roster_item_t *)*(++(ngr->item_count)));
        ngr->items[ngr->item_count -1] = item;
        

}



static void
add_item(jabbah_roster_t *roster, jabbah_node_t *item)
{
        jabbah_node_t        *group = NULL;
        jabbah_roster_item_t *it    = NULL;
        jabbah_attr_list_t   *attr  = NULL;
        int                   group_count = 0;
        
        it = (jabbah_roster_item_t *)malloc(sizeof(jabbah_roster_item_t));
        it->name         = NULL;
        it->jid          = NULL;
        it->subscription = SUB_NONE;
        it->res          = NULL;
        it->res_count    = 0;
        
        attr = item->attributes;

        //
        // Set attributes of the item
        //
        while (attr != NULL) {
                if (!strcmp(attr->name, "name")) {
                        it->name = (char *)malloc(sizeof(char)*(strlen(attr->value)+1));
                        strncpy(it->name, attr->value, strlen(attr->value));
                        it->name[strlen(attr->value)] = '\0';
                } else if (!strcmp(attr->name, "jid")) {
                        it->jid = (char *)malloc(sizeof(char)*(strlen(attr->value)+1));
                        strncpy(it->jid, attr->value, strlen(attr->value));
                        it->jid[strlen(attr->value)] = '\0';
                } else if (!strcmp(attr->name, "subscription")) {
                        if (!strcmp(attr->value, "to")) {
                                it->subscription = SUB_TO;
                        } else if (!strcmp(attr->value, "from")) {
                                it->subscription = SUB_FROM;
                        } else if (!strcmp(attr->value, "both")) {
                                it->subscription = SUB_BOTH;
                        }
                }
                attr = attr->next;
        }

        //
        // If item has no name, copy his jid to name
        //
        if (it->name == NULL) {
                it->name = (char *)malloc(sizeof(char)*(strlen(it->jid)+1));
                strncpy(it->name, it->jid, strlen(it->jid));
                it->name[strlen(it->jid)] = '\0';
        }

        //
        // Check if item belong to some groups and add to them
        //
        group = item->subnodes;

        while (group != NULL) {
                if (!strcmp(group->name, "group") && group->value != NULL) {
                        add_to_group(roster, it, group->value);
                        group_count++;
                }

                group = group->next;    
        }                            

        if (group_count == 0) {
                roster->nogroups = (jabbah_roster_item_t **)realloc(roster->nogroups,
                                                                         sizeof(jabbah_roster_item_t *)*
                                                                         (++(roster->nogroup_count)));
                roster->nogroups[roster->nogroup_count - 1] = it;
        }

        roster->items = (jabbah_roster_item_t **)realloc(roster->items,
                                                              sizeof(jabbah_roster_item_t *)*
                                                              (++(roster->item_count)));
        roster->items[roster->item_count - 1] = it;
        printf("Dodalem %d element\n", roster->item_count);
}


void
roster_get(jabbah_context_t *cnx)
{
        if (cnx->roster != NULL) {
                roster_free(cnx);
                cnx->roster = NULL;
        }

        iq_send_query(cnx, IQ_GET, "jabber:iq:roster", NULL, roster_parse_node);
}


jabbah_roster_t *
roster_wait(jabbah_context_t *cnx)
{
        roster_get(cnx);

        while (cnx->roster == NULL) sleep(1);

        return cnx->roster;
}


void
roster_free(jabbah_context_t *cnx)
{
        jabbah_roster_t *r = cnx->roster;
        int              i, j;

        if (r == NULL)
                return;
        
        //
        // Free nogroups list
        //
        if (r->nogroups != NULL) {
                free(r->nogroups);
                r->nogroup_count = 0;
        }

        //
        // Free groups
        //
        for (i = 0; i < r->group_count; i++) {
                free(r->groups[i]->name);
                free(r->groups[i]->items);
                free(r->groups[i]);
        }
        if (r->groups != NULL)
                free(r->groups);

        //
        // Free items
        //
        for (i = 0; i < r->item_count; i++) {
                free(r->items[i]->name);
                free(r->items[i]->jid);
                for(j = 0; j < r->items[i]->res_count; j++) {
                        if (r->items[i]->res[j]->resource != NULL)
                                free(r->items[i]->res[j]->resource);
                        if (r->items[i]->res[j]->status != NULL)
                                free(r->items[i]->res[j]->status);
                        free(r->items[i]->res[j]);
                }
                if (r->items[i]->res != NULL)
                        free(r->items[i]->res);
                free(r->items[i]);
        }
        if (r->items != NULL)
                free(r->items);

        //
        // Free roster itself
        //
        free(r);
                

        cnx->roster = NULL;
}



void
roster_parse_node (jabbah_context_t *cnx, jabbah_node_t *node)
{
        jabbah_node_t *query = NULL;
        jabbah_node_t *item  = NULL;
        query = node->subnodes;
        jabbah_roster_t *r = NULL;
        
	printf("Entered parse node\n");
        if (cnx->roster != NULL)
                return;
	printf("Going..\n");
        
        while (query != NULL && strcmp(query->name, "query"))
                query = query->next;

        if (query == NULL)
                return;
        
	printf("Going..(have %s node)\n", query->name);
	
        if (strcmp(query->namespace, "jabber:iq:roster"))
                return;
	
        printf("Going..(namespace approved)\n");
	
        r = (jabbah_roster_t *)malloc(sizeof(jabbah_roster_t));
        r->item_count  = 0;
        r->group_count = 0;
        r->nogroup_count  = 0;
        r->items  = NULL;
        r->groups = NULL;
        r->nogroups  = NULL;

        item = query->subnodes;

        while (item != NULL) {
		printf("Add item\n");
                add_item(r, item);
		item = item->next;
	}

        cnx->roster = r;

}
