/*
 * Copyright (C) 2010 Sony Ericsson Mobile Communications AB.
 * Copyright (C) 2012 Sony Mobile Communications AB.
 *
 * Authors: Marcus Bauer <marcus.bauer@sonyericsson.com>
 *          Stefan Karlsson <stefan3.karlsson@sonymobile.com>
 *
 * NOTE: This file has been created by Sony Ericsson Mobile Communications AB.
 *       This file contains code from: bma150.c
 *       The orginal bma150.c header is included below:
 *
 * Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 *
 * START
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
/*
 * Protocol driver for Bosch BMA2XX accelerometer
 *
 */
#include <linux/bma2xx.h>
#include <linux/ctype.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/spi/spi.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#define BMA2XX_VENDORID                  0x0001
#define AUTOSUSPEND_MS                   50
#define MIN_RATE                         1
#define MAX_RATE                         10000

MODULE_LICENSE("GPL v2");
MODULE_ALIAS("bma2xx");

/* bma2xx register restrictions */
#define BMA2XX_LAST_REG 0x3F
#define BMA2XX_NA (1 << 8)
#define BMA2XX_RD (2 << 8)
#define BMA2XX_WR (4 << 8)
#define BMA2XX_RW (8 << 8)
static uint16_t bma2xx_mask[] = {
	0xFF | BMA2XX_RD, /* 00 */
	0xFF | BMA2XX_RD, /* 01 */
	0xC1 | BMA2XX_RD, /* 02 */
	0xFF | BMA2XX_RD, /* 03 */
	0xC1 | BMA2XX_RD, /* 04 */
	0xFF | BMA2XX_RD, /* 05 */
	0xC1 | BMA2XX_RD, /* 06 */
	0xFF | BMA2XX_RD, /* 07 */
	0xFF | BMA2XX_RD, /* 08 */
	0xF7 | BMA2XX_RD, /* 09 */
	0x80 | BMA2XX_RD, /* 0A */
	0xFF | BMA2XX_RD, /* 0B */
	0xFF | BMA2XX_RD, /* 0C */
	0x00 | BMA2XX_NA, /* 0D */
	0x00 | BMA2XX_NA, /* 0E */
	0x0F | BMA2XX_RW, /* 0F */
	0x1F | BMA2XX_RW, /* 10 */
	0xFE | BMA2XX_RW, /* 11 */
	0x00 | BMA2XX_NA, /* 12 */
	0xC0 | BMA2XX_RW, /* 13 */
	0xFF | BMA2XX_RW, /* 14 */
	0x00 | BMA2XX_NA, /* 15 */
	0xF7 | BMA2XX_RW, /* 16 */
	0x1F | BMA2XX_RW, /* 17 */
	0x00 | BMA2XX_NA, /* 18 */
	0xF7 | BMA2XX_RW, /* 19 */
	0x81 | BMA2XX_RW, /* 1A */
	0xF7 | BMA2XX_RW, /* 1B */
	0x00 | BMA2XX_NA, /* 1C */
	0x00 | BMA2XX_NA, /* 1D */
	0x37 | BMA2XX_RW, /* 1E */
	0x00 | BMA2XX_NA, /* 1F */
	0x0F | BMA2XX_RW, /* 20 */
	0x8F | BMA2XX_RW, /* 21 */
	0xFF | BMA2XX_RW, /* 22 */
	0xFF | BMA2XX_RW, /* 23 */
	0xc7 | BMA2XX_RW, /* 24 */
	0xFF | BMA2XX_RW, /* 25 */
	0xFF | BMA2XX_RW, /* 26 */
	0x03 | BMA2XX_RW, /* 27 */
	0xFF | BMA2XX_RW, /* 28 */
	0x00 | BMA2XX_NA, /* 29 */
	0xC7 | BMA2XX_RW, /* 2A */
	0xDF | BMA2XX_RW, /* 2B */
	0x7F | BMA2XX_RW, /* 2C */
	0x3F | BMA2XX_RW, /* 2D */
	0x3F | BMA2XX_RW, /* 2E */
	0x30 | BMA2XX_RW, /* 2F */
	0x00 | BMA2XX_NA, /* 30 */
	0x01 | BMA2XX_RD, /* 31 */
	0x00 | BMA2XX_NA, /* 32 */
	0x0F | BMA2XX_RW, /* 33 */
	0x03 | BMA2XX_RW, /* 34 */
	0x00 | BMA2XX_NA, /* 35 */
	0xF3 | BMA2XX_RW, /* 36 */
	0x7F | BMA2XX_RW, /* 37 */
	0xFF | BMA2XX_RW, /* 38 */
	0xFF | BMA2XX_RW, /* 39 */
	0xFF | BMA2XX_RW, /* 3A */
	0xFF | BMA2XX_RW, /* 3B */
	0xFF | BMA2XX_RW, /* 3C */
	0xFF | BMA2XX_RW, /* 3D */
	0x00 | BMA2XX_NA, /* 3E */
	0x00 | BMA2XX_NA, /* 3F */
};

