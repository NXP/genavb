/*
 * Copyright 2018-2020, 2022-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file		sr_class.c
 \brief	SR classes functions
 \details

 \copyright Copyright 2018-2020, 2022-2023 NXP.
*/
#include "genavb/sr_class.h"

struct _sr_class_struct {
	/* Set to 1 to enable, 0 to disable. Only two classes can be enabled at a time */
	unsigned int	enabled;
	sr_class_t 		type;
	sr_prio_t		prio;
	unsigned int	max_timing_uncertainty;
	unsigned int	max_transit_time;
	unsigned int	interval_P;
	unsigned int	interval_Q;
	unsigned int	max_interval_frames;
	unsigned int    pcp;
	unsigned int    id;
};

static struct _sr_class_struct _sr_class[SR_CLASS_MAX] = {
		{
				.enabled = 1,
				.type = SR_CLASS_A,
				.prio = SR_PRIO_HIGH,
				.max_timing_uncertainty = SR_CLASS_A_MAX_TIMING_UNCERTAINTY,
				.max_transit_time = SR_CLASS_A_MAX_TRANSIT_TIME,
				.interval_P = SR_CLASS_A_INTERVAL_P,
				.interval_Q = SR_CLASS_A_INTERVAL_Q,
				.max_interval_frames = SR_CLASS_A_MAX_INTERVAL_FRAMES,
				.pcp = SR_CLASS_HIGH_PCP,
				.id = SR_CLASS_HIGH_ID
		},
		{
				.enabled = 1,
				.type = SR_CLASS_B,
				.prio = SR_PRIO_LOW,
				.max_timing_uncertainty = SR_CLASS_B_MAX_TIMING_UNCERTAINTY,
				.max_transit_time = SR_CLASS_B_MAX_TRANSIT_TIME,
				.interval_P = SR_CLASS_B_INTERVAL_P,
				.interval_Q = SR_CLASS_B_INTERVAL_Q,
				.max_interval_frames = SR_CLASS_B_MAX_INTERVAL_FRAMES,
				.pcp = 	SR_CLASS_LOW_PCP,
				.id = SR_CLASS_LOW_ID
		},
		{
				.enabled = 0,
				.type = SR_CLASS_C,
				.prio = SR_PRIO_MIN,
				.max_timing_uncertainty = SR_CLASS_C_MAX_TIMING_UNCERTAINTY,
				.max_transit_time = SR_CLASS_C_MAX_TRANSIT_TIME,
				.interval_P = SR_CLASS_C_INTERVAL_P,
				.interval_Q = SR_CLASS_C_INTERVAL_Q,
				.max_interval_frames = SR_CLASS_C_MAX_INTERVAL_FRAMES,
				.pcp = SR_CLASS_LOW_PCP,
				.id = SR_CLASS_LOW_ID
		},
		{
				.enabled = 0,
				.type = SR_CLASS_D,
				.prio = SR_PRIO_MIN,
				.max_timing_uncertainty = SR_CLASS_D_MAX_TIMING_UNCERTAINTY,
				.max_transit_time = SR_CLASS_D_MAX_TRANSIT_TIME,
				.interval_P = SR_CLASS_D_INTERVAL_P,
				.interval_Q = SR_CLASS_D_INTERVAL_Q,
				.max_interval_frames = SR_CLASS_D_MAX_INTERVAL_FRAMES,
				.pcp = SR_CLASS_LOW_PCP,
				.id = SR_CLASS_LOW_ID
		},
		{
				.enabled = 0,
				.type = SR_CLASS_E,
				.prio = SR_PRIO_MIN,
				.max_timing_uncertainty = SR_CLASS_E_MAX_TIMING_UNCERTAINTY,
				.max_transit_time = SR_CLASS_E_MAX_TRANSIT_TIME,
				.interval_P = SR_CLASS_E_INTERVAL_P,
				.interval_Q = SR_CLASS_E_INTERVAL_Q,
				.max_interval_frames = SR_CLASS_E_MAX_INTERVAL_FRAMES,
				.pcp = SR_CLASS_LOW_PCP,
				.id = SR_CLASS_LOW_ID
		}
};

static sr_class_t _sr_class_high = SR_CLASS_A;
static sr_class_t _sr_class_low = SR_CLASS_B;

static sr_class_t _prio_to_class[SR_PRIO_MAX] = {SR_CLASS_A, SR_CLASS_B};
const static unsigned int _prio_to_pcp[SR_PRIO_MAX] = {SR_CLASS_HIGH_PCP, SR_CLASS_LOW_PCP};
const static unsigned int _prio_to_id[SR_PRIO_MAX] = {SR_CLASS_HIGH_ID, SR_CLASS_LOW_ID};
static sr_class_t _pcp_to_class[8] = {
	SR_CLASS_NONE,
	SR_CLASS_NONE,
	SR_CLASS_B,
	SR_CLASS_A,
	SR_CLASS_NONE,
	SR_CLASS_NONE,
	SR_CLASS_NONE,
	SR_CLASS_NONE
};

sr_class_t sr_class_high(void)
{
	return _sr_class_high;
}

