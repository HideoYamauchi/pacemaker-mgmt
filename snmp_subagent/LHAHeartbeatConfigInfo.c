/*
 * Note: this file originally auto-generated by mib2c using
 *        : mib2c.scalar.conf,v 1.7 2003/04/08 14:57:04 dts12 Exp $
 */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "LHAHeartbeatConfigInfo.h"

#include "hbagent.h"
#include <hb_api.h>

/** Initializes the LHAHeartbeatConfigInfo module */
void
init_LHAHeartbeatConfigInfo(void)
{
    static oid LHAHBVersion_oid[] = { 1,3,6,1,4,1,4682,7,1 };
    static oid LHAAPIAuth_oid[] = { 1,3,6,1,4,1,4682,7,21 };
    static oid LHADebugFile_oid[] = { 1,3,6,1,4,1,4682,7,24 };
    static oid LHADebug_oid[] = { 1,3,6,1,4,1,4682,7,19 };
    static oid LHAHOPFudge_oid[] = { 1,3,6,1,4,1,4682,7,2 };
    static oid LHARespawn_oid[] = { 1,3,6,1,4,1,4682,7,15 };
    static oid LHADeadTime_oid[] = { 1,3,6,1,4,1,4682,7,4 };
    static oid LHAInitDead_oid[] = { 1,3,6,1,4,1,4682,7,7 };
    static oid LHAUDPPort_oid[] = { 1,3,6,1,4,1,4682,7,10 };
    static oid LHALogFile_oid[] = { 1,3,6,1,4,1,4682,7,23 };
    static oid LHAStonith_oid[] = { 1,3,6,1,4,1,4682,7,13 };
    static oid LHABaudRate_oid[] = { 1,3,6,1,4,1,4682,7,9 };
    static oid LHNiceFailBack_oid[] = { 1,3,6,1,4,1,4682,7,11 };
    static oid LHARTPriority_oid[] = { 1,3,6,1,4,1,4682,7,18 };
    static oid LHAMsgFmt_oid[] = { 1,3,6,1,4,1,4682,7,22 };
    static oid LHARealTime_oid[] = { 1,3,6,1,4,1,4682,7,17 };
    static oid LHAStonithHost_oid[] = { 1,3,6,1,4,1,4682,7,14 };
    static oid LHANormalPoll_oid[] = { 1,3,6,1,4,1,4682,7,20 };
    static oid LHAGenMethod_oid[] = { 1,3,6,1,4,1,4682,7,16 };
    static oid LHAWarnTime_oid[] = { 1,3,6,1,4,1,4682,7,6 };
    static oid LHADeadPing_oid[] = { 1,3,6,1,4,1,4682,7,5 };
    static oid LHAKeepAlive_oid[] = { 1,3,6,1,4,1,4682,7,3 };
    static oid LHAWatchdogTimer_oid[] = { 1,3,6,1,4,1,4682,7,8 };
    static oid LHALogFacility_oid[] = { 1,3,6,1,4,1,4682,7,25 };
    static oid LHAutoFailBack_oid[] = { 1,3,6,1,4,1,4682,7,12 };

  DEBUGMSGTL(("LHAHeartbeatConfigInfo", "Initializing\n"));

    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHAHBVersion", handle_LHAHBVersion,
                               LHAHBVersion_oid, OID_LENGTH(LHAHBVersion_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHAAPIAuth", handle_LHAAPIAuth,
                               LHAAPIAuth_oid, OID_LENGTH(LHAAPIAuth_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHADebugFile", handle_LHADebugFile,
                               LHADebugFile_oid, OID_LENGTH(LHADebugFile_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHADebug", handle_LHADebug,
                               LHADebug_oid, OID_LENGTH(LHADebug_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHAHOPFudge", handle_LHAHOPFudge,
                               LHAHOPFudge_oid, OID_LENGTH(LHAHOPFudge_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHARespawn", handle_LHARespawn,
                               LHARespawn_oid, OID_LENGTH(LHARespawn_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHADeadTime", handle_LHADeadTime,
                               LHADeadTime_oid, OID_LENGTH(LHADeadTime_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHAInitDead", handle_LHAInitDead,
                               LHAInitDead_oid, OID_LENGTH(LHAInitDead_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHAUDPPort", handle_LHAUDPPort,
                               LHAUDPPort_oid, OID_LENGTH(LHAUDPPort_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHALogFile", handle_LHALogFile,
                               LHALogFile_oid, OID_LENGTH(LHALogFile_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHAStonith", handle_LHAStonith,
                               LHAStonith_oid, OID_LENGTH(LHAStonith_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHABaudRate", handle_LHABaudRate,
                               LHABaudRate_oid, OID_LENGTH(LHABaudRate_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHNiceFailBack", handle_LHNiceFailBack,
                               LHNiceFailBack_oid, OID_LENGTH(LHNiceFailBack_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHARTPriority", handle_LHARTPriority,
                               LHARTPriority_oid, OID_LENGTH(LHARTPriority_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHAMsgFmt", handle_LHAMsgFmt,
                               LHAMsgFmt_oid, OID_LENGTH(LHAMsgFmt_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHARealTime", handle_LHARealTime,
                               LHARealTime_oid, OID_LENGTH(LHARealTime_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHAStonithHost", handle_LHAStonithHost,
                               LHAStonithHost_oid, OID_LENGTH(LHAStonithHost_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHANormalPoll", handle_LHANormalPoll,
                               LHANormalPoll_oid, OID_LENGTH(LHANormalPoll_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHAGenMethod", handle_LHAGenMethod,
                               LHAGenMethod_oid, OID_LENGTH(LHAGenMethod_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHAWarnTime", handle_LHAWarnTime,
                               LHAWarnTime_oid, OID_LENGTH(LHAWarnTime_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHADeadPing", handle_LHADeadPing,
                               LHADeadPing_oid, OID_LENGTH(LHADeadPing_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHAKeepAlive", handle_LHAKeepAlive,
                               LHAKeepAlive_oid, OID_LENGTH(LHAKeepAlive_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHAWatchdogTimer", handle_LHAWatchdogTimer,
                               LHAWatchdogTimer_oid, OID_LENGTH(LHAWatchdogTimer_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHALogFacility", handle_LHALogFacility,
                               LHALogFacility_oid, OID_LENGTH(LHALogFacility_oid),
                               HANDLER_CAN_RONLY
        ));
    netsnmp_register_scalar(
        netsnmp_create_handler_registration("LHAutoFailBack", handle_LHAutoFailBack,
                               LHAutoFailBack_oid, OID_LENGTH(LHAutoFailBack_oid),
                               HANDLER_CAN_RONLY
        ));
}

int
handle_LHAHBVersion(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    
    char * value;

    if (hbconfig_get_str_value(KEY_HBVERSION, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;


        default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHAAPIAuth(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_APIPERM, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

        default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHADebugFile(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_DBGFILE, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

        default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHADebug(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_DEBUGLEVEL, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

        default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHAHOPFudge(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_HOPS, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

        default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHARespawn(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_CLIENT_CHILD, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

        default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHADeadTime(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_DEADTIME, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

        default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHAInitDead(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_INITDEAD, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

        default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHAUDPPort(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_UDPPORT, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

        default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHALogFile(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_LOGFILE, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

        default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHAStonith(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_STONITH, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

        default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHABaudRate(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_BAUDRATE, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

        default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHNiceFailBack(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_FAILBACK, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

       default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHARTPriority(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_RT_PRIO, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

        default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHAMsgFmt(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_MSGFMT, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

       default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHARealTime(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_REALTIME, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

       default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHAStonithHost(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_STONITHHOST, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

       default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHANormalPoll(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_NORMALPOLL, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

       default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHAGenMethod(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_GEN_METH, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

        default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHAWarnTime(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_WARNTIME, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

        default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHADeadPing(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_DEADPING, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

        default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHAKeepAlive(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_KEEPALIVE, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

       default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHAWatchdogTimer(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_WATCHDOG, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

        default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHALogFacility(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_FACILITY, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

        default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
int
handle_LHAutoFailBack(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    char * value;

    if (hbconfig_get_str_value(KEY_AUTOFAIL, &value) != HA_OK) 
	return SNMP_ERR_GENERR;

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     (u_char *) value,
				     strlen(value) + 1);
            break;

       default:
            /* we should never get here, so this is a really bad error */
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