/*
 * Data returned from accelerometer.
 * Temp is in units of 0.5 degrees C
 */
struct bma2xx_accel_data {
	short            accel_x;
	short            accel_y;
	short            accel_z;
	int              temp;
};

struct bma2xx_data {
	struct input_dev            *ip_dev;
	struct i2c_client           *ic_dev;
#ifdef CONFIG_DEBUG_FS
	struct dentry               *dbfs_root;
	struct dentry               *dbfs_regs;
#endif
	struct bma2xx_platform_data *pdata;
	u8                           chip_id;
	unsigned char                shift;
	atomic_t                     rate;
	struct workqueue_struct	    *workqueue;
	struct delayed_work          work;
	bool                         powered;
};

static int bma2xx_ic_read(struct i2c_client *ic_dev, u8 reg, u8 *buf, int len)
{
	int rc;
	rc = i2c_smbus_read_i2c_block_data(ic_dev, reg, len, buf);

	if (rc > 0)
		return 0;
	if (rc != len)
		return -ENXIO;
	return rc;
}

int bma2xx_ic_userwrite(struct i2c_client *ic_dev, u8 reg, u8 val)
{
	int rc;

	if (reg > BMA2XX_LAST_REG) {
		dev_err(&ic_dev->dev, "%s: reg 0x%.02X, out of range.\n",
			__func__, reg);
		return -EINVAL;
	}
	if (bma2xx_mask[reg] & BMA2XX_NA) {
		dev_err(&ic_dev->dev, "%s: reg 0x%.02X, reserved.\n",
			__func__, reg);
		return -EINVAL;
	}
	if (bma2xx_mask[reg] & BMA2XX_RD) {
		dev_err(&ic_dev->dev, "%s: reg 0x%.02X, read only.\n",
			__func__, reg);
		return -EINVAL;
	}
	if (val & ~bma2xx_mask[reg]) {
		dev_err(&ic_dev->dev, "%s: reg 0x%.02X val 0x%.02X, out of "
					"bounds.\n", __func__, reg, val);
		return -EINVAL;
	}
	rc = i2c_smbus_write_byte_data(ic_dev, reg, val);
	return rc;
}

static inline int bma2xx_ic_write(struct i2c_client *ic_dev, u8 reg, u8 val)
{
	return i2c_smbus_write_byte_data(ic_dev, reg, val);
}

/* 10/12bit to 16 bit will give a 6/4 bit offset, shift
   it down to represent 1024 lsb/g for alla ranges */
static inline u8 bma2xx_range2shift(u8 range)
{
	switch (range) {
	case BMA2XX_RANGE_16G:
		return 1;
	case BMA2XX_RANGE_8G:
		return 2;
	case BMA2XX_RANGE_4G:
		return 3;
	default:
	case BMA2XX_RANGE_2G:
		return 4;
	}
}

#if defined(CONFIG_DEBUG_FS)
static int bma2xx_dbfs_open(struct inode *inode, struct file *fp)
{
	fp->private_data = inode->i_private;
	return 0;
}