static void set_sr_class_high(sr_class_t sr_class)
{
	_sr_class_high = sr_class;
}

sr_class_t sr_class_low(void)
{
	return _sr_class_low;
}

static void set_sr_class_low(sr_class_t sr_class)
{
	_sr_class_low = sr_class;
}

/** Returns if a given SR class is enabled
 * \ingroup stream
 * \return 			1 if class is enabled, 0 if not.
 * \param sr_class		SR class to check
 */
unsigned int sr_class_enabled(sr_class_t sr_class)
{
	if (sr_class >= SR_CLASS_MAX)
		return 0;

	return _sr_class[sr_class].enabled;
}

unsigned int sr_class_prio(sr_class_t sr_class)
{
	return _sr_class[sr_class].prio;
}

unsigned int sr_class_max_timing_uncertainty(sr_class_t sr_class)
{
	return _sr_class[sr_class].max_timing_uncertainty;
}

unsigned int sr_class_max_transit_time(sr_class_t sr_class)
{
	return _sr_class[sr_class].max_transit_time;
}

unsigned int sr_class_max_interval_frames(sr_class_t sr_class)
{
	return _sr_class[sr_class].max_interval_frames;
}

unsigned int sr_class_interval_p(sr_class_t sr_class)
{
	return _sr_class[sr_class].interval_P;
}

unsigned int sr_class_interval_q(sr_class_t sr_class)
{
	return _sr_class[sr_class].interval_Q;
}

sr_class_t sr_prio_class(sr_prio_t prio)
{
	return _prio_to_class[prio];
}

unsigned int sr_prio_pcp(sr_prio_t prio)
{
	return _prio_to_pcp[prio];
}

unsigned int sr_prio_id(sr_prio_t prio)
{
	return _prio_to_id[prio];
}

sr_class_t sr_pcp_class(unsigned int pcp)
{
	return _pcp_to_class[pcp];
}

unsigned int sr_class_pcp(sr_class_t sr_class)
{
	return _sr_class[sr_class].pcp;
}

unsigned int sr_class_id(sr_class_t sr_class)
{
	return _sr_class[sr_class].id;
}

int sr_class_config(uint8_t *sr_class)
{
	int i;
	sr_class_t tmp;

	if ((sr_class[0] < SR_CLASS_MIN)
	 || (sr_class[0] >= SR_CLASS_MAX)
	 || (sr_class[1] < SR_CLASS_MIN)
	 || (sr_class[1] >= SR_CLASS_MAX)
	 || (sr_class[0] == sr_class[1]))
		return -1;

	for (i = SR_CLASS_MIN; i < SR_CLASS_MAX; i++)
	{
		_sr_class[i].enabled = 0;
		_sr_class[i].prio    = SR_PRIO_MIN;
		_sr_class[i].id    = SR_CLASS_LOW_ID;
		_sr_class[i].pcp   = SR_CLASS_LOW_PCP;
	}

	set_sr_class_high(SR_CLASS_NONE);
	set_sr_class_low(SR_CLASS_NONE);
	_prio_to_class[SR_PRIO_HIGH] = SR_CLASS_NONE;
	_prio_to_class[SR_PRIO_LOW] = SR_CLASS_NONE;
	_pcp_to_class[SR_CLASS_LOW_PCP] = SR_CLASS_NONE;
	_pcp_to_class[SR_CLASS_HIGH_PCP] = SR_CLASS_NONE;

	if (sr_class[0] > sr_class[1])
	{
		tmp = sr_class[0];
		sr_class[0] = sr_class[1];
		sr_class[1] = tmp;
	}

	if (sr_class[0] != SR_CLASS_NONE)
	{
		_sr_class[sr_class[0]].enabled = 1;
		_sr_class[sr_class[0]].prio    = SR_PRIO_HIGH;
		_sr_class[sr_class[0]].id    = SR_CLASS_HIGH_ID;
		_sr_class[sr_class[0]].pcp   = SR_CLASS_HIGH_PCP;
		set_sr_class_high(sr_class[0]);
		_prio_to_class[SR_PRIO_HIGH] = sr_class[0];
		_pcp_to_class[SR_CLASS_HIGH_PCP] = sr_class[0];

		if (sr_class[1] != SR_CLASS_NONE)
		{
			_sr_class[sr_class[1]].enabled = 1;
			_sr_class[sr_class[1]].prio    = SR_PRIO_LOW;
			_sr_class[sr_class[1]].id    = SR_CLASS_LOW_ID;
			_sr_class[sr_class[1]].pcp   = SR_CLASS_LOW_PCP;
			set_sr_class_low(sr_class[1]);
			_prio_to_class[SR_PRIO_LOW] = sr_class[1];
			_pcp_to_class[SR_CLASS_LOW_PCP] = sr_class[1];
		}
	}

	return 0;
}

sr_class_t str_to_sr_class(const char *s)
{
	int sr_class = SR_CLASS_A + s[0] - 'A';

	if (s[1] != '\0' || sr_class < SR_CLASS_A || sr_class >= SR_CLASS_MAX)
		return SR_CLASS_NONE;
	else
		return sr_class;
}
