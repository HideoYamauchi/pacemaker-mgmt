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

#include <unistd.h>
#include <glib.h>
#include <regex.h>
#include <dirent.h>

#include <hb_config.h>
#include <clplumbing/cl_log.h>
#include <clplumbing/cl_syslog.h>
#include <clplumbing/lsb_exitcodes.h>

#include <crm/cib.h>
#include <crm/msg_xml.h>
#include <crm/pengine/status.h>

#ifdef SUPPORT_AIS
#undef SUPPORT_AIS
#endif

#ifdef SUPPORT_HEARTBEAT
#undef SUPPORT_HEARTBEAT
#endif

#include <pygui_internal.h>

#include "mgmt_internal.h"

extern resource_t *group_find_child(resource_t *rsc, const char *id);
extern crm_data_t * do_calculations(
	pe_working_set_t *data_set, crm_data_t *xml_input, ha_time_t *now);

cib_t*	cib_conn = NULL;
int in_shutdown = FALSE;
int init_crm(int cache_cib);
void final_crm(void);

static void on_cib_diff(const char *event, crm_data_t *msg);

static char* on_get_cluster_type(char* argv[], int argc);
static char* on_get_cib_version(char* argv[], int argc);
static char* on_get_crm_schema(char* argv[], int argc);
static char* on_get_crm_dtd(char* argv[], int argc);

static char* on_get_crm_metadata(char* argv[], int argc);
static char* on_crm_attribute(char* argv[], int argc);
static char* on_get_activenodes(char* argv[], int argc);
static char* on_get_crmnodes(char* argv[], int argc);
static char* on_get_dc(char* argv[], int argc);

static char* on_migrate_rsc(char* argv[], int argc);
static char* on_set_node_standby(char* argv[], int argc);
static char* on_get_node_config(char* argv[], int argc);
static char* on_get_running_rsc(char* argv[], int argc);

static char* on_del_rsc(char* argv[], int argc);
static char* on_cleanup_rsc(char* argv[], int argc);
static char* on_move_rsc(char* argv[], int argc);

static char* on_update_master(char* argv[], int argc);
static char* on_get_master(char* argv[], int argc);

static char* on_get_all_rsc(char* argv[], int argc);
static char* on_get_rsc_type(char* argv[], int argc);
static char* on_get_sub_rsc(char* argv[], int argc);
static char* on_get_rsc_running_on(char* argv[], int argc);
static char* on_get_rsc_status(char* argv[], int argc);
static char* on_op_status2str(char* argv[], int argc);

static char* on_crm_rsc_cmd(char* argv[], int argc);
static char* on_set_rsc_attr(char* argv[], int argc);
static char* on_get_rsc_attr(char* argv[], int argc);
static char* on_del_rsc_attr(char* argv[], int argc);

static char* on_get_constraints(char* argv[], int argc);
static char* on_get_constraint(char* argv[], int argc);
static char* on_update_constraint(char* argv[], int argc);
static char* on_delete_constraint(char* argv[], int argc);

/* new CRUD protocol */
static char* on_cib_create(char* argv[], int argc);
static char* on_cib_query(char* argv[], int argc);
static char* on_cib_update(char* argv[], int argc);
static char* on_cib_replace(char* argv[], int argc);
static char* on_cib_delete(char* argv[], int argc);
/* end new protocol */

static char* on_gen_cluster_report(char* argv[], int argc);
static char* on_get_pe_inputs(char* argv[], int argc);
static char* on_gen_pe_graph(char* argv[], int argc);
static char* on_gen_pe_info(char* argv[], int argc);

static void get_meta_attributes_id(const char* rsc_id, char* id);
static int delete_object(const char* type, const char* entry, const char* id, crm_data_t** output);
static GList* find_xml_node_list(crm_data_t *root, const char *search_path);
static int refresh_lrm(IPC_Channel *crmd_channel, const char *host_uname);
static int delete_lrm_rsc(IPC_Channel *crmd_channel, const char *host_uname, const char *rsc_id);
static pe_working_set_t* get_data_set(void);
static void free_data_set(pe_working_set_t* data_set);
static void on_cib_connection_destroy(gpointer user_data);
static char* crm_failed_msg(crm_data_t* output, int rc);
static const char* uname2id(const char* node);
static resource_t* get_parent(resource_t* child);
static const char* get_rsc_tag(resource_t* rsc);
static int cl_msg_swap_offset(crm_data_t* msg, int offset1, int offset2);
int regex_match(const char *regex, const char *str);

pe_working_set_t* cib_cached = NULL;
int cib_cache_enable = FALSE;

#define GET_RESOURCE()	rsc = pe_find_resource(data_set->resources, argv[1]);	\
	if (rsc == NULL) {						\
		free_data_set(data_set);				\
		return strdup(MSG_FAIL"\nno such resource");		\
	}

/* internal functions */
GList* find_xml_node_list(crm_data_t *root, const char *child_name)
{
	GList* list = NULL;
	xml_child_iter_filter(root, child, child_name,
			      list = g_list_append(list, child));
	return list;
}

int
delete_object(const char* type, const char* entry, const char* id, crm_data_t** output) 
{
	int rc;
	crm_data_t* cib_object = NULL;
	char xml[MAX_STRLEN];

	snprintf(xml, MAX_STRLEN, "<%s id=\"%s\">", entry, id);

	cib_object = string2xml(xml);
	if(cib_object == NULL) {
		return -1;
	}
	
	mgmt_log(LOG_INFO, "(delete)xml:%s",xml);

	rc = cib_conn->cmds->delete(
			cib_conn, type, cib_object, cib_sync_call);
	free_xml(cib_object);
	return rc;
}

pe_working_set_t*
get_data_set(void) 
{
	pe_working_set_t* data_set;
	
	if (cib_cache_enable) {
		if (cib_cached != NULL) {
			return cib_cached;
		}
	}
	
	data_set = (pe_working_set_t*)malloc(sizeof(pe_working_set_t));
	if (data_set == NULL) {
		mgmt_log(LOG_ERR, "%s:Can't alloc memory for data set.",__FUNCTION__);
		return NULL;
	}
	set_working_set_defaults(data_set);
	data_set->input = get_cib_copy(cib_conn);
	data_set->now = new_ha_date(TRUE);

	cluster_status(data_set);
	
	if (cib_cache_enable) {
		cib_cached = data_set;
	}
	return data_set;
}

void 
free_data_set(pe_working_set_t* data_set)
{
	/* we only release the cib when cib is not cached.
	   the cached cib will be released in on_cib_diff() */
	if (!cib_cache_enable) {
		cleanup_calculations(data_set);
		free(data_set);
	}
}	
char* 
crm_failed_msg(crm_data_t* output, int rc) 
{
	const char* reason = NULL;
	crm_data_t* failed_tag;
	char* ret;
	
	/* beekhof:
		you can pretend that the return code is success, 
		its an internal CIB thing*/
	if (rc == cib_diff_resync) {
		if (output != NULL) {
			free_xml(output);
		}
		return strdup(MSG_OK);
	}
	
	ret = strdup(MSG_FAIL);
	ret = mgmt_msg_append(ret, cib_error2string((enum cib_errors)rc));
	
	if (output == NULL) {
		return ret;
	}
	
	failed_tag = find_entity(output, XML_FAIL_TAG_CIB, NULL);
	if (failed_tag != NULL) {
		reason = crm_element_value(failed_tag, XML_FAILCIB_ATTR_REASON);
		if (reason != NULL) {
			ret = mgmt_msg_append(ret, reason);
		}
	}
	free_xml(output);
	
	return ret;
}
const char*
uname2id(const char* uname)
{
	node_t* node;
	GList* cur;
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	cur = data_set->nodes;
	while (cur != NULL) {
		node = (node_t*) cur->data;
		if (strncmp(uname,node->details->uname,MAX_STRLEN) == 0) {
			free_data_set(data_set);
			return node->details->id;
		}
		cur = g_list_next(cur);
	}
	free_data_set(data_set);
	return NULL;
}
static resource_t* 
get_parent(resource_t* child)
{
	GList* cur;
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	cur = data_set->resources;
	while (cur != NULL) {
		resource_t* rsc = (resource_t*)cur->data;
		if(is_not_set(rsc->flags, pe_rsc_orphan) || rsc->role != RSC_ROLE_STOPPED) {
			GList* child_list = rsc->fns->children(rsc);
			if (g_list_find(child_list, child) != NULL) {
				free_data_set(data_set);
				return rsc;
			}
		}
		cur = g_list_next(cur);
	}
	free_data_set(data_set);
	return NULL;
}
static const char* 
get_rsc_tag(resource_t* rsc)
{
	switch (rsc->variant) {
		case pe_native:
			return "primitive";
		case pe_group:
			return "group";
		case pe_clone:
			return "clone";
		case pe_master:
			return "master_slave";
		case pe_unknown:
		default:
			return "unknown";
	}
	
}

