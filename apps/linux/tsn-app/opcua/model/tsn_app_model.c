/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * WARNING: This is a generated file.
 * Any manual changes will be overwritten. */

#include "tsn_app_model.h"

/* TsnAppType - ns=1;i=15138 */

static UA_StatusCode function_tsn_app_model_0_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TsnAppType");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Base type for tsn_app");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECTTYPE, UA_NODEID_NUMERIC(ns[1], 15138LU),
					  UA_NODEID_NUMERIC(ns[0], 58LU), UA_NODEID_NUMERIC(ns[0], 45LU),
					  UA_QUALIFIEDNAME(ns[1], "TsnAppType"), UA_NODEID_NULL,
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_0_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15138LU));
}

/* TsnApp - ns=1;i=15240 */

static UA_StatusCode function_tsn_app_model_1_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.eventNotifier = true;
	attr.displayName = UA_LOCALIZEDTEXT("", "TsnApp");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "TsnApp object");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15240LU),
					  UA_NODEID_NUMERIC(ns[0], 85LU), UA_NODEID_NUMERIC(ns[0], 35LU),
					  UA_QUALIFIEDNAME(ns[1], "TsnApp"), UA_NODEID_NUMERIC(ns[1], 15138LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_1_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15240LU));
}

/* SocketStatsType - ns=1;i=15084 */

static UA_StatusCode function_tsn_app_model_2_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "SocketStatsType");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Base type for all the sockets");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECTTYPE, UA_NODEID_NUMERIC(ns[1], 15084LU),
					  UA_NODEID_NUMERIC(ns[0], 58LU), UA_NODEID_NUMERIC(ns[0], 45LU),
					  UA_QUALIFIEDNAME(ns[1], "SocketStatsType"), UA_NODEID_NULL,
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_2_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15084LU));
}

/* SocketStats - ns=1;i=15244 */

static UA_StatusCode function_tsn_app_model_3_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "SocketStats");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Statistics for the sockets");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15244LU),
					  UA_NODEID_NUMERIC(ns[1], 15240LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SocketStats"), UA_NODEID_NUMERIC(ns[1], 15084LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_3_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15244LU));
}

/* SocketStats - ns=1;i=15142 */

static UA_StatusCode function_tsn_app_model_4_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "SocketStats");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Statistics for the sockets");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15142LU),
					  UA_NODEID_NUMERIC(ns[1], 15138LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SocketStats"), UA_NODEID_NUMERIC(ns[1], 15084LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15142LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_4_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15142LU));
}

/* NetworkSocketType - ns=1;i=15079 */

static UA_StatusCode function_tsn_app_model_5_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "NetworkSocketType");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Frames from low-level network socket");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECTTYPE, UA_NODEID_NUMERIC(ns[1], 15079LU),
					  UA_NODEID_NUMERIC(ns[0], 58LU), UA_NODEID_NUMERIC(ns[0], 45LU),
					  UA_QUALIFIEDNAME(ns[1], "NetworkSocketType"), UA_NODEID_NULL,
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_5_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15079LU));
}

/* Direction - ns=1;i=15080 */

static UA_StatusCode function_tsn_app_model_6_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Direction");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Rx or tx");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15080LU),
					  UA_NODEID_NUMERIC(ns[1], 15079LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Direction"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15080LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_6_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15080LU));
}

/* NetRxSocket1 - ns=1;i=15128 */

static UA_StatusCode function_tsn_app_model_7_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "NetRxSocket1");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "NetRxSocket1");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15128LU),
					  UA_NODEID_NUMERIC(ns[1], 15084LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NetRxSocket1"), UA_NODEID_NUMERIC(ns[1], 15079LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15128LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_7_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15128LU));
}

/* FramesErr - ns=1;i=15132 */

static UA_StatusCode function_tsn_app_model_8_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "FramesErr");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Frames errors number");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15132LU),
					  UA_NODEID_NUMERIC(ns[1], 15128LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "FramesErr"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15132LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_8_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15132LU));
}

/* Id - ns=1;i=15130 */

static UA_StatusCode function_tsn_app_model_9_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Id");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Identifier");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15130LU),
					  UA_NODEID_NUMERIC(ns[1], 15128LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Id"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15130LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_9_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15130LU));
}

/* Frames - ns=1;i=15131 */

static UA_StatusCode function_tsn_app_model_10_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Frames");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Frames number");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15131LU),
					  UA_NODEID_NUMERIC(ns[1], 15128LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Frames"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15131LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_10_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15131LU));
}

/* Direction - ns=1;i=15129 */

static UA_StatusCode function_tsn_app_model_11_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Direction");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Rx or tx");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15129LU),
					  UA_NODEID_NUMERIC(ns[1], 15128LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Direction"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15129LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_11_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15129LU));
}

/* Frames - ns=1;i=15082 */

static UA_StatusCode function_tsn_app_model_12_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Frames");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Frames number");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15082LU),
					  UA_NODEID_NUMERIC(ns[1], 15079LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Frames"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15082LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_12_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15082LU));
}

/* NetRxSocket1 - ns=1;i=15288 */

static UA_StatusCode function_tsn_app_model_13_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "NetRxSocket1");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "NetRxSocket1");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15288LU),
					  UA_NODEID_NUMERIC(ns[1], 15244LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NetRxSocket1"), UA_NODEID_NUMERIC(ns[1], 15079LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_13_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15288LU));
}

/* Id - ns=1;i=15290 */

static UA_StatusCode function_tsn_app_model_14_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Id");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Identifier");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15290LU),
					  UA_NODEID_NUMERIC(ns[1], 15288LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Id"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_14_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15290LU));
}

/* Direction - ns=1;i=15289 */

static UA_StatusCode function_tsn_app_model_15_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Direction");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Rx or tx");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15289LU),
					  UA_NODEID_NUMERIC(ns[1], 15288LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Direction"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_15_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15289LU));
}

/* Frames - ns=1;i=15291 */

static UA_StatusCode function_tsn_app_model_16_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Frames");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Frames number");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15291LU),
					  UA_NODEID_NUMERIC(ns[1], 15288LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Frames"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_16_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15291LU));
}

/* FramesErr - ns=1;i=15292 */

static UA_StatusCode function_tsn_app_model_17_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "FramesErr");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Frames errors number");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15292LU),
					  UA_NODEID_NUMERIC(ns[1], 15288LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "FramesErr"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_17_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15292LU));
}

/* NetRxSocket1 - ns=1;i=15186 */

static UA_StatusCode function_tsn_app_model_18_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "NetRxSocket1");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "NetRxSocket1");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15186LU),
					  UA_NODEID_NUMERIC(ns[1], 15142LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NetRxSocket1"), UA_NODEID_NUMERIC(ns[1], 15079LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15186LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_18_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15186LU));
}

/* FramesErr - ns=1;i=15190 */

static UA_StatusCode function_tsn_app_model_19_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "FramesErr");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Frames errors number");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15190LU),
					  UA_NODEID_NUMERIC(ns[1], 15186LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "FramesErr"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15190LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_19_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15190LU));
}

/* Id - ns=1;i=15188 */

static UA_StatusCode function_tsn_app_model_20_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Id");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Identifier");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15188LU),
					  UA_NODEID_NUMERIC(ns[1], 15186LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Id"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15188LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_20_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15188LU));
}

/* Direction - ns=1;i=15187 */

static UA_StatusCode function_tsn_app_model_21_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Direction");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Rx or tx");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15187LU),
					  UA_NODEID_NUMERIC(ns[1], 15186LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Direction"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15187LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_21_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15187LU));
}

/* Frames - ns=1;i=15189 */

static UA_StatusCode function_tsn_app_model_22_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Frames");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Frames number");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15189LU),
					  UA_NODEID_NUMERIC(ns[1], 15186LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Frames"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15189LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_22_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15189LU));
}

/* NetRxSocket0 - ns=1;i=15123 */

static UA_StatusCode function_tsn_app_model_23_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "NetRxSocket0");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "NetRxSocket0");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15123LU),
					  UA_NODEID_NUMERIC(ns[1], 15084LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NetRxSocket0"), UA_NODEID_NUMERIC(ns[1], 15079LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15123LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_23_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15123LU));
}

/* Direction - ns=1;i=15124 */

static UA_StatusCode function_tsn_app_model_24_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Direction");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Rx or tx");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15124LU),
					  UA_NODEID_NUMERIC(ns[1], 15123LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Direction"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15124LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_24_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15124LU));
}

/* FramesErr - ns=1;i=15127 */

static UA_StatusCode function_tsn_app_model_25_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "FramesErr");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Frames errors number");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15127LU),
					  UA_NODEID_NUMERIC(ns[1], 15123LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "FramesErr"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15127LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_25_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15127LU));
}

/* Id - ns=1;i=15125 */

static UA_StatusCode function_tsn_app_model_26_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Id");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Identifier");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15125LU),
					  UA_NODEID_NUMERIC(ns[1], 15123LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Id"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15125LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_26_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15125LU));
}

/* Frames - ns=1;i=15126 */

static UA_StatusCode function_tsn_app_model_27_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Frames");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Frames number");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15126LU),
					  UA_NODEID_NUMERIC(ns[1], 15123LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Frames"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15126LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_27_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15126LU));
}

/* NetTxSocket0 - ns=1;i=15293 */

static UA_StatusCode function_tsn_app_model_28_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "NetTxSocket0");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "NetTxSocket0");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15293LU),
					  UA_NODEID_NUMERIC(ns[1], 15244LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NetTxSocket0"), UA_NODEID_NUMERIC(ns[1], 15079LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_28_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15293LU));
}

/* FramesErr - ns=1;i=15297 */

static UA_StatusCode function_tsn_app_model_29_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "FramesErr");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Frames errors number");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15297LU),
					  UA_NODEID_NUMERIC(ns[1], 15293LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "FramesErr"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_29_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15297LU));
}

/* Id - ns=1;i=15295 */

static UA_StatusCode function_tsn_app_model_30_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Id");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Identifier");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15295LU),
					  UA_NODEID_NUMERIC(ns[1], 15293LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Id"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_30_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15295LU));
}

/* Frames - ns=1;i=15296 */

static UA_StatusCode function_tsn_app_model_31_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Frames");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Frames number");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15296LU),
					  UA_NODEID_NUMERIC(ns[1], 15293LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Frames"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_31_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15296LU));
}

/* Direction - ns=1;i=15294 */

static UA_StatusCode function_tsn_app_model_32_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Direction");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Rx or tx");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15294LU),
					  UA_NODEID_NUMERIC(ns[1], 15293LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Direction"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_32_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15294LU));
}

/* NetRxSocket0 - ns=1;i=15283 */

static UA_StatusCode function_tsn_app_model_33_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "NetRxSocket0");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "NetRxSocket0");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15283LU),
					  UA_NODEID_NUMERIC(ns[1], 15244LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NetRxSocket0"), UA_NODEID_NUMERIC(ns[1], 15079LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_33_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15283LU));
}

/* Id - ns=1;i=15285 */

static UA_StatusCode function_tsn_app_model_34_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Id");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Identifier");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15285LU),
					  UA_NODEID_NUMERIC(ns[1], 15283LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Id"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_34_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15285LU));
}

/* Frames - ns=1;i=15286 */

static UA_StatusCode function_tsn_app_model_35_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Frames");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Frames number");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15286LU),
					  UA_NODEID_NUMERIC(ns[1], 15283LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Frames"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_35_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15286LU));
}

/* FramesErr - ns=1;i=15287 */

static UA_StatusCode function_tsn_app_model_36_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "FramesErr");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Frames errors number");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15287LU),
					  UA_NODEID_NUMERIC(ns[1], 15283LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "FramesErr"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_36_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15287LU));
}

/* Direction - ns=1;i=15284 */

static UA_StatusCode function_tsn_app_model_37_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Direction");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Rx or tx");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15284LU),
					  UA_NODEID_NUMERIC(ns[1], 15283LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Direction"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_37_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15284LU));
}

/* NetRxSocket0 - ns=1;i=15181 */

static UA_StatusCode function_tsn_app_model_38_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "NetRxSocket0");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "NetRxSocket0");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15181LU),
					  UA_NODEID_NUMERIC(ns[1], 15142LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NetRxSocket0"), UA_NODEID_NUMERIC(ns[1], 15079LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15181LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_38_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15181LU));
}

/* Id - ns=1;i=15183 */

static UA_StatusCode function_tsn_app_model_39_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Id");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Identifier");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15183LU),
					  UA_NODEID_NUMERIC(ns[1], 15181LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Id"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15183LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_39_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15183LU));
}

/* Frames - ns=1;i=15184 */

static UA_StatusCode function_tsn_app_model_40_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Frames");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Frames number");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15184LU),
					  UA_NODEID_NUMERIC(ns[1], 15181LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Frames"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15184LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_40_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15184LU));
}

/* FramesErr - ns=1;i=15185 */

static UA_StatusCode function_tsn_app_model_41_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "FramesErr");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Frames errors number");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15185LU),
					  UA_NODEID_NUMERIC(ns[1], 15181LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "FramesErr"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15185LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_41_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15185LU));
}

/* Direction - ns=1;i=15182 */

static UA_StatusCode function_tsn_app_model_42_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Direction");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Rx or tx");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15182LU),
					  UA_NODEID_NUMERIC(ns[1], 15181LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Direction"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15182LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_42_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15182LU));
}

/* Id - ns=1;i=15081 */

static UA_StatusCode function_tsn_app_model_43_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Id");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Identifier");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15081LU),
					  UA_NODEID_NUMERIC(ns[1], 15079LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Id"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15081LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_43_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15081LU));
}

/* NetTxSocket0 - ns=1;i=15133 */

static UA_StatusCode function_tsn_app_model_44_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "NetTxSocket0");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "NetTxSocket0");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15133LU),
					  UA_NODEID_NUMERIC(ns[1], 15084LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NetTxSocket0"), UA_NODEID_NUMERIC(ns[1], 15079LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15133LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_44_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15133LU));
}

