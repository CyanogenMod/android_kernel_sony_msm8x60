/* arch/arm/mach-msm/felica-blue_common.c
 *
 * Copyright (C) 2010-2011 Sony Ericsson Mobile Communications AB.
 * Copyright (C) 2012 Sony Mobile Communications AB.
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
#include <linux/mfd/pm8xxx/pm8921.h>
#ifdef CONFIG_SONY_FELICA_NFC_SUPPORT
#include <mach/gpiomux.h>
#endif

#include "board-8960.h"

#define PM_GPIO_FELICA_LOCK	31
#define PM_GPIO_FELICA_FF	32
#define PM_GPIO_FELICA_PON	33

#ifdef CONFIG_SONY_FELICA_NFC_SUPPORT
#define PM_GPIO_NFC_EXT_LDO_EN	35
#define MSM_GPIO_S_COMBO_INTU	7
#define MSM_GPIO_S_COMBO_HSEL	8
#define GSBI10_UART_TX_GPIO	71
#define GSBI10_UART_RX_GPIO	72
#endif

#define MSM_GPIO_FELICA_RFS	19
#define MSM_GPIO_FELICA_INT	106

#define CEN_RETRY_MAX	5

#ifdef CONFIG_SONY_FELICA_NFC_SUPPORT
static struct regulator *hvdd_reg;
static struct gpiomux_setting gpio_2ma_no_pull_in = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir  = GPIOMUX_IN,
};
static struct gpiomux_setting felica_uart = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE
};
static void setup_uart_gpio(struct felica_dev *dev)
{
	if (msm_gpiomux_write(GSBI10_UART_RX_GPIO, GPIOMUX_SUSPENDED,
			&gpio_2ma_no_pull_in, NULL)) {
		dev_dbg(dev->dev, "%s: msm_gpiomux_write %d failed.",
			__func__, GSBI10_UART_RX_GPIO);
	}
	if (msm_gpiomux_write(GSBI10_UART_TX_GPIO, GPIOMUX_ACTIVE,
			&felica_uart, NULL)) {
		dev_dbg(dev->dev, "%s: msm_gpiomux_write %d failed.",
			__func__, GSBI10_UART_TX_GPIO);
	}
	if (msm_gpiomux_write(GSBI10_UART_RX_GPIO, GPIOMUX_ACTIVE,
			&felica_uart, NULL)) {
		dev_dbg(dev->dev, "%s: msm_gpiomux_write %d failed.",
			__func__, GSBI10_UART_RX_GPIO);
	}
}
#endif

static int felica_device_init(struct felica_dev *dev)
{
	int ret;
#ifdef CONFIG_SONY_FELICA_NFC_SUPPORT
	struct regulator *vreg;
#endif

	if (!dev)
		return -EINVAL;
	dev_dbg(dev->dev, ": %s\n", __func__);

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
#ifdef CONFIG_SONY_FELICA_NFC_SUPPORT
	ret = gpio_request(
		PM8921_GPIO_PM_TO_SYS(PM_GPIO_NFC_EXT_LDO_EN),
		"nfc_ext_ldo_en");
	if (ret) {
		dev_err(dev->dev, "%s: request err NFC_EXT_LDO_EN%d\n",
			__func__, ret);
		goto err_nfc_ext_ldo_en;
	}
	ret = gpio_request(MSM_GPIO_S_COMBO_INTU, "snfc_intu");
	if (ret) {
		dev_err(dev->dev, "%s: request err INTU%d\n",
			__func__, ret);
		goto err_intu;
	}
	ret = gpio_request(MSM_GPIO_S_COMBO_HSEL, "snfc_hsel");
	if (ret) {
		dev_err(dev->dev, "%s: request err HSEL%d\n",
			__func__, ret);
		goto err_hsel;
	}
#endif
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

#ifdef CONFIG_SONY_FELICA_NFC_SUPPORT
	vreg = regulator_get(NULL, "8921_lvs6");
	if (IS_ERR(vreg)) {
		pr_err("%s: failed to get vreg 8921_lvs6!\n", __func__);
		dev_err(dev->dev, "%s: Get HVDD failed\n",
				__func__);
		goto err_hvdd;
	}
	hvdd_reg = vreg;
#endif

	return 0;

#ifdef CONFIG_SONY_FELICA_NFC_SUPPORT
err_hvdd:
	gpio_free(MSM_GPIO_FELICA_INT);
#endif
err_int:
	gpio_free(MSM_GPIO_FELICA_RFS);
err_rfs:
#ifdef CONFIG_SONY_FELICA_NFC_SUPPORT
	gpio_free(MSM_GPIO_S_COMBO_HSEL);
err_hsel:
	gpio_free(MSM_GPIO_S_COMBO_INTU);
err_intu:
	gpio_free(PM8921_GPIO_PM_TO_SYS(PM_GPIO_NFC_EXT_LDO_EN));
err_nfc_ext_ldo_en:
#endif
	gpio_free(PM8921_GPIO_PM_TO_SYS(PM_GPIO_FELICA_PON));
err_pon:
	gpio_free(PM8921_GPIO_PM_TO_SYS(PM_GPIO_FELICA_FF));
err_ff:
	gpio_free(PM8921_GPIO_PM_TO_SYS(PM_GPIO_FELICA_LOCK));
err_lock:
	return -ENODEV;
}

static int felica_device_release(struct felica_dev *dev)
{
	if (!dev)
		return -EINVAL;
	dev_dbg(dev->dev, ": %s\n", __func__);

#ifdef CONFIG_SONY_FELICA_NFC_SUPPORT
	gpio_free(MSM_GPIO_S_COMBO_INTU);
	gpio_free(MSM_GPIO_S_COMBO_HSEL);
#endif
	gpio_free(MSM_GPIO_FELICA_INT);
	gpio_free(MSM_GPIO_FELICA_RFS);
	gpio_free(PM8921_GPIO_PM_TO_SYS(PM_GPIO_FELICA_PON));
	gpio_free(PM8921_GPIO_PM_TO_SYS(PM_GPIO_FELICA_FF));
	gpio_free(PM8921_GPIO_PM_TO_SYS(PM_GPIO_FELICA_LOCK));
#ifdef CONFIG_SONY_FELICA_NFC_SUPPORT
	gpio_free(PM8921_GPIO_PM_TO_SYS(PM_GPIO_NFC_EXT_LDO_EN));
#endif

	return 0;
}

static int felica_cen_init(struct felica_dev *dev)
{
	if (!dev)
		return -EINVAL;
	dev_dbg(dev->device_cen.this_device, ": %s\n", __func__);

	return 0;
}

static int felica_cen_read(u8 *buf, struct felica_dev *dev)
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

static int felica_cen_write(u8 arg, struct felica_dev *dev)
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
		felica_cen_read(&state, dev);
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

static int felica_pon_init(struct felica_dev *dev)
{
	if (!dev)
		return -EINVAL;
	dev_dbg(dev->device_pon.this_device, ": %s\n", __func__);

	return 0;
}

static void felica_pon_write(int val, struct felica_dev *dev)
{
	if (!dev)
		return;
	dev_dbg(dev->device_pon.this_device, ": %s\n", __func__);

#ifdef CONFIG_SONY_FELICA_NFC_SUPPORT
	if (val && !regulator_is_enabled(hvdd_reg)) {
		regulator_enable(hvdd_reg);
		msleep_interruptible(1);
		setup_uart_gpio(dev);
	}
#endif

	gpio_set_value_cansleep(PM8921_GPIO_PM_TO_SYS(PM_GPIO_FELICA_PON), val);
}

static void felica_pon_release(struct felica_dev *dev)
{
	if (!dev)
		return;
	dev_dbg(dev->device_pon.this_device, ": %s\n", __func__);
}

static int felica_tvdd_on(struct felica_dev *dev)
{
	if (!dev)
		return -EINVAL;
	dev_dbg(dev->dev, ": %s\n", __func__);

	return 0;
}

static void felica_tvdd_off(struct felica_dev *dev)
{
	if (!dev)
		return;
	dev_dbg(dev->dev, ": %s\n", __func__);
}

static int felica_rfs_init(struct felica_dev *dev)
{
	if (!dev)
		return -EINVAL;
	dev_dbg(dev->device_rfs.this_device, ": %s\n", __func__);

	return 0;
}

static int felica_rfs_read(struct felica_dev *dev)
{
	if (!dev)
		return -EINVAL;
	dev_dbg(dev->device_rfs.this_device, ": %s\n", __func__);

	return gpio_get_value(MSM_GPIO_FELICA_RFS);
}

static void felica_rfs_release(struct felica_dev *dev)
{
	if (!dev)
		return;
	dev_dbg(dev->device_rfs.this_device, ": %s\n", __func__);
}

static int felica_int_init(struct felica_dev *dev)
{
	if (!dev)
		return -EINVAL;
	dev_dbg(dev->dev, ": %s\n", __func__);

	return 0;
}

static int felica_int_read(struct felica_dev *dev)
{
	if (!dev)
		return -EINVAL;
	dev_dbg(dev->dev, ": %s\n", __func__);

	return gpio_get_value(MSM_GPIO_FELICA_INT);
}

static void felica_int_release(struct felica_dev *dev)
{
	if (!dev)
		return;
	dev_dbg(dev->dev, ": %s\n", __func__);
}

#ifdef CONFIG_SONY_FELICA_NFC_SUPPORT
static int snfc_intu_init(struct felica_dev *dev)
{
	if (!dev)
		return -EINVAL;
	dev_dbg(dev->dev, ": %s\n", __func__);
	return 0;
}

static int snfc_intu_read(struct felica_dev *dev)
{
	if (!dev)
		return -EINVAL;
	dev_dbg(dev->dev, ": %s\n", __func__);
	return gpio_get_value(MSM_GPIO_S_COMBO_INTU);
}

static void snfc_intu_release(struct felica_dev *dev)
{
	if (!dev)
		return;
	dev_dbg(dev->dev, ": %s\n", __func__);
}

static int snfc_hsel_init(struct felica_dev *dev)
{
	if (!dev)
		return -EINVAL;
	dev_dbg(dev->dev, ": %s\n", __func__);
	return 0;
}

static void snfc_hsel_write(int val, struct felica_dev *dev)
{
	if (!dev)
		return;
	dev_dbg(dev->dev, ": %s\n", __func__);
	gpio_set_value_cansleep(MSM_GPIO_S_COMBO_HSEL, val);
}

static void snfc_hsel_release(struct felica_dev *dev)
{
	if (!dev)
		return;
	dev_dbg(dev->dev, ": %s\n", __func__);
}

static int snfc_ldo_init(struct felica_dev *dev)
{
	if (!dev)
		return -EINVAL;
	dev_dbg(dev->dev, ": %s\n", __func__);
	return 0;

}

static void snfc_ldo_write(int val, struct felica_dev *dev)
{
	if (!dev)
		return;
	dev_dbg(dev->dev, ": %s\n", __func__);
	gpio_set_value_cansleep(
		PM8921_GPIO_PM_TO_SYS(PM_GPIO_NFC_EXT_LDO_EN), val);
}

static void snfc_ldo_release(struct felica_dev *dev)
{
	if (!dev)
		return;
	dev_dbg(dev->dev, ": %s\n", __func__);
}
#endif

static struct felica_platform_data felica_pfdata = {
	.cen_pfdata = {
		.cen_init = felica_cen_init,
		.cen_read = felica_cen_read,
		.cen_write = felica_cen_write,
	},
	.pon_pfdata = {
		.pon_init = felica_pon_init,
		.pon_write = felica_pon_write,
		.pon_release = felica_pon_release,
		.tvdd_on = felica_tvdd_on,
		.tvdd_off = felica_tvdd_off,
	},
	.rfs_pfdata = {
		.rfs_init = felica_rfs_init,
		.rfs_read = felica_rfs_read,
		.rfs_release = felica_rfs_release,
		.irq_rfs = MSM_GPIO_TO_INT(MSM_GPIO_FELICA_RFS),
	},
	.int_pfdata = {
		.int_init = felica_int_init,
		.int_read = felica_int_read,
		.int_release = felica_int_release,
		.irq_int = MSM_GPIO_TO_INT(MSM_GPIO_FELICA_INT),
	},
#ifdef CONFIG_SONY_FELICA_NFC_SUPPORT
	.intu_pfdata = {
		.intu_init = snfc_intu_init,
		.intu_read = snfc_intu_read,
		.intu_release = snfc_intu_release,
		.irq_intu = MSM_GPIO_TO_INT(MSM_GPIO_S_COMBO_INTU),
	},
	.hsel_pfdata = {
		.hsel_init = snfc_hsel_init,
		.hsel_write = snfc_hsel_write,
		.hsel_release = snfc_hsel_release,
	},
	.ldo_pfdata = {
		.ldo_init = snfc_ldo_init,
		.ldo_write = snfc_ldo_write,
		.ldo_release = snfc_ldo_release,
	},
#endif
	.gpio_init = felica_device_init,
	.reg_release = felica_device_release,
};

struct platform_device sony_felica_device = {
	.name = FELICA_DEV_NAME,
	.id = 0,
	.dev = {
		.platform_data = &felica_pfdata,
	},
};