static void
get_meta_attributes_id(const char* rsc_id, char* id)
{
	resource_t* rsc;
	const char* cur_id;
	pe_working_set_t* data_set;
	crm_data_t* attrs;
	
	data_set = get_data_set();
	rsc = pe_find_resource(data_set->resources, rsc_id);	
	if (rsc == NULL) {
		snprintf(id, MAX_STRLEN, "%s_meta_attrs", rsc_id);
		free_data_set(data_set);
		return;
	}
	attrs = find_entity(rsc->xml, "meta_attributes", NULL);
	if (attrs == NULL) {
		snprintf(id, MAX_STRLEN, "%s_meta_attrs", rsc_id);
		free_data_set(data_set);
		return;
	}
	cur_id = crm_element_value(attrs, "id");
	if (cur_id == NULL) {
		snprintf(id, MAX_STRLEN, "%s_meta_attrs", rsc_id);
		free_data_set(data_set);
		return;
	}
	STRNCPY(id, cur_id, MAX_STRLEN);
	free_data_set(data_set);
	return;					

}

/* mgmtd functions */
int
init_crm(int cache_cib)
{
	int ret = cib_ok;
	int i, max_try = 5;
	
	mgmt_log(LOG_INFO,"init_crm");
	crm_log_level = LOG_ERR;
	cib_conn = cib_new();
	in_shutdown = FALSE;
	
	cib_cache_enable = cache_cib?TRUE:FALSE;
	cib_cached = NULL;
	
	for (i = 0; i < max_try ; i++) {
		ret = cib_conn->cmds->signon(cib_conn, client_name, cib_command);
		if (ret == cib_ok) {
			break;
		}
		mgmt_log(LOG_INFO,"login to cib: %d, ret:%d",i,ret);
		sleep(1);
	}
	if (ret != cib_ok) {
		mgmt_log(LOG_INFO,"login to cib failed");
		cib_conn = NULL;
		return -1;
	}

	ret = cib_conn->cmds->add_notify_callback(cib_conn, T_CIB_DIFF_NOTIFY
						  , on_cib_diff);
	ret = cib_conn->cmds->set_connection_dnotify(cib_conn
			, on_cib_connection_destroy);

	reg_msg(MSG_CLUSTER_TYPE, on_get_cluster_type);
	reg_msg(MSG_CIB_VERSION, on_get_cib_version);
	reg_msg(MSG_CRM_SCHEMA, on_get_crm_schema);
	reg_msg(MSG_CRM_DTD, on_get_crm_dtd);
	reg_msg(MSG_CRM_METADATA, on_get_crm_metadata);
	reg_msg(MSG_CRM_ATTRIBUTE, on_crm_attribute);
	
	reg_msg(MSG_DC, on_get_dc);
	reg_msg(MSG_ACTIVENODES, on_get_activenodes);
	reg_msg(MSG_CRMNODES, on_get_crmnodes);
	reg_msg(MSG_NODE_CONFIG, on_get_node_config);
	reg_msg(MSG_RUNNING_RSC, on_get_running_rsc);

	reg_msg(MSG_MIGRATE, on_migrate_rsc);
	reg_msg(MSG_STANDBY, on_set_node_standby);
	
	reg_msg(MSG_DEL_RSC, on_del_rsc);
	reg_msg(MSG_CLEANUP_RSC, on_cleanup_rsc);
	reg_msg(MSG_MOVE_RSC, on_move_rsc);
	
	reg_msg(MSG_ALL_RSC, on_get_all_rsc);
	reg_msg(MSG_SUB_RSC, on_get_sub_rsc);
	reg_msg(MSG_RSC_RUNNING_ON, on_get_rsc_running_on);
	reg_msg(MSG_RSC_STATUS, on_get_rsc_status);
	reg_msg(MSG_RSC_TYPE, on_get_rsc_type);
	reg_msg(MSG_OP_STATUS2STR, on_op_status2str);

	reg_msg(MSG_CRM_RSC_CMD, on_crm_rsc_cmd);
	reg_msg(MSG_SET_RSC_ATTR, on_set_rsc_attr);
	reg_msg(MSG_GET_RSC_ATTR, on_get_rsc_attr);
	reg_msg(MSG_DEL_RSC_ATTR, on_del_rsc_attr);
		
	reg_msg(MSG_UPDATE_MASTER, on_update_master);
	reg_msg(MSG_GET_MASTER, on_get_master);

	reg_msg(MSG_GET_CONSTRAINTS, on_get_constraints);
	reg_msg(MSG_GET_CONSTRAINT, on_get_constraint);
	reg_msg(MSG_DEL_CONSTRAINT, on_delete_constraint);
	reg_msg(MSG_UP_CONSTRAINT, on_update_constraint);

	reg_msg(MSG_GEN_CLUSTER_REPORT, on_gen_cluster_report);
	reg_msg(MSG_GET_PE_INPUTS, on_get_pe_inputs);
	reg_msg(MSG_GEN_PE_GRAPH, on_gen_pe_graph);
	reg_msg(MSG_GEN_PE_INFO, on_gen_pe_info);
	
	reg_msg(MSG_CIB_CREATE, on_cib_create);
	reg_msg(MSG_CIB_QUERY, on_cib_query);
	reg_msg(MSG_CIB_UPDATE, on_cib_update);
	reg_msg(MSG_CIB_REPLACE, on_cib_replace);
	reg_msg(MSG_CIB_DELETE, on_cib_delete);
	return 0;
}	
void
final_crm(void)
{
	if(cib_conn != NULL) {
		in_shutdown = TRUE;
		cib_conn->cmds->signoff(cib_conn);
		cib_conn = NULL;
	}
}

/* event handler */
void
on_cib_diff(const char *event, crm_data_t *msg)
{
	if (debug_level) {
		mgmt_debug(LOG_DEBUG,"update cib finished");
	}
	if (cib_cache_enable) {
		if (cib_cached != NULL) {
			cleanup_calculations(cib_cached);
			free(cib_cached);
			cib_cached = NULL;
		}
	}
	
	fire_event(EVT_CIB_CHANGED);
}
void
on_cib_connection_destroy(gpointer user_data)
{
	if (!in_shutdown) {
		fire_event(EVT_DISCONNECTED);
		cib_conn = NULL;
	}
	return;
}

/* cluster  functions */
char* 
on_get_cluster_type(char* argv[], int argc)
{
	char* ret = NULL;

	if (is_openais_cluster()) {
		ret = strdup(MSG_OK);
		ret = mgmt_msg_append(ret, "openais");
	}
	else if (is_heartbeat_cluster()) {
		ret = strdup(MSG_OK);
		ret = mgmt_msg_append(ret, "heartbeat");
	}
	else {
		ret = strdup(MSG_FAIL);
	}
	return ret;
}

char* 
on_get_cib_version(char* argv[], int argc)
{
	const char* version = NULL;
	pe_working_set_t* data_set;
	char* ret;
	
	data_set = get_data_set();
	version = crm_element_value(data_set->input, "num_updates");
	if (version != NULL) {
		ret = strdup(MSG_OK);
		ret = mgmt_msg_append(ret, version);
	}
	else {
		ret = strdup(MSG_FAIL);
	}	
	free_data_set(data_set);
	return ret;
}

static char*
on_get_crm_schema(char* argv[], int argc)
{
	const char *schema_file = NULL;
	const char *validate_type = NULL;
	const char *file_name = NULL;
	char buf[MAX_STRLEN];	
	char* ret = NULL;
	FILE *fstream = NULL;

	ARGC_CHECK(3);
	validate_type = argv[1];
	file_name = argv[2];

	if (STRNCMP_CONST(validate_type, "") == 0){
		schema_file = HA_NOARCHDATAHBDIR"/crm.dtd";
	}
	else if (STRNCMP_CONST(validate_type, "pacemaker-0.6") == 0){
		schema_file = DTD_DIRECTORY"/crm.dtd";
	}
	else if (STRNCMP_CONST(validate_type, "transitional-0.6") == 0){
		schema_file = DTD_DIRECTORY"/crm-transitional.dtd";
	}
	else{
		if (STRNCMP_CONST(file_name, "") == 0){
			snprintf(buf, sizeof(buf), DTD_DIRECTORY"/%s.rng", validate_type);
			schema_file = buf;
		}
		else{
			snprintf(buf, sizeof(buf), DTD_DIRECTORY"/%s", file_name);
			schema_file = buf;
		}
	}
	if ((fstream = fopen(schema_file, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on fopen %s: %s",
			 schema_file, strerror(errno));
		return strdup(MSG_FAIL);
	}

	ret = strdup(MSG_OK);
	while (!feof(fstream)){
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fstream) != NULL){
			ret = mgmt_msg_append(ret, buf);
			ret[strlen(ret)-1] = '\0';
		}
		else{
			sleep(1);
		}
	}

	if (fclose(fstream) == -1)
		mgmt_log(LOG_WARNING, "failed to fclose stream");

	return ret;
}

static char*
on_get_crm_dtd(char* argv[], int argc)
{
	const char *dtd_file = HA_NOARCHDATAHBDIR"/crm.dtd";
	char buf[MAX_STRLEN];	
	char* ret = NULL;
	FILE *fstream = NULL;

	ARGC_CHECK(1);

	if ((fstream = fopen(dtd_file, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on fopen %s: %s",
			 dtd_file, strerror(errno));
		return strdup(MSG_FAIL);
	}

	ret = strdup(MSG_OK);
	while (!feof(fstream)){
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fstream) != NULL){
			ret = mgmt_msg_append(ret, buf);
			ret[strlen(ret)-1] = '\0';
		}
		else{
			sleep(1);
		}
	}

	if (fclose(fstream) == -1)
		mgmt_log(LOG_WARNING, "failed to fclose stream");

	return ret;
}

