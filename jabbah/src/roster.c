#include <stdio.h>
#include <stdlib.h>

#include "common.h"
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


        //
        // If it is a transport - move it to transports list or to items list
        //
        
        if (strstr(it->jid, "@") == NULL) {
                roster->transports = (jabbah_roster_item_t **)realloc(roster->transports,
                                                                      sizeof(jabbah_roster_item_t *)*
                                                                      (++(roster->transport_count)));
                roster->transports[roster->transport_count - 1] = it;
        } else {
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
        }
}


int
roster_item_compare(const void *a, const void *b)
{
        jabbah_roster_item_t **aa = (jabbah_roster_item_t **)a;
        jabbah_roster_item_t **bb = (jabbah_roster_item_t **)b;

        return strcmp((*aa)->name, (*bb)->name);
}


int
roster_group_compare(const void *a, const void *b)
{
        jabbah_roster_group_t **aa = (jabbah_roster_group_t **)a;
        jabbah_roster_group_t **bb = (jabbah_roster_group_t **)b;

        return strcmp((*aa)->name,(*bb)->name);
}


int
roster_res_compare(const void *a, const void *b)
{
        jabbah_logged_res_t **aa = (jabbah_logged_res_t **)a;
        jabbah_logged_res_t **bb = (jabbah_logged_res_t **)b;

        return ((*aa)->prio - (*bb)->prio);
}