/* Direction - ns=1;i=15134 */

static UA_StatusCode function_tsn_app_model_45_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Direction");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Rx or tx");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15134LU),
					  UA_NODEID_NUMERIC(ns[1], 15133LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Direction"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15134LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_45_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15134LU));
}

/* Frames - ns=1;i=15136 */

static UA_StatusCode function_tsn_app_model_46_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Frames");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Frames number");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15136LU),
					  UA_NODEID_NUMERIC(ns[1], 15133LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Frames"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15136LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_46_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15136LU));
}

/* FramesErr - ns=1;i=15137 */

static UA_StatusCode function_tsn_app_model_47_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "FramesErr");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Frames errors number");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15137LU),
					  UA_NODEID_NUMERIC(ns[1], 15133LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "FramesErr"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15137LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_47_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15137LU));
}

/* Id - ns=1;i=15135 */

static UA_StatusCode function_tsn_app_model_48_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Id");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Identifier");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15135LU),
					  UA_NODEID_NUMERIC(ns[1], 15133LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Id"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15135LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_48_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15135LU));
}

/* NetTxSocket0 - ns=1;i=15191 */

static UA_StatusCode function_tsn_app_model_49_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "NetTxSocket0");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "NetTxSocket0");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15191LU),
					  UA_NODEID_NUMERIC(ns[1], 15142LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NetTxSocket0"), UA_NODEID_NUMERIC(ns[1], 15079LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15191LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_49_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15191LU));
}

/* FramesErr - ns=1;i=15195 */

static UA_StatusCode function_tsn_app_model_50_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "FramesErr");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Frames errors number");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15195LU),
					  UA_NODEID_NUMERIC(ns[1], 15191LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "FramesErr"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15195LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_50_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15195LU));
}

/* Frames - ns=1;i=15194 */

static UA_StatusCode function_tsn_app_model_51_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Frames");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Frames number");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15194LU),
					  UA_NODEID_NUMERIC(ns[1], 15191LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Frames"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15194LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_51_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15194LU));
}

/* Id - ns=1;i=15193 */

static UA_StatusCode function_tsn_app_model_52_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Id");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Identifier");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15193LU),
					  UA_NODEID_NUMERIC(ns[1], 15191LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Id"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15193LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_52_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15193LU));
}

/* Direction - ns=1;i=15192 */

static UA_StatusCode function_tsn_app_model_53_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Direction");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Rx or tx");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15192LU),
					  UA_NODEID_NUMERIC(ns[1], 15191LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Direction"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15192LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_53_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15192LU));
}

/* FramesErr - ns=1;i=15083 */

static UA_StatusCode function_tsn_app_model_54_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "FramesErr");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Frames errors number");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15083LU),
					  UA_NODEID_NUMERIC(ns[1], 15079LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "FramesErr"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15083LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_54_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15083LU));
}

/* CyclicRxSocketType - ns=1;i=15060 */

static UA_StatusCode function_tsn_app_model_55_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "CyclicRxSocketType");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Application level socket");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECTTYPE, UA_NODEID_NUMERIC(ns[1], 15060LU),
					  UA_NODEID_NUMERIC(ns[0], 58LU), UA_NODEID_NUMERIC(ns[0], 45LU),
					  UA_QUALIFIEDNAME(ns[1], "CyclicRxSocketType"), UA_NODEID_NULL,
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_55_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15060LU));
}

/* Link - ns=1;i=15066 */

static UA_StatusCode function_tsn_app_model_56_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Link");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Link up or down");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15066LU),
					  UA_NODEID_NUMERIC(ns[1], 15060LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Link"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15066LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_56_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15066LU));
}

/* CyclicRxSocket1 - ns=1;i=15104 */

static UA_StatusCode function_tsn_app_model_57_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "CyclicRxSocket1");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "CyclicRxSocket1");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15104LU),
					  UA_NODEID_NUMERIC(ns[1], 15084LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "CyclicRxSocket1"), UA_NODEID_NUMERIC(ns[1], 15060LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15104LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_57_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15104LU));
}

/* ErrId - ns=1;i=15107 */

static UA_StatusCode function_tsn_app_model_58_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ErrId");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Error parameter");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15107LU),
					  UA_NODEID_NUMERIC(ns[1], 15104LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ErrId"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15107LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_58_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15107LU));
}

/* PeerId - ns=1;i=15105 */

static UA_StatusCode function_tsn_app_model_59_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "PeerId");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Peer identifier");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15105LU),
					  UA_NODEID_NUMERIC(ns[1], 15104LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "PeerId"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15105LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_59_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15105LU));
}

/* ErrUnderflow - ns=1;i=15109 */

static UA_StatusCode function_tsn_app_model_60_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ErrUnderflow");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Error parameter");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15109LU),
					  UA_NODEID_NUMERIC(ns[1], 15104LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ErrUnderflow"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15109LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_60_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15109LU));
}

/* ErrTs - ns=1;i=15108 */

static UA_StatusCode function_tsn_app_model_61_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ErrTs");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Error parameter");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15108LU),
					  UA_NODEID_NUMERIC(ns[1], 15104LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ErrTs"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15108LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_61_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15108LU));
}

/* ValidFrames - ns=1;i=15106 */

static UA_StatusCode function_tsn_app_model_62_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ValidFrames");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of valid frames");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15106LU),
					  UA_NODEID_NUMERIC(ns[1], 15104LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ValidFrames"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15106LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_62_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15106LU));
}

/* Link - ns=1;i=15110 */

static UA_StatusCode function_tsn_app_model_63_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Link");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Link up or down");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15110LU),
					  UA_NODEID_NUMERIC(ns[1], 15104LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Link"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15110LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_63_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15110LU));
}

/* CyclicRxSocket1 - ns=1;i=15264 */

static UA_StatusCode function_tsn_app_model_64_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "CyclicRxSocket1");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "CyclicRxSocket1");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15264LU),
					  UA_NODEID_NUMERIC(ns[1], 15244LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "CyclicRxSocket1"), UA_NODEID_NUMERIC(ns[1], 15060LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_64_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15264LU));
}

/* ErrUnderflow - ns=1;i=15269 */

static UA_StatusCode function_tsn_app_model_65_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ErrUnderflow");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Error parameter");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15269LU),
					  UA_NODEID_NUMERIC(ns[1], 15264LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ErrUnderflow"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_65_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15269LU));
}

/* ValidFrames - ns=1;i=15266 */

static UA_StatusCode function_tsn_app_model_66_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ValidFrames");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of valid frames");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15266LU),
					  UA_NODEID_NUMERIC(ns[1], 15264LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ValidFrames"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_66_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15266LU));
}

/* ErrId - ns=1;i=15267 */

static UA_StatusCode function_tsn_app_model_67_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ErrId");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Error parameter");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15267LU),
					  UA_NODEID_NUMERIC(ns[1], 15264LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ErrId"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_67_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15267LU));
}

/* Link - ns=1;i=15270 */

static UA_StatusCode function_tsn_app_model_68_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Link");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Link up or down");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15270LU),
					  UA_NODEID_NUMERIC(ns[1], 15264LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Link"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_68_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15270LU));
}

/* PeerId - ns=1;i=15265 */

static UA_StatusCode function_tsn_app_model_69_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "PeerId");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Peer identifier");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15265LU),
					  UA_NODEID_NUMERIC(ns[1], 15264LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "PeerId"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_69_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15265LU));
}

/* ErrTs - ns=1;i=15268 */

static UA_StatusCode function_tsn_app_model_70_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ErrTs");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Error parameter");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15268LU),
					  UA_NODEID_NUMERIC(ns[1], 15264LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ErrTs"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_70_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15268LU));
}

/* CyclicRxSocket0 - ns=1;i=15245 */

static UA_StatusCode function_tsn_app_model_71_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "CyclicRxSocket0");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "CyclicRxSocket0");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15245LU),
					  UA_NODEID_NUMERIC(ns[1], 15244LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "CyclicRxSocket0"), UA_NODEID_NUMERIC(ns[1], 15060LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_71_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15245LU));
}

/* Link - ns=1;i=15251 */

static UA_StatusCode function_tsn_app_model_72_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Link");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Link up or down");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15251LU),
					  UA_NODEID_NUMERIC(ns[1], 15245LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Link"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_72_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15251LU));
}

/* PeerId - ns=1;i=15246 */

static UA_StatusCode function_tsn_app_model_73_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "PeerId");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Peer identifier");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15246LU),
					  UA_NODEID_NUMERIC(ns[1], 15245LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "PeerId"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_73_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15246LU));
}

/* ValidFrames - ns=1;i=15247 */

static UA_StatusCode function_tsn_app_model_74_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ValidFrames");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of valid frames");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15247LU),
					  UA_NODEID_NUMERIC(ns[1], 15245LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ValidFrames"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_74_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15247LU));
}

/* ErrId - ns=1;i=15248 */

static UA_StatusCode function_tsn_app_model_75_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ErrId");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Error parameter");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15248LU),
					  UA_NODEID_NUMERIC(ns[1], 15245LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ErrId"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_75_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15248LU));
}

/* ErrUnderflow - ns=1;i=15250 */

static UA_StatusCode function_tsn_app_model_76_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ErrUnderflow");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Error parameter");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15250LU),
					  UA_NODEID_NUMERIC(ns[1], 15245LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ErrUnderflow"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_76_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15250LU));
}

/* ErrTs - ns=1;i=15249 */

static UA_StatusCode function_tsn_app_model_77_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ErrTs");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Error parameter");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15249LU),
					  UA_NODEID_NUMERIC(ns[1], 15245LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ErrTs"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_77_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15249LU));
}

/* CyclicRxSocket0 - ns=1;i=15143 */

static UA_StatusCode function_tsn_app_model_78_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "CyclicRxSocket0");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "CyclicRxSocket0");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15143LU),
					  UA_NODEID_NUMERIC(ns[1], 15142LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "CyclicRxSocket0"), UA_NODEID_NUMERIC(ns[1], 15060LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15143LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_78_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15143LU));
}

/* ErrTs - ns=1;i=15147 */

static UA_StatusCode function_tsn_app_model_79_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ErrTs");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Error parameter");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15147LU),
					  UA_NODEID_NUMERIC(ns[1], 15143LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ErrTs"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15147LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_79_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15147LU));
}

/* ErrUnderflow - ns=1;i=15148 */

static UA_StatusCode function_tsn_app_model_80_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ErrUnderflow");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Error parameter");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15148LU),
					  UA_NODEID_NUMERIC(ns[1], 15143LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ErrUnderflow"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15148LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_80_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15148LU));
}

/* PeerId - ns=1;i=15144 */

static UA_StatusCode function_tsn_app_model_81_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "PeerId");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Peer identifier");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15144LU),
					  UA_NODEID_NUMERIC(ns[1], 15143LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "PeerId"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15144LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_81_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15144LU));
}

/* ValidFrames - ns=1;i=15145 */

static UA_StatusCode function_tsn_app_model_82_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ValidFrames");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of valid frames");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15145LU),
					  UA_NODEID_NUMERIC(ns[1], 15143LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ValidFrames"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15145LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_82_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15145LU));
}

/* Link - ns=1;i=15149 */

static UA_StatusCode function_tsn_app_model_83_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Link");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Link up or down");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15149LU),
					  UA_NODEID_NUMERIC(ns[1], 15143LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Link"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15149LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_83_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15149LU));
}

/* ErrId - ns=1;i=15146 */

static UA_StatusCode function_tsn_app_model_84_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ErrId");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Error parameter");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15146LU),
					  UA_NODEID_NUMERIC(ns[1], 15143LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ErrId"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15146LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_84_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15146LU));
}

/* ErrId - ns=1;i=15063 */

static UA_StatusCode function_tsn_app_model_85_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ErrId");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Error parameter");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15063LU),
					  UA_NODEID_NUMERIC(ns[1], 15060LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ErrId"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15063LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_85_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15063LU));
}

/* ErrUnderflow - ns=1;i=15065 */

static UA_StatusCode function_tsn_app_model_86_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ErrUnderflow");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Error parameter");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15065LU),
					  UA_NODEID_NUMERIC(ns[1], 15060LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ErrUnderflow"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15065LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_86_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15065LU));
}

/* ValidFrames - ns=1;i=15062 */

static UA_StatusCode function_tsn_app_model_87_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ValidFrames");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of valid frames");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15062LU),
					  UA_NODEID_NUMERIC(ns[1], 15060LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ValidFrames"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15062LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_87_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15062LU));
}

/* PeerId - ns=1;i=15061 */

static UA_StatusCode function_tsn_app_model_88_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "PeerId");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Peer identifier");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15061LU),
					  UA_NODEID_NUMERIC(ns[1], 15060LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "PeerId"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15061LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_88_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15061LU));
}

/* CyclicRxSocket1 - ns=1;i=15162 */

static UA_StatusCode function_tsn_app_model_89_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "CyclicRxSocket1");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "CyclicRxSocket1");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15162LU),
					  UA_NODEID_NUMERIC(ns[1], 15142LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "CyclicRxSocket1"), UA_NODEID_NUMERIC(ns[1], 15060LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15162LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_89_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15162LU));
}

/* PeerId - ns=1;i=15163 */

static UA_StatusCode function_tsn_app_model_90_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "PeerId");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Peer identifier");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15163LU),
					  UA_NODEID_NUMERIC(ns[1], 15162LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "PeerId"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15163LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_90_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15163LU));
}

/* ValidFrames - ns=1;i=15164 */

static UA_StatusCode function_tsn_app_model_91_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ValidFrames");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of valid frames");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15164LU),
					  UA_NODEID_NUMERIC(ns[1], 15162LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ValidFrames"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15164LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_91_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15164LU));
}

/* Link - ns=1;i=15168 */