static char*
on_crm_attribute(char* argv[], int argc)
{
	char cmd[MAX_STRLEN];
	char buf[MAX_STRLEN];
	char* ret = NULL;
	const char* nv_regex = "^[A-Za-z0-9_-]+$";
	FILE *fstream = NULL;


	snprintf(cmd, sizeof(cmd), "crm_attribute -t %s", argv[1]);

	if (regex_match(nv_regex, argv[3])){
		if (STRNCMP_CONST(argv[2], "get") == 0){
			strncat(cmd, " -Q -G", sizeof(cmd)-strlen(cmd)-1);
		}
		else if (STRNCMP_CONST(argv[2], "del") == 0){
			strncat(cmd, " -D", sizeof(cmd)-strlen(cmd)-1);
		}
		strncat(cmd, " -n \"", sizeof(cmd)-strlen(cmd)-1);
		strncat(cmd, argv[3], sizeof(cmd)-strlen(cmd)-1);
		strncat(cmd, "\"", sizeof(cmd)-strlen(cmd)-1);
	}
	else {
		mgmt_log(LOG_ERR, "invalid attribute name specified: \"%s\"", argv[3]);
		return strdup(MSG_FAIL"\nInvalid attribute name");
	}

	if (STRNCMP_CONST(argv[2], "set") == 0){
		if (regex_match(nv_regex, argv[4])){
			strncat(cmd, " -v \"", sizeof(cmd)-strlen(cmd)-1);
			strncat(cmd, argv[4], sizeof(cmd)-strlen(cmd)-1);
			strncat(cmd, "\"", sizeof(cmd)-strlen(cmd)-1);
		}
		else {
			mgmt_log(LOG_ERR, "invalid attribute value specified: \"%s\"", argv[4]);
			return strdup(MSG_FAIL"\nInvalid attribute value");
		}
	}

	strncat(cmd, " 2>&1", sizeof(cmd)-strlen(cmd)-1);

	if ((fstream = popen(cmd, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on popen %s: %s",
			 cmd, strerror(errno));
		return strdup(MSG_FAIL"\nInvoke crm_attribute failed");
	}

	if (STRNCMP_CONST(argv[2], "get") == 0){
		ret = strdup(MSG_OK);
	}
	else{
		ret = strdup(MSG_FAIL);
	}

	while (!feof(fstream)){
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fstream) != NULL){
			ret = mgmt_msg_append(ret, buf);
			ret[strlen(ret)-1] = '\0';
		}
		else{
			sleep(1);
		}
	}

	if (pclose(fstream) == -1)
		mgmt_log(LOG_WARNING, "failed to close pipe");

	return ret;

}

static char*
on_get_crm_metadata(char* argv[], int argc)
{
	char cmd[MAX_STRLEN];
	char buf[MAX_STRLEN];	
	char* ret = NULL;
	FILE *fstream = NULL;

	ARGC_CHECK(2);

	if (STRNCMP_CONST(argv[1], "pengine") != 0 &&
			STRNCMP_CONST(argv[1], "crmd") != 0) {
		return strdup(MSG_FAIL);
	}

	snprintf(cmd, sizeof(cmd), BIN_DIR"/%s metadata", argv[1]);
	if ((fstream = popen(cmd, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on popen %s: %s",
			 cmd, strerror(errno));
		return strdup(MSG_FAIL);
	}

	ret = strdup(MSG_OK);
	while (!feof(fstream)){
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fstream) != NULL){
			ret = mgmt_msg_append(ret, buf);
			ret[strlen(ret)-1] = '\0';
		}
		else{
			sleep(1);
		}
	}

	if (pclose(fstream) == -1)
		mgmt_log(LOG_WARNING, "failed to close pipe");

	return ret;
}

/* node functions */
char*
on_get_activenodes(char* argv[], int argc)
{
	node_t* node;
	GList* cur;
	char* ret;
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	cur = data_set->nodes;
	ret = strdup(MSG_OK);
	while (cur != NULL) {
		node = (node_t*) cur->data;
		if (node->details->online) {
			ret = mgmt_msg_append(ret, node->details->uname);
		}
		cur = g_list_next(cur);
	}
	free_data_set(data_set);
	return ret;
}

char*
on_get_crmnodes(char* argv[], int argc)
{
	node_t* node;
	GList* cur;
	char* ret;
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	cur = data_set->nodes;
	ret = strdup(MSG_OK);
	while (cur != NULL) {
		node = (node_t*) cur->data;
		ret = mgmt_msg_append(ret, node->details->uname);
		cur = g_list_next(cur);
	}
	free_data_set(data_set);
	return ret;
}

char* 
on_get_dc(char* argv[], int argc)
{
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	if (data_set->dc_node != NULL) {
		char* ret = strdup(MSG_OK);
		ret = mgmt_msg_append(ret, data_set->dc_node->details->uname);
		free_data_set(data_set);
		return ret;
	}
	free_data_set(data_set);
	return strdup(MSG_FAIL);
}


char*
on_get_node_config(char* argv[], int argc)
{
	node_t* node;
	GList* cur;
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	cur = data_set->nodes;
	ARGC_CHECK(2);
	while (cur != NULL) {
		node = (node_t*) cur->data;
		if (strncmp(argv[1],node->details->uname,MAX_STRLEN) == 0) {
			char* ret = strdup(MSG_OK);
			ret = mgmt_msg_append(ret, node->details->uname);
			ret = mgmt_msg_append(ret, node->details->online?"True":"False");
			ret = mgmt_msg_append(ret, node->details->standby?"True":"False");
			ret = mgmt_msg_append(ret, node->details->unclean?"True":"False");
			ret = mgmt_msg_append(ret, node->details->shutdown?"True":"False");
			ret = mgmt_msg_append(ret, node->details->expected_up?"True":"False");
			ret = mgmt_msg_append(ret, node->details->is_dc?"True":"False");
			ret = mgmt_msg_append(ret, node->details->type==node_ping?"ping":"member");
			free_data_set(data_set);
			return ret;
		}
		cur = g_list_next(cur);
	}
	free_data_set(data_set);
	return strdup(MSG_FAIL);
}

char*
on_get_running_rsc(char* argv[], int argc)
{
	node_t* node;
	GList* cur;
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	cur = data_set->nodes;
	ARGC_CHECK(2);
	while (cur != NULL) {
		node = (node_t*) cur->data;
		if (node->details->online) {
			if (strncmp(argv[1],node->details->uname,MAX_STRLEN) == 0) {
				GList* cur_rsc;
				char* ret = strdup(MSG_OK);
				cur_rsc = node->details->running_rsc;
				while(cur_rsc != NULL) {
					resource_t* rsc = (resource_t*)cur_rsc->data;
					ret = mgmt_msg_append(ret, rsc->id);
					cur_rsc = g_list_next(cur_rsc);
				}
				free_data_set(data_set);
				return ret;
			}
		}
		cur = g_list_next(cur);
	}
	free_data_set(data_set);
	return strdup(MSG_FAIL);
}

char*
on_migrate_rsc(char* argv[], int argc)
{
	const char* id = NULL;
	char cmd[MAX_STRLEN];
	char buf[MAX_STRLEN];
	pe_working_set_t* data_set;
	resource_t* rsc;
	char* ret = NULL;
	const char* duration_regex = "^[A-Za-z0-9:-]+$";
	FILE *fstream = NULL;

	ARGC_CHECK(5)
	data_set = get_data_set();
	GET_RESOURCE()
	free_data_set(data_set);

	snprintf(cmd, sizeof(cmd), "crm_resource -M -r %s", argv[1]);

	if (STRNCMP_CONST(argv[2], "") != 0){
		id = uname2id(argv[2]);
		if (id == NULL) {
			return strdup(MSG_FAIL"\nNo such node");
		}
		else{
			strncat(cmd, " -H ", sizeof(cmd)-strlen(cmd)-1);
			strncat(cmd, argv[2], sizeof(cmd)-strlen(cmd)-1);
		}
	}

	if (STRNCMP_CONST(argv[3], "true") == 0){
		strncat(cmd, " -f", sizeof(cmd)-strlen(cmd)-1);
	}

	if (STRNCMP_CONST(argv[4], "") != 0){
		if (regex_match(duration_regex, argv[4])) {
			strncat(cmd, " -u \"", sizeof(cmd)-strlen(cmd)-1);
			strncat(cmd, argv[4], sizeof(cmd)-strlen(cmd)-1);
			strncat(cmd, "\"", sizeof(cmd)-strlen(cmd)-1);
		}
		else {
			mgmt_log(LOG_ERR, "invalid duration specified: \"%s\"", argv[1]);
			return strdup(MSG_FAIL"\nInvalid duration.\nPlease refer to "
					"http://en.wikipedia.org/wiki/ISO_8601#Duration for examples of valid durations");
		}
	}

	strncat(cmd, " 2>&1", sizeof(cmd)-strlen(cmd)-1);

	if ((fstream = popen(cmd, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on popen %s: %s",
			 cmd, strerror(errno));
		return strdup(MSG_FAIL"\nMigrate failed");
	}

	ret = strdup(MSG_FAIL);
	while (!feof(fstream)){
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fstream) != NULL){
			ret = mgmt_msg_append(ret, buf);
			ret[strlen(ret)-1] = '\0';
		}
		else{
			sleep(1);
		}
	}

	if (pclose(fstream) == -1)
		mgmt_log(LOG_WARNING, "failed to close pipe");

	return ret;

}

