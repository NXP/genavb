/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief OS specific GenAVB DSA API
 \details OS specific DSA API definition for the GenAVB library

 \copyright Copyright 2023-2024 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_DSA_API_H_
#define _OS_GENAVB_PUBLIC_DSA_API_H_

/** Add hardware table rules used for adding/deleting DSA TAG
 *
 * \ingroup 		dsa
 * \return 		::GENAVB_SUCCESS or negative error code.
 * \param cpu_port	logical port number of DSA switch cpu port
 * \param mac_addr	MAC address of master Ethernet port on MPU
 * \param slave_port	logical port number of DSA switch slave port
 */
int genavb_port_dsa_add(unsigned int cpu_port, uint8_t *mac_addr, unsigned int slave_port);

/** Delete hardware table rules used for adding/deleting DSA TAG
 *
 * \ingroup		dsa
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param slave_port	logical port number of DSA switch slave port
 */
int genavb_port_dsa_delete(unsigned int slave_port);

#endif /* _OS_GENAVB_PUBLIC_DSA_API_H_ */
