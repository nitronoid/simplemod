// SPDX-License-Identifier: GPL-2.0+

#include <linux/module.h>
#include "simple_context.h"

/**
 * DOC: Simple module
 *
 * Simple, no frills module.
 * This file serves as the module entry point and initializes/terminates a
 * simple context stored as global data.
 */

static struct simple_context g_ctx;

static int __init simple_driver_init(void)
{
	return simple_context_init(&g_ctx);
}
module_init(simple_driver_init);

static void __exit simple_driver_exit(void)
{
	simple_context_term(&g_ctx);
}
module_exit(simple_driver_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple module");
MODULE_AUTHOR("<jackjdiver@gmail.com>");
MODULE_VERSION("1.0");