char*
on_set_node_standby(char* argv[], int argc)
{
	int rc;
	const char* id = NULL;
	const char* attr_value = NULL;

	ARGC_CHECK(3);
	id = uname2id(argv[1]);
	if (id == NULL) {
		return strdup(MSG_FAIL"\nNo such node");
	}

	if (STRNCMP_CONST(argv[2], "on") == 0 || STRNCMP_CONST(argv[2], "true") == 0){
		attr_value = "true";
	}
	else if (STRNCMP_CONST(argv[2], "off") == 0 || STRNCMP_CONST(argv[2], "false") == 0){
		attr_value = "false";
	}
	else{
		return strdup(MSG_FAIL"\nInvalid attribute value");
	}

	rc = set_standby(cib_conn, id, NULL, attr_value);
	if (rc < 0) {
		return crm_failed_msg(NULL, rc);
	}
	return strdup(MSG_OK);
}

/*
char*
on_set_node_standby(char* argv[], int argc)
{
	int rc;
	const char* id = NULL;
	crm_data_t* fragment = NULL;
	crm_data_t* cib_object = NULL;
	crm_data_t* output = NULL;
	char xml[MAX_STRLEN];

	ARGC_CHECK(3);
	id = uname2id(argv[1]);
	if (id == NULL) {
		return strdup(MSG_FAIL"\nno such node");
	}
	
	snprintf(xml, MAX_STRLEN, 
		"<node id=\"%s\"><instance_attributes id=\"nodes-%s\">"
		"<attributes><nvpair id=\"standby-%s\" name=\"standby\" value=\"%s\"/>"
           	"</attributes></instance_attributes></node>", 
           	id, id, id, argv[2]);

	cib_object = string2xml(xml);
	if(cib_object == NULL) {
		return strdup(MSG_FAIL);
	}

	fragment = create_cib_fragment(cib_object, "nodes");

	mgmt_log(LOG_INFO, "(update)xml:%s",xml);

	rc = cib_conn->cmds->update(
			cib_conn, "nodes", fragment, cib_sync_call);

	free_xml(fragment);
	free_xml(cib_object);
	if (rc < 0) {
		return crm_failed_msg(output, rc);
	}
	free_xml(output);
	return strdup(MSG_OK);

}
*/
/* resource functions */
/* add/delete resource */
char*
on_del_rsc(char* argv[], int argc)
{
	int rc;
	resource_t* rsc;
	crm_data_t* cib_object = NULL;
	crm_data_t* output = NULL;
	char xml[MAX_STRLEN];
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	GET_RESOURCE()

	snprintf(xml, MAX_STRLEN, "<%s id=\"%s\"/>",get_rsc_tag(rsc), rsc->id);
	free_data_set(data_set);

	cib_object = string2xml(xml);
	if(cib_object == NULL) {
		return strdup(MSG_FAIL);
	}

	mgmt_log(LOG_INFO, "(delete resources)xml:%s",xml);
	rc = cib_conn->cmds->delete(
			cib_conn, "resources", cib_object, cib_sync_call);
	
	free_xml(cib_object);
	if (rc < 0) {
		return crm_failed_msg(output, rc);
	}
	free_xml(output);
	return strdup(MSG_OK);
}
static int
delete_lrm_rsc(IPC_Channel *crmd_channel, const char *host_uname, const char *rsc_id)
{
	crm_data_t *cmd = NULL;
	crm_data_t *msg_data = NULL;
	crm_data_t *rsc = NULL;
	crm_data_t *params = NULL;
	char our_pid[11];
	char *key = NULL; 
	
	snprintf(our_pid, 10, "%d", getpid());
	our_pid[10] = '\0';
	key = crm_concat(client_name, our_pid, '-');
	
	msg_data = create_xml_node(NULL, XML_GRAPH_TAG_RSC_OP);
	crm_xml_add(msg_data, XML_ATTR_TRANSITION_KEY, key);
	
	rsc = create_xml_node(msg_data, XML_CIB_TAG_RESOURCE);
	crm_xml_add(rsc, XML_ATTR_ID, rsc_id);

	params = create_xml_node(msg_data, XML_TAG_ATTRS);
	crm_xml_add(params, XML_ATTR_CRM_VERSION, CRM_FEATURE_SET);
	
	cmd = create_request(CRM_OP_LRM_DELETE, msg_data, host_uname,
			     CRM_SYSTEM_CRMD, client_name, our_pid);

	free_xml(msg_data);
	crm_free(key);

	if(send_ipc_message(crmd_channel, cmd)) {
		free_xml(cmd);
		return 0;
	}
	free_xml(cmd);
	return -1;
}

static int
refresh_lrm(IPC_Channel *crmd_channel, const char *host_uname)
{
	crm_data_t *cmd = NULL;
	char our_pid[11];
	
	snprintf(our_pid, 10, "%d", getpid());
	our_pid[10] = '\0';
	
	cmd = create_request(CRM_OP_LRM_REFRESH, NULL, host_uname,
			     CRM_SYSTEM_CRMD, client_name, our_pid);
	
	if(send_ipc_message(crmd_channel, cmd)) {
		free_xml(cmd);
		return 0;
	}
	free_xml(cmd);
	return -1;
}

char*
on_cleanup_rsc(char* argv[], int argc)
{
	IPC_Channel *crmd_channel = NULL;
	char our_pid[11];
	char *now_s = NULL;
	time_t now = time(NULL);
	char *dest_node = NULL;
	int rc;
	
	ARGC_CHECK(3);
	snprintf(our_pid, 10, "%d", getpid());
	our_pid[10] = '\0';
	
	init_client_ipc_comms(CRM_SYSTEM_CRMD, NULL,
				    NULL, &crmd_channel);

	send_hello_message(crmd_channel, our_pid, client_name, "0", "1");
	delete_lrm_rsc(crmd_channel, argv[1], argv[2]);
	refresh_lrm(crmd_channel, NULL); 
	
	rc = query_node_uuid(cib_conn, argv[1], &dest_node);
	if (rc != cib_ok) {
		mgmt_log(LOG_WARNING, "Could not map uname=%s to a UUID: %s\n",
				argv[1], cib_error2string(rc));
	} else {
		delete_attr(cib_conn, cib_sync_call, XML_CIB_TAG_STATUS, dest_node, NULL,
				NULL, crm_concat("fail-count", argv[2], '-'), NULL, FALSE);
		mgmt_log(LOG_INFO, "Delete fail-count for %s from %s", argv[2], argv[1]);
	}
	/* force the TE to start a transition */
	sleep(2); /* wait for the refresh */
	now_s = crm_itoa(now);
	update_attr(cib_conn, cib_sync_call,
		    XML_CIB_TAG_CRMCONFIG, NULL, NULL, NULL, "last-lrm-refresh", now_s, FALSE);
	crm_free(now_s);

	
	return strdup(MSG_OK);
}

int
cl_msg_swap_offset(crm_data_t* msg, int offset1, int offset2)
{
#if 0
    /* this code doesn't work when crm_data_t is an alias for xmlNode */
	char* name;
	int nlen;
	void* value;
	int vlen;
	int type;
	
	name = msg->names[offset1];
	nlen = msg->nlens[offset1];
	value = msg->values[offset1];
	vlen = msg->vlens[offset1];
	type = msg->types[offset1];
		
	msg->names[offset1] = msg->names[offset2];
	msg->nlens[offset1] = msg->nlens[offset2];
	msg->values[offset1] = msg->values[offset2];
	msg->vlens[offset1] = msg->vlens[offset2];
	msg->types[offset1] = msg->types[offset2];
		
	msg->names[offset2] = name;
	msg->nlens[offset2] = nlen;
	msg->values[offset2] = value;
	msg->vlens[offset2] = vlen;
	msg->types[offset2] = type;
	
	return HA_OK;
#else
	return HA_FAIL;;
#endif
}

char*
on_move_rsc(char* argv[], int argc)
{
	int i = 0, rc, pos = -1;
	int first_child = -1;
	int last_child = -1;
	const char* child_id;
	resource_t* rsc;
	resource_t* parent;
	pe_working_set_t* data_set;
	crm_data_t* output = NULL;
	
	data_set = get_data_set();
	GET_RESOURCE()
	parent = get_parent(rsc);
	if (parent == NULL || parent->variant != pe_group) {
		free_data_set(data_set);
		return strdup(MSG_FAIL);
	}
	xml_child_iter(
		parent->xml, child,
		const char *name = crm_element_name(child);
		if (STRNCMP_CONST(name, "primitive") == 0) {
		    if (first_child == -1) {
			first_child = i;
		    }
		    last_child = i;
		    child_id = crm_element_value(child, "id");
		    if (strcmp(child_id, argv[1]) == 0) {
			mgmt_log(LOG_INFO,"find %s !", child_id);
			pos = i;
		    }
		}
		i++;
	    );
	
	if (STRNCMP_CONST(argv[2],"up")==0) {
		if (pos-1<first_child) {
			free_data_set(data_set);
			return strdup(MSG_FAIL);
		}
		cl_msg_swap_offset(parent->xml, pos-1, pos);
	}
	else if (STRNCMP_CONST(argv[2],"down")==0) {
		if (pos+1>last_child) {
			free_data_set(data_set);
			return strdup(MSG_FAIL);
		}
		cl_msg_swap_offset(parent->xml, pos, pos+1);
	}
	else {
		free_data_set(data_set);
		return strdup(MSG_FAIL);
	}
	mgmt_log(LOG_INFO, "on_move_rsc:%s", dump_xml_formatted(parent->xml)); /* Memory leak! */
	
	rc = cib_conn->cmds->variant_op(
			cib_conn, CIB_OP_REPLACE, NULL,"resources",
			parent->xml, &output, cib_sync_call);

	free_data_set(data_set);
	
	if (rc < 0) {
		return crm_failed_msg(output, rc);
	}
	free_xml(output);
	
	return strdup(MSG_OK);
}