static UA_StatusCode function_tsn_app_model_92_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Link");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Link up or down");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15168LU),
					  UA_NODEID_NUMERIC(ns[1], 15162LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Link"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15168LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_92_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15168LU));
}

/* ErrId - ns=1;i=15165 */

static UA_StatusCode function_tsn_app_model_93_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ErrId");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Error parameter");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15165LU),
					  UA_NODEID_NUMERIC(ns[1], 15162LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ErrId"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15165LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_93_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15165LU));
}

/* ErrTs - ns=1;i=15166 */

static UA_StatusCode function_tsn_app_model_94_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ErrTs");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Error parameter");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15166LU),
					  UA_NODEID_NUMERIC(ns[1], 15162LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ErrTs"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15166LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_94_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15166LU));
}

/* ErrUnderflow - ns=1;i=15167 */

static UA_StatusCode function_tsn_app_model_95_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ErrUnderflow");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Error parameter");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15167LU),
					  UA_NODEID_NUMERIC(ns[1], 15162LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ErrUnderflow"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15167LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_95_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15167LU));
}

/* CyclicRxSocket0 - ns=1;i=15085 */

static UA_StatusCode function_tsn_app_model_96_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "CyclicRxSocket0");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "CyclicRxSocket0");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15085LU),
					  UA_NODEID_NUMERIC(ns[1], 15084LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "CyclicRxSocket0"), UA_NODEID_NUMERIC(ns[1], 15060LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15085LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_96_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15085LU));
}

/* Link - ns=1;i=15091 */

static UA_StatusCode function_tsn_app_model_97_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Link");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Link up or down");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15091LU),
					  UA_NODEID_NUMERIC(ns[1], 15085LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Link"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15091LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_97_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15091LU));
}

/* ErrTs - ns=1;i=15089 */

static UA_StatusCode function_tsn_app_model_98_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ErrTs");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Error parameter");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15089LU),
					  UA_NODEID_NUMERIC(ns[1], 15085LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ErrTs"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15089LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_98_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15089LU));
}

/* PeerId - ns=1;i=15086 */

static UA_StatusCode function_tsn_app_model_99_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "PeerId");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Peer identifier");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15086LU),
					  UA_NODEID_NUMERIC(ns[1], 15085LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "PeerId"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15086LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_99_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15086LU));
}

/* ValidFrames - ns=1;i=15087 */

static UA_StatusCode function_tsn_app_model_100_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ValidFrames");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of valid frames");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15087LU),
					  UA_NODEID_NUMERIC(ns[1], 15085LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ValidFrames"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15087LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_100_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15087LU));
}

/* ErrUnderflow - ns=1;i=15090 */

static UA_StatusCode function_tsn_app_model_101_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ErrUnderflow");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Error parameter");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15090LU),
					  UA_NODEID_NUMERIC(ns[1], 15085LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ErrUnderflow"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15090LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_101_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15090LU));
}

/* ErrId - ns=1;i=15088 */

static UA_StatusCode function_tsn_app_model_102_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ErrId");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Error parameter");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15088LU),
					  UA_NODEID_NUMERIC(ns[1], 15085LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ErrId"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15088LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_102_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15088LU));
}

/* ErrTs - ns=1;i=15064 */

static UA_StatusCode function_tsn_app_model_103_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ErrTs");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Error parameter");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15064LU),
					  UA_NODEID_NUMERIC(ns[1], 15060LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ErrTs"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15064LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_103_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15064LU));
}

/* TaskStatsType - ns=1;i=15016 */

static UA_StatusCode function_tsn_app_model_104_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TaskStatsType");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Statistics of the task");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECTTYPE, UA_NODEID_NUMERIC(ns[1], 15016LU),
					  UA_NODEID_NUMERIC(ns[0], 58LU), UA_NODEID_NUMERIC(ns[0], 45LU),
					  UA_QUALIFIEDNAME(ns[1], "TaskStatsType"), UA_NODEID_NULL,
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_104_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15016LU));
}

/* SchedTimeout - ns=1;i=15021 */

static UA_StatusCode function_tsn_app_model_105_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SchedTimeout");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "SchedTimeout");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15021LU),
					  UA_NODEID_NUMERIC(ns[1], 15016LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SchedTimeout"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15021LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_105_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15021LU));
}

/* Sched - ns=1;i=15017 */

static UA_StatusCode function_tsn_app_model_106_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Sched");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Sched");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15017LU),
					  UA_NODEID_NUMERIC(ns[1], 15016LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Sched"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15017LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_106_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15017LU));
}

/* ClockDiscount - ns=1;i=15022 */

static UA_StatusCode function_tsn_app_model_107_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ClockDiscount");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "ClockDiscount");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15022LU),
					  UA_NODEID_NUMERIC(ns[1], 15016LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ClockDiscount"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15022LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_107_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15022LU));
}

/* TaskStats - ns=1;i=15298 */

static UA_StatusCode function_tsn_app_model_108_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TaskStats");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Statistics");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15298LU),
					  UA_NODEID_NUMERIC(ns[1], 15240LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "TaskStats"), UA_NODEID_NUMERIC(ns[1], 15016LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_108_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15298LU));
}

/* ClockErr - ns=1;i=15305 */

static UA_StatusCode function_tsn_app_model_109_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ClockErr");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "ClockErr");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15305LU),
					  UA_NODEID_NUMERIC(ns[1], 15298LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ClockErr"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_109_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15305LU));
}

/* Sched - ns=1;i=15299 */

static UA_StatusCode function_tsn_app_model_110_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Sched");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Sched");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15299LU),
					  UA_NODEID_NUMERIC(ns[1], 15298LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Sched"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_110_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15299LU));
}

/* ClockDiscount - ns=1;i=15304 */

static UA_StatusCode function_tsn_app_model_111_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ClockDiscount");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "ClockDiscount");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15304LU),
					  UA_NODEID_NUMERIC(ns[1], 15298LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ClockDiscount"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_111_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15304LU));
}

/* SchedEarly - ns=1;i=15300 */

static UA_StatusCode function_tsn_app_model_112_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SchedEarly");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "SchedEarly");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15300LU),
					  UA_NODEID_NUMERIC(ns[1], 15298LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SchedEarly"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_112_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15300LU));
}

/* SchedMissed - ns=1;i=15302 */

static UA_StatusCode function_tsn_app_model_113_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SchedMissed");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "SchedMissed");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15302LU),
					  UA_NODEID_NUMERIC(ns[1], 15298LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SchedMissed"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_113_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15302LU));
}

/* SchedLate - ns=1;i=15301 */

static UA_StatusCode function_tsn_app_model_114_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SchedLate");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "SchedLate");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15301LU),
					  UA_NODEID_NUMERIC(ns[1], 15298LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SchedLate"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_114_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15301LU));
}

/* SchedTimeout - ns=1;i=15303 */

static UA_StatusCode function_tsn_app_model_115_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SchedTimeout");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "SchedTimeout");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15303LU),
					  UA_NODEID_NUMERIC(ns[1], 15298LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SchedTimeout"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_115_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15303LU));
}

/* ClockErr - ns=1;i=15023 */

static UA_StatusCode function_tsn_app_model_116_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ClockErr");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "ClockErr");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15023LU),
					  UA_NODEID_NUMERIC(ns[1], 15016LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ClockErr"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15023LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_116_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15023LU));
}

/* SchedEarly - ns=1;i=15018 */

static UA_StatusCode function_tsn_app_model_117_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SchedEarly");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "SchedEarly");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15018LU),
					  UA_NODEID_NUMERIC(ns[1], 15016LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SchedEarly"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15018LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_117_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15018LU));
}

/* SchedMissed - ns=1;i=15020 */

static UA_StatusCode function_tsn_app_model_118_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SchedMissed");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "SchedMissed");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15020LU),
					  UA_NODEID_NUMERIC(ns[1], 15016LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SchedMissed"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15020LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_118_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15020LU));
}

/* SchedLate - ns=1;i=15019 */

static UA_StatusCode function_tsn_app_model_119_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SchedLate");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "SchedLate");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15019LU),
					  UA_NODEID_NUMERIC(ns[1], 15016LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SchedLate"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15019LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_119_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15019LU));
}

/* TaskStats - ns=1;i=15196 */

static UA_StatusCode function_tsn_app_model_120_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TaskStats");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Statistics");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15196LU),
					  UA_NODEID_NUMERIC(ns[1], 15138LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "TaskStats"), UA_NODEID_NUMERIC(ns[1], 15016LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15196LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_120_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15196LU));
}

/* SchedLate - ns=1;i=15199 */

static UA_StatusCode function_tsn_app_model_121_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SchedLate");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "SchedLate");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15199LU),
					  UA_NODEID_NUMERIC(ns[1], 15196LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SchedLate"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15199LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_121_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15199LU));
}

/* ClockDiscount - ns=1;i=15202 */

static UA_StatusCode function_tsn_app_model_122_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ClockDiscount");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "ClockDiscount");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15202LU),
					  UA_NODEID_NUMERIC(ns[1], 15196LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ClockDiscount"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15202LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_122_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15202LU));
}

/* SchedEarly - ns=1;i=15198 */

static UA_StatusCode function_tsn_app_model_123_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SchedEarly");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "SchedEarly");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15198LU),
					  UA_NODEID_NUMERIC(ns[1], 15196LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SchedEarly"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15198LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_123_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15198LU));
}

/* ClockErr - ns=1;i=15203 */

static UA_StatusCode function_tsn_app_model_124_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "ClockErr");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "ClockErr");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15203LU),
					  UA_NODEID_NUMERIC(ns[1], 15196LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ClockErr"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15203LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_124_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15203LU));
}

/* Sched - ns=1;i=15197 */

static UA_StatusCode function_tsn_app_model_125_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Sched");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Sched");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15197LU),
					  UA_NODEID_NUMERIC(ns[1], 15196LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Sched"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15197LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_125_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15197LU));
}

/* SchedMissed - ns=1;i=15200 */

static UA_StatusCode function_tsn_app_model_126_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SchedMissed");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "SchedMissed");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15200LU),
					  UA_NODEID_NUMERIC(ns[1], 15196LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SchedMissed"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15200LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_126_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15200LU));
}

/* SchedTimeout - ns=1;i=15201 */

static UA_StatusCode function_tsn_app_model_127_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SchedTimeout");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "SchedTimeout");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15201LU),
					  UA_NODEID_NUMERIC(ns[1], 15196LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SchedTimeout"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15201LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_127_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15201LU));
}

/* ConfigurationType - ns=1;i=15013 */

static UA_StatusCode function_tsn_app_model_128_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "ConfigurationType");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Base type for the app configuration");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECTTYPE, UA_NODEID_NUMERIC(ns[1], 15013LU),
					  UA_NODEID_NUMERIC(ns[0], 58LU), UA_NODEID_NUMERIC(ns[0], 45LU),
					  UA_QUALIFIEDNAME(ns[1], "ConfigurationType"), UA_NODEID_NULL,
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_128_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15013LU));
}

/* Role - ns=1;i=15014 */

static UA_StatusCode function_tsn_app_model_129_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Role");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Role of the endpoint : controller or io_device");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15014LU),
					  UA_NODEID_NUMERIC(ns[1], 15013LU), UA_NODEID_NUMERIC(ns[0], 46LU),
					  UA_QUALIFIEDNAME(ns[1], "Role"), UA_NODEID_NUMERIC(ns[0], 68LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15014LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_129_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15014LU));
}

/* Configuration - ns=1;i=15241 */

static UA_StatusCode function_tsn_app_model_130_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "Configuration");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Configuration for the app");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15241LU),
					  UA_NODEID_NUMERIC(ns[1], 15240LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Configuration"), UA_NODEID_NUMERIC(ns[1], 15013LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_130_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15241LU));
}

/* NumPeers - ns=1;i=15243 */

static UA_StatusCode function_tsn_app_model_131_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "NumPeers");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of peers");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15243LU),
					  UA_NODEID_NUMERIC(ns[1], 15241LU), UA_NODEID_NUMERIC(ns[0], 46LU),
					  UA_QUALIFIEDNAME(ns[1], "NumPeers"), UA_NODEID_NUMERIC(ns[0], 68LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_131_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15243LU));
}

/* Role - ns=1;i=15242 */

static UA_StatusCode function_tsn_app_model_132_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Role");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Role of the endpoint : controller or io_device");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15242LU),
					  UA_NODEID_NUMERIC(ns[1], 15241LU), UA_NODEID_NUMERIC(ns[0], 46LU),
					  UA_QUALIFIEDNAME(ns[1], "Role"), UA_NODEID_NUMERIC(ns[0], 68LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_132_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15242LU));
}

/* NumPeers - ns=1;i=15015 */

static UA_StatusCode function_tsn_app_model_133_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "NumPeers");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of peers");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15015LU),
					  UA_NODEID_NUMERIC(ns[1], 15013LU), UA_NODEID_NUMERIC(ns[0], 46LU),
					  UA_QUALIFIEDNAME(ns[1], "NumPeers"), UA_NODEID_NUMERIC(ns[0], 68LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15015LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_133_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15015LU));
}

/* Configuration - ns=1;i=15139 */

static UA_StatusCode function_tsn_app_model_134_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "Configuration");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Configuration for the app");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15139LU),
					  UA_NODEID_NUMERIC(ns[1], 15138LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Configuration"), UA_NODEID_NUMERIC(ns[1], 15013LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15139LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_134_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15139LU));
}

/* Role - ns=1;i=15140 */

static UA_StatusCode function_tsn_app_model_135_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 12LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Role");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Role of the endpoint : controller or io_device");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15140LU),
					  UA_NODEID_NUMERIC(ns[1], 15139LU), UA_NODEID_NUMERIC(ns[0], 46LU),
					  UA_QUALIFIEDNAME(ns[1], "Role"), UA_NODEID_NUMERIC(ns[0], 68LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15140LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_135_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15140LU));
}