static ssize_t bma2xx_dbfs_write(struct file *fp, const char __user *buf,
				 size_t count, loff_t *f_pos)
{
	u8                          *p;
	u8                          *np;
	u8                          *mbuf;
	int                          rc;
	unsigned int                 val;
	u8                           reg;
	u8                           data;
	struct bma2xx_data          *bd;

	/* format of write data is "A[A] D[D]" eg. "AA DD", "A D" etc
	   where A is address in hex, D is data in hex.
	   Multiple address/data pairs may be separated by spaces.
	*/
	if (count < 3)
		return 0;

	bd = fp->private_data;

	mbuf = kzalloc(count, GFP_KERNEL);
	if (!mbuf) {
		rc = -ENOMEM;
		goto dbfs_write_exit;
	}

	if (copy_from_user(mbuf, buf, count)) {
		rc = -EFAULT;
		goto dbfs_write_exit_copy;
	}

	p = mbuf;

	while (isspace(*p))
		p++;
	do {
		val = simple_strtoul(p, (char **)&np, 16);
		if ((val > BMA2XX_LAST_REG) || (p == np)) {
			rc = -EINVAL;
			goto dbfs_write_exit_copy;
		}
		while (isspace(*np) && ((np - mbuf) < count))
			np++;
		p = np;
		reg = (u8)val;

		val = simple_strtoul(p, (char **)&np, 16);
		if ((val > 0xFF)  || (p == np)) {
			rc = -EINVAL;
			goto dbfs_write_exit_copy;
		}
		while (isspace(*np) && ((np - mbuf) < count))
			np++;
		p = np;
		data = (u8)val;

		rc = bma2xx_ic_userwrite(bd->ic_dev, reg, data);

		/* update here to avoid checking g-range at each interrupt */
		if ((!rc) && (reg == BMA2XX_RANGE_REG))
			bd->shift = bma2xx_range2shift(data);

	} while (!rc && (np - mbuf) < count);

	if (rc)
		goto dbfs_write_exit;
	kfree(mbuf);

	return count;

dbfs_write_exit_copy:
	kfree(mbuf);
dbfs_write_exit:
	return rc;
}

static ssize_t bma2xx_dbfs_read(struct file *fp, char __user *buf,
				size_t count, loff_t *f_pos)
{
	u8                           rx;
	u8                           mbuf[8];
	int                          rc;
	int                          copy_size;
	struct bma2xx_data          *bd;

	bd = fp->private_data;
	if ((int)*f_pos > BMA2XX_LAST_REG) {
		rc = 0;
		goto dbfs_read_exit;
	}

	rc = bma2xx_ic_read(bd->ic_dev, (u8)*f_pos, &rx, 1);
	if (rc)
		goto dbfs_read_exit;

	snprintf(mbuf, ARRAY_SIZE(mbuf), "%02x %02x\n", (u8)*f_pos, rx);
	copy_size = min(count, strnlen(mbuf, ARRAY_SIZE(mbuf)) + 1);
	if (copy_to_user(buf, mbuf, copy_size))
		return -EFAULT;
	(*f_pos)++;

	return copy_size;
dbfs_read_exit:
	return rc;
}

static const struct file_operations dbfs_fops = {
	.owner    = THIS_MODULE,
	.open     = bma2xx_dbfs_open,
	.read     = bma2xx_dbfs_read,
	.write    = bma2xx_dbfs_write,
};

static void bma2xx_create_dbfs_entry(struct bma2xx_data *bd)
{
	char buf[16];

	snprintf(buf, sizeof(buf), BMA2XX_NAME);
	bd->dbfs_root = debugfs_create_dir(buf, NULL);
	if (bd->dbfs_root == NULL)
		goto dbfs_err_root;

	bd->dbfs_regs = debugfs_create_file("registers",
					   S_IRUGO | S_IWUGO,
					   bd->dbfs_root, bd,
					   &dbfs_fops);
	if (bd->dbfs_regs == NULL)
		goto dbfs_err_regs;
	return;

dbfs_err_regs:
	debugfs_remove(bd->dbfs_root);
dbfs_err_root:
	return;
}

static void bma2xx_remove_dbfs_entry(struct bma2xx_data *bd)
{
	if (bd->dbfs_regs)
		debugfs_remove(bd->dbfs_regs);
	if (bd->dbfs_root)
		debugfs_remove(bd->dbfs_root);
}
#else
static void bma2xx_create_dbfs_entry(struct bma2xx_data *bd) { }