/* get all resources*/
char*
on_get_all_rsc(char* argv[], int argc)
{
	GList* cur;
	char* ret;
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	ret = strdup(MSG_OK);
	cur = data_set->resources;
	while (cur != NULL) {
		resource_t* rsc = (resource_t*)cur->data;
		if(is_not_set(rsc->flags, pe_rsc_orphan) || rsc->role != RSC_ROLE_STOPPED) {
			ret = mgmt_msg_append(ret, rsc->id);
		}
		cur = g_list_next(cur);
	}
	free_data_set(data_set);
	return ret;
}
/* basic information of resource */
char*
on_get_rsc_running_on(char* argv[], int argc)
{
	resource_t* rsc;
	char* ret;
	GList* cur;
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	GET_RESOURCE()

	ret = strdup(MSG_OK);
	cur = rsc->running_on;
	while (cur != NULL) {
		node_t* node = (node_t*)cur->data;
		ret = mgmt_msg_append(ret, node->details->uname);
		cur = g_list_next(cur);
	}
	free_data_set(data_set);
	return ret;
}
char*
on_get_rsc_status(char* argv[], int argc)
{
	resource_t* rsc;
	char* ret;
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	GET_RESOURCE()
	ret = strdup(MSG_OK);
	switch (rsc->variant) {
		case pe_unknown:
			ret = mgmt_msg_append(ret, "unknown");
			break;
		case pe_native:
			if(is_set(rsc->flags, pe_rsc_start_pending)) {
				ret = mgmt_msg_append(ret, "starting");
				break;
			}
			if(is_not_set(rsc->flags, pe_rsc_managed)) {
				ret = mgmt_msg_append(ret, "unmanaged");
				break;
			}
			if(is_set(rsc->flags, pe_rsc_failed)) {
				ret = mgmt_msg_append(ret, "failed");
				break;
			}
			if( g_list_length(rsc->running_on) == 0) {
				ret = mgmt_msg_append(ret, "not running");
				break;
			}
			if( g_list_length(rsc->running_on) > 1) {
				ret = mgmt_msg_append(ret, "multi-running");
				break;
			}
			if( rsc->role==RSC_ROLE_SLAVE ) {
				ret = mgmt_msg_append(ret, "running (Slave)");		
			}
			else if( rsc->role==RSC_ROLE_MASTER) {
				ret = mgmt_msg_append(ret, "running (Master)");		
			}
			else {
				ret = mgmt_msg_append(ret, "running");		
			}
			break;
		case pe_group:
			ret = mgmt_msg_append(ret, "group");
			break;
		case pe_clone:
			ret = mgmt_msg_append(ret, "clone");
			break;
		case pe_master:
			ret = mgmt_msg_append(ret, "master");
			break;
	}
	ret = mgmt_msg_append(ret, crm_itoa(rsc->migration_threshold));
	free_data_set(data_set);
	return ret;
}

char*
on_get_rsc_type(char* argv[], int argc)
{
	resource_t* rsc;
	char* ret;
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	GET_RESOURCE()

	ret = strdup(MSG_OK);

	switch (rsc->variant) {
		case pe_unknown:
			ret = mgmt_msg_append(ret, "unknown");
			break;
		case pe_native:
			ret = mgmt_msg_append(ret, "native");
			break;
		case pe_group:
			ret = mgmt_msg_append(ret, "group");
			break;
		case pe_clone:
			ret = mgmt_msg_append(ret, "clone");
			break;
		case pe_master:
			ret = mgmt_msg_append(ret, "master");
			break;
	}
	free_data_set(data_set);
	return ret;
}

char*
on_op_status2str(char* argv[], int argc)
{
	int op_status;
	char* ret = strdup(MSG_OK);

	op_status = atoi(argv[1]);
	ret = mgmt_msg_append(ret, op_status2text(op_status));
	return ret;
}

char*
on_get_sub_rsc(char* argv[], int argc)
{
	resource_t* rsc;
	char* ret;
	GList* cur = NULL;
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	GET_RESOURCE()
		
	cur = rsc->fns->children(rsc);
	
	ret = strdup(MSG_OK);
	while (cur != NULL) {
		resource_t* rsc = (resource_t*)cur->data;
		ret = mgmt_msg_append(ret, rsc->id);
		cur = g_list_next(cur);
	}
	free_data_set(data_set);
	return ret;
}

char*
on_crm_rsc_cmd(char* argv[], int argc)
{
	char cmd[MAX_STRLEN];
	char* ret = NULL;
	FILE* fstream = NULL;
	char buf[MAX_STRLEN];

	ARGC_CHECK(3)

	if (STRNCMP_CONST(argv[1], "refresh") == 0){
		strncpy(cmd, "crm_resource -R", sizeof(cmd)-1) ;
	}
	else if (STRNCMP_CONST(argv[1], "reprobe") == 0){
		strncpy(cmd, "crm_resource -P", sizeof(cmd)-1) ;
	}
	else{
		return strdup(MSG_FAIL"\nNo such command");
	}

	if (strlen(argv[2]) > 0){
		if (uname2id(argv[2]) == NULL){
			return strdup(MSG_FAIL"\nNo such node");
		}
		else{
			strncat(cmd, " -H ", sizeof(cmd)-strlen(cmd)-1);
			strncat(cmd, argv[2], sizeof(cmd)-strlen(cmd)-1);
		}
	}

	strncat(cmd, " 2>&1", sizeof(cmd)-strlen(cmd)-1);

	if ((fstream = popen(cmd, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on popen %s: %s", cmd, strerror(errno));
		return strdup(MSG_FAIL"\nDo crm_resource command failed");
	}

	ret = strdup(MSG_FAIL);
	while (!feof(fstream)){
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fstream) != NULL){
			ret = mgmt_msg_append(ret, buf);
			ret[strlen(ret)-1] = '\0';
		}
		else{
			sleep(1);
		}
	}

	if (pclose(fstream) == -1){
		mgmt_log(LOG_WARNING, "failed to close pipe");
	}

	return ret;
}

char*
on_set_rsc_attr(char* argv[], int argc)
{
	char cmd[MAX_STRLEN];
	char buf[MAX_STRLEN];
	pe_working_set_t* data_set;
	resource_t* rsc;
	char* ret = NULL;
	const char* nv_regex = "^[A-Za-z0-9_-]+$";
	FILE *fstream = NULL;

	ARGC_CHECK(5)
	data_set = get_data_set();
	GET_RESOURCE()
	free_data_set(data_set);

	if (STRNCMP_CONST(argv[2], "meta") == 0){
		snprintf(cmd, sizeof(cmd), "crm_resource --meta -r %s", argv[1]);
	}
	else{
		snprintf(cmd, sizeof(cmd), "crm_resource -r %s", argv[1]);
	}
	
	if (regex_match(nv_regex, argv[3])) {
		strncat(cmd, " -p \"", sizeof(cmd)-strlen(cmd)-1);
		strncat(cmd, argv[3], sizeof(cmd)-strlen(cmd)-1);
		strncat(cmd, "\"", sizeof(cmd)-strlen(cmd)-1);
	}
	else {
		mgmt_log(LOG_ERR, "invalid attribute name specified: \"%s\"", argv[3]);
		return strdup(MSG_FAIL"\nInvalid attribute name");
	}

	if (regex_match(nv_regex, argv[4])) {
		strncat(cmd, " -v \"", sizeof(cmd)-strlen(cmd)-1);
		strncat(cmd, argv[4], sizeof(cmd)-strlen(cmd)-1);
		strncat(cmd, "\"", sizeof(cmd)-strlen(cmd)-1);
	}
	else {
		mgmt_log(LOG_ERR, "invalid attribute value specified: \"%s\"", argv[4]);
		return strdup(MSG_FAIL"\nInvalid attribute value");
	}

	strncat(cmd, " 2>&1", sizeof(cmd)-strlen(cmd)-1);

	if ((fstream = popen(cmd, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on popen %s: %s",
			 cmd, strerror(errno));
		return strdup(MSG_FAIL"\nSet the named attribute failed");
	}

	ret = strdup(MSG_FAIL);
	while (!feof(fstream)){
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fstream) != NULL){
			ret = mgmt_msg_append(ret, buf);
			ret[strlen(ret)-1] = '\0';
		}
		else{
			sleep(1);
		}
	}

	if (pclose(fstream) == -1)
		mgmt_log(LOG_WARNING, "failed to close pipe");

	return ret;
}