/* NumPeers - ns=1;i=15141 */

static UA_StatusCode function_tsn_app_model_136_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "NumPeers");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of peers");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15141LU),
					  UA_NODEID_NUMERIC(ns[1], 15139LU), UA_NODEID_NUMERIC(ns[0], 46LU),
					  UA_QUALIFIEDNAME(ns[1], "NumPeers"), UA_NODEID_NUMERIC(ns[0], 68LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15141LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_136_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15141LU));
}

/* HistogramType - ns=1;i=15009 */

static UA_StatusCode function_tsn_app_model_137_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "HistogramType");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Base type for all histograms");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECTTYPE, UA_NODEID_NUMERIC(ns[1], 15009LU),
					  UA_NODEID_NUMERIC(ns[0], 58LU), UA_NODEID_NUMERIC(ns[0], 45LU),
					  UA_QUALIFIEDNAME(ns[1], "HistogramType"), UA_NODEID_NULL,
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_137_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15009LU));
}

/* SchedErrHisto - ns=1;i=15314 */

static UA_StatusCode function_tsn_app_model_138_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "SchedErrHisto");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "SchedErrHisto");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15314LU),
					  UA_NODEID_NUMERIC(ns[1], 15298LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SchedErrHisto"), UA_NODEID_NUMERIC(ns[1], 15009LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_138_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15314LU));
}

/* SlotSize - ns=1;i=15316 */

static UA_StatusCode function_tsn_app_model_139_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SlotSize");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Slot size");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15316LU),
					  UA_NODEID_NUMERIC(ns[1], 15314LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SlotSize"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_139_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15316LU));
}

/* Slots - ns=1;i=15317 */

static UA_StatusCode function_tsn_app_model_140_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	attr.valueRank = 1;
	attr.arrayDimensionsSize = 1;
	UA_UInt32 arrayDimensions[1];
	arrayDimensions[0] = 0;
	attr.arrayDimensions = &arrayDimensions[0];
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Slots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Array for the repartition of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15317LU),
					  UA_NODEID_NUMERIC(ns[1], 15314LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Slots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_140_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15317LU));
}

/* NSlots - ns=1;i=15315 */

static UA_StatusCode function_tsn_app_model_141_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "NSlots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of slots");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15315LU),
					  UA_NODEID_NUMERIC(ns[1], 15314LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NSlots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_141_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15315LU));
}

/* ProcTimeHisto - ns=1;i=15224 */

static UA_StatusCode function_tsn_app_model_142_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "ProcTimeHisto");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Processing time histogram");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15224LU),
					  UA_NODEID_NUMERIC(ns[1], 15196LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ProcTimeHisto"), UA_NODEID_NUMERIC(ns[1], 15009LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15224LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_142_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15224LU));
}

/* SlotSize - ns=1;i=15226 */

static UA_StatusCode function_tsn_app_model_143_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SlotSize");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Slot size");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15226LU),
					  UA_NODEID_NUMERIC(ns[1], 15224LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SlotSize"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15226LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_143_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15226LU));
}

/* Slots - ns=1;i=15227 */

static UA_StatusCode function_tsn_app_model_144_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	attr.valueRank = 1;
	attr.arrayDimensionsSize = 1;
	UA_UInt32 arrayDimensions[1];
	arrayDimensions[0] = 0;
	attr.arrayDimensions = &arrayDimensions[0];
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Slots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Array for the repartition of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15227LU),
					  UA_NODEID_NUMERIC(ns[1], 15224LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Slots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15227LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_144_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15227LU));
}

/* NSlots - ns=1;i=15225 */

static UA_StatusCode function_tsn_app_model_145_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "NSlots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of slots");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15225LU),
					  UA_NODEID_NUMERIC(ns[1], 15224LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NSlots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15225LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_145_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15225LU));
}

/* TrafficLatencyHisto - ns=1;i=15260 */

static UA_StatusCode function_tsn_app_model_146_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TrafficLatencyHisto");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Traffic latency histogram");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15260LU),
					  UA_NODEID_NUMERIC(ns[1], 15245LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "TrafficLatencyHisto"),
					  UA_NODEID_NUMERIC(ns[1], 15009LU), (const UA_NodeAttributes *)&attr,
					  &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_146_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15260LU));
}

/* SlotSize - ns=1;i=15262 */

static UA_StatusCode function_tsn_app_model_147_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SlotSize");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Slot size");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15262LU),
					  UA_NODEID_NUMERIC(ns[1], 15260LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SlotSize"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_147_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15262LU));
}

/* Slots - ns=1;i=15263 */

static UA_StatusCode function_tsn_app_model_148_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	attr.valueRank = 1;
	attr.arrayDimensionsSize = 1;
	UA_UInt32 arrayDimensions[1];
	arrayDimensions[0] = 0;
	attr.arrayDimensions = &arrayDimensions[0];
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Slots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Array for the repartition of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15263LU),
					  UA_NODEID_NUMERIC(ns[1], 15260LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Slots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_148_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15263LU));
}

/* NSlots - ns=1;i=15261 */

static UA_StatusCode function_tsn_app_model_149_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "NSlots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of slots");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15261LU),
					  UA_NODEID_NUMERIC(ns[1], 15260LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NSlots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_149_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15261LU));
}

/* ProcTimeHisto - ns=1;i=15044 */

static UA_StatusCode function_tsn_app_model_150_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "ProcTimeHisto");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Processing time histogram");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15044LU),
					  UA_NODEID_NUMERIC(ns[1], 15016LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ProcTimeHisto"), UA_NODEID_NUMERIC(ns[1], 15009LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15044LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_150_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15044LU));
}

/* Slots - ns=1;i=15047 */

static UA_StatusCode function_tsn_app_model_151_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	attr.valueRank = 1;
	attr.arrayDimensionsSize = 1;
	UA_UInt32 arrayDimensions[1];
	arrayDimensions[0] = 0;
	attr.arrayDimensions = &arrayDimensions[0];
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Slots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Array for the repartition of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15047LU),
					  UA_NODEID_NUMERIC(ns[1], 15044LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Slots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15047LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_151_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15047LU));
}

/* NSlots - ns=1;i=15045 */

static UA_StatusCode function_tsn_app_model_152_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "NSlots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of slots");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15045LU),
					  UA_NODEID_NUMERIC(ns[1], 15044LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NSlots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15045LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_152_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15045LU));
}

/* SlotSize - ns=1;i=15046 */

static UA_StatusCode function_tsn_app_model_153_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SlotSize");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Slot size");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15046LU),
					  UA_NODEID_NUMERIC(ns[1], 15044LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SlotSize"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15046LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_153_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15046LU));
}

/* TrafficLatencyHisto - ns=1;i=15075 */

static UA_StatusCode function_tsn_app_model_154_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TrafficLatencyHisto");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Traffic latency histogram");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15075LU),
					  UA_NODEID_NUMERIC(ns[1], 15060LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "TrafficLatencyHisto"),
					  UA_NODEID_NUMERIC(ns[1], 15009LU), (const UA_NodeAttributes *)&attr,
					  &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15075LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_154_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15075LU));
}

/* NSlots - ns=1;i=15076 */

static UA_StatusCode function_tsn_app_model_155_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "NSlots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of slots");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15076LU),
					  UA_NODEID_NUMERIC(ns[1], 15075LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NSlots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15076LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_155_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15076LU));
}

/* SlotSize - ns=1;i=15077 */

static UA_StatusCode function_tsn_app_model_156_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SlotSize");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Slot size");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15077LU),
					  UA_NODEID_NUMERIC(ns[1], 15075LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SlotSize"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15077LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_156_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15077LU));
}

/* Slots - ns=1;i=15078 */

static UA_StatusCode function_tsn_app_model_157_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	attr.valueRank = 1;
	attr.arrayDimensionsSize = 1;
	UA_UInt32 arrayDimensions[1];
	arrayDimensions[0] = 0;
	attr.arrayDimensions = &arrayDimensions[0];
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Slots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Array for the repartition of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15078LU),
					  UA_NODEID_NUMERIC(ns[1], 15075LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Slots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15078LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_157_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15078LU));
}

/* TrafficLatencyHisto - ns=1;i=15119 */

static UA_StatusCode function_tsn_app_model_158_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TrafficLatencyHisto");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Traffic latency histogram");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15119LU),
					  UA_NODEID_NUMERIC(ns[1], 15104LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "TrafficLatencyHisto"),
					  UA_NODEID_NUMERIC(ns[1], 15009LU), (const UA_NodeAttributes *)&attr,
					  &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15119LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_158_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15119LU));
}

/* SlotSize - ns=1;i=15121 */

static UA_StatusCode function_tsn_app_model_159_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SlotSize");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Slot size");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15121LU),
					  UA_NODEID_NUMERIC(ns[1], 15119LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SlotSize"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15121LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_159_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15121LU));
}

/* Slots - ns=1;i=15122 */

static UA_StatusCode function_tsn_app_model_160_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	attr.valueRank = 1;
	attr.arrayDimensionsSize = 1;
	UA_UInt32 arrayDimensions[1];
	arrayDimensions[0] = 0;
	attr.arrayDimensions = &arrayDimensions[0];
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Slots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Array for the repartition of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15122LU),
					  UA_NODEID_NUMERIC(ns[1], 15119LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Slots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15122LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_160_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15122LU));
}

/* NSlots - ns=1;i=15120 */

static UA_StatusCode function_tsn_app_model_161_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "NSlots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of slots");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15120LU),
					  UA_NODEID_NUMERIC(ns[1], 15119LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NSlots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15120LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_161_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15120LU));
}

/* TrafficLatencyHisto - ns=1;i=15279 */

static UA_StatusCode function_tsn_app_model_162_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TrafficLatencyHisto");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Traffic latency histogram");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15279LU),
					  UA_NODEID_NUMERIC(ns[1], 15264LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "TrafficLatencyHisto"),
					  UA_NODEID_NUMERIC(ns[1], 15009LU), (const UA_NodeAttributes *)&attr,
					  &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_162_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15279LU));
}

/* Slots - ns=1;i=15282 */

static UA_StatusCode function_tsn_app_model_163_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	attr.valueRank = 1;
	attr.arrayDimensionsSize = 1;
	UA_UInt32 arrayDimensions[1];
	arrayDimensions[0] = 0;
	attr.arrayDimensions = &arrayDimensions[0];
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Slots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Array for the repartition of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15282LU),
					  UA_NODEID_NUMERIC(ns[1], 15279LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Slots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_163_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15282LU));
}

/* SlotSize - ns=1;i=15281 */

static UA_StatusCode function_tsn_app_model_164_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SlotSize");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Slot size");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15281LU),
					  UA_NODEID_NUMERIC(ns[1], 15279LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SlotSize"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_164_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15281LU));
}

/* NSlots - ns=1;i=15280 */

static UA_StatusCode function_tsn_app_model_165_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "NSlots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of slots");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15280LU),
					  UA_NODEID_NUMERIC(ns[1], 15279LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NSlots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_165_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15280LU));
}

/* TotalTimeHisto - ns=1;i=15236 */

static UA_StatusCode function_tsn_app_model_166_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TotalTimeHisto");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Total time histogram");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15236LU),
					  UA_NODEID_NUMERIC(ns[1], 15196LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "TotalTimeHisto"), UA_NODEID_NUMERIC(ns[1], 15009LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15236LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_166_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15236LU));
}

/* NSlots - ns=1;i=15237 */

static UA_StatusCode function_tsn_app_model_167_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "NSlots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of slots");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15237LU),
					  UA_NODEID_NUMERIC(ns[1], 15236LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NSlots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15237LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_167_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15237LU));
}

/* Slots - ns=1;i=15239 */

static UA_StatusCode function_tsn_app_model_168_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	attr.valueRank = 1;
	attr.arrayDimensionsSize = 1;
	UA_UInt32 arrayDimensions[1];
	arrayDimensions[0] = 0;
	attr.arrayDimensions = &arrayDimensions[0];
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Slots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Array for the repartition of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15239LU),
					  UA_NODEID_NUMERIC(ns[1], 15236LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Slots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15239LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_168_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15239LU));
}

/* SlotSize - ns=1;i=15238 */

static UA_StatusCode function_tsn_app_model_169_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SlotSize");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Slot size");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15238LU),
					  UA_NODEID_NUMERIC(ns[1], 15236LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SlotSize"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15238LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_169_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15238LU));
}

/* Slots - ns=1;i=15012 */

static UA_StatusCode function_tsn_app_model_170_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	attr.valueRank = 1;
	attr.arrayDimensionsSize = 1;
	UA_UInt32 arrayDimensions[1];
	arrayDimensions[0] = 0;
	attr.arrayDimensions = &arrayDimensions[0];
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Slots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Array for the repartition of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15012LU),
					  UA_NODEID_NUMERIC(ns[1], 15009LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Slots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15012LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_170_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15012LU));
}

/* TrafficLatencyHisto - ns=1;i=15177 */

static UA_StatusCode function_tsn_app_model_171_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TrafficLatencyHisto");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Traffic latency histogram");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15177LU),
					  UA_NODEID_NUMERIC(ns[1], 15162LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "TrafficLatencyHisto"),
					  UA_NODEID_NUMERIC(ns[1], 15009LU), (const UA_NodeAttributes *)&attr,
					  &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15177LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_171_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15177LU));
}

/* SlotSize - ns=1;i=15179 */

static UA_StatusCode function_tsn_app_model_172_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SlotSize");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Slot size");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15179LU),
					  UA_NODEID_NUMERIC(ns[1], 15177LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SlotSize"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15179LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_172_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15179LU));
}

/* NSlots - ns=1;i=15178 */

static UA_StatusCode function_tsn_app_model_173_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "NSlots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of slots");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15178LU),
					  UA_NODEID_NUMERIC(ns[1], 15177LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NSlots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15178LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_173_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15178LU));
}

