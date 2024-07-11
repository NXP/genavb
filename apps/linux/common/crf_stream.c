/*
 * Copyright 2017-2020, 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <genavb/genavb.h>

#include "log.h"
#include "avb_stream.h"
#include "crf_stream.h"
#include "clock_domain.h"
#include "msrp.h"

aar_crf_stream_t *crf_stream_get(unsigned int domain_index)
{
	if (domain_index < MAX_CLOCK_DOMAIN) {
		return &g_crf_streams[domain_index];
	}
	return NULL;
}

int crf_stream_create(unsigned int domain_index)
{
	int rc;
	aar_crf_stream_t *crf;

	struct avb_handle *avb_handle = avbstream_get_avb_handle();

	crf = crf_stream_get(domain_index);

	if (!crf) {
		ERR("CRF stream not found for domain_index (%u)\n", domain_index);
		goto err_crf;
	}

	if (crf->stream_handle) {
		ERR("CRF stream handle already created");
		goto err_crf;
	}

	INF("stream_id: " STREAM_STR_FMT, STREAM_STR(crf->stream_params.stream_id));
	INF("dst_mac: " MAC_STR_FMT, MAC_STR(crf->stream_params.dst_mac));

	crf->cur_batch_size = avbstream_batch_size(crf->batch_size_ns, &crf->stream_params);

	/* The app is not aware of which SR classes are enabled, so different values are tried */
	rc = avb_stream_create(avb_handle, &crf->stream_handle, &crf->stream_params, &crf->cur_batch_size, 0);
	if (rc != AVB_SUCCESS) {
		crf->stream_params.stream_class = SR_CLASS_C;

		rc = avb_stream_create(avb_handle, &crf->stream_handle, &crf->stream_params, &crf->cur_batch_size, 0);
		if (rc != AVB_SUCCESS) {
			crf->stream_params.stream_class = SR_CLASS_E;

			rc = avb_stream_create(avb_handle, &crf->stream_handle, &crf->stream_params, &crf->cur_batch_size, 0);
			if (rc != AVB_SUCCESS) {
				crf->stream_params.stream_class = SR_CLASS_A;

				rc = avb_stream_create(avb_handle, &crf->stream_handle, &crf->stream_params, &crf->cur_batch_size, 0);
				if (rc != AVB_SUCCESS) {
					crf->stream_params.stream_class = SR_CLASS_D;

					rc = avb_stream_create(avb_handle, &crf->stream_handle, &crf->stream_params, &crf->cur_batch_size, 0);
					if (rc != AVB_SUCCESS) {
						ERR("create CRF stream failed, err %d", rc);
						goto err_avb;
					}
				}
			}
		}
	}

	if (crf->stream_params.direction == AVTP_DIRECTION_TALKER) {
		rc = msrp_talker_register(&crf->stream_params);
		if (rc != AVB_SUCCESS) {
			ERR("msrp_talker_register error, rc = %d", rc);
			goto err_msrp;
		}
	}
	else {
		rc = msrp_listener_register(&crf->stream_params);
		if (rc != AVB_SUCCESS) {
			ERR("msrp_talker_register error, rc = %d", rc);
			goto err_msrp;
		}
	}

	return 0;

err_msrp:
	avb_stream_destroy(crf->stream_handle);

err_avb:
err_crf:
	return -1;
}

int crf_stream_destroy(unsigned int domain_index)
{
	int rc;
	aar_crf_stream_t *crf;

	crf = crf_stream_get(domain_index);

	if (!crf) {
		ERR("CRF stream not found for domain_index (%u)\n", domain_index);
		rc = -1;
		goto exit;
	}

	if (!crf->stream_handle) {
		rc = 0;
		goto exit;
	}

	rc = avb_stream_destroy(crf->stream_handle);
	if (rc != AVB_SUCCESS)
		ERR("avb_stream_destroy error, rc = %d", rc);

	crf->stream_handle = NULL;

	if (crf->stream_params.direction == AVTP_DIRECTION_TALKER)
		msrp_talker_deregister(&crf->stream_params);
	else
		msrp_listener_deregister(&crf->stream_params);

exit:
	return rc;
}

int crf_connect(media_clock_role_t role, unsigned int domain_index, struct avb_stream_params *stream_params)
{
	struct avb_handle *avb_handle = avbstream_get_avb_handle();
	aar_crf_stream_t *crf;
	int rc;

	if (stream_params)
		stream_params->clock_domain = crf_get_clock_domain(domain_index);

	crf = crf_stream_get(domain_index);
	if (!crf) {
		ERR("CRF stream not found for domain_index (%u)\n", domain_index);
		goto err;
	}

	if (crf->stream_handle) {
		ERR("CRF stream already connected for domain_index (%u)", domain_index);
		goto err;
	}

	rc = clock_domain_set_role(role, crf_get_clock_domain(domain_index), stream_params);
	if (rc != AVB_SUCCESS) {
		ERR("clock_domain_set_role failed, rc = %d", rc);
		goto err;
	}

	/* Update CRF stream params */
	if (stream_params)
		memcpy(&crf->stream_params, stream_params, sizeof(*stream_params));

	crf->cur_batch_size = avbstream_batch_size(crf->batch_size_ns, &crf->stream_params);

	rc = avb_stream_create(avb_handle, &crf->stream_handle, &crf->stream_params, &crf->cur_batch_size, 0);
	if (rc != AVB_SUCCESS) {
		ERR("avb_stream_create failed, rc = %d", rc);
		goto err;
	}

	return 0;

err:
	return -1;
}

void crf_disconnect(unsigned int domain_index)
{
	aar_crf_stream_t *crf;

	crf = crf_stream_get(domain_index);
	if (!crf) {
		ERR("CRF stream not found for domain_index (%u)\n", domain_index);
		return;
	}

	if (!crf->stream_handle) {
		ERR("CRF stream already disconnected for domain_index (%u)", domain_index);
		return;
	}

	if (avb_stream_destroy(crf->stream_handle) != AVB_SUCCESS)
		ERR("avb_stream_destroy error");

	crf->stream_handle = NULL;
}

int crf_configure(unsigned int domain_index, avtp_direction_t direction, unsigned int flags)
{
	aar_crf_stream_t *crf;

	crf = crf_stream_get(domain_index);

	if (!crf) {
		ERR("CRF stream not found for domain_index (%u)\n", domain_index);
		goto err_crf;
	}

	crf->stream_params.direction = direction;
	crf->stream_params.flags = flags;

	return 0;

err_crf:
	return -1;
}