char*
on_get_rsc_attr(char* argv[], int argc)
{
	char cmd[MAX_STRLEN];
	char buf[MAX_STRLEN];
	pe_working_set_t* data_set;
	resource_t* rsc;
	char* ret = NULL;
	const char* nv_regex = "^[A-Za-z0-9_-]+$";
	FILE *fstream = NULL;

	ARGC_CHECK(4)
	data_set = get_data_set();
	GET_RESOURCE()
	free_data_set(data_set);

	if (STRNCMP_CONST(argv[2], "meta") == 0){
		snprintf(cmd, sizeof(cmd), "crm_resource --meta -r %s", argv[1]);
	}
	else{
		snprintf(cmd, sizeof(cmd), "crm_resource -r %s", argv[1]);
	}
	
	if (regex_match(nv_regex, argv[3])) {
		strncat(cmd, " -g \"", sizeof(cmd)-strlen(cmd)-1);
		strncat(cmd, argv[3], sizeof(cmd)-strlen(cmd)-1);
		strncat(cmd, "\"", sizeof(cmd)-strlen(cmd)-1);
	}
	else {
		mgmt_log(LOG_ERR, "invalid attribute name specified: \"%s\"", argv[3]);
		return strdup(MSG_FAIL"\nInvalid attribute name");
	}

	if ((fstream = popen(cmd, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on popen %s: %s",
			 cmd, strerror(errno));
		return strdup(MSG_FAIL"\nGet the named attribute failed");
	}

	ret = strdup(MSG_OK);
	while (!feof(fstream)){
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fstream) != NULL){
			ret = mgmt_msg_append(ret, buf);
			ret[strlen(ret)-1] = '\0';
		}
		else{
			sleep(1);
		}
	}

	if (pclose(fstream) == -1)
		mgmt_log(LOG_WARNING, "failed to close pipe");

	return ret;
}

char*
on_del_rsc_attr(char* argv[], int argc)
{
	char cmd[MAX_STRLEN];
	char buf[MAX_STRLEN];
	pe_working_set_t* data_set;
	resource_t* rsc;
	char* ret = NULL;
	const char* nv_regex = "^[A-Za-z0-9_-]+$";
	FILE *fstream = NULL;

	ARGC_CHECK(4)
	data_set = get_data_set();
	GET_RESOURCE()
	free_data_set(data_set);

	if (STRNCMP_CONST(argv[2], "meta") == 0){
		snprintf(cmd, sizeof(cmd), "crm_resource --meta -r %s", argv[1]);
	}
	else{
		snprintf(cmd, sizeof(cmd), "crm_resource -r %s", argv[1]);
	}
	
	if (regex_match(nv_regex, argv[3])) {
		strncat(cmd, " -d \"", sizeof(cmd)-strlen(cmd)-1);
		strncat(cmd, argv[3], sizeof(cmd)-strlen(cmd)-1);
		strncat(cmd, "\"", sizeof(cmd)-strlen(cmd)-1);
	}
	else {
		mgmt_log(LOG_ERR, "invalid attribute name specified: \"%s\"", argv[3]);
		return strdup(MSG_FAIL"\nInvalid attribute name");
	}

	strncat(cmd, " 2>&1", sizeof(cmd)-strlen(cmd)-1);

	if ((fstream = popen(cmd, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on popen %s: %s",
			 cmd, strerror(errno));
		return strdup(MSG_FAIL"\nGet the named attribute failed");
	}

	ret = strdup(MSG_FAIL);
	while (!feof(fstream)){
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fstream) != NULL){
			ret = mgmt_msg_append(ret, buf);
			ret[strlen(ret)-1] = '\0';
		}
		else{
			sleep(1);
		}
	}

	if (pclose(fstream) == -1)
		mgmt_log(LOG_WARNING, "failed to close pipe");

	return ret;
}

/* master functions */
char*
on_get_master(char* argv[], int argc)
{
	resource_t* rsc;
	char* ret;
	char* parameter=NULL;
	pe_working_set_t* data_set;
	
	data_set = get_data_set();
	GET_RESOURCE()
	
	ret = strdup(MSG_OK);
	ret = mgmt_msg_append(ret, rsc->id);
	
	parameter = rsc->fns->parameter(rsc, NULL, FALSE
	,	XML_RSC_ATTR_INCARNATION_MAX, data_set);
	ret = mgmt_msg_append(ret, parameter);
	if (parameter != NULL) {
		free(parameter);
	}

	parameter = rsc->fns->parameter(rsc, NULL, FALSE
	,	XML_RSC_ATTR_INCARNATION_NODEMAX, data_set);
	ret = mgmt_msg_append(ret, parameter);
	if (parameter != NULL) {
		free(parameter);
	}

	parameter = rsc->fns->parameter(rsc, NULL, FALSE
	,	XML_RSC_ATTR_MASTER_MAX, data_set);
	ret = mgmt_msg_append(ret, parameter);
	if (parameter != NULL) {
		free(parameter);
	}

	parameter = rsc->fns->parameter(rsc, NULL, FALSE
	,	XML_RSC_ATTR_MASTER_NODEMAX, data_set);
	ret = mgmt_msg_append(ret, parameter);
	if (parameter != NULL) {
		free(parameter);
	}

	free_data_set(data_set);
	return ret;
}
char*
on_update_master(char* argv[], int argc)
{
	int rc;
	crm_data_t* fragment = NULL;
	crm_data_t* cib_object = NULL;
	crm_data_t* output = NULL;
	char xml[MAX_STRLEN];
	char meta_attrs_id[MAX_STRLEN];	

	ARGC_CHECK(6);
	get_meta_attributes_id(argv[1], meta_attrs_id);
	snprintf(xml,MAX_STRLEN,
		 "<master_slave id=\"%s\"><meta_attributes id=\"%s\"><attributes>" \
		 "<nvpair id=\"%s_clone_max\" name=\"clone_max\" value=\"%s\"/>" \
		 "<nvpair id=\"%s_clone_node_max\" name=\"clone_node_max\" value=\"%s\"/>" \
		 "<nvpair id=\"%s_master_max\" name=\"master_max\" value=\"%s\"/>" \
		 "<nvpair id=\"%s_master_node_max\" name=\"master_node_max\" value=\"%s\"/>" \
		 "</attributes></meta_attributes></master_slave>",
		 argv[1],meta_attrs_id,argv[1],argv[2],argv[1],
		 argv[3],argv[1],argv[4],argv[1],argv[5]);

	cib_object = string2xml(xml);
	if(cib_object == NULL) {
		return strdup(MSG_FAIL);
	}
	mgmt_log(LOG_INFO, "on_update_master:%s",xml);
	fragment = create_cib_fragment(cib_object, "resources");
	rc = cib_conn->cmds->update(cib_conn, "resources", fragment, cib_sync_call);
	free_xml(fragment);
	free_xml(cib_object);
	if (rc < 0) {
		return crm_failed_msg(output, rc);
	}
	free_xml(output);
	return strdup(MSG_OK);

}

/* constraints functions */
char*
on_get_constraints(char* argv[], int argc)
{
	char* ret;
	GList* list;
	GList* cur;
	crm_data_t* cos = NULL;
	pe_working_set_t* data_set;
	
	ARGC_CHECK(2);
	
	data_set = get_data_set();
	cos = get_xpath_object("//configuration/constraints", data_set->input, LOG_DEBUG);
	if (cos == NULL) {
		free_data_set(data_set);
		return  strdup(MSG_FAIL);
	}
	ret = strdup(MSG_OK);
	list = find_xml_node_list(cos, argv[1]);
	cur = list;
	while (cur != NULL) {
		crm_data_t* location = (crm_data_t*)cur->data;
		ret = mgmt_msg_append(ret, crm_element_value(location, "id"));
		
		cur = g_list_next(cur);
	}
	g_list_free(list);
	free_data_set(data_set);
	return ret;
}

char*
on_get_constraint(char* argv[], int argc)
{
	char* ret;
	GList* list;
	GList* cur;
	crm_data_t* rule;
	
	GList* expr_list, *expr_cur;
	crm_data_t* cos = NULL;
	pe_working_set_t* data_set;
	int i;
	
	data_set = get_data_set();
	cos = get_xpath_object("//configuration/constraints", data_set->input, LOG_DEBUG);
	if (cos == NULL) {
		free_data_set(data_set);
		return  strdup(MSG_FAIL);
	}
	ret = strdup(MSG_OK);
	list = find_xml_node_list(cos, argv[1]);
	cur = list;
	while (cur != NULL) {
		crm_data_t* constraint = (crm_data_t*)cur->data;
		if (strncmp(argv[2],crm_element_value(constraint, "id"), MAX_STRLEN)==0) {
			if (STRNCMP_CONST(argv[1],"rsc_location")==0) {
				ret = mgmt_msg_append(ret, crm_element_value(constraint, "id"));
				ret = mgmt_msg_append(ret, crm_element_value(constraint, "rsc"));
				rule = find_xml_node(constraint,"rule",TRUE);
				ret = mgmt_msg_append(ret, crm_element_value(rule, "score"));
				ret = mgmt_msg_append(ret, crm_element_value(rule, "boolean_op"));
				expr_list = find_xml_node_list(rule, "expression");
				expr_cur = expr_list;
				while(expr_cur) {
					crm_data_t* expr = (crm_data_t*)expr_cur->data;
					ret = mgmt_msg_append(ret, crm_element_value(expr, "id"));
					ret = mgmt_msg_append(ret, crm_element_value(expr, "attribute"));
					ret = mgmt_msg_append(ret, crm_element_value(expr, "operation"));
					ret = mgmt_msg_append(ret, crm_element_value(expr, "value"));
					expr_cur = g_list_next(expr_cur);
				}
				g_list_free(expr_list);
			}
			else if (STRNCMP_CONST(argv[1],"rsc_order")==0 ||
					STRNCMP_CONST(argv[1],"rsc_colocation")==0) {
				for(i = 3; i < argc; i++){
					ret = mgmt_msg_append(ret, crm_element_value(constraint, argv[i]));
				}
			}
			break;
		}
		cur = g_list_next(cur);
	}
	g_list_free(list);
	free_data_set(data_set);
	return ret;
}
char*
on_delete_constraint(char* argv[], int argc)
{
	int rc;
	crm_data_t * output = NULL;
	ARGC_CHECK(3)

	if ((rc=delete_object("constraints", argv[1], argv[2], &output)) < 0) {
		return crm_failed_msg(output, rc);
	}
	return strdup(MSG_OK);
}

