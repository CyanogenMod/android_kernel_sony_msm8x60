/* kernel/arch/arm/mach-msm/felica-blue_common.c
 *
 * Copyright (C) 2010-2011 Sony Ericsson Mobile Communications AB.
 * Copyright (C) 2012 Sony Mobile Communications AB.
 *
 * Author: Hiroaki.Kuriyama <Hiroaki.Kuriyama@sonyericsson.com>
 * Author: Satomi.Watanabe <Satomi.Watanabe@sonyericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/felica.h>
#include <linux/delay.h>
#include "pm8921-gpio-mpp-blue.h"

#define PM_GPIO_FELICA_LOCK   31
#define PM_GPIO_FELICA_FF     32
#define PM_GPIO_FELICA_PON    33
#define MSM_GPIO_FELICA_RFS   19
#define MSM_GPIO_FELICA_INT   106

#define CEN_RETRY_MAX   5

static int semc_felica_device_init(struct felica_dev *dev)
{
	int ret;

	if (!dev)
		return -EINVAL;
	dev_dbg(dev->dev, ": %s\n", __func__);

	/* GPIO request */
	ret = gpio_request(
		PM8921_GPIO_PM_TO_SYS(PM_GPIO_FELICA_LOCK), "felica_lock");
	if (ret) {
		dev_err(dev->dev, "%s: request err LOCK%d\n",
				__func__, ret);
		goto err_lock;
	}
	ret = gpio_request(
		PM8921_GPIO_PM_TO_SYS(PM_GPIO_FELICA_FF), "felica_ff");
	if (ret) {
		dev_err(dev->dev, "%s: request err FF%d\n",
				__func__, ret);
		goto err_ff;
	}
	ret = gpio_request(
		PM8921_GPIO_PM_TO_SYS(PM_GPIO_FELICA_PON), "felica_pon");
	if (ret) {
		dev_err(dev->dev, "%s: request err PON%d\n",
				__func__, ret);
		goto err_pon;
	}
	ret = gpio_request(MSM_GPIO_FELICA_RFS, "felica_rfs");
	if (ret) {
		dev_err(dev->dev, "%s: request err RFS%d\n",
				__func__, ret);
		goto err_rfs;
	}
	ret = gpio_request(MSM_GPIO_FELICA_INT, "felica_int");
	if (ret) {
		dev_err(dev->dev, "%s: request err INT%d\n",
				__func__, ret);
		goto err_int;
	}

	return 0;

err_int:
	gpio_free(MSM_GPIO_FELICA_RFS);
err_rfs:
	gpio_free(PM8921_GPIO_PM_TO_SYS(PM_GPIO_FELICA_PON));
err_pon:
	gpio_free(PM8921_GPIO_PM_TO_SYS(PM_GPIO_FELICA_FF));
err_ff:
	gpio_free(PM8921_GPIO_PM_TO_SYS(PM_GPIO_FELICA_LOCK));
err_lock:
	return -ENODEV;
}

static int semc_felica_device_release(struct felica_dev *dev)
{
	if (!dev)
		return -EINVAL;
	dev_dbg(dev->dev, ": %s\n", __func__);

	gpio_free(MSM_GPIO_FELICA_INT);
	gpio_free(MSM_GPIO_FELICA_RFS);
	gpio_free(PM8921_GPIO_PM_TO_SYS(PM_GPIO_FELICA_PON));
	gpio_free(PM8921_GPIO_PM_TO_SYS(PM_GPIO_FELICA_FF));
	gpio_free(PM8921_GPIO_PM_TO_SYS(PM_GPIO_FELICA_LOCK));

	return 0;
}

static int semc_felica_cen_init(struct felica_dev *dev)
{
	if (!dev)
		return -EINVAL;
	dev_dbg(dev->device_cen.this_device, ": %s\n", __func__);

	return 0;
}

static int semc_felica_cen_read(u8 *buf, struct felica_dev *dev)
{
	int st = 0;

	if (!buf || !dev)
		return -EINVAL;
	dev_dbg(dev->device_cen.this_device, ": %s\n", __func__);

	st = gpio_get_value_cansleep(
		PM8921_GPIO_PM_TO_SYS(PM_GPIO_FELICA_LOCK));

	dev_dbg(dev->device_cen.this_device,
			": FELICA_LOCK = 0x%x\n", st);
	*buf = (st) ? 0x1 : 0x0;

	return 0;
}