static void bma2xx_remove_dbfs_entry(struct bma2xx_data *bd) { }
#endif

static inline void bma2xx_suspend_sleep(struct bma2xx_data *bd)
{
	if (bd->chip_id != BMA2XX_CHIP_ID_BMA250)
		usleep(450);
}

static inline void bma2xx_reset_sleep(struct bma2xx_data *bd)
{
	if (bd->chip_id != BMA2XX_CHIP_ID_BMA250)
		usleep(1800);
	else
		usleep(4000);
}

static int bma2xx_power_down(struct bma2xx_data *bd)
{
	int rc = 0;

	if (bd->powered) {
		if (bd->chip_id == BMA2XX_CHIP_ID_BMA250)
			rc = bma2xx_ic_write(bd->ic_dev, BMA2XX_MODE_CTRL_REG,
						BMA2XX_MODE_SUSPEND);
		else
			rc = bma2xx_ic_write(bd->ic_dev, BMA2XX_MODE_CTRL_REG,
						BMA2XX_MODE_DEEPSUSPEND);
		if (rc)
			dev_err(&bd->ic_dev->dev, "%s: Failed to suspend "
							"chip\n", __func__);
		if (bd->pdata->power_down) {
			if (!bd->pdata->power_down(&bd->ic_dev->dev))
				bd->powered = false;
			else
				dev_err(&bd->ic_dev->dev, "%s: Failed to power"
					" down externally\n", __func__);
		} else {
			bd->powered = false;
		}
	}
	return rc;
}

static int bma2xx_power_up(struct bma2xx_data *bd)
{
	int rc;
	if (!bd->powered) {
		if (bd->pdata->power_up) {
			rc = bd->pdata->power_up(&bd->ic_dev->dev);
			if (!rc) {
				bd->powered = true;
			} else {
				dev_err(&bd->ic_dev->dev, "%s: Failed to power"
						" up externally\n", __func__);
				return rc;
			}
		} else {
			bd->powered = true;
		}
	}
	bma2xx_suspend_sleep(bd);
	rc = bma2xx_ic_write(bd->ic_dev, BMA2XX_RESET_REG,
						BMA2XX_RESET);
	if (rc) {
		dev_err(&bd->ic_dev->dev, "%s: Failed to reset chip\n",
							 __func__);
		return rc;
	}
	bma2xx_reset_sleep(bd);
	return 0;
}

static int bma2xx_read_chip_id(struct bma2xx_data *bd)
{
	int rc = 0;

	rc = bma2xx_ic_read(bd->ic_dev, BMA2XX_CHIP_ID_REG,
						&bd->chip_id, 1);
	if (rc) {
		dev_err(&bd->ic_dev->dev, "%s: unable to read chip id\n",
								__func__);
		goto read_id_exit;
	}

	dev_info(&bd->ic_dev->dev, "bma2xx: detected chip id %d\n",
							bd->chip_id);
read_id_exit:
	return rc;
}


static inline int bma2xx_report_data(struct bma2xx_data *bd)
{
	struct bma2xx_accel_data    data;
	int                         rc;
	u8                          rx_buf[7];
	int                         len;
	u8                          mask;

	if (bd->chip_id == BMA2XX_CHIP_ID_BMA250) {
		len = 7;
		mask = 0xC0;
	} else {
		len = 6;
		mask = 0xF0;
	}

	rc = bma2xx_ic_read(bd->ic_dev, BMA2XX_X_AXIS_LSB_REG, rx_buf, len);
	if (rc) {
		dev_err(&bd->ic_dev->dev,
			"%s: device failed, error %d\n", __func__, rc);
		return rc;
	}

	data.accel_x = (rx_buf[1] << 8) | (rx_buf[0] & mask);
	data.accel_y = (rx_buf[3] << 8) | (rx_buf[2] & mask);
	data.accel_z = (rx_buf[5] << 8) | (rx_buf[4] & mask);

	/* sensitivty 1024lsb/g for all g-ranges */
	data.accel_x = data.accel_x >> bd->shift;
	data.accel_y = data.accel_y >> bd->shift;
	data.accel_z = data.accel_z >> bd->shift;

	input_report_abs(bd->ip_dev, ABS_X, data.accel_x);
	input_report_abs(bd->ip_dev, ABS_Y, data.accel_y);
	input_report_abs(bd->ip_dev, ABS_Z, data.accel_z);

	if (bd->chip_id == BMA2XX_CHIP_ID_BMA250) {
		/* sensitivty 0.5C, center temprature 24C */
		data.temp = (signed char)rx_buf[6] + 24*2;
		input_report_abs(bd->ip_dev, ABS_MISC, data.temp);
	}

	input_sync(bd->ip_dev);
	return 0;
}

