/* arch/arm/mach-msm/irda-blue_common.c
 *
 * Copyright (C) 2011-2012 Sony Ericsson Mobile Communications AB.
 * Copyright (C) 2012 Sony Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/wakelock.h>
#include <linux/regulator/gpio-regulator.h>

#include "board-8960.h"

#define PM_GPIO_IRDA_PWDOWN 17
#define IRDA_WAKE_LOCK_TIMEOUT 120000

static struct wake_lock wlock;

static int semc_irda_power(int enable)
{
	static struct pm_gpio irda_pwdown_param = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 1,
		.pull = PM_GPIO_PULL_NO,
		.vin_sel = PM_GPIO_VIN_S4,
		.out_strength = PM_GPIO_STRENGTH_LOW,
		.function = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol = 0,
		.disable_pin = 0,
	};
	int ret;

	irda_pwdown_param.output_value = !enable;

	ret = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(PM_GPIO_IRDA_PWDOWN),
					&irda_pwdown_param);
	if (ret) {
		pr_err("pm_gpio_config 17 failed (%d), ret=%d\n", !enable, ret);
		return -EINVAL;
	}
	if (enable)
		wake_lock_timeout(&wlock, IRDA_WAKE_LOCK_TIMEOUT);
	else
		wake_unlock(&wlock);

	return 0;
}

static ssize_t irda_msm_ctrl_store(
			struct class *class,
			struct class_attribute *attr,
			const char *buf,
			size_t count)
{
	unsigned long value;
	int ret;

	if (strict_strtoul(buf, 0, &value)) {
		pr_err("%s: Invalid value\n", __func__);
		ret = -EINVAL;
		goto err_out;
	}
	if (value)
		ret = semc_irda_power(1);
	else
		ret = semc_irda_power(0);

	if (ret) {
		pr_err("%s: power control failed\n", __func__);
		goto err_out;
	}

	return count;

err_out:
	return ret;
}

static CLASS_ATTR(irda, S_IRUGO | S_IWUSR | S_IWGRP,
			NULL, irda_msm_ctrl_store);
static struct class irda_class = {
	.name = "irda",
};

static int __init irda_class_init(void)
{
	int error;

	error = class_register(&irda_class);
	if (error) {
		pr_err("%s: class_register failed\n", __func__);
		goto error_class_register;
	}
	error = gpio_request(PM8921_GPIO_PM_TO_SYS(PM_GPIO_IRDA_PWDOWN),
				"IRDA_PWDOWN");
	if (error) {
		pr_err("%s: GPIO %d: gpio_request failed\n", __func__,
			PM_GPIO_IRDA_PWDOWN);
		goto error_gpio_request;
	}
	error = class_create_file(&irda_class, &class_attr_irda);
	if (error) {
		pr_err("%s: class_create_file failed\n", __func__);
		goto error_class_create_file;
	}
	wake_lock_init(&wlock, WAKE_LOCK_IDLE, "irda-blue_common");
	error = semc_irda_power(0);
	if (error) {
		pr_err("%s: irda power off failed\n", __func__);
		goto error_semc_irda_power;
	}

	return 0;

error_semc_irda_power:
	wake_lock_destroy(&wlock);
	class_remove_file(&irda_class, &class_attr_irda);
error_class_create_file:
	gpio_free(PM8921_GPIO_PM_TO_SYS(PM_GPIO_IRDA_PWDOWN));
error_gpio_request:
	class_unregister(&irda_class);
error_class_register:
	return error;
}

static void __exit irda_class_exit(void)
{
	wake_lock_destroy(&wlock);
	class_remove_file(&irda_class, &class_attr_irda);
	gpio_free(PM8921_GPIO_PM_TO_SYS(PM_GPIO_IRDA_PWDOWN));
	class_unregister(&irda_class);
}

module_init(irda_class_init);
module_exit(irda_class_exit);
