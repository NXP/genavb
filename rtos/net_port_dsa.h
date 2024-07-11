/*
* Copyright 2023-2024 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#ifndef _RTOS_NET_PORT_DSA_H_
#define _RTOS_NET_PORT_DSA_H_

/* Custom 12-bit VID field used in 802.1Q DSA TAG
 *
 * +-----------+-----+-----------------+-----------+-----------------------+
 * | 11  | 10  |  9  |  8  |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
 * +-----------+-----+-----------------+-----------+-----------------------+
 * |    RSV    | VBID|    SWITCH_ID    |   VBID    |          PORT         |
 * +-----------+-----+-----------------+-----------+-----------------------+
 *
 * RSV - VID[11:10]:
 *		Reserved. Must be set to 3 (0b11).
 *
 * SWITCH_ID - VID[8:6]:
 * 		Index of switch within DSA tree. Unused.
 *
 * VBID - { VID[9], VID[5:4] }:
 * 		Virtual bridge ID. Unused.
 *
 * PORT - VID[3:0]:
 *		Index of switch port.
 */

#define DSA_TAG_8021Q_RSV	0xC00

#endif /* _RTOS_NET_PORT_DSA_H_ */
