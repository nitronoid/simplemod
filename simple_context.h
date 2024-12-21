/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef _SIMPLEMOD_SIMPLE_CONTEXT_H_
#define _SIMPLEMOD_SIMPLE_CONTEXT_H_

#include <linux/list.h>
#include <linux/mutex.h>

/**
 * struct simple_context - A simple context for misc use.
 */
struct simple_context {
    /** @proc_entry: Handle to a procfs node. */
	struct proc_dir_entry *proc_entry;
	struct {
		/** @buf_list.head: Sentinel head node of the buffer list. */
		struct list_head head;
		/** @buf_list.lock: Mutex to protect modifications to the list. */
		struct mutex lock;
	} buf_list;
};

int simple_context_init(struct simple_context *ctx);

void simple_context_term(struct simple_context *ctx);

#endif /* _SIMPLEMOD_SIMPLE_CONTEXT_H_ */
