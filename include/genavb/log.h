/*
 * Copyright 2020, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB public logging API
 \details logging API definition for the GenAVB library

 \copyright Copyright 2020, 2023 NXP
*/

#ifndef _GENAVB_PUBLIC_LOG_API_H_
#define _GENAVB_PUBLIC_LOG_API_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \cond RTOS
 */

/**
 * \ingroup generic
 * GenAVB log level. Messages at or below the configured log level will be printed in the log file.
 */
typedef enum {
	GENAVB_LOG_LEVEL_CRIT = 0, /**< Critical messages. Lowest verbosity level */
	GENAVB_LOG_LEVEL_ERR,	   /**< Error messages. */
	GENAVB_LOG_LEVEL_INIT,	   /**< Initialization messages. */
	GENAVB_LOG_LEVEL_INFO,	   /**< Informational messages. */
	GENAVB_LOG_LEVEL_DEBUG	   /**< Debug messages. Highest verbosity level */
} genavb_log_level_t;

/**
 * \ingroup generic
 * GenAVB log component ID.
 */
typedef enum {
	GENAVB_LOG_COMPONENT_ID_AVTP = 0, /**< AVTP component */
	GENAVB_LOG_COMPONENT_ID_AVDECC,	  /**< AVDECC component */
	GENAVB_LOG_COMPONENT_ID_SRP,	  /**< SRP component */
	GENAVB_LOG_COMPONENT_ID_MAAP,	  /**< MAAP component */
	GENAVB_LOG_COMPONENT_ID_COMMON,	  /**< common layer */
	GENAVB_LOG_COMPONENT_ID_OS,	  /**< OS specific layer*/
	GENAVB_LOG_COMPONENT_ID_GPTP,	  /**< gPTP component */
	GENAVB_LOG_COMPONENT_ID_API,	  /**< API component */
	GENAVB_LOG_COMPONENT_ID_MGMT,	  /**< Management component */
} genavb_log_component_id_t;

/**
 * \endcond
 */

/* OS specific headers */
#include "os/log.h"

#ifdef __cplusplus
}
#endif

#endif /* _GENAVB_PUBLIC_LOG_API_H_ */