/* Slots - ns=1;i=15180 */

static UA_StatusCode function_tsn_app_model_174_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	attr.valueRank = 1;
	attr.arrayDimensionsSize = 1;
	UA_UInt32 arrayDimensions[1];
	arrayDimensions[0] = 0;
	attr.arrayDimensions = &arrayDimensions[0];
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Slots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Array for the repartition of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15180LU),
					  UA_NODEID_NUMERIC(ns[1], 15177LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Slots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15180LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_174_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15180LU));
}

/* NSlots - ns=1;i=15010 */

static UA_StatusCode function_tsn_app_model_175_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "NSlots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of slots");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15010LU),
					  UA_NODEID_NUMERIC(ns[1], 15009LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NSlots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15010LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_175_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15010LU));
}

/* ProcTimeHisto - ns=1;i=15326 */

static UA_StatusCode function_tsn_app_model_176_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "ProcTimeHisto");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Processing time histogram");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15326LU),
					  UA_NODEID_NUMERIC(ns[1], 15298LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ProcTimeHisto"), UA_NODEID_NUMERIC(ns[1], 15009LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_176_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15326LU));
}

/* Slots - ns=1;i=15329 */

static UA_StatusCode function_tsn_app_model_177_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	attr.valueRank = 1;
	attr.arrayDimensionsSize = 1;
	UA_UInt32 arrayDimensions[1];
	arrayDimensions[0] = 0;
	attr.arrayDimensions = &arrayDimensions[0];
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Slots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Array for the repartition of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15329LU),
					  UA_NODEID_NUMERIC(ns[1], 15326LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Slots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_177_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15329LU));
}

/* NSlots - ns=1;i=15327 */

static UA_StatusCode function_tsn_app_model_178_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "NSlots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of slots");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15327LU),
					  UA_NODEID_NUMERIC(ns[1], 15326LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NSlots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_178_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15327LU));
}

/* SlotSize - ns=1;i=15328 */

static UA_StatusCode function_tsn_app_model_179_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SlotSize");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Slot size");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15328LU),
					  UA_NODEID_NUMERIC(ns[1], 15326LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SlotSize"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_179_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15328LU));
}

/* TrafficLatencyHisto - ns=1;i=15158 */

static UA_StatusCode function_tsn_app_model_180_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TrafficLatencyHisto");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Traffic latency histogram");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15158LU),
					  UA_NODEID_NUMERIC(ns[1], 15143LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "TrafficLatencyHisto"),
					  UA_NODEID_NUMERIC(ns[1], 15009LU), (const UA_NodeAttributes *)&attr,
					  &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15158LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_180_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15158LU));
}

/* Slots - ns=1;i=15161 */

static UA_StatusCode function_tsn_app_model_181_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	attr.valueRank = 1;
	attr.arrayDimensionsSize = 1;
	UA_UInt32 arrayDimensions[1];
	arrayDimensions[0] = 0;
	attr.arrayDimensions = &arrayDimensions[0];
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Slots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Array for the repartition of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15161LU),
					  UA_NODEID_NUMERIC(ns[1], 15158LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Slots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15161LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_181_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15161LU));
}

/* NSlots - ns=1;i=15159 */

static UA_StatusCode function_tsn_app_model_182_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "NSlots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of slots");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15159LU),
					  UA_NODEID_NUMERIC(ns[1], 15158LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NSlots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15159LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_182_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15159LU));
}

/* SlotSize - ns=1;i=15160 */

static UA_StatusCode function_tsn_app_model_183_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SlotSize");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Slot size");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15160LU),
					  UA_NODEID_NUMERIC(ns[1], 15158LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SlotSize"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15160LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_183_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15160LU));
}

/* SlotSize - ns=1;i=15011 */

static UA_StatusCode function_tsn_app_model_184_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SlotSize");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Slot size");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15011LU),
					  UA_NODEID_NUMERIC(ns[1], 15009LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SlotSize"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15011LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_184_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15011LU));
}

/* TotalTimeHisto - ns=1;i=15056 */

static UA_StatusCode function_tsn_app_model_185_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TotalTimeHisto");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Total time histogram");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15056LU),
					  UA_NODEID_NUMERIC(ns[1], 15016LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "TotalTimeHisto"), UA_NODEID_NUMERIC(ns[1], 15009LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15056LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_185_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15056LU));
}

/* Slots - ns=1;i=15059 */

static UA_StatusCode function_tsn_app_model_186_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	attr.valueRank = 1;
	attr.arrayDimensionsSize = 1;
	UA_UInt32 arrayDimensions[1];
	arrayDimensions[0] = 0;
	attr.arrayDimensions = &arrayDimensions[0];
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Slots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Array for the repartition of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15059LU),
					  UA_NODEID_NUMERIC(ns[1], 15056LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Slots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15059LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_186_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15059LU));
}

/* SlotSize - ns=1;i=15058 */

static UA_StatusCode function_tsn_app_model_187_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SlotSize");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Slot size");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15058LU),
					  UA_NODEID_NUMERIC(ns[1], 15056LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SlotSize"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15058LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_187_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15058LU));
}

/* NSlots - ns=1;i=15057 */

static UA_StatusCode function_tsn_app_model_188_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "NSlots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of slots");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15057LU),
					  UA_NODEID_NUMERIC(ns[1], 15056LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NSlots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15057LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_188_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15057LU));
}

/* SchedErrHisto - ns=1;i=15212 */

static UA_StatusCode function_tsn_app_model_189_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "SchedErrHisto");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "SchedErrHisto");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15212LU),
					  UA_NODEID_NUMERIC(ns[1], 15196LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SchedErrHisto"), UA_NODEID_NUMERIC(ns[1], 15009LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15212LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_189_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15212LU));
}

/* NSlots - ns=1;i=15213 */

static UA_StatusCode function_tsn_app_model_190_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "NSlots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of slots");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15213LU),
					  UA_NODEID_NUMERIC(ns[1], 15212LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NSlots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15213LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_190_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15213LU));
}

/* SlotSize - ns=1;i=15214 */

static UA_StatusCode function_tsn_app_model_191_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SlotSize");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Slot size");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15214LU),
					  UA_NODEID_NUMERIC(ns[1], 15212LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SlotSize"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15214LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_191_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15214LU));
}

/* Slots - ns=1;i=15215 */

static UA_StatusCode function_tsn_app_model_192_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	attr.valueRank = 1;
	attr.arrayDimensionsSize = 1;
	UA_UInt32 arrayDimensions[1];
	arrayDimensions[0] = 0;
	attr.arrayDimensions = &arrayDimensions[0];
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Slots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Array for the repartition of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15215LU),
					  UA_NODEID_NUMERIC(ns[1], 15212LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Slots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15215LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_192_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15215LU));
}

/* SchedErrHisto - ns=1;i=15032 */

static UA_StatusCode function_tsn_app_model_193_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "SchedErrHisto");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "SchedErrHisto");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15032LU),
					  UA_NODEID_NUMERIC(ns[1], 15016LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SchedErrHisto"), UA_NODEID_NUMERIC(ns[1], 15009LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15032LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_193_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15032LU));
}

/* SlotSize - ns=1;i=15034 */

static UA_StatusCode function_tsn_app_model_194_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SlotSize");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Slot size");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15034LU),
					  UA_NODEID_NUMERIC(ns[1], 15032LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SlotSize"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15034LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_194_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15034LU));
}

/* Slots - ns=1;i=15035 */

static UA_StatusCode function_tsn_app_model_195_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	attr.valueRank = 1;
	attr.arrayDimensionsSize = 1;
	UA_UInt32 arrayDimensions[1];
	arrayDimensions[0] = 0;
	attr.arrayDimensions = &arrayDimensions[0];
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Slots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Array for the repartition of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15035LU),
					  UA_NODEID_NUMERIC(ns[1], 15032LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Slots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15035LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_195_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15035LU));
}

/* NSlots - ns=1;i=15033 */

static UA_StatusCode function_tsn_app_model_196_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "NSlots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of slots");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15033LU),
					  UA_NODEID_NUMERIC(ns[1], 15032LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NSlots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15033LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_196_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15033LU));
}

/* TrafficLatencyHisto - ns=1;i=15100 */

static UA_StatusCode function_tsn_app_model_197_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TrafficLatencyHisto");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Traffic latency histogram");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15100LU),
					  UA_NODEID_NUMERIC(ns[1], 15085LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "TrafficLatencyHisto"),
					  UA_NODEID_NUMERIC(ns[1], 15009LU), (const UA_NodeAttributes *)&attr,
					  &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15100LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_197_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15100LU));
}

/* Slots - ns=1;i=15103 */

static UA_StatusCode function_tsn_app_model_198_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	attr.valueRank = 1;
	attr.arrayDimensionsSize = 1;
	UA_UInt32 arrayDimensions[1];
	arrayDimensions[0] = 0;
	attr.arrayDimensions = &arrayDimensions[0];
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Slots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Array for the repartition of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15103LU),
					  UA_NODEID_NUMERIC(ns[1], 15100LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Slots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15103LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_198_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15103LU));
}

/* SlotSize - ns=1;i=15102 */

static UA_StatusCode function_tsn_app_model_199_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SlotSize");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Slot size");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15102LU),
					  UA_NODEID_NUMERIC(ns[1], 15100LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SlotSize"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15102LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_199_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15102LU));
}

/* NSlots - ns=1;i=15101 */

static UA_StatusCode function_tsn_app_model_200_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "NSlots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of slots");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15101LU),
					  UA_NODEID_NUMERIC(ns[1], 15100LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NSlots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15101LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_200_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15101LU));
}

/* TotalTimeHisto - ns=1;i=15338 */

static UA_StatusCode function_tsn_app_model_201_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TotalTimeHisto");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Total time histogram");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15338LU),
					  UA_NODEID_NUMERIC(ns[1], 15298LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "TotalTimeHisto"), UA_NODEID_NUMERIC(ns[1], 15009LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_201_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15338LU));
}

/* NSlots - ns=1;i=15339 */

static UA_StatusCode function_tsn_app_model_202_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "NSlots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Number of slots");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15339LU),
					  UA_NODEID_NUMERIC(ns[1], 15338LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "NSlots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_202_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15339LU));
}

/* SlotSize - ns=1;i=15340 */

static UA_StatusCode function_tsn_app_model_203_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "SlotSize");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Slot size");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15340LU),
					  UA_NODEID_NUMERIC(ns[1], 15338LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SlotSize"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_203_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15340LU));
}

/* Slots - ns=1;i=15341 */

static UA_StatusCode function_tsn_app_model_204_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	attr.valueRank = 1;
	attr.arrayDimensionsSize = 1;
	UA_UInt32 arrayDimensions[1];
	arrayDimensions[0] = 0;
	attr.arrayDimensions = &arrayDimensions[0];
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 7LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Slots");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Array for the repartition of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15341LU),
					  UA_NODEID_NUMERIC(ns[1], 15338LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Slots"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_204_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15341LU));
}

/* StatsType - ns=1;i=15001 */

static UA_StatusCode function_tsn_app_model_205_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "StatsType");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Base type for all statistics");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECTTYPE, UA_NODEID_NUMERIC(ns[1], 15001LU),
					  UA_NODEID_NUMERIC(ns[0], 58LU), UA_NODEID_NUMERIC(ns[0], 45LU),
					  UA_QUALIFIEDNAME(ns[1], "StatsType"), UA_NODEID_NULL,
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_205_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15001LU));
}

/* TotalTimeStats - ns=1;i=15228 */

static UA_StatusCode function_tsn_app_model_206_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TotalTimeStats");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Total time statistics");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15228LU),
					  UA_NODEID_NUMERIC(ns[1], 15196LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "TotalTimeStats"), UA_NODEID_NUMERIC(ns[1], 15001LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15228LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_206_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15228LU));
}

/* Ms - ns=1;i=15234 */

static UA_StatusCode function_tsn_app_model_207_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Ms");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean square of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15234LU),
					  UA_NODEID_NUMERIC(ns[1], 15228LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Ms"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15234LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_207_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15234LU));
}

/* Min - ns=1;i=15229 */

static UA_StatusCode function_tsn_app_model_208_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Min");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15229LU),
					  UA_NODEID_NUMERIC(ns[1], 15228LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Min"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15229LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_208_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15229LU));
}

/* AbsMax - ns=1;i=15232 */

static UA_StatusCode function_tsn_app_model_209_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMax");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15232LU),
					  UA_NODEID_NUMERIC(ns[1], 15228LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMax"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15232LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_209_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15232LU));
}

/* Max - ns=1;i=15231 */

static UA_StatusCode function_tsn_app_model_210_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Max");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15231LU),
					  UA_NODEID_NUMERIC(ns[1], 15228LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Max"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15231LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_210_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15231LU));
}

/* AbsMin - ns=1;i=15233 */

static UA_StatusCode function_tsn_app_model_211_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMin");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15233LU),
					  UA_NODEID_NUMERIC(ns[1], 15228LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMin"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15233LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_211_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15233LU));
}

/* Variance - ns=1;i=15235 */

static UA_StatusCode function_tsn_app_model_212_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Variance");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Variance of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15235LU),
					  UA_NODEID_NUMERIC(ns[1], 15228LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Variance"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15235LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_212_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15235LU));
}

/* Mean - ns=1;i=15230 */

static UA_StatusCode function_tsn_app_model_213_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Mean");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15230LU),
					  UA_NODEID_NUMERIC(ns[1], 15228LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Mean"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15230LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_213_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15230LU));
}

/* TrafficLatencyStats - ns=1;i=15150 */

static UA_StatusCode function_tsn_app_model_214_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TrafficLatencyStats");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Traffic latency statistics");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15150LU),
					  UA_NODEID_NUMERIC(ns[1], 15143LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "TrafficLatencyStats"),
					  UA_NODEID_NUMERIC(ns[1], 15001LU), (const UA_NodeAttributes *)&attr,
					  &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15150LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_214_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15150LU));
}