static int bma2xx_write_settings(struct bma2xx_data *bd)
{
	int rc = 0;

	rc = bma2xx_ic_write(bd->ic_dev, BMA2XX_RANGE_REG, bd->pdata->range
								& 0x0F);
	if (rc)
		goto error;
	if (bd->chip_id == BMA2XX_CHIP_ID_BMA250)
		rc = bma2xx_ic_write(bd->ic_dev, BMA2XX_BW_SEL_REG,
							BMA2XX_BW_31_25HZ);
	else
		rc = bma2xx_ic_write(bd->ic_dev, BMA2XX_BW_SEL_REG,
							BMA2XX_BW_125HZ);
	if (rc)
		goto error;
	return rc;
error:
	dev_err(&bd->ic_dev->dev, "%s: device failed, error %d\n",
							__func__, rc);
	return rc;
}

static void work_f(struct work_struct *work)
{
	struct bma2xx_data *bd =
		container_of(work, struct bma2xx_data, work.work);
	int rc;

	queue_delayed_work(bd->workqueue, &bd->work,
				msecs_to_jiffies(atomic_read(&bd->rate)));
	pm_runtime_mark_last_busy(&bd->ic_dev->dev);
	rc = pm_runtime_get_sync(&bd->ic_dev->dev);
	if (rc < 0) {
		dev_err(&bd->ic_dev->dev, "%s: Failed to runtime resume; "
				"rc = %d\n", __func__, rc);
		pm_runtime_set_suspended(&bd->ic_dev->dev);
		pm_runtime_put_noidle(&bd->ic_dev->dev);
		return;
	}

	bma2xx_report_data(bd);
	pm_runtime_put_autosuspend(&bd->ic_dev->dev);
}

static ssize_t bma2xx_rate_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct bma2xx_data *bd = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&bd->rate));
}

static ssize_t bma2xx_rate_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	int rc = 0;
	struct bma2xx_data *bd = dev_get_drvdata(dev);
	unsigned long val;

	rc = strict_strtoul(buf, 10, &val);
	if (rc)
		return rc;
	if (val < MIN_RATE || val > MAX_RATE)
		return -EINVAL;

	atomic_set(&bd->rate, val);
	return count;
}

static ssize_t bma2xx_chip_id_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct bma2xx_data *bd = dev_get_drvdata(dev);
	return snprintf(buf, PAGE_SIZE, "%d\n", bd->chip_id);
}

static struct device_attribute attributes[] = {
	__ATTR(bma2xx_rate, 0644, bma2xx_rate_show, bma2xx_rate_store),
	__ATTR(bma2xx_chip_id, 0444, bma2xx_chip_id_show, NULL),
};

static int add_sysfs_interfaces(struct device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		if (device_create_file(dev, attributes + i))
			goto undo;
	return 0;
undo:
	for (; i >= 0 ; i--)
		device_remove_file(dev, attributes + i);
	dev_err(dev, "%s: failed to create sysfs interface\n", __func__);
	return -ENODEV;
}

static void remove_sysfs_interfaces(struct device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		device_remove_file(dev, attributes + i);
}

static int bma2xx_runtime_resume(struct device *dev)
{
	struct bma2xx_data *bd = dev_get_drvdata(dev);
	int rc;

	rc = bma2xx_power_up(bd);
	if (rc) {
		dev_err(dev, "%s: failed to power up; error %d\n",
								__func__, rc);
		return -EAGAIN;
	}
	rc = bma2xx_write_settings(bd);
	if (rc) {
		dev_err(dev, "%s: failed to write settings; error %d\n"
							, __func__, rc);
		return -EAGAIN;
	}
	usleep(20000);
	return 0;
}