static int semc_felica_cen_write(u8 arg, struct felica_dev *dev)
{
	int i;
	u8 state;

	if (!dev)
		return -EINVAL;
	dev_dbg(dev->device_cen.this_device, ": %s\n", __func__);

	if (arg > 0x1) {
		dev_err(dev->device_cen.this_device,
			"%s: Error. Invalid val @CEN write.\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < CEN_RETRY_MAX; i++) {
		semc_felica_cen_read(&state, dev);
		if (arg == state)
			goto end;
		gpio_set_value_cansleep(
			PM8921_GPIO_PM_TO_SYS(PM_GPIO_FELICA_FF), 0);
		msleep_interruptible(1);
		gpio_set_value_cansleep(
			PM8921_GPIO_PM_TO_SYS(PM_GPIO_FELICA_FF), 1);
		msleep_interruptible(1);
		gpio_set_value_cansleep(
			PM8921_GPIO_PM_TO_SYS(PM_GPIO_FELICA_FF), 0);
	}
	dev_err(dev->device_cen.this_device,
			"%s: Error. Cannot write CEN.\n", __func__);
	return -EIO;

end:
	return 0;
}

static int semc_felica_pon_init(struct felica_dev *dev)
{
	if (!dev)
		return -EINVAL;
	dev_dbg(dev->device_pon.this_device, ": %s\n", __func__);

	return 0;
}

static void semc_felica_pon_write(int val, struct felica_dev *dev)
{
	if (!dev)
		return;
	dev_dbg(dev->device_pon.this_device, ": %s\n", __func__);

	gpio_set_value_cansleep(PM8921_GPIO_PM_TO_SYS(PM_GPIO_FELICA_PON), val);
}

static void semc_felica_pon_release(struct felica_dev *dev)
{
	if (!dev)
		return;
	dev_dbg(dev->device_pon.this_device, ": %s\n", __func__);
}

static int semc_felica_tvdd_on(struct felica_dev *dev)
{
	if (!dev)
		return -EINVAL;
	dev_dbg(dev->dev, ": %s\n", __func__);

	return 0;
}

static void semc_felica_tvdd_off(struct felica_dev *dev)
{
	if (!dev)
		return;
	dev_dbg(dev->dev, ": %s\n", __func__);
}

static int semc_felica_rfs_init(struct felica_dev *dev)
{
	if (!dev)
		return -EINVAL;
	dev_dbg(dev->device_rfs.this_device, ": %s\n", __func__);

	return 0;
}

static int semc_felica_rfs_read(struct felica_dev *dev)
{
	if (!dev)
		return -EINVAL;
	dev_dbg(dev->device_rfs.this_device, ": %s\n", __func__);

	return gpio_get_value(MSM_GPIO_FELICA_RFS);
}

static void semc_felica_rfs_release(struct felica_dev *dev)
{
	if (!dev)
		return;
	dev_dbg(dev->device_rfs.this_device, ": %s\n", __func__);
}

static int semc_felica_int_init(struct felica_dev *dev)
{
	if (!dev)
		return -EINVAL;
	dev_dbg(dev->dev, ": %s\n", __func__);

	return 0;
}

static int semc_felica_int_read(struct felica_dev *dev)
{
	if (!dev)
		return -EINVAL;
	dev_dbg(dev->dev, ": %s\n", __func__);

	return gpio_get_value(MSM_GPIO_FELICA_INT);
}

static void semc_felica_int_release(struct felica_dev *dev)
{
	if (!dev)
		return;
	dev_dbg(dev->dev, ": %s\n", __func__);
}

static struct felica_platform_data semc_felica_pfdata = {
	.cen_pfdata = {
		.cen_init = semc_felica_cen_init,
		.cen_read = semc_felica_cen_read,
		.cen_write = semc_felica_cen_write,
	},
	.pon_pfdata = {
		.pon_init = semc_felica_pon_init,
		.pon_write = semc_felica_pon_write,
		.pon_release = semc_felica_pon_release,
		.tvdd_on = semc_felica_tvdd_on,
		.tvdd_off = semc_felica_tvdd_off,
	},
	.rfs_pfdata = {
		.rfs_init = semc_felica_rfs_init,
		.rfs_read = semc_felica_rfs_read,
		.rfs_release = semc_felica_rfs_release,
	},
	.int_pfdata = {
		.int_init = semc_felica_int_init,
		.int_read = semc_felica_int_read,
		.int_release = semc_felica_int_release,
		.irq_int = MSM_GPIO_TO_INT(MSM_GPIO_FELICA_INT),
	},
	.gpio_init = semc_felica_device_init,
	.reg_release = semc_felica_device_release,
};

struct platform_device semc_felica_device = {
	.name = FELICA_DEV_NAME,
	.id = 0,
	.dev  = {
		.platform_data = &semc_felica_pfdata,
	},
};
