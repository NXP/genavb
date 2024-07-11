/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018-2020, 2022-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief OS specific GenAVB public initialization API
 \details OS specific initialization API definition for the GenAVB library

 \copyright Copyright 2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016, 2018-2020, 2022-2023 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_INIT_API_H_
#define _OS_GENAVB_PUBLIC_INIT_API_H_

/**
 * \ingroup init
 * GenAVB global configuration.
 */
struct genavb_config {
	struct management_config management_config;
	struct fgptp_config fgptp_config;
	struct avtp_config avtp_config;
	struct srp_config srp_config;
	struct avdecc_config avdecc_config;
	struct maap_config maap_config;
	struct hsr_config hsr_config;
};

/** Gets a copy of the GenAVB default configuration.
 * \ingroup init
 * \param[out] config	pointer to the caller configuration storage.
 */
void genavb_get_default_config(struct genavb_config *config);

/** Sets the GenAVB configuration (used at initialization).
 * \ingroup init
 * \param[in] config	pointer to a GenAVB configuration. The configuration is only used when genavb_init() is called.
 */
void genavb_set_config(struct genavb_config *config);


#endif /* _OS_GENAVB_PUBLIC_INIT_API_H_ */