#ifdef CONFIG_PM_RUNTIME
static int bma2xx_runtime_suspend(struct device *dev)
{
	struct bma2xx_data *bd = dev_get_drvdata(dev);
	int rc;

	rc = bma2xx_power_down(bd);
	if (rc)
		dev_err(dev,
			"%s: failed to power down; error %d\n", __func__, rc);
	return 0;
}
#endif

static int bma2xx_open_acc(struct input_dev *dev)
{
	struct bma2xx_data *bd = input_get_drvdata(dev);

	queue_delayed_work(bd->workqueue, &bd->work,
		msecs_to_jiffies(atomic_read(&bd->rate)));
	return 0;
}

static void bma2xx_release_acc(struct input_dev *dev)
{
	struct bma2xx_data *bd = input_get_drvdata(dev);

	cancel_delayed_work_sync(&bd->work);
}

#ifdef CONFIG_PM_SLEEP
static int bma2xx_resume(struct device *dev)
{
	struct bma2xx_data *bd = dev_get_drvdata(dev);

	bma2xx_runtime_resume(dev);
	if (bd->ip_dev->users)
		queue_delayed_work(bd->workqueue, &bd->work,
			msecs_to_jiffies(atomic_read(&bd->rate)));
	return 0;
}

static int bma2xx_suspend(struct device *dev)
{
	struct bma2xx_data *bd = dev_get_drvdata(dev);

	cancel_delayed_work_sync(&bd->work);
	bma2xx_power_down(bd);
	return 0;
}
#endif

static int __devinit bma2xx_probe(struct i2c_client *ic_dev,
		const struct i2c_device_id *id)
{
	struct bma2xx_data *bd;
	int                 rc;
	struct bma2xx_platform_data *pdata = ic_dev->dev.platform_data;

	if (!pdata) {
		rc = -EINVAL;
		goto probe_exit;
	}

	bd = kzalloc(sizeof(struct bma2xx_data), GFP_KERNEL);
	if (!bd) {
		rc = -ENOMEM;
		goto probe_exit;
	}

	bd->ic_dev = ic_dev;
	bd->pdata = pdata;
	bd->shift = bma2xx_range2shift(bd->pdata->range);
	bd->powered = false;
	INIT_DELAYED_WORK(&bd->work, work_f);
	/* initial configuration */
	atomic_set(&bd->rate, 200);

	i2c_set_clientdata(ic_dev, bd);

	if (bd->pdata->setup) {
		rc = bd->pdata->setup(&bd->ic_dev->dev);
		if (rc) {
			dev_err(&bd->ic_dev->dev,
				"%s: failed to setup in probe"
				" with error %d\n", __func__, rc);
			goto probe_err_setup;
		}
	}

	pm_runtime_set_active(&bd->ic_dev->dev);
	pm_runtime_enable(&bd->ic_dev->dev);

	rc = bma2xx_runtime_resume(&bd->ic_dev->dev);
	if (rc) {
		dev_err(&bd->ic_dev->dev, "%s: failed to power up;"
					" rc = %d\n", __func__, rc);
		bma2xx_power_down(bd);
		goto probe_err_config;
	}

	rc = bma2xx_read_chip_id(bd);
	if (rc) {
		dev_err(&bd->ic_dev->dev, "%s: failed to read chip id;"
					" rc = %d\n", __func__, rc);
		bma2xx_power_down(bd);
		goto probe_err_config;
	}


	bd->workqueue = create_singlethread_workqueue("bma2xx_workqueue");
	if (!bd->workqueue) {
		dev_err(&bd->ic_dev->dev, "%s, failed to allocate workqueue\n",
								__func__);
		rc = -ENOMEM;
		goto probe_err_config;
	}

	bma2xx_create_dbfs_entry(bd);

	bd->ip_dev = input_allocate_device();
	if (!bd->ip_dev) {
		rc = -ENOMEM;
		goto probe_err_reg;
	}
	input_set_drvdata(bd->ip_dev, bd);
	bd->ip_dev->open       = bma2xx_open_acc;
	bd->ip_dev->close      = bma2xx_release_acc;
	bd->ip_dev->name       = BMA2XX_NAME;
	bd->ip_dev->id.vendor  = BMA2XX_VENDORID;
	bd->ip_dev->id.product = 1;
	bd->ip_dev->id.version = 1;
	__set_bit(EV_ABS,       bd->ip_dev->evbit);
	__set_bit(ABS_X,        bd->ip_dev->absbit);
	__set_bit(ABS_Y,        bd->ip_dev->absbit);
	__set_bit(ABS_Z,        bd->ip_dev->absbit);
	if (bd->chip_id == BMA2XX_CHIP_ID_BMA250)
		__set_bit(ABS_MISC,     bd->ip_dev->absbit);
	input_set_abs_params(bd->ip_dev, ABS_X, -16384, 16383, 0, 0);
	input_set_abs_params(bd->ip_dev, ABS_Y, -16384, 16383, 0, 0);
	input_set_abs_params(bd->ip_dev, ABS_Z, -16384, 16383, 0, 0);
	if (bd->chip_id == BMA2XX_CHIP_ID_BMA250)
		input_set_abs_params(bd->ip_dev, ABS_MISC, -80, 175, 0, 0);

	rc = input_register_device(bd->ip_dev);
	if (rc) {
		input_free_device(bd->ip_dev);
		goto probe_err_reg;
	}

	rc = add_sysfs_interfaces(&bd->ic_dev->dev);
	if (rc)
		goto probe_err_sysfs;

	pm_runtime_set_autosuspend_delay(&bd->ic_dev->dev, AUTOSUSPEND_MS);
	pm_runtime_use_autosuspend(&bd->ic_dev->dev);
	pm_runtime_mark_last_busy(&bd->ic_dev->dev);

	return rc;
probe_err_sysfs:
	input_unregister_device(bd->ip_dev);
probe_err_reg:
	destroy_workqueue(bd->workqueue);
	bma2xx_remove_dbfs_entry(bd);
probe_err_config:
	pm_runtime_disable(&bd->ic_dev->dev);
	if (bd->pdata->teardown)
		bd->pdata->teardown(&bd->ic_dev->dev);
probe_err_setup:
	i2c_set_clientdata(ic_dev, NULL);
	kfree(bd);
probe_exit:
	return rc;
}

