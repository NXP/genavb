/*
 * Copyright (c) 2018, David Blackman and Sebastiano Vigna (vigna@acm.org)
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: CC0-1.0
 *
 * Based on code written in 2018 by David Blackman and Sebastiano Vigna
 * (vigna@acm.org)
 *
 * To the extent possible under law, the author has dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * See <http://creativecommons.org/publicdomain/zero/1.0/>.
 *
 * From: https://prng.di.unimi.it/xoroshiro64starstar.c
 *
 * This is xoroshiro64** 1.0, our 32-bit all-purpose, rock-solid,
 * small-state generator. It is extremely fast and it passes all tests we
 * are aware of, but its state space is not large enough for any parallel
 * application.
 *
 * For generating just single-precision (i.e., 32-bit) floating-point
 * numbers, xoroshiro64* is even faster.
 *
 * The state must be seeded so that it is not everywhere zero.
 */

#include "prng_32.h"

static uint32_t s[2];

static inline uint32_t rotl(const uint32_t x, int k)
{
	return (x << k) | (x >> (32 - k));
}

void xoroshiro64starstar_init_state(uint64_t seed)
{
	s[0] = (uint32_t) seed;
	s[1] = seed << 32;
}

uint32_t xoroshiro64starstar_next(void)
{
	const uint32_t s0 = s[0];
	uint32_t s1 = s[1];
	const uint32_t result = rotl(s0 * 0x9E3779BB, 5) * 5;

	s1 ^= s0;
	s[0] = rotl(s0, 26) ^ s1 ^ (s1 << 9); /* a, b */
	s[1] = rotl(s1, 13); /* c */

	return result;
}
