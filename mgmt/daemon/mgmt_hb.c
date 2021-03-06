/*
 * Linux HA management library
 *
 * Author: Huang Zhen <zhenhltc@cn.ibm.com>
 * Copyright (c) 2005 International Business Machines
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <pygui_internal.h>

#include <unistd.h>
#include <string.h>
#include <glib.h>

#include <heartbeat.h>
#include <clplumbing/cl_log.h>
#include <clplumbing/cl_syslog.h>
#include <clplumbing/GSource.h>

#include <hb_api.h>

#include "mgmt_internal.h"

int init_heartbeat(void);
void final_heartbeat(void);

static ll_cluster_t * hb = NULL;
static char* on_get_allnodes(char* argv[], int argc);
static char* on_get_hb_config(char* argv[], int argc);
static char* on_get_nodetype(char* argv[], int argc);
static gboolean on_hb_input(ll_cluster_t* hb, gpointer data);
static char* on_echo(char* argv[], int argc);
static void on_hb_quit(gpointer);
char* hb_config = NULL;

const char* param_name[] = {
	"quorum_server",
	"use_logd",
	"autojoin",
	"apiauth",
	"auto_failback",
	"baud",
	"debug",
	"debugfile",
	"deadping",
	"deadtime",
	"hbversion",
	"hopfudge",
	"initdead",
	"keepalive",
	"logfacility",
	"logfile",
	"msgfmt",
	"nice_failback",
	"node",
	"normalpoll",
	"stonith",
	"udpport",
	"warntime",
	"watchdog",
	"cluster"
};

char* 
on_get_allnodes(char* argv[], int argc)
{
	const char* name = NULL;
	char* ret = strdup(MSG_OK);
	if (hb->llc_ops->init_nodewalk(hb) != HA_OK) {
		mgmt_log(LOG_ERR, "Cannot start node walk");
		mgmt_log(LOG_ERR, "REASON: %s", hb->llc_ops->errmsg(hb));
		free(ret);
		return strdup(MSG_FAIL);
	}
	while((name = hb->llc_ops->nextnode(hb))!= NULL) {
		ret = mgmt_msg_append(ret, name);
	}
	if (hb->llc_ops->end_nodewalk(hb) != HA_OK) {
		mgmt_log(LOG_ERR, "Cannot end node walk");
		mgmt_log(LOG_ERR, "REASON: %s", hb->llc_ops->errmsg(hb));
		free(ret);
		return strdup(MSG_FAIL);
	}
	
	return ret;
}

char*
on_get_hb_config(char* argv[], int argc)
{
	int i;
	char* value = NULL;
	if (hb_config == NULL) {
		hb_config = strdup(MSG_OK);
		for (i = 0; i < sizeof(param_name)/sizeof(param_name[0]); i++) {
			value = hb->llc_ops->get_parameter(hb, param_name[i]);
			hb_config = mgmt_msg_append(hb_config, param_name[i]);
			hb_config = mgmt_msg_append(hb_config, value!=NULL?value:""); 
			if (value != NULL) {
				free(value);
			}	
		}	
	}
	return strdup(hb_config);
}

char*
on_get_nodetype(char* argv[], int argc)
{
	const char* type;
	char* ret;
	type = hb->llc_ops->node_type(hb,argv[1]);
	if (type != NULL) {
		ret = strdup(MSG_OK);
		ret = mgmt_msg_append(ret, type);
	}
	else {
		ret = strdup(MSG_FAIL);
	}
	return ret;
}

int
init_heartbeat(void)
{
	hb = ll_cluster_new("heartbeat");
	if (hb->llc_ops->signon(hb, client_name)!= HA_OK) {
		mgmt_log(LOG_ERR, "Cannot sign on with heartbeat");
		mgmt_log(LOG_ERR, "REASON: %s", hb->llc_ops->errmsg(hb));
		hb->llc_ops->delete(hb);
		hb = NULL;
		return -1;
	}
	G_main_add_ll_cluster(G_PRIORITY_LOW, hb,
			FALSE, on_hb_input, NULL, on_hb_quit);
	
	reg_msg(MSG_ALLNODES, on_get_allnodes);
	reg_msg(MSG_HB_CONFIG, on_get_hb_config);
	reg_msg(MSG_NODE_TYPE, on_get_nodetype);
	reg_msg(MSG_ECHO, on_echo);	

	return 0;
}

void
final_heartbeat(void)
{
	if (hb != NULL) {
		hb->llc_ops->delete(hb);
		hb = NULL;
	}
	fire_event(EVT_DISCONNECTED);
}

gboolean
on_hb_input(ll_cluster_t *hb, gpointer data)
{
	struct ha_msg* msg;
	IPC_Channel *chan = NULL;

	if(hb != NULL) {
		chan = hb->llc_ops->ipcchan(hb);
	}
	if (chan == NULL || !IPC_ISRCONN(chan)) {
		fire_event(EVT_DISCONNECTED);
		mgmt_log(LOG_ERR, "Lost connection to heartbeat service.");
		hb = NULL;
		return FALSE;
	}
	msg = hb->llc_ops->readmsg(hb, 0);
	if (msg != NULL) {
		ha_msg_del(msg);
	}
	return TRUE;
}
void
on_hb_quit(gpointer user_data)
{
	return;
}

char* 
on_echo(char* argv[], int argc)
{
	char* ret = strdup(MSG_OK);
	ret = mgmt_msg_append(ret, argv[1]);

	return ret;
}

