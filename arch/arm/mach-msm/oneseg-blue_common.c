/* arch/arm/mach-msm/oneseg-blue_common.c
 *
 * Copyright (C) 2012 Sony Ericsson Mobile Communications AB.
 * Copyright (C) 2012 Sony Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/err.h>

#include <mach/oneseg_tunerpm.h>

#define D_ONESEG_DEVICE_PORT_RESET	80
#define D_ONESEG_DEVICE_PORT_POWER	82

static int oneseg_tunerpm_init(struct device *dev)
{
	int ret;

	ret = gpio_request(D_ONESEG_DEVICE_PORT_POWER, "Oneseg tuner HW reset");
	if (ret) {
		dev_err(dev, "PWR request %d\n", ret);
		return ret;
	}
	ret = gpio_request(D_ONESEG_DEVICE_PORT_RESET,
			"Oneseg tuner HW reset");
	if (ret) {
		dev_err(dev, "RST request %d\n", ret);
		gpio_free(D_ONESEG_DEVICE_PORT_POWER);
		return ret;
	}
	return 0;
}

static int oneseg_tunerpm_free(struct device *dev)
{
	gpio_free(D_ONESEG_DEVICE_PORT_POWER);
	gpio_free(D_ONESEG_DEVICE_PORT_RESET);
	return 0;
}

static int oneseg_tunerpm_power_control(struct device *dev, int on)
{
	gpio_set_value(D_ONESEG_DEVICE_PORT_POWER, on);
	return 0;
}

static int oneseg_tunerpm_reset_control(struct device *dev, int on)
{
	gpio_set_value(D_ONESEG_DEVICE_PORT_RESET, on);
	return 0;
}

static struct oneseg_tunerpm_platform_data oneseg_tunerpm_data = {
	.init = oneseg_tunerpm_init,
	.free = oneseg_tunerpm_free,
	.reset_control = oneseg_tunerpm_reset_control,
	.power_control = oneseg_tunerpm_power_control,
};

struct platform_device oneseg_tunerpm_device = {
	.name = D_ONESEG_TUNERPM_DRIVER_NAME,
	.id = 0,
	.dev  = {
		.platform_data = &oneseg_tunerpm_data,
	},
};