jabbah_roster_item_t *
roster_find(jabbah_context_t *cnx, char *jid)
{
        int i = 0;

        for (i = 0; i < cnx->roster->item_count; i++) {
                if (!strcmp(cnx->roster->items[i]->jid, jid))
                        return cnx->roster->items[i];
        }

        for (i = 0; i < cnx->roster->transport_count; i++) {
                if (!strcmp(cnx->roster->transports[i]->jid, jid))
                        return cnx->roster->transports[i];
        }

        
        
        return NULL;
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
        
        cnx->roster = NULL;
        
        //
        // Free nogroups list
        //
        if (r->nogroups != NULL) {
                free(r->nogroups);
                r->nogroup_count = 0;
        }

        //
        // Free transports list
        //
        if (r->transports != NULL) {
                for (i = 0; i < r->transport_count; i++) {
                        free(r->transports[i]->name);
                        free(r->transports[i]->jid);
                        for(j = 0; j < r->transports[i]->res_count; j++) {
                                if (r->transports[i]->res[j]->resource != NULL)
                                        free(r->transports[i]->res[j]->resource);
                                if (r->transports[i]->res[j]->status != NULL)
                                        free(r->transports[i]->res[j]->status);
                                free(r->transports[i]->res[j]);
                        }
                        if (r->transports[i]->res != NULL)
                                free(r->transports[i]->res);
                        free(r->transports[i]);
                }
                free(r->transports);
                r->transport_count = 0;
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
roster_parse_pres_node(jabbah_context_t *cnx, jabbah_node_t *node)
{
        jabbah_attr_list_t *attr = node->attributes;
        char               *type = NULL;
        char               *from = NULL;

        while (attr != NULL) {
                if (!strcmp(attr->name, "type")) {
                        type = (char *)malloc(sizeof(char)*(strlen(attr->value) + 1));
                        strlcpy(type, attr->value, strlen(attr->value));
                } else if (!strcmp(attr->name, "from")) {
                        from = (char *)malloc(sizeof(char)*(strlen(attr->value) + 1));
                        strlcpy(from, attr->value, strlen(attr->value));
                }
                attr = attr->next;
        }

        
                        
        
}


void
roster_parse_node (jabbah_context_t *cnx, jabbah_node_t *node)
{
        jabbah_node_t *query = NULL;
        jabbah_node_t *item  = NULL;
        query = node->subnodes;
        jabbah_roster_t *r = NULL;
        int i;
        
        if (cnx->roster != NULL)
                return;
        
        while (query != NULL && strcmp(query->name, "query"))
                query = query->next;

        if (query == NULL)
                return;
        
	
        if (strcmp(query->namespace, "jabber:iq:roster"))
                return;
	
        r = (jabbah_roster_t *)malloc(sizeof(jabbah_roster_t));
        r->item_count  = 0;
        r->group_count = 0;
        r->nogroup_count  = 0;
        r->transport_count = 0;
        r->items  = NULL;
        r->groups = NULL;
        r->nogroups  = NULL;
        r->transports = NULL;

        item = query->subnodes;

        while (item != NULL) {
                add_item(r, item);
		item = item->next;
	}

        // sort groups
        qsort(r->groups, r->group_count, sizeof(jabbah_roster_group_t *), roster_group_compare);
        // Sort items
        qsort(r->items, r->item_count, sizeof(jabbah_roster_item_t *), roster_item_compare);
        qsort(r->transports, r->transport_count, sizeof(jabbah_roster_item_t *), roster_item_compare);
        qsort(r->nogroups, r->nogroup_count, sizeof(jabbah_roster_item_t *), roster_item_compare);
        
        for (i = 0; i < r->group_count; i++) 
                qsort(r->groups[i]->items, r->groups[i]->item_count, sizeof(jabbah_roster_item_t *), roster_item_compare);

        cnx->roster = r;

}


void
roster_parse_presence(jabbah_context_t *cnx, jabbah_presence_t *pres)
{
        jabbah_roster_item_t *it  = NULL;
        jabbah_logged_res_t  *rsc = NULL;
        char *jid = NULL;
        char *res = NULL;
        int   pos = 0;
        int   r_add = 1;
        int   i = 0;
        
        if (pres->jid == NULL) return;

        //
        // Find JID - resource separator.
        //
        for (i = 0; i < strlen(pres->jid); i++) 
                if (pres->jid[i] == '/') {
                        pos = i;
                        break;
                }

        //
        // Allocate memory for jid and resource and copy that
        //
        jid = (char *)malloc(sizeof(char)*(pos+1));
        if (jid == NULL) return;
        for (i = 0; i < pos; i++) jid[i] = pres->jid[i];
        jid[pos] = '\0';
        
        res = (char *)malloc(sizeof(char)*(strlen(pres->jid)- pos));
        if (res == NULL) {
                free(jid);
                return;
        }
        for (i = pos + 1; i < strlen(pres->jid); i++)
                res[pos - 1 - i] = pres->jid[i];
        res[strlen(pres->jid) - pos - 1] = '\0';
        
        //
        // Get roster item of chosen JID
        //
        it = roster_find(cnx, jid);
        if (it == NULL) {
                free(jid);
                free(res);
                return;
        }

        //
        // Check if there is this resource online.
        // If yes - change it
        // if no - add it if presence is different than offline
        //
        pos = -1;
        for (i = 0; i < it->res_count; i++)
                if (!strcmp(it->res[i]->resource, res)) {
                        rsc = it->res[i];
                        r_add = 0;
                        pos   = i;
                }

        if (r_add == 1 && pres->state != STATE_OFFLINE) {
                rsc = (jabbah_logged_res_t *)malloc(sizeof(jabbah_logged_res_t));
                rsc->resource = (char *)malloc(sizeof(char)*(strlen(res)+1));
                strncpy(rsc->resource, res, strlen(res));
                rsc->resource[strlen(res)] = '\0';

                it->res = (jabbah_logged_res_t **)realloc(it->res, sizeof(jabbah_logged_res_t *)*(++(it->res_count)));
                it->res[it->res_count - 1] = rsc;
                rsc->prio = pres->priority;

                qsort(it->res, it->res_count, sizeof(jabbah_logged_res_t *), roster_res_compare);
        }

        if (rsc == NULL) {
                free(jid);
                free(res);
                return;
        }

        //
        // If state is offline - remove resource and resort the rest.
        // Otherwise - change it
        //
        if (pres->state == STATE_OFFLINE) {
                if (pos > -1) {
                        for (i = pos + 1; i < it->res_count; i++) 
                                it->res[i - 1] = it->res[i];
                        it->res[it->res_count - 1] = NULL;

                        if (rsc->status != NULL)
                                free(rsc->status);

                        if (rsc->resource != NULL)
                                free(rsc->resource);

                        free(rsc);
                }
        } else {
                
                rsc->state = pres->state;
                
                if (rsc->status != NULL)
                        free(rsc->status);
                
                if (pres->status != NULL) {
                        rsc->status = (char *)malloc(sizeof(char)*(strlen(pres->status)+1));
                        strncpy(rsc->status, pres->status, strlen(pres->status));
                        rsc->status[strlen(pres->status)] = '\0';
                }
        }

        free(jid);
        free(res);
}