/* AbsMin - ns=1;i=15155 */

static UA_StatusCode function_tsn_app_model_215_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMin");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15155LU),
					  UA_NODEID_NUMERIC(ns[1], 15150LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMin"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15155LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_215_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15155LU));
}

/* AbsMax - ns=1;i=15154 */

static UA_StatusCode function_tsn_app_model_216_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMax");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15154LU),
					  UA_NODEID_NUMERIC(ns[1], 15150LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMax"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15154LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_216_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15154LU));
}

/* Ms - ns=1;i=15156 */

static UA_StatusCode function_tsn_app_model_217_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Ms");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean square of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15156LU),
					  UA_NODEID_NUMERIC(ns[1], 15150LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Ms"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15156LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_217_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15156LU));
}

/* Max - ns=1;i=15153 */

static UA_StatusCode function_tsn_app_model_218_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Max");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15153LU),
					  UA_NODEID_NUMERIC(ns[1], 15150LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Max"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15153LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_218_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15153LU));
}

/* Variance - ns=1;i=15157 */

static UA_StatusCode function_tsn_app_model_219_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Variance");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Variance of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15157LU),
					  UA_NODEID_NUMERIC(ns[1], 15150LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Variance"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15157LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_219_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15157LU));
}

/* Mean - ns=1;i=15152 */

static UA_StatusCode function_tsn_app_model_220_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Mean");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15152LU),
					  UA_NODEID_NUMERIC(ns[1], 15150LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Mean"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15152LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_220_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15152LU));
}

/* Min - ns=1;i=15151 */

static UA_StatusCode function_tsn_app_model_221_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Min");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15151LU),
					  UA_NODEID_NUMERIC(ns[1], 15150LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Min"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15151LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_221_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15151LU));
}

/* Variance - ns=1;i=15008 */

static UA_StatusCode function_tsn_app_model_222_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Variance");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Variance of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15008LU),
					  UA_NODEID_NUMERIC(ns[1], 15001LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Variance"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15008LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_222_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15008LU));
}

/* ProcTimeStats - ns=1;i=15036 */

static UA_StatusCode function_tsn_app_model_223_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "ProcTimeStats");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Processing time statistics");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15036LU),
					  UA_NODEID_NUMERIC(ns[1], 15016LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ProcTimeStats"), UA_NODEID_NUMERIC(ns[1], 15001LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15036LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_223_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15036LU));
}

/* Mean - ns=1;i=15038 */

static UA_StatusCode function_tsn_app_model_224_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Mean");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15038LU),
					  UA_NODEID_NUMERIC(ns[1], 15036LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Mean"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15038LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_224_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15038LU));
}

/* Max - ns=1;i=15039 */

static UA_StatusCode function_tsn_app_model_225_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Max");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15039LU),
					  UA_NODEID_NUMERIC(ns[1], 15036LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Max"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15039LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_225_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15039LU));
}

/* AbsMax - ns=1;i=15040 */

static UA_StatusCode function_tsn_app_model_226_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMax");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15040LU),
					  UA_NODEID_NUMERIC(ns[1], 15036LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMax"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15040LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_226_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15040LU));
}

/* Variance - ns=1;i=15043 */

static UA_StatusCode function_tsn_app_model_227_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Variance");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Variance of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15043LU),
					  UA_NODEID_NUMERIC(ns[1], 15036LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Variance"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15043LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_227_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15043LU));
}

/* Ms - ns=1;i=15042 */

static UA_StatusCode function_tsn_app_model_228_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Ms");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean square of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15042LU),
					  UA_NODEID_NUMERIC(ns[1], 15036LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Ms"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15042LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_228_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15042LU));
}

/* Min - ns=1;i=15037 */

static UA_StatusCode function_tsn_app_model_229_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Min");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15037LU),
					  UA_NODEID_NUMERIC(ns[1], 15036LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Min"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15037LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_229_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15037LU));
}

/* AbsMin - ns=1;i=15041 */

static UA_StatusCode function_tsn_app_model_230_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMin");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15041LU),
					  UA_NODEID_NUMERIC(ns[1], 15036LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMin"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15041LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_230_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15041LU));
}

/* Min - ns=1;i=15002 */

static UA_StatusCode function_tsn_app_model_231_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Min");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15002LU),
					  UA_NODEID_NUMERIC(ns[1], 15001LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Min"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15002LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_231_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15002LU));
}

/* ProcTimeStats - ns=1;i=15216 */

static UA_StatusCode function_tsn_app_model_232_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "ProcTimeStats");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Processing time statistics");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15216LU),
					  UA_NODEID_NUMERIC(ns[1], 15196LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ProcTimeStats"), UA_NODEID_NUMERIC(ns[1], 15001LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15216LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_232_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15216LU));
}

/* Variance - ns=1;i=15223 */

static UA_StatusCode function_tsn_app_model_233_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Variance");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Variance of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15223LU),
					  UA_NODEID_NUMERIC(ns[1], 15216LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Variance"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15223LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_233_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15223LU));
}

/* Max - ns=1;i=15219 */

static UA_StatusCode function_tsn_app_model_234_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Max");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15219LU),
					  UA_NODEID_NUMERIC(ns[1], 15216LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Max"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15219LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_234_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15219LU));
}

/* AbsMin - ns=1;i=15221 */

static UA_StatusCode function_tsn_app_model_235_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMin");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15221LU),
					  UA_NODEID_NUMERIC(ns[1], 15216LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMin"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15221LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_235_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15221LU));
}

/* Mean - ns=1;i=15218 */

static UA_StatusCode function_tsn_app_model_236_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Mean");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15218LU),
					  UA_NODEID_NUMERIC(ns[1], 15216LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Mean"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15218LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_236_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15218LU));
}

/* AbsMax - ns=1;i=15220 */

static UA_StatusCode function_tsn_app_model_237_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMax");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15220LU),
					  UA_NODEID_NUMERIC(ns[1], 15216LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMax"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15220LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_237_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15220LU));
}

/* Ms - ns=1;i=15222 */

static UA_StatusCode function_tsn_app_model_238_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Ms");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean square of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15222LU),
					  UA_NODEID_NUMERIC(ns[1], 15216LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Ms"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15222LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_238_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15222LU));
}

/* Min - ns=1;i=15217 */

static UA_StatusCode function_tsn_app_model_239_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Min");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15217LU),
					  UA_NODEID_NUMERIC(ns[1], 15216LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Min"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15217LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_239_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15217LU));
}

/* Max - ns=1;i=15004 */

static UA_StatusCode function_tsn_app_model_240_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Max");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15004LU),
					  UA_NODEID_NUMERIC(ns[1], 15001LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Max"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15004LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_240_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15004LU));
}

/* Ms - ns=1;i=15007 */

static UA_StatusCode function_tsn_app_model_241_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Ms");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean square of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15007LU),
					  UA_NODEID_NUMERIC(ns[1], 15001LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Ms"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15007LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_241_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15007LU));
}

/* TrafficLatencyStats - ns=1;i=15252 */

static UA_StatusCode function_tsn_app_model_242_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TrafficLatencyStats");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Traffic latency statistics");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15252LU),
					  UA_NODEID_NUMERIC(ns[1], 15245LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "TrafficLatencyStats"),
					  UA_NODEID_NUMERIC(ns[1], 15001LU), (const UA_NodeAttributes *)&attr,
					  &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_242_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15252LU));
}

/* AbsMax - ns=1;i=15256 */

static UA_StatusCode function_tsn_app_model_243_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMax");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15256LU),
					  UA_NODEID_NUMERIC(ns[1], 15252LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMax"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_243_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15256LU));
}

/* Max - ns=1;i=15255 */

static UA_StatusCode function_tsn_app_model_244_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Max");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15255LU),
					  UA_NODEID_NUMERIC(ns[1], 15252LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Max"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_244_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15255LU));
}

/* Min - ns=1;i=15253 */

static UA_StatusCode function_tsn_app_model_245_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Min");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15253LU),
					  UA_NODEID_NUMERIC(ns[1], 15252LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Min"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_245_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15253LU));
}

/* Variance - ns=1;i=15259 */

static UA_StatusCode function_tsn_app_model_246_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Variance");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Variance of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15259LU),
					  UA_NODEID_NUMERIC(ns[1], 15252LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Variance"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_246_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15259LU));
}

/* Ms - ns=1;i=15258 */

static UA_StatusCode function_tsn_app_model_247_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Ms");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean square of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15258LU),
					  UA_NODEID_NUMERIC(ns[1], 15252LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Ms"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_247_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15258LU));
}

/* Mean - ns=1;i=15254 */

static UA_StatusCode function_tsn_app_model_248_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Mean");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15254LU),
					  UA_NODEID_NUMERIC(ns[1], 15252LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Mean"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_248_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15254LU));
}

/* AbsMin - ns=1;i=15257 */

static UA_StatusCode function_tsn_app_model_249_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMin");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15257LU),
					  UA_NODEID_NUMERIC(ns[1], 15252LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMin"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_249_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15257LU));
}

/* TotalTimeStats - ns=1;i=15330 */

static UA_StatusCode function_tsn_app_model_250_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TotalTimeStats");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Total time statistics");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15330LU),
					  UA_NODEID_NUMERIC(ns[1], 15298LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "TotalTimeStats"), UA_NODEID_NUMERIC(ns[1], 15001LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_250_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15330LU));
}

/* Ms - ns=1;i=15336 */

static UA_StatusCode function_tsn_app_model_251_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Ms");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean square of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15336LU),
					  UA_NODEID_NUMERIC(ns[1], 15330LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Ms"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_251_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15336LU));
}

/* Variance - ns=1;i=15337 */

static UA_StatusCode function_tsn_app_model_252_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Variance");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Variance of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15337LU),
					  UA_NODEID_NUMERIC(ns[1], 15330LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Variance"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_252_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15337LU));
}

/* AbsMax - ns=1;i=15334 */

static UA_StatusCode function_tsn_app_model_253_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMax");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15334LU),
					  UA_NODEID_NUMERIC(ns[1], 15330LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMax"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_253_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15334LU));
}

/* Max - ns=1;i=15333 */

static UA_StatusCode function_tsn_app_model_254_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Max");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15333LU),
					  UA_NODEID_NUMERIC(ns[1], 15330LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Max"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_254_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15333LU));
}

/* AbsMin - ns=1;i=15335 */

static UA_StatusCode function_tsn_app_model_255_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMin");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15335LU),
					  UA_NODEID_NUMERIC(ns[1], 15330LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMin"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_255_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15335LU));
}

/* Min - ns=1;i=15331 */

static UA_StatusCode function_tsn_app_model_256_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Min");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15331LU),
					  UA_NODEID_NUMERIC(ns[1], 15330LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Min"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_256_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15331LU));
}

/* Mean - ns=1;i=15332 */

static UA_StatusCode function_tsn_app_model_257_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Mean");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15332LU),
					  UA_NODEID_NUMERIC(ns[1], 15330LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Mean"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_257_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15332LU));
}

/* ProcTimeStats - ns=1;i=15318 */

static UA_StatusCode function_tsn_app_model_258_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "ProcTimeStats");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Processing time statistics");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15318LU),
					  UA_NODEID_NUMERIC(ns[1], 15298LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "ProcTimeStats"), UA_NODEID_NUMERIC(ns[1], 15001LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_258_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15318LU));
}

/* Max - ns=1;i=15321 */

static UA_StatusCode function_tsn_app_model_259_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Max");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15321LU),
					  UA_NODEID_NUMERIC(ns[1], 15318LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Max"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_259_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15321LU));
}

/* Mean - ns=1;i=15320 */

static UA_StatusCode function_tsn_app_model_260_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Mean");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15320LU),
					  UA_NODEID_NUMERIC(ns[1], 15318LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Mean"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_260_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15320LU));
}

/* Variance - ns=1;i=15325 */

static UA_StatusCode function_tsn_app_model_261_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Variance");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Variance of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15325LU),
					  UA_NODEID_NUMERIC(ns[1], 15318LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Variance"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_261_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15325LU));
}

/* Min - ns=1;i=15319 */

static UA_StatusCode function_tsn_app_model_262_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Min");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15319LU),
					  UA_NODEID_NUMERIC(ns[1], 15318LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Min"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_262_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15319LU));
}

/* Ms - ns=1;i=15324 */

static UA_StatusCode function_tsn_app_model_263_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Ms");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean square of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15324LU),
					  UA_NODEID_NUMERIC(ns[1], 15318LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Ms"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_263_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15324LU));
}

/* AbsMax - ns=1;i=15322 */

static UA_StatusCode function_tsn_app_model_264_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMax");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15322LU),
					  UA_NODEID_NUMERIC(ns[1], 15318LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMax"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_264_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15322LU));
}

/* AbsMin - ns=1;i=15323 */

static UA_StatusCode function_tsn_app_model_265_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMin");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15323LU),
					  UA_NODEID_NUMERIC(ns[1], 15318LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMin"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_265_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15323LU));
}

/* SchedErrStats - ns=1;i=15306 */

static UA_StatusCode function_tsn_app_model_266_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "SchedErrStats");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "SchedErrStats");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15306LU),
					  UA_NODEID_NUMERIC(ns[1], 15298LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SchedErrStats"), UA_NODEID_NUMERIC(ns[1], 15001LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_266_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15306LU));
}

/* Max - ns=1;i=15309 */

static UA_StatusCode function_tsn_app_model_267_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Max");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15309LU),
					  UA_NODEID_NUMERIC(ns[1], 15306LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Max"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_267_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15309LU));
}

/* Ms - ns=1;i=15312 */

static UA_StatusCode function_tsn_app_model_268_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Ms");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean square of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15312LU),
					  UA_NODEID_NUMERIC(ns[1], 15306LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Ms"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_268_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15312LU));
}