char*
on_update_constraint(char* argv[], int argc)
{
	int rc;
	crm_data_t* fragment = NULL;
	crm_data_t* cib_object = NULL;
	crm_data_t* output = NULL;
	int i;
	char xml[MAX_STRLEN];
	char buf[MAX_STRLEN];

	if (STRNCMP_CONST(argv[1],"rsc_location")==0) {
		snprintf(xml, MAX_STRLEN,
			 "<rsc_location id=\"%s\" rsc=\"%s\">" \
				"<rule id=\"prefered_%s\" score=\"%s\" boolean_op=\"%s\">",
		 	 argv[2], argv[3], argv[2], argv[4], argv[5]);
		for (i = 0; i < (argc-6)/4; i++) {
			char expr[MAX_STRLEN];
			snprintf(expr, MAX_STRLEN,
				 "<expression attribute=\"%s\" id=\"%s\" operation=\"%s\" value=\"%s\"/>",
			 	 argv[6+i*4+1],argv[6+i*4],argv[6+i*4+2],argv[6+i*4+3]);
			strncat(xml, expr, sizeof(xml)-strlen(xml)-1);
		}
		strncat(xml, "</rule></rsc_location>",
				sizeof(xml)-strlen(xml)-1);
	}
	else if (STRNCMP_CONST(argv[1],"rsc_order")==0 ||
			STRNCMP_CONST(argv[1],"rsc_colocation")==0) {
		snprintf(xml, MAX_STRLEN, "<%s", argv[1]);
		for (i = 2; i < argc-1; i += 2){
			snprintf(buf, MAX_STRLEN, " %s=\"%s\"", argv[i], argv[i+1]);
			strncat(xml, buf, sizeof(xml)-strlen(xml)-1);
		}
		strncat(xml, " />", sizeof(xml)-strlen(xml)-1);
	}
	cib_object = string2xml(xml);
	if(cib_object == NULL) {
		return strdup(MSG_FAIL);
	}
	mgmt_log(LOG_INFO, "on_update_constraint:%s",xml);
	fragment = create_cib_fragment(cib_object, "constraints");

	rc = cib_conn->cmds->update(
			cib_conn, "constraints", fragment, cib_sync_call);

	free_xml(fragment);
	free_xml(cib_object);
	if (rc < 0) {
		return crm_failed_msg(output, rc);
	}
	free_xml(output);
	return strdup(MSG_OK);
}

char*
on_cib_create(char* argv[], int argc)
{
	int rc;
	crm_data_t* cib_object = NULL;
	crm_data_t* output = NULL;
	const char* type = NULL;
	const char* xmls = NULL;
	ARGC_CHECK(3)

	type = argv[1];
	xmls = argv[2];
	cib_object = string2xml(xmls);
	if (cib_object == NULL) {
		return strdup(MSG_FAIL);
	}

	mgmt_log(LOG_INFO, "CIB create: %s", type);
		
	rc = cib_conn->cmds->create(cib_conn, type, cib_object, cib_sync_call);
	free_xml(cib_object);
	if (rc < 0) {
		return crm_failed_msg(output, rc);
	} else {
		free_xml(output);
		return strdup(MSG_OK);
	}
}

/*
char*
on_cib_query(char* argv[], int argc)
{
	const char* type = NULL;
	char cmd[MAX_STRLEN];
	char buf[MAX_STRLEN];	
	char* ret = strdup(MSG_OK);
	FILE *fstream = NULL;
	ARGC_CHECK(2)

	type = argv[1];
	mgmt_log(LOG_INFO, "CIB query: %s", type);
		
	snprintf(cmd, sizeof(cmd), "cibadmin -Q -o %s", type);
	if ((fstream = popen(cmd, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on popen %s: %s",
			 cmd, strerror(errno));
		return strdup(MSG_FAIL);
	}

	while (!feof(fstream)){
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fstream) != NULL){
			ret = mgmt_msg_append(ret, buf);
			ret[strlen(ret)-1] = '\0';
		}
		else{
			sleep(1);
		}
	}

	if (pclose(fstream) == -1)
		mgmt_log(LOG_WARNING, "failed to close pipe");

	return ret;
}
*/

char*
on_cib_query(char* argv[], int argc)
{
	int rc;
	crm_data_t* output = NULL;
	const char* type = NULL;
	char* ret = NULL;
	ARGC_CHECK(2)

	type = argv[1];
	mgmt_log(LOG_INFO, "CIB query: %s", type);
		
	rc = cib_conn->cmds->query(cib_conn, type, &output, cib_sync_call|cib_scope_local);
	if (rc < 0) {
		return crm_failed_msg(output, rc);
	} else {
		ret = strdup(MSG_OK);
		ret = mgmt_msg_append(ret, dump_xml_formatted(output));
#if 0		
		mgmt_log(LOG_INFO, "%s", dump_xml_formatted(output)); 
#endif
		free_xml(output);
		return ret;
	}
}

char*
on_cib_update(char* argv[], int argc)
{
	int rc;
	crm_data_t* fragment = NULL;
	crm_data_t* cib_object = NULL;
	crm_data_t* output = NULL;
	const char* type = NULL;
	const char* xmls = NULL;
	ARGC_CHECK(3)

	type = argv[1];
	xmls = argv[2];
	cib_object = string2xml(xmls);
	if (cib_object == NULL) {
		return strdup(MSG_FAIL);
	}

	mgmt_log(LOG_INFO, "CIB update: %s", xmls);
		
	fragment = create_cib_fragment(cib_object, type);
	rc = cib_conn->cmds->update(cib_conn, type, fragment, cib_sync_call);
	free_xml(fragment);
	free_xml(cib_object);
	if (rc < 0) {
		return crm_failed_msg(output, rc);
	} else {
		free_xml(output);
		return strdup(MSG_OK);
	}
}

char*
on_cib_replace(char* argv[], int argc)
{
	int rc;
	/*crm_data_t* fragment = NULL;*/
	crm_data_t* cib_object = NULL;
	crm_data_t* output = NULL;
	const char* type = argv[0];
	const char* xmls = argv[1];
	ARGC_CHECK(3)

	type = argv[1];
	xmls = argv[2];
	cib_object = string2xml(xmls);
	if (cib_object == NULL) {
		return strdup(MSG_FAIL);
	}

	mgmt_log(LOG_INFO, "CIB replace: %s", type);
		
	/*fragment = create_cib_fragment(cib_object, type);*/
	rc = cib_conn->cmds->replace(cib_conn, type, cib_object, cib_sync_call);
	/*free_xml(fragment);*/
	free_xml(cib_object);
	if (rc < 0) {
		return crm_failed_msg(output, rc);
	} else {
		free_xml(output);
		return strdup(MSG_OK);
	}
}

char*
on_cib_delete(char* argv[], int argc)
{
	int rc;
	crm_data_t* cib_object = NULL;
	crm_data_t* output = NULL;
	const char* type = argv[0];
	const char* xmls = argv[1];	
	ARGC_CHECK(3)
	
	type = argv[1];
	xmls = argv[2];	
	cib_object = string2xml(xmls);
	if (cib_object == NULL) {
		return strdup(MSG_FAIL);
	}
	mgmt_log(LOG_INFO, "CIB delete: %s", type);

	rc = cib_conn->cmds->delete(cib_conn, type, cib_object, cib_sync_call);
	free_xml(cib_object);	
	if (rc < 0) {
		return crm_failed_msg(output, rc);
	} else {
		free_xml(output);
		return strdup(MSG_OK);
	}
}		

