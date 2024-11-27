/*
 * Copyright 2018-2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Single linked list implementation
 @details Single linked list data types and helper functions.
*/

#ifndef _S_LIST_H_
#define _S_LIST_H_

#include "common/types.h"

struct slist_node {
	struct slist_node *next;
};

struct slist_head {
	struct slist_node *first;
};

static inline void slist_head_init(struct slist_head *head)
{
	head->first = NULL;
}

static inline void slist_add_head(struct slist_head *head, struct slist_node *entry)
{
	entry->next = head->first;
	head->first = entry;
}

#ifdef S_LIST_ADD_TAIL
static inline void slist_add_tail(struct slist_head *head, struct slist_node *entry)
{
	if (head->first != NULL) {
		struct slist_node *node = head->first;
		while (node->next)
			node = node->next;
		node->next = entry;
	} else {
		head->first = entry;
	}
	entry->next = NULL;
}
#endif

static inline void slist_del(struct slist_head *head, struct slist_node *entry)
{
	struct slist_node *prev = (struct slist_node *)head;

	while (prev) {
		if (prev->next == entry) {
			prev->next = entry->next;
			entry->next = NULL;
			break;
		}
		prev = prev->next;
	}
}

#define slist_first(head)	((head)->first)
#define slist_empty(head)	((head)->first == NULL)
#define slist_next(entry)	((entry)->next)
#define slist_is_last(entry)	((entry) == NULL)

#define slist_for_each(head, entry) \
	for (entry = slist_first(head); \
	     !slist_is_last(entry); \
	     entry = slist_next(entry))

#define slist_for_each_safe(head, entry, next) \
	for (entry = slist_first(head), next = !slist_is_last(entry) ? slist_next(entry): NULL; \
	     !slist_is_last(entry); \
	     entry = next, next = !slist_is_last(entry) ? slist_next(entry): NULL)

#endif /* _S_LIST_H_ */
