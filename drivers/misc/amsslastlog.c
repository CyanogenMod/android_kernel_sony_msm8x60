/* drivers/misc/amsslastlog.c
 *
 * Copyright (C) 2012 Sony Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include "../../arch/arm/mach-msm/smd_private.h"

struct amss_lastlog {
	uint32_t sig;
	uint32_t size;
	uint8_t  data[0];
};

#define AMSS_LASTLOG_SIG 0x12095015

static char *amsslastlog;
static size_t amsslastlog_size;
static void *amsslastlog_backup_addr;
static size_t amsslastlog_backup_size;

static ssize_t amsslastlog_read(struct file *file, char __user *buf,
				    size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	if (pos >= amsslastlog_size)
		return 0;

	count = min(len, (size_t)(amsslastlog_size - pos));
	if (copy_to_user(buf, amsslastlog + pos, count))
		return -EFAULT;

	*offset += count;
	return count;
}

static const struct file_operations amsslastlog_file_ops = {
	.owner = THIS_MODULE,
	.read = amsslastlog_read,
};


static int amsslastlog_backup_handler(struct notifier_block *this,
				unsigned long event, void *ptr)
{
	struct amss_lastlog *buffer = amsslastlog_backup_addr;
	char *smem_errlog = NULL;
	size_t size;

	smem_errlog = smem_get_entry(SMEM_ERR_CRASH_LOG, &size);
	if (smem_errlog && !memcmp(smem_errlog, "ERR", 3)) {
		buffer->sig = AMSS_LASTLOG_SIG;
		buffer->size = size - 2 * sizeof(unsigned int);
		if (buffer->size > amsslastlog_backup_size)
			buffer->size = amsslastlog_backup_size;

		printk(KERN_INFO "Backup of amsslog %d bytes\n", buffer->size);
		memcpy(buffer->data, smem_errlog, buffer->size);
	} else {
		printk(KERN_INFO "No amsslog to backup\n");
	}

	return NOTIFY_DONE;
}


static struct notifier_block panic_amsslastlog_backup = {
	.notifier_call  = amsslastlog_backup_handler,
};

static int amsslastlog_driver_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct proc_dir_entry *entry;
	struct amss_lastlog *buffer;
	size_t size;
	int ret = 0;

	res = platform_get_resource_byname(pdev,
		IORESOURCE_MEM, "amsslastlog");
	if (!res || !res->start || res->end < res->start) {
		printk(KERN_ERR "%s: amsslastlog resource invalid/absent\n",
				__func__);
		return -ENODEV;
	}

	size = res->end - res->start + 1;
	buffer = (struct amss_lastlog *)ioremap(res->start, size);
	if (buffer == NULL) {
		printk(KERN_ERR "amsslastlog: failed to map memory\n");
		return -ENOMEM;
	}
	/* Check for old buffer to map to proc entry */
	if (buffer->sig == AMSS_LASTLOG_SIG) {
		printk(KERN_INFO
		       "amsslastlog:old log found at 0x%x\n",
			(unsigned int)buffer);
		amsslastlog = kmalloc(size, GFP_KERNEL);
		if (amsslastlog == NULL) {
			printk(KERN_ERR
			       "amsslastlog: failed to allocate copy buffer\n");
			ret = -ENOMEM;
		} else {
			amsslastlog_size = min(buffer->size, size);
			memcpy(amsslastlog, buffer->data, amsslastlog_size);
			printk(KERN_INFO "amsslastlog:copied %d bytes\n",
				amsslastlog_size);
			entry = create_proc_entry("last_amsslog",
						   S_IFREG | S_IRUGO, NULL);
			if (!entry) {
				printk(KERN_ERR
				       "amsslastlog: failed to create proc entry\n");
				kfree(amsslastlog);
				amsslastlog = NULL;
				ret = -ENOMEM;
			} else {
				entry->proc_fops = &amsslastlog_file_ops;
				entry->size = amsslastlog_size;
			}
		}
		memset(buffer, 0, size);
		iounmap(buffer);
	} else {
		printk(KERN_INFO "amsslastlog: no old log\n");
	}

	/* Store addr/size of current buffer for later save */
	amsslastlog_backup_size = res->end - res->start + 1;
	amsslastlog_backup_addr = ioremap_nocache(res->start,
		amsslastlog_backup_size);
	if (!amsslastlog_backup_addr)
		printk(KERN_ERR "FAILED to nocache map amsslastlog_addr!!\n");

	/* Register to panic chain so log is saved during reboot */
	atomic_notifier_chain_register(&panic_notifier_list,
		&panic_amsslastlog_backup);
	return 0;
}

static struct platform_driver amsslastlog_driver = {
	.probe = amsslastlog_driver_probe,
	.driver = {
		.name	= "amsslastlog",
	},
};

static int __init amsslastlog_module_init(void)
{
	int err;
	err = platform_driver_register(&amsslastlog_driver);
	return err;
}

MODULE_AUTHOR("Sony Mobile Communications AB");
MODULE_DESCRIPTION("amss last log");
MODULE_LICENSE("GPL V2");

module_init(amsslastlog_module_init);