/* Min - ns=1;i=15307 */

static UA_StatusCode function_tsn_app_model_269_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Min");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15307LU),
					  UA_NODEID_NUMERIC(ns[1], 15306LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Min"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_269_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15307LU));
}

/* Mean - ns=1;i=15308 */

static UA_StatusCode function_tsn_app_model_270_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Mean");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15308LU),
					  UA_NODEID_NUMERIC(ns[1], 15306LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Mean"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_270_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15308LU));
}

/* AbsMin - ns=1;i=15311 */

static UA_StatusCode function_tsn_app_model_271_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMin");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15311LU),
					  UA_NODEID_NUMERIC(ns[1], 15306LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMin"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_271_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15311LU));
}

/* Variance - ns=1;i=15313 */

static UA_StatusCode function_tsn_app_model_272_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Variance");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Variance of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15313LU),
					  UA_NODEID_NUMERIC(ns[1], 15306LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Variance"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_272_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15313LU));
}

/* AbsMax - ns=1;i=15310 */

static UA_StatusCode function_tsn_app_model_273_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMax");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15310LU),
					  UA_NODEID_NUMERIC(ns[1], 15306LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMax"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_273_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15310LU));
}

/* SchedErrStats - ns=1;i=15204 */

static UA_StatusCode function_tsn_app_model_274_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "SchedErrStats");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "SchedErrStats");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15204LU),
					  UA_NODEID_NUMERIC(ns[1], 15196LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SchedErrStats"), UA_NODEID_NUMERIC(ns[1], 15001LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15204LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_274_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15204LU));
}

/* Mean - ns=1;i=15206 */

static UA_StatusCode function_tsn_app_model_275_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Mean");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15206LU),
					  UA_NODEID_NUMERIC(ns[1], 15204LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Mean"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15206LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_275_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15206LU));
}

/* Max - ns=1;i=15207 */

static UA_StatusCode function_tsn_app_model_276_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Max");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15207LU),
					  UA_NODEID_NUMERIC(ns[1], 15204LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Max"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15207LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_276_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15207LU));
}

/* AbsMin - ns=1;i=15209 */

static UA_StatusCode function_tsn_app_model_277_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMin");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15209LU),
					  UA_NODEID_NUMERIC(ns[1], 15204LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMin"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15209LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_277_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15209LU));
}

/* Min - ns=1;i=15205 */

static UA_StatusCode function_tsn_app_model_278_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Min");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15205LU),
					  UA_NODEID_NUMERIC(ns[1], 15204LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Min"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15205LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_278_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15205LU));
}

/* Ms - ns=1;i=15210 */

static UA_StatusCode function_tsn_app_model_279_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Ms");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean square of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15210LU),
					  UA_NODEID_NUMERIC(ns[1], 15204LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Ms"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15210LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_279_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15210LU));
}

/* AbsMax - ns=1;i=15208 */

static UA_StatusCode function_tsn_app_model_280_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMax");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15208LU),
					  UA_NODEID_NUMERIC(ns[1], 15204LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMax"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15208LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_280_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15208LU));
}

/* Variance - ns=1;i=15211 */

static UA_StatusCode function_tsn_app_model_281_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Variance");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Variance of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15211LU),
					  UA_NODEID_NUMERIC(ns[1], 15204LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Variance"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15211LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_281_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15211LU));
}

/* SchedErrStats - ns=1;i=15024 */

static UA_StatusCode function_tsn_app_model_282_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "SchedErrStats");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "SchedErrStats");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15024LU),
					  UA_NODEID_NUMERIC(ns[1], 15016LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "SchedErrStats"), UA_NODEID_NUMERIC(ns[1], 15001LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15024LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_282_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15024LU));
}

/* Max - ns=1;i=15027 */

static UA_StatusCode function_tsn_app_model_283_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Max");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15027LU),
					  UA_NODEID_NUMERIC(ns[1], 15024LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Max"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15027LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_283_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15027LU));
}

/* Mean - ns=1;i=15026 */

static UA_StatusCode function_tsn_app_model_284_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Mean");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15026LU),
					  UA_NODEID_NUMERIC(ns[1], 15024LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Mean"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15026LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_284_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15026LU));
}

/* Ms - ns=1;i=15030 */

static UA_StatusCode function_tsn_app_model_285_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Ms");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean square of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15030LU),
					  UA_NODEID_NUMERIC(ns[1], 15024LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Ms"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15030LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_285_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15030LU));
}

/* Min - ns=1;i=15025 */

static UA_StatusCode function_tsn_app_model_286_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Min");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15025LU),
					  UA_NODEID_NUMERIC(ns[1], 15024LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Min"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15025LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_286_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15025LU));
}

/* Variance - ns=1;i=15031 */

static UA_StatusCode function_tsn_app_model_287_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Variance");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Variance of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15031LU),
					  UA_NODEID_NUMERIC(ns[1], 15024LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Variance"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15031LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_287_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15031LU));
}

/* AbsMax - ns=1;i=15028 */

static UA_StatusCode function_tsn_app_model_288_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMax");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15028LU),
					  UA_NODEID_NUMERIC(ns[1], 15024LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMax"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15028LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_288_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15028LU));
}

/* AbsMin - ns=1;i=15029 */

static UA_StatusCode function_tsn_app_model_289_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMin");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15029LU),
					  UA_NODEID_NUMERIC(ns[1], 15024LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMin"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15029LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_289_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15029LU));
}

/* TrafficLatencyStats - ns=1;i=15111 */

static UA_StatusCode function_tsn_app_model_290_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TrafficLatencyStats");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Traffic latency statistics");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15111LU),
					  UA_NODEID_NUMERIC(ns[1], 15104LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "TrafficLatencyStats"),
					  UA_NODEID_NUMERIC(ns[1], 15001LU), (const UA_NodeAttributes *)&attr,
					  &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15111LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_290_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15111LU));
}

/* Max - ns=1;i=15114 */

static UA_StatusCode function_tsn_app_model_291_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Max");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15114LU),
					  UA_NODEID_NUMERIC(ns[1], 15111LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Max"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15114LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_291_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15114LU));
}

/* Mean - ns=1;i=15113 */

static UA_StatusCode function_tsn_app_model_292_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Mean");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15113LU),
					  UA_NODEID_NUMERIC(ns[1], 15111LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Mean"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15113LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_292_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15113LU));
}

/* Ms - ns=1;i=15117 */

static UA_StatusCode function_tsn_app_model_293_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Ms");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean square of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15117LU),
					  UA_NODEID_NUMERIC(ns[1], 15111LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Ms"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15117LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_293_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15117LU));
}

/* Variance - ns=1;i=15118 */

static UA_StatusCode function_tsn_app_model_294_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Variance");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Variance of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15118LU),
					  UA_NODEID_NUMERIC(ns[1], 15111LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Variance"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15118LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_294_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15118LU));
}

/* AbsMin - ns=1;i=15116 */

static UA_StatusCode function_tsn_app_model_295_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMin");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15116LU),
					  UA_NODEID_NUMERIC(ns[1], 15111LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMin"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15116LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_295_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15116LU));
}

/* AbsMax - ns=1;i=15115 */

static UA_StatusCode function_tsn_app_model_296_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMax");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15115LU),
					  UA_NODEID_NUMERIC(ns[1], 15111LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMax"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15115LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_296_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15115LU));
}

/* Min - ns=1;i=15112 */

static UA_StatusCode function_tsn_app_model_297_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Min");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15112LU),
					  UA_NODEID_NUMERIC(ns[1], 15111LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Min"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15112LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_297_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15112LU));
}

/* AbsMin - ns=1;i=15006 */

static UA_StatusCode function_tsn_app_model_298_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMin");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15006LU),
					  UA_NODEID_NUMERIC(ns[1], 15001LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMin"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15006LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_298_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15006LU));
}

/* Mean - ns=1;i=15003 */

static UA_StatusCode function_tsn_app_model_299_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Mean");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15003LU),
					  UA_NODEID_NUMERIC(ns[1], 15001LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Mean"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15003LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_299_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15003LU));
}

/* AbsMax - ns=1;i=15005 */

static UA_StatusCode function_tsn_app_model_300_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMax");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15005LU),
					  UA_NODEID_NUMERIC(ns[1], 15001LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMax"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15005LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_300_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15005LU));
}

/* TotalTimeStats - ns=1;i=15048 */

static UA_StatusCode function_tsn_app_model_301_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TotalTimeStats");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Total time statistics");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15048LU),
					  UA_NODEID_NUMERIC(ns[1], 15016LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "TotalTimeStats"), UA_NODEID_NUMERIC(ns[1], 15001LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
					  NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15048LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_301_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15048LU));
}

/* Ms - ns=1;i=15054 */

static UA_StatusCode function_tsn_app_model_302_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Ms");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean square of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15054LU),
					  UA_NODEID_NUMERIC(ns[1], 15048LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Ms"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15054LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_302_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15054LU));
}

/* Min - ns=1;i=15049 */

static UA_StatusCode function_tsn_app_model_303_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Min");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15049LU),
					  UA_NODEID_NUMERIC(ns[1], 15048LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Min"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15049LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_303_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15049LU));
}

/* Max - ns=1;i=15051 */

static UA_StatusCode function_tsn_app_model_304_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Max");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15051LU),
					  UA_NODEID_NUMERIC(ns[1], 15048LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Max"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15051LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_304_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15051LU));
}

/* AbsMax - ns=1;i=15052 */

static UA_StatusCode function_tsn_app_model_305_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMax");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15052LU),
					  UA_NODEID_NUMERIC(ns[1], 15048LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMax"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15052LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_305_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15052LU));
}

/* AbsMin - ns=1;i=15053 */

static UA_StatusCode function_tsn_app_model_306_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMin");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15053LU),
					  UA_NODEID_NUMERIC(ns[1], 15048LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMin"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15053LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_306_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15053LU));
}

/* Mean - ns=1;i=15050 */

static UA_StatusCode function_tsn_app_model_307_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Mean");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15050LU),
					  UA_NODEID_NUMERIC(ns[1], 15048LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Mean"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15050LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_307_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15050LU));
}

/* Variance - ns=1;i=15055 */

static UA_StatusCode function_tsn_app_model_308_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Variance");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Variance of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15055LU),
					  UA_NODEID_NUMERIC(ns[1], 15048LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Variance"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15055LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_308_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15055LU));
}

/* TrafficLatencyStats - ns=1;i=15092 */

static UA_StatusCode function_tsn_app_model_309_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TrafficLatencyStats");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Traffic latency statistics");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15092LU),
					  UA_NODEID_NUMERIC(ns[1], 15085LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "TrafficLatencyStats"),
					  UA_NODEID_NUMERIC(ns[1], 15001LU), (const UA_NodeAttributes *)&attr,
					  &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15092LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_309_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15092LU));
}

/* Min - ns=1;i=15093 */

static UA_StatusCode function_tsn_app_model_310_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Min");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15093LU),
					  UA_NODEID_NUMERIC(ns[1], 15092LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Min"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15093LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_310_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15093LU));
}

/* AbsMax - ns=1;i=15096 */

static UA_StatusCode function_tsn_app_model_311_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMax");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15096LU),
					  UA_NODEID_NUMERIC(ns[1], 15092LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMax"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15096LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_311_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15096LU));
}

/* Mean - ns=1;i=15094 */

static UA_StatusCode function_tsn_app_model_312_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Mean");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15094LU),
					  UA_NODEID_NUMERIC(ns[1], 15092LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Mean"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15094LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_312_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15094LU));
}

/* Variance - ns=1;i=15099 */

static UA_StatusCode function_tsn_app_model_313_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Variance");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Variance of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15099LU),
					  UA_NODEID_NUMERIC(ns[1], 15092LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Variance"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15099LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_313_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15099LU));
}

/* AbsMin - ns=1;i=15097 */

static UA_StatusCode function_tsn_app_model_314_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMin");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15097LU),
					  UA_NODEID_NUMERIC(ns[1], 15092LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMin"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15097LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_314_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15097LU));
}

/* Ms - ns=1;i=15098 */

static UA_StatusCode function_tsn_app_model_315_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Ms");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean square of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15098LU),
					  UA_NODEID_NUMERIC(ns[1], 15092LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Ms"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15098LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_315_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15098LU));
}

/* Max - ns=1;i=15095 */

static UA_StatusCode function_tsn_app_model_316_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Max");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15095LU),
					  UA_NODEID_NUMERIC(ns[1], 15092LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Max"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15095LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_316_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15095LU));
}

/* TrafficLatencyStats - ns=1;i=15271 */

static UA_StatusCode function_tsn_app_model_317_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TrafficLatencyStats");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Traffic latency statistics");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15271LU),
					  UA_NODEID_NUMERIC(ns[1], 15264LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "TrafficLatencyStats"),
					  UA_NODEID_NUMERIC(ns[1], 15001LU), (const UA_NodeAttributes *)&attr,
					  &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_317_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15271LU));
}

/* Variance - ns=1;i=15278 */

static UA_StatusCode function_tsn_app_model_318_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Variance");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Variance of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15278LU),
					  UA_NODEID_NUMERIC(ns[1], 15271LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Variance"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_318_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15278LU));
}

/* AbsMin - ns=1;i=15276 */

static UA_StatusCode function_tsn_app_model_319_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMin");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15276LU),
					  UA_NODEID_NUMERIC(ns[1], 15271LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMin"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_319_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15276LU));
}

/* Max - ns=1;i=15274 */

static UA_StatusCode function_tsn_app_model_320_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Max");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15274LU),
					  UA_NODEID_NUMERIC(ns[1], 15271LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Max"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_320_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15274LU));
}

/* Min - ns=1;i=15272 */

static UA_StatusCode function_tsn_app_model_321_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Min");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15272LU),
					  UA_NODEID_NUMERIC(ns[1], 15271LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Min"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_321_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15272LU));
}