static int __devexit bma2xx_remove(struct i2c_client *ic_dev)
{
	struct bma2xx_data *bd;

	bd = i2c_get_clientdata(ic_dev);

	cancel_delayed_work_sync(&bd->work);
	remove_sysfs_interfaces(&bd->ic_dev->dev);
	input_unregister_device(bd->ip_dev);
	destroy_workqueue(bd->workqueue);
	bma2xx_remove_dbfs_entry(bd);

	if (!pm_runtime_suspended(&bd->ip_dev->dev))
		bma2xx_power_down(bd);
	pm_runtime_disable(&bd->ip_dev->dev);

	if (bd->pdata->teardown)
		bd->pdata->teardown(&ic_dev->dev);
	i2c_set_clientdata(ic_dev, NULL);
	kfree(bd);
	return 0;
}

static const struct i2c_device_id bma2xx_i2c_id[] = {
	{BMA2XX_NAME, 0},
	{}
};

static const struct dev_pm_ops bma2xx_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(bma2xx_suspend, bma2xx_resume)
	SET_RUNTIME_PM_OPS(bma2xx_runtime_suspend, bma2xx_runtime_resume, NULL)
};

static struct i2c_driver bma2xx_driver = {
	.driver = {
		.name  = BMA2XX_NAME,
		.owner = THIS_MODULE,
		.pm	= &bma2xx_pm_ops,
	},
	.probe         = bma2xx_probe,
	.remove        = __devexit_p(bma2xx_remove),
	.id_table      = bma2xx_i2c_id,
};

static int __init bma2xx_init(void)
{
	return i2c_add_driver(&bma2xx_driver);
}
module_init(bma2xx_init);

static void __exit bma2xx_exit(void)
{
	i2c_del_driver(&bma2xx_driver);
}
module_exit(bma2xx_exit);
