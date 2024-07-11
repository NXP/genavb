/*
* Copyright 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file msgintr.h
 @brief RTOS specific Message Interrupt block implementation
 @details
*/

#ifndef _RTOS_NET_PORT_MSGINTR_H_
#define _RTOS_NET_PORT_MSGINTR_H_

/**
 * \brief Entry point for the MSGINTR driver and it initializes local driver context
 *
 * \return int value of 0 for success and -1 for error
 */
int msgintr_init(void);

/**
 * \brief Exit point for the MSGINTR driver and it frees all the resources
 *
 */
void msgintr_exit(void);

/**
 * \brief This function helps to get the message interrupt address for MSI-X Entry and
 *  saves the user context with callback and corresponding data
 * \param base
 * \param channel
 * \param callback
 * \param data
 * \return uint32_t value containing the message interrupt address
 */
uint32_t msgintr_init_vector(void *base, uint8_t channel, void (*callback)(void *data), void *data);

/**
 * \brief Resets the callback for msgintr instance to ensure proper cleanup of resources
 *
 * \param base
 * \param channel
 */
void msgintr_reset_vector(void *base, uint8_t channel);

/**
 * \brief This functions helps to get the message interrupt address for the MSI-X Entry
 *
 * \param base
 * \param channel
 * \return uint32_t value containing the message interrupt address
 */
uint32_t msgintr_msix_msgaddr(void *base, uint8_t channel);

#endif /* _RTOS_NET_PORT_MSGINTR_H_ */