/* Ms - ns=1;i=15277 */

static UA_StatusCode function_tsn_app_model_322_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Ms");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean square of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15277LU),
					  UA_NODEID_NUMERIC(ns[1], 15271LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Ms"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_322_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15277LU));
}

/* Mean - ns=1;i=15273 */

static UA_StatusCode function_tsn_app_model_323_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Mean");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15273LU),
					  UA_NODEID_NUMERIC(ns[1], 15271LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Mean"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_323_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15273LU));
}

/* AbsMax - ns=1;i=15275 */

static UA_StatusCode function_tsn_app_model_324_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMax");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15275LU),
					  UA_NODEID_NUMERIC(ns[1], 15271LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMax"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_324_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15275LU));
}

/* TrafficLatencyStats - ns=1;i=15067 */

static UA_StatusCode function_tsn_app_model_325_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TrafficLatencyStats");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Traffic latency statistics");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15067LU),
					  UA_NODEID_NUMERIC(ns[1], 15060LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "TrafficLatencyStats"),
					  UA_NODEID_NUMERIC(ns[1], 15001LU), (const UA_NodeAttributes *)&attr,
					  &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15067LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_325_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15067LU));
}

/* Min - ns=1;i=15068 */

static UA_StatusCode function_tsn_app_model_326_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Min");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15068LU),
					  UA_NODEID_NUMERIC(ns[1], 15067LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Min"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15068LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_326_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15068LU));
}

/* Variance - ns=1;i=15074 */

static UA_StatusCode function_tsn_app_model_327_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Variance");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Variance of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15074LU),
					  UA_NODEID_NUMERIC(ns[1], 15067LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Variance"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15074LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_327_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15074LU));
}

/* Mean - ns=1;i=15069 */

static UA_StatusCode function_tsn_app_model_328_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Mean");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15069LU),
					  UA_NODEID_NUMERIC(ns[1], 15067LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Mean"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15069LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_328_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15069LU));
}

/* AbsMax - ns=1;i=15071 */

static UA_StatusCode function_tsn_app_model_329_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMax");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15071LU),
					  UA_NODEID_NUMERIC(ns[1], 15067LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMax"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15071LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_329_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15071LU));
}

/* Max - ns=1;i=15070 */

static UA_StatusCode function_tsn_app_model_330_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Max");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15070LU),
					  UA_NODEID_NUMERIC(ns[1], 15067LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Max"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15070LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_330_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15070LU));
}

/* Ms - ns=1;i=15073 */

static UA_StatusCode function_tsn_app_model_331_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Ms");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean square of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15073LU),
					  UA_NODEID_NUMERIC(ns[1], 15067LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Ms"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15073LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_331_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15073LU));
}

/* AbsMin - ns=1;i=15072 */

static UA_StatusCode function_tsn_app_model_332_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMin");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15072LU),
					  UA_NODEID_NUMERIC(ns[1], 15067LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMin"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15072LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_332_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15072LU));
}

/* TrafficLatencyStats - ns=1;i=15169 */

static UA_StatusCode function_tsn_app_model_333_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_ObjectAttributes attr = UA_ObjectAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("", "TrafficLatencyStats");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Traffic latency statistics");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(ns[1], 15169LU),
					  UA_NODEID_NUMERIC(ns[1], 15162LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "TrafficLatencyStats"),
					  UA_NODEID_NUMERIC(ns[1], 15001LU), (const UA_NodeAttributes *)&attr,
					  &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15169LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_333_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15169LU));
}

/* Max - ns=1;i=15172 */

static UA_StatusCode function_tsn_app_model_334_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Max");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15172LU),
					  UA_NODEID_NUMERIC(ns[1], 15169LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Max"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15172LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_334_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15172LU));
}

/* Ms - ns=1;i=15175 */

static UA_StatusCode function_tsn_app_model_335_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Ms");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean square of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15175LU),
					  UA_NODEID_NUMERIC(ns[1], 15169LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Ms"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15175LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_335_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15175LU));
}

/* Mean - ns=1;i=15171 */

static UA_StatusCode function_tsn_app_model_336_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Mean");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Mean of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15171LU),
					  UA_NODEID_NUMERIC(ns[1], 15169LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Mean"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15171LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_336_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15171LU));
}

/* Min - ns=1;i=15170 */

static UA_StatusCode function_tsn_app_model_337_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Min");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15170LU),
					  UA_NODEID_NUMERIC(ns[1], 15169LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Min"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15170LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_337_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15170LU));
}

/* Variance - ns=1;i=15176 */

static UA_StatusCode function_tsn_app_model_338_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 9LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "Variance");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Variance of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15176LU),
					  UA_NODEID_NUMERIC(ns[1], 15169LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "Variance"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15176LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_338_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15176LU));
}

/* AbsMax - ns=1;i=15173 */

static UA_StatusCode function_tsn_app_model_339_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMax");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute maximum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15173LU),
					  UA_NODEID_NUMERIC(ns[1], 15169LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMax"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15173LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_339_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15173LU));
}

/* AbsMin - ns=1;i=15174 */

static UA_StatusCode function_tsn_app_model_340_begin(UA_Server *server, UA_UInt16 *ns)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.minimumSamplingInterval = 0.000000;
	attr.userAccessLevel = 1;
	attr.accessLevel = 1;
	/* Value rank inherited */
	attr.valueRank = -1;
	attr.dataType = UA_NODEID_NUMERIC(ns[0], 6LU);
	attr.displayName = UA_LOCALIZEDTEXT("", "AbsMin");
#ifdef UA_ENABLE_NODESET_COMPILER_DESCRIPTIONS
	attr.description = UA_LOCALIZEDTEXT("", "Absolute minimum of the values");
#endif
	retVal |= UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(ns[1], 15174LU),
					  UA_NODEID_NUMERIC(ns[1], 15169LU), UA_NODEID_NUMERIC(ns[0], 47LU),
					  UA_QUALIFIEDNAME(ns[1], "AbsMin"), UA_NODEID_NUMERIC(ns[0], 63LU),
					  (const UA_NodeAttributes *)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
					  NULL, NULL);
	retVal |= UA_Server_addReference(server, UA_NODEID_NUMERIC(ns[1], 15174LU), UA_NODEID_NUMERIC(ns[0], 37LU),
					 UA_EXPANDEDNODEID_NUMERIC(ns[0], 78LU), true);
	return retVal;
}

static UA_StatusCode function_tsn_app_model_340_finish(UA_Server *server, UA_UInt16 *ns)
{
	return UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(ns[1], 15174LU));
}

UA_StatusCode tsn_app_model(UA_Server *server)
{
	UA_StatusCode retVal = UA_STATUSCODE_GOOD;
	/* Use namespace ids generated by the server */
	UA_UInt16 ns[2];
	ns[0] = UA_Server_addNamespace(server, "http://opcfoundation.org/UA/");
	ns[1] = UA_Server_addNamespace(server, "https://opcua/UA/Tsn/");

	/* Load custom datatype definitions into the server */
	bool dummy = (!(retVal = function_tsn_app_model_0_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_1_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_2_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_3_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_4_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_5_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_6_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_7_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_8_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_9_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_10_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_11_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_12_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_13_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_14_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_15_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_16_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_17_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_18_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_19_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_20_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_21_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_22_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_23_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_24_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_25_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_26_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_27_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_28_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_29_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_30_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_31_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_32_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_33_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_34_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_35_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_36_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_37_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_38_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_39_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_40_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_41_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_42_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_43_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_44_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_45_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_46_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_47_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_48_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_49_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_50_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_51_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_52_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_53_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_54_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_55_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_56_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_57_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_58_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_59_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_60_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_61_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_62_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_63_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_64_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_65_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_66_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_67_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_68_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_69_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_70_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_71_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_72_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_73_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_74_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_75_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_76_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_77_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_78_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_79_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_80_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_81_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_82_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_83_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_84_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_85_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_86_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_87_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_88_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_89_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_90_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_91_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_92_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_93_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_94_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_95_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_96_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_97_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_98_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_99_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_100_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_101_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_102_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_103_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_104_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_105_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_106_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_107_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_108_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_109_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_110_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_111_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_112_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_113_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_114_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_115_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_116_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_117_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_118_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_119_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_120_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_121_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_122_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_123_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_124_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_125_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_126_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_127_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_128_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_129_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_130_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_131_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_132_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_133_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_134_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_135_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_136_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_137_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_138_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_139_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_140_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_141_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_142_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_143_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_144_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_145_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_146_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_147_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_148_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_149_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_150_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_151_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_152_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_153_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_154_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_155_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_156_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_157_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_158_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_159_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_160_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_161_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_162_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_163_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_164_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_165_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_166_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_167_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_168_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_169_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_170_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_171_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_172_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_173_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_174_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_175_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_176_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_177_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_178_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_179_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_180_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_181_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_182_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_183_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_184_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_185_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_186_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_187_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_188_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_189_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_190_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_191_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_192_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_193_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_194_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_195_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_196_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_197_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_198_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_199_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_200_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_201_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_202_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_203_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_204_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_205_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_206_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_207_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_208_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_209_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_210_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_211_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_212_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_213_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_214_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_215_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_216_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_217_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_218_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_219_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_220_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_221_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_222_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_223_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_224_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_225_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_226_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_227_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_228_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_229_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_230_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_231_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_232_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_233_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_234_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_235_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_236_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_237_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_238_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_239_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_240_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_241_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_242_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_243_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_244_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_245_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_246_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_247_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_248_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_249_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_250_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_251_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_252_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_253_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_254_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_255_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_256_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_257_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_258_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_259_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_260_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_261_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_262_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_263_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_264_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_265_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_266_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_267_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_268_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_269_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_270_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_271_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_272_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_273_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_274_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_275_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_276_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_277_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_278_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_279_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_280_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_281_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_282_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_283_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_284_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_285_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_286_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_287_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_288_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_289_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_290_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_291_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_292_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_293_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_294_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_295_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_296_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_297_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_298_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_299_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_300_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_301_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_302_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_303_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_304_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_305_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_306_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_307_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_308_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_309_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_310_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_311_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_312_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_313_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_314_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_315_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_316_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_317_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_318_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_319_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_320_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_321_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_322_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_323_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_324_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_325_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_326_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_327_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_328_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_329_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_330_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_331_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_332_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_333_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_334_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_335_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_336_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_337_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_338_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_339_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_340_begin(server, ns)) &&
		      !(retVal = function_tsn_app_model_340_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_339_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_338_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_337_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_336_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_335_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_334_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_333_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_332_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_331_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_330_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_329_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_328_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_327_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_326_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_325_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_324_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_323_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_322_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_321_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_320_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_319_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_318_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_317_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_316_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_315_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_314_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_313_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_312_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_311_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_310_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_309_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_308_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_307_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_306_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_305_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_304_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_303_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_302_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_301_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_300_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_299_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_298_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_297_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_296_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_295_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_294_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_293_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_292_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_291_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_290_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_289_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_288_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_287_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_286_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_285_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_284_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_283_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_282_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_281_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_280_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_279_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_278_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_277_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_276_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_275_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_274_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_273_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_272_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_271_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_270_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_269_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_268_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_267_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_266_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_265_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_264_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_263_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_262_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_261_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_260_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_259_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_258_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_257_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_256_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_255_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_254_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_253_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_252_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_251_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_250_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_249_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_248_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_247_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_246_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_245_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_244_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_243_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_242_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_241_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_240_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_239_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_238_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_237_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_236_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_235_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_234_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_233_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_232_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_231_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_230_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_229_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_228_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_227_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_226_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_225_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_224_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_223_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_222_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_221_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_220_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_219_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_218_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_217_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_216_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_215_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_214_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_213_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_212_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_211_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_210_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_209_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_208_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_207_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_206_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_205_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_204_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_203_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_202_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_201_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_200_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_199_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_198_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_197_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_196_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_195_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_194_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_193_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_192_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_191_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_190_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_189_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_188_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_187_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_186_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_185_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_184_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_183_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_182_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_181_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_180_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_179_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_178_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_177_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_176_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_175_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_174_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_173_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_172_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_171_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_170_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_169_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_168_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_167_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_166_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_165_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_164_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_163_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_162_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_161_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_160_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_159_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_158_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_157_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_156_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_155_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_154_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_153_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_152_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_151_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_150_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_149_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_148_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_147_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_146_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_145_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_144_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_143_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_142_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_141_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_140_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_139_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_138_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_137_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_136_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_135_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_134_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_133_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_132_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_131_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_130_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_129_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_128_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_127_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_126_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_125_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_124_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_123_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_122_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_121_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_120_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_119_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_118_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_117_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_116_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_115_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_114_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_113_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_112_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_111_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_110_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_109_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_108_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_107_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_106_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_105_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_104_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_103_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_102_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_101_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_100_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_99_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_98_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_97_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_96_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_95_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_94_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_93_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_92_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_91_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_90_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_89_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_88_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_87_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_86_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_85_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_84_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_83_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_82_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_81_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_80_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_79_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_78_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_77_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_76_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_75_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_74_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_73_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_72_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_71_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_70_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_69_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_68_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_67_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_66_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_65_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_64_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_63_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_62_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_61_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_60_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_59_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_58_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_57_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_56_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_55_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_54_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_53_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_52_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_51_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_50_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_49_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_48_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_47_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_46_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_45_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_44_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_43_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_42_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_41_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_40_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_39_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_38_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_37_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_36_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_35_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_34_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_33_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_32_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_31_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_30_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_29_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_28_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_27_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_26_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_25_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_24_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_23_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_22_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_21_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_20_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_19_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_18_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_17_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_16_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_15_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_14_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_13_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_12_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_11_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_10_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_9_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_8_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_7_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_6_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_5_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_4_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_3_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_2_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_1_finish(server, ns)) &&
		      !(retVal = function_tsn_app_model_0_finish(server, ns)));
	(void)(dummy);
	return retVal;
}
