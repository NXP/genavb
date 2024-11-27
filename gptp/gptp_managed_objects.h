/*
* Copyright 2018, 2020-2021, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief GPTP managed objects definition
 @details
*/

#ifndef _GPTP_MANAGED_OBJECTS_H_
#define _GPTP_MANAGED_OBJECTS_H_

#include "common/managed_objects.h"

#define GPTP_MAX_NODES 6

#define GPTP_DEFAULT_DATA_SET_NUM_LEAVES 15
#define GPTP_CURRENT_DATA_SET_NUM_LEAVES 9
#define GPTP_PARENT_PARAMETER_NUM_LEAVES 8
#define GPTP_TIME_PROPERTIES_NUM_LEAVES 7
#define GPTP_PORT_PARAMETER_DATA_SET_NUM_LEAVES 22
#define GPTP_PORT_PARAMETER_STATS_NUM_LEAVES 17


/* Managed object tree definition, 802.1AS-2011, section 14 */
MODULE(gptp_managed_objects, GPTP_MAX_NODES,
	CONTAINER(default_parameter_data_set, GPTP_DEFAULT_DATA_SET_NUM_LEAVES,
		LEAF(clockIdentity);
		LEAF(numberPorts);
		LEAF(clockClass);
		LEAF(clockAccuracy);
		LEAF(offsetScaledLogVariance);
		LEAF(priority1);
		LEAF(priority2);
		LEAF(gmCapable);
		LEAF(currentUtcOffset);
		LEAF(currentUtcOffetValid);
		LEAF(leap59);
		LEAF(leap61);
		LEAF(timeTraceable);
		LEAF(frequencyTraceable);
		LEAF(timeSource);
	);

	CONTAINER(current_parameter_data_set, GPTP_CURRENT_DATA_SET_NUM_LEAVES,
		LEAF(stepsRemoved);
		LEAF(offsetFromMaster);
		LEAF(lastGmPhaseChange);
		LEAF(lastGmFreqChange);
		LEAF(gmTimebaseIndicator);
		LEAF(gmChangeCount);
		LEAF(timeOfLastGmChangeEvent);
		LEAF(timeOfLastGmPhaseChangeEvent);
		LEAF(timeOfLastGmFreqChangeEvent);
	);

	CONTAINER(parent_parameter_data_set, GPTP_PARENT_PARAMETER_NUM_LEAVES,
		LEAF(parentPortIdentity);
		LEAF(cumulativeRateRatio);
		LEAF(grandMasterIdentity);
		LEAF(grandMasterClockClass);
		LEAF(grandMasterClockAccuracy);
		LEAF(grandMasterOffsetScaledLogVariance);
		LEAF(grandMasterPriority1);
		LEAF(grandMasterPriority2);
	);

	CONTAINER(time_properties_parameter_data_set, GPTP_TIME_PROPERTIES_NUM_LEAVES,
		LEAF(currentUtcOffset);
		LEAF(currentUtcOffsetValid);
		LEAF(leap59);
		LEAF(leap61);
		LEAF(timeTraceable);
		LEAF(frequencyTraceable);
		LEAF(timeSource);
	);

	LIST(port_parameter_data_set, 1,
		LIST_ENTRY(port, 1, GPTP_PORT_PARAMETER_DATA_SET_NUM_LEAVES,
			LEAF(portID);
			LEAF(portIdentity);
			LEAF(portRole);
			LEAF(pttPortEnabled);
			LEAF(isMeasuringDleay);
			LEAF(asCapable);
			LEAF(neighborPropDelay);
			LEAF(neighborPropDelayThresh);
			LEAF(delayAsymmetry);
			LEAF(neighborRateRatio);
			LEAF(initialLogAnnounceInterval);
			LEAF(currentLogAnnounceInterval);
			LEAF(announceReceiptTimeout);
			LEAF(initialLogSyncInterval);
			LEAF(currentLogSyncInterval);
			LEAF(syncReceiptTimeout);
			LEAF(syncReceiptTimeoutTimeInterval);
			LEAF(initialLogPdelayReqInterval);
			LEAF(currentLogPdelayReqInterval);
			LEAF(allowedLostResponses);
			LEAF(allowedFaults);
			LEAF(versionNumber);
		);
	);

	LIST(port_parameter_statistics, 1,
		LIST_ENTRY(port, 1, GPTP_PORT_PARAMETER_STATS_NUM_LEAVES,
			LEAF(portID);
			LEAF(rxSyncCount);
			LEAF(rxFollowUpCount);
			LEAF(rxPdelayRequestCount);
			LEAF(rxPdelayResponseCount);
			LEAF(rxPdelayResponseFollowUpCount);
			LEAF(rxAnnounceCount);
			LEAF(rxPTPPacketDiscardCount);
			LEAF(syncReceiptTimeoutCount);
			LEAF(announceReceiptTimeoutCount);
			LEAF(pdelayAllowedLostResponsesExceededCount);
			LEAF(txSyncCount);
			LEAF(txFollowUpCount);
			LEAF(txPdelayRequestCount);
			LEAF(txPdelayResponseCount);
			LEAF(txPdelayResponseFollowUpCount);
			LEAF(txAnnounceCount);
		);
	);
);


struct gptp_ctx;

void gptp_managed_objects_init(struct gptp_managed_objects *module, struct gptp_ctx *gptp);
unsigned int gptp_managed_objects_get(struct gptp_managed_objects *module, u8 *in, u8 *in_end, u8 *out, u8 *out_end);
unsigned int gptp_managed_objects_set(struct gptp_managed_objects *module, u8 *in, u8 *in_end, u8 *out, u8 *out_end);

#endif /* _GPTP_MANAGED_OBJECTS_H_ */