static char*
on_gen_cluster_report(char* argv[], int argc)
{
	char cmd[MAX_STRLEN];
	char buf[MAX_STRLEN];
	char filename[MAX_STRLEN];
	const char *tempdir = "/tmp";
	char *dest = tempnam(tempdir, "clrp.");
	struct dirent *dirp;
	DIR *dp;
	char *ret = NULL;
	FILE *fstream = NULL;
	const char* date_regex = \
		"^[0-9]{4}-[0-1][0-9]-[0-3][0-9] [0-2][0-9]:[0-5][0-9]:[0-5][0-9]$";

	ARGC_CHECK(3);

	if (regex_match(date_regex, argv[1])) {
		snprintf(buf, sizeof(buf), "-f \"%s\"", argv[1]);
	}
	else {
		mgmt_log(LOG_ERR, "cluster_report: invalid \"from\" date expression: \"%s\"", argv[1]);
		free(dest);
		return strdup(MSG_FAIL"\nInvalid \"from\" date expression");
	}

	if (strnlen(argv[2], MAX_STRLEN) != 0) {
		if (regex_match(date_regex, argv[2])) {
			strncat(buf, " -t \"", sizeof(buf)-strlen(buf)-1);
			strncat(buf, argv[2], sizeof(buf)-strlen(buf)-1);
			strncat(buf, "\"", sizeof(buf)-strlen(buf)-1);
		}
		else {
			mgmt_log(LOG_ERR, "cluster_report: invalid \"to\" date expression: \"%s\"", argv[2]);
			free(dest);
			return strdup(MSG_FAIL"\nInvalid \"to\" date expression");
		}
	}

	if (is_openais_cluster()){
		snprintf(cmd, sizeof(cmd), "hb_report -ADC %s %s", buf, dest);
	}
	else{
		snprintf(cmd, sizeof(cmd), "hb_report -DC %s %s", buf, dest);
	}

	mgmt_log(LOG_INFO, "cluster_report: %s", cmd);
	if (system(cmd) < 0) {
		mgmt_log(LOG_ERR, "cluster_report: error on system \"%s\": %s", cmd, strerror(errno));
		free(dest);
		return strdup(MSG_FAIL"\nError on execute the cluster report command");
	}

	if ((dp = opendir(tempdir)) == NULL){
		mgmt_log(LOG_ERR, "cluster_report: error on opendir \"%s\": %s", tempdir, strerror(errno));
		free(dest);
		return strdup(MSG_FAIL"\nCannot open the temporary directory");
	}

	while ((dirp = readdir(dp)) != NULL) {
		if (dirp->d_type == DT_REG && strstr(dirp->d_name, basename(dest)) != NULL
				&& strstr(dirp->d_name, "tar") != NULL){
			snprintf(filename, sizeof(filename), "%s/%s", tempdir, dirp->d_name);
			break;
		}
	}

	if (closedir(dp) < 0){
		mgmt_log(LOG_WARNING, "cluster_report: failed to closedir \"%s\": %s", tempdir, strerror(errno) );
	}

	free(dest);

	if (strnlen(filename, MAX_STRLEN) == 0) {
		mgmt_log(LOG_ERR, "cluster_report: failed to generate a cluster report");
		return strdup(MSG_FAIL"\nFailed to generate a cluster report");
	}

	mgmt_log(LOG_INFO, "cluster_report: execute the cluster report command successfully");

	snprintf(cmd, sizeof(cmd), "/usr/bin/base64 %s", filename);
	if ((fstream = popen(cmd, "r")) == NULL) {
		mgmt_log(LOG_ERR, "cluster_report: error on popen \"%s\": %s", cmd, strerror(errno));
		unlink(filename);
		return strdup(MSG_FAIL"\nFailed to encode the cluster report to base64");
	 }

	ret = strdup(MSG_OK);
	ret = mgmt_msg_append(ret, filename);
	while (!feof(fstream)) {
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fstream) != NULL) {
			ret = mgmt_msg_append(ret, buf);
			ret[strlen(ret)-1] = '\0';
		}
		else {
			sleep(1);
		}
	}
	if (pclose(fstream) == -1){
		mgmt_log(LOG_WARNING, "cluster_report: failed to close pipe");
	}

	mgmt_log(LOG_INFO, "cluster_report: send out the report");
	unlink(filename);
	return ret;
}

static const char* pe_working_dir = HA_VARLIBDIR"/heartbeat/pengine";

static char*
on_get_pe_inputs(char* argv[], int argc)
{
	char* ret = NULL;
	struct dirent *dirp;
	DIR *dp;
	char fullpath[MAX_STRLEN];
	struct stat statbuf;
	char buf[MAX_STRLEN];

	if ((dp = opendir(pe_working_dir)) == NULL){
		mgmt_log(LOG_ERR, "error on opendir \"%s\": %s", pe_working_dir, strerror(errno));
		return strdup(MSG_FAIL"\nCannot open the pengine working directory");
	}

	ret = strdup(MSG_OK);
	while ((dirp = readdir(dp)) != NULL) {
		if (dirp->d_type == DT_REG && strstr(dirp->d_name, "pe-input-") != NULL
				&& strstr(dirp->d_name, "bz2") != NULL){
			memset(fullpath, 0, sizeof(fullpath));
			snprintf(fullpath, sizeof(fullpath), "%s/%s", pe_working_dir, dirp->d_name);
			if (stat(fullpath, &statbuf) < 0){
				mgmt_log(LOG_ERR, "Cannot stat the file \"%s\"", fullpath);
				continue;
			}
			if (S_ISREG(statbuf.st_mode)){
				memset(buf, 0, sizeof(buf));
				snprintf(buf, sizeof(buf), "%s %ld", dirp->d_name, (long int)statbuf.st_mtime);
				ret = mgmt_msg_append(ret, buf);
			}
		}
	}

	if (closedir(dp) < 0){
		mgmt_log(LOG_WARNING, "failed to closedir \"%s\": %s", pe_working_dir, strerror(errno) );
	}
	return ret;
}

static char*
on_gen_pe_graph(char* argv[], int argc)
{
	char* ret = NULL;
	char cmd[MAX_STRLEN];
	char buf[MAX_STRLEN];
	char *dotfile = NULL;
	FILE *fstream = NULL;

	ARGC_CHECK(2)
	if (STRNCMP_CONST(argv[1], "live") == 0){
		strncpy(cmd, "ptest -L", sizeof(cmd)-1);
	}
	else{
		snprintf(cmd, sizeof(cmd), "ptest -x %s/pe-input-%s.bz2", pe_working_dir, argv[1]);
	}

	strncat(cmd, " -D ", sizeof(cmd)-strlen(cmd)-1);
	dotfile = tempnam("/tmp", "dot.");
	strncat(cmd, dotfile, sizeof(cmd)-strlen(cmd)-1);

	if (system(cmd) < 0){
		mgmt_log(LOG_ERR, "error on execute \"%s\": %s", cmd, strerror(errno));
		free(dotfile);
		return strdup(MSG_FAIL"\nError on execute the ptest command");
	}

	if ((fstream = fopen(dotfile, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on fopen %s: %s", dotfile, strerror(errno));
		free(dotfile);
		unlink(dotfile);
		return strdup(MSG_FAIL"\nError on read the transition graph file");
	}

	ret = strdup(MSG_OK);
	while (!feof(fstream)){
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fstream) != NULL){
			ret = mgmt_msg_append(ret, buf);
			ret[strlen(ret)-1] = '\0';
		}
		else{
			sleep(1);
		}
	}

	if (fclose(fstream) == -1){
		mgmt_log(LOG_WARNING, "failed to fclose stream");
	}

	unlink(dotfile);
	free(dotfile);
	return ret;
}

static char*
on_gen_pe_info(char* argv[], int argc)
{
	char* ret = NULL;
	char cmd[MAX_STRLEN];
	int i;
	char buf[MAX_STRLEN];
	FILE *fstream = NULL;

	ARGC_CHECK(3)
	if (STRNCMP_CONST(argv[1], "live") == 0){
		strncpy(cmd, "ptest -L", sizeof(cmd)-1);
	}
	else{
		snprintf(cmd, sizeof(cmd), "ptest -x %s/pe-input-%s.bz2", pe_working_dir, argv[1]);
	}
	
	if (STRNCMP_CONST(argv[2], "scores") == 0){
		strncat(cmd, " -s", sizeof(cmd)-strlen(cmd)-1);
	}
	else{
		for (i = 0; i < atoi(argv[2]); i++){
			if (i == 0){
				strncat(cmd, " -V", sizeof(cmd)-strlen(cmd)-1);
			}
			else{
				strncat(cmd, "V", sizeof(cmd)-strlen(cmd)-1);
			}
		}
	}

	strncat(cmd, " 2>&1", sizeof(cmd)-strlen(cmd)-1);

	if ((fstream = popen(cmd, "r")) == NULL){
		mgmt_log(LOG_ERR, "error on popen \"%s\": %s", cmd, strerror(errno));
		return strdup(MSG_FAIL"\nError on popen the ptest command");
	}

	ret = strdup(MSG_OK);
	while (!feof(fstream)){
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fstream) != NULL){
			ret = mgmt_msg_append(ret, buf);
			ret[strlen(ret)-1] = '\0';
		}
		else{
			sleep(1);
		}
	}

	if (fclose(fstream) == -1){
		mgmt_log(LOG_WARNING, "failed to fclose stream");
	}

	return ret;
}

int
regex_match(const char *regex, const char *str)
{
	regex_t preg;
	int ret;

	if (regcomp(&preg, regex, REG_EXTENDED|REG_NOSUB) != 0){
		mgmt_log(LOG_ERR, "error regcomp regular expression: \"%s\"", regex);
		return 0;
	}

	ret = regexec(&preg, str, 0, NULL, 0);
	if (ret != 0) {
		mgmt_log(LOG_WARNING, "no match or error regexec: \"%s\" \"%s\"", regex, str);
	}

	regfree(&preg);
	return (ret == 0);
}
