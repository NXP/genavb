/*
 * Copyright 2017, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Standard library OS abstraction
 @details
*/

#ifndef _OS_ASSERT_H_
#define _OS_ASSERT_H_

/** abort the program if assertion is false
 * \return	no returned value
 * \param expr	expression to assert
 *
 * os_assert(expr)
 *
 */

void os_assert(int expr);

#include "osal/assert.h"

#endif /* _OS_ASSERT_H_ */
