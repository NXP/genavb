/*
 * Copyright 2018-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>

#include <genavb/genavb.h>
#include "common.h"

extern int gptp_main(struct genavb_handle *avb_h, int argc, char *argv[]);
extern int srp_main(struct genavb_handle *avb_h, int argc, char *argv[]);

int main(int argc, char *argv[])
{
	struct genavb_handle *avb_h = NULL;
	int rc;

	rc = genavb_init(&avb_h, 0);
	if (rc != GENAVB_SUCCESS) {
		printf("genavb_init() failed: %s\n", genavb_strerror(rc));
		rc = -1;
		goto exit;
	}

	if (argc < 2) {
		usage();
		goto exit;
	}

	if (!strcmp("ptp", argv[1]) ) {
		rc = gptp_main(avb_h, argc - 1, argv + 1);
	} else if (!strcmp("srp", argv[1]))  {
		rc = srp_main(avb_h, argc - 1 , argv + 1);
	} else {
		usage();
		goto exit;
	}

exit:
	if (avb_h)
		genavb_exit(avb_h);

	return rc;
}
