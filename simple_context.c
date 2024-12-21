// SPDX-License-Identifier: GPL-2.0+

#include "simple_context.h"

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <uapi/linux/stat.h>

/**
 * struct simple_buffer_entry - Buffer list node.
 */
struct simple_buffer_entry {
    /** @node: list entry. */
	struct list_head node;
    /** @len: length of the buffer data array. */
	size_t len;
    /** @buf: buffer data using flexible array. */
	char buf[];
};

/**
 * DOC: Virtual file entry overview
 *
 * A single procfs entry is created, which defers most functionality to the
 * seq_file API. The seq_file helpers iterate over a list of buffer entries,
 * concatenating their data arrays.
 * A write hook is implemented in the procfs ops to copy a userspace buffer into
 * a new list entry.
 * Rather than relying on a globally accessible list, the list_head is pulled
 * from the procfs node private data.
 */

static void *simple_context_seq_start(struct seq_file *seq, loff_t *pos)
{
	struct simple_context *ctx = pde_data(file_inode(seq->file));

	/* Prevent modification of the list while reading */
	mutex_lock(&ctx->buf_list.lock);

	return seq_list_start(&ctx->buf_list.head, *pos);
}

static void *simple_context_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
	struct simple_context *ctx = pde_data(file_inode(seq->file));

	return seq_list_next(v, &ctx->buf_list.head, pos);
}

static void simple_context_seq_stop(struct seq_file *seq, void *v)
{
	struct simple_context *ctx = pde_data(file_inode(seq->file));

	mutex_unlock(&ctx->buf_list.lock);
}

static int simple_context_seq_show(struct seq_file *seq, void *v)
{
	struct simple_buffer_entry *entry =
		list_entry(v, struct simple_buffer_entry, node);

	/* Ignore any overflow here, and allow the seq_file to grow */
	(void)seq_write(seq, entry->buf, entry->len);
	return 0;
}

static int simple_context_proc_open(struct inode *inode, struct file *file)
{
	static const struct seq_operations seq_ops = {
		.start = simple_context_seq_start,
		.next = simple_context_seq_next,
		.stop = simple_context_seq_stop,
		.show = simple_context_seq_show,
	};
	return seq_open(file, &seq_ops);
}

static ssize_t simple_context_proc_write(struct file *file,
				    const char __user *user_buf, size_t len,
				    loff_t *pos)
{
	struct simple_context *ctx = pde_data(file_inode(file));
	struct simple_buffer_entry *entry;
	ssize_t written;

	entry = kzalloc(struct_size(entry, buf, len), GFP_KERNEL);
	if (!entry)
		return 0;


	/* Copy the buffer over and track bytes written */
	written = len - copy_from_user(entry->buf, user_buf, len);
	entry->len = written;

	if (!written) {
		kfree(entry);
		return -EFAULT;
	}

	mutex_lock(&ctx->buf_list.lock);
	list_add_tail(&entry->node, &ctx->buf_list.head);
	mutex_unlock(&ctx->buf_list.lock);

	*pos += written;

	return written;
}

static int simple_context_procfs_init(struct simple_context *ctx)
{
	static const struct proc_ops proc_ops = {
		.proc_open = simple_context_proc_open,
		.proc_read_iter = seq_read_iter,
		.proc_lseek = seq_lseek,
		.proc_release = seq_release,
		.proc_write = simple_context_proc_write,
	};

	ctx->proc_entry = proc_create_data("simple", S_IRUSR | S_IWUSR, NULL,
					   &proc_ops, ctx);

	if (!ctx->proc_entry)
		return -ENOMEM;

	return 0;
}

static void simple_context_procfs_term(struct simple_context *ctx)
{
	proc_remove(ctx->proc_entry);
}

static int simple_context_buffer_list_init(struct simple_context *ctx)
{
	INIT_LIST_HEAD(&ctx->buf_list.head);
	mutex_init(&ctx->buf_list.lock);

	return 0;
}

static void simple_context_buffer_list_term(struct simple_context *ctx)
{
	struct list_head *cur, *tmp;

	/* We don't need to lock here as the procfs node was already destroyed */
	list_for_each_safe(cur, tmp, &ctx->buf_list.head)
	{
		struct simple_buffer_entry const *entry =
			list_entry(cur, struct simple_buffer_entry, node);
		kfree(entry);
	}

	mutex_destroy(&ctx->buf_list.lock);
}

/**
 * DOC: Initialization and teardown
 *
 * An array of init/term function pointer pairs are used for configuring various
 * chunks of the simple context. In case of error, unwinding the already
 * initialized state is managed by a helper function.
 * While this is possibly over-engineered for a simple module, this approach
 * avoids the prevalent but error prone "cascading goto's" style of error
 * recovery.
 */

/**
 * struct simple_context_init_term - Pair of corresponding init/term functions.
 */
struct simple_context_init_term {
    /** @init: Initializer. */
	int (*init)(struct simple_context *);
    /** @term: Destroyer. */
	void (*term)(struct simple_context *);
};

static const struct simple_context_init_term g_ctx_init[] = {
	{ simple_context_buffer_list_init, simple_context_buffer_list_term },
	{ simple_context_procfs_init, simple_context_procfs_term },
};

static void simple_context_term_partial(struct simple_context *ctx, int i)
{
	while (i-- > 0) {
		if (g_ctx_init[i].term)
			g_ctx_init[i].term(ctx);
	}
}

/**
 * simple_context_init() - Initialize a simple_context
 * @ctx: The context to initialize
 *
 * Initialize a pre-allocated simple_context in stages, with full unwinding in
 * case of error.
 *
 * Return: 0 in case of success, otherwise an error code.
 */
int simple_context_init(struct simple_context *ctx)
{
	int ret = 0;
	int i;

	for (i = 0; !ret && i < ARRAY_SIZE(g_ctx_init); ++i) {
		if (!g_ctx_init[i].init)
			continue;
		ret = g_ctx_init[i].init(ctx);
	}

	if (ret) {
		simple_context_term_partial(ctx, i);
		kfree(ctx);
	}

	return ret;
}

/**
 * simple_context_term() - Destroy a simple_context
 * @ctx: The context to destroy
 */
void simple_context_term(struct simple_context *ctx)
{
	simple_context_term_partial(ctx, ARRAY_SIZE(g_ctx_init));
}
