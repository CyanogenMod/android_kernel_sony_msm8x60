/*
 * vibrator-lc898300.c -  Linux kernel modules for linear vibrator
 *
 * Copyright (C) 2012 Sony Ericsson Mobile Communications AB
 * Copyright (c) 2012 Sony Mobile Communications AB
 *
 * Authors: Ardiana Karppinen <ardiana.karppinen@sonyericsson.com>
 *          Stefan Karlsson <stefan3.karlsson@sonyericsson.com>
 *          Yevgen Pronenko <yevgen.pronenko@sonymobile.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 */
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/vibrator-lc898300.h>

#include "../staging/android/timed_output.h"

#define LC898300_REG_HBPW       0x01
#define LC898300_REG_RESOFRQ    0x02
#define LC898300_REG_STARTUP    0x03
#define LC898300_REG_BRAKE      0x04
#define LC898300_MIN_ON      10

static unsigned long intensity;
static int gIntensity = VIB_CMD_PWM_8_15;

enum vib_cmd_conf {
	VIB_CMD_PROBE,
	VIB_CMD_SUSPEND,
	VIB_CMD_RESUME,
	VIB_CMD_REMOVE,
};

struct lc898300_data {
	struct i2c_client *client;
	struct device *dev;
	struct lc898300_platform_data *pdata;
	struct timed_output_dev timed_dev;
	struct hrtimer vib_timer;
	struct mutex lock;
	bool on;
};

static int lc898300_set_intensity(struct lc898300_data *data, int val);

static const struct i2c_device_id lc898300_id[] = {
	{ LC898300_I2C_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, lc898300_id);

static void vib_on(struct lc898300_data *data)
{
	if (!data->pdata->en_gpio_setup(true))
		data->on = true;
	else
		dev_err(data->dev, "Failed to set enable pin\n");
}

static void vib_off(struct lc898300_data *data)
{
	if (!data->pdata->en_gpio_setup(false))
		data->on = false;
	else
		dev_err(data->dev, "Failed to unset enable pin\n");
}

static void lc898300_vib_enable(struct timed_output_dev *dev, int value)
{
	struct lc898300_data *data = container_of(dev, struct lc898300_data,
					 timed_dev);

	/* set intensity */
	lc898300_set_intensity(data, gIntensity);

	mutex_lock(&data->lock);
	hrtimer_cancel(&data->vib_timer);

	if (value >= 0 && value < LC898300_MIN_ON) {
		if (value != 0 || data->on) {
			if (!data->on)
				vib_on(data);
			hrtimer_start(&data->vib_timer,
				ktime_set(0, LC898300_MIN_ON *
					NSEC_PER_MSEC),	HRTIMER_MODE_REL);
		}
	} else if (value > LC898300_MIN_ON) {
		if (!data->on)
			vib_on(data);
		hrtimer_start(&data->vib_timer,
			ktime_set(value / MSEC_PER_SEC, (value % MSEC_PER_SEC)
					* NSEC_PER_MSEC), HRTIMER_MODE_REL);
	}
	mutex_unlock(&data->lock);
}

static int lc898300_configure(struct lc898300_data *data, int val)
{
	struct lc898300_platform_data *pdata = data->pdata;
	struct lc898300_vib_cmd *vib_cmd_info = pdata->vib_cmd_info;
	int rc = 0;

	if (val == VIB_CMD_PROBE || val == VIB_CMD_RESUME) {
		if (val == VIB_CMD_PROBE) {
			rc = pdata->gpio_allocate(data->dev);
			if (rc)
				return rc;
			rc = pdata->power_config(data->dev, true);
			if (rc)
				goto error1;
		}
		rc = pdata->power_enable(data->dev, true);
		if (rc)
			goto error2;

		if(pdata->rstb_gpio_setup(true))
			dev_err(data->dev, "Failed to set reset pin\n");

		udelay(200);
		rc = i2c_smbus_write_byte_data(data->client, LC898300_REG_HBPW,
					vib_cmd_info->vib_cmd_intensity);

		rc = rc ? rc : i2c_smbus_write_byte_data(data->client,
					LC898300_REG_RESOFRQ,
					vib_cmd_info->vib_cmd_resonance);

		rc = rc ? rc : i2c_smbus_write_byte_data(data->client,
					LC898300_REG_STARTUP,
					vib_cmd_info->vib_cmd_startup);

		rc = rc ? rc : i2c_smbus_write_byte_data(data->client,
					LC898300_REG_BRAKE,
					vib_cmd_info->vib_cmd_brake);
	} else if (val == VIB_CMD_REMOVE || val == VIB_CMD_SUSPEND) {
		hrtimer_cancel(&data->vib_timer);
		vib_off(data);

		if(pdata->rstb_gpio_setup(false))
			dev_err(data->dev, "Failed to unset reset pin\n");
		pdata->power_enable(data->dev, false);
		if (val == VIB_CMD_REMOVE) {
			pdata->gpio_release(data->dev);
			pdata->power_config(data->dev, false);
		}
	}

	return rc;

error2:
	pdata->power_config(data->dev, false);
error1:
	pdata->gpio_release(data->dev);
	return rc;
}

static int lc898300_vib_get_time(struct timed_output_dev *dev)
{
	struct lc898300_data *data = container_of(dev, struct lc898300_data,
							 timed_dev);
	int remain;
	if (hrtimer_active(&data->vib_timer)) {
		ktime_t r = hrtimer_get_remaining(&data->vib_timer);
		remain = (int)ktime_to_ms(r);
		return remain > 0 ? remain : 0;
	} else {
		return 0;
	}
}

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
	struct lc898300_data *data = container_of(timer, struct lc898300_data,
							 vib_timer);
	vib_off(data);
	return HRTIMER_NORESTART;
}

static ssize_t attr_intensity_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int count;

	count = sprintf(buf, "%d\n", gIntensity);
	pr_info("[lc898300] current intensity: %d\n", gIntensity);

	return count;
}

ssize_t attr_intensity_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t size)
{
	if (kstrtoul(buf, 0, &intensity))
		pr_err("[lc898300] error while storing new intensity\n");

	/* make sure new intensity is in range */
	if(intensity > VIB_CMD_PWM_15_15) {
		intensity = VIB_CMD_PWM_15_15;
	} else if (intensity < VIB_CMD_PWM_OFF) {
		intensity = VIB_CMD_PWM_OFF;
	}
	gIntensity = intensity;
	pr_info("[lc898300] new intensity: %d\n", gIntensity);

	return size;
}

static int lc898300_set_intensity(struct lc898300_data *data, int val)
{
	struct lc898300_platform_data *pdata = data->pdata;
	struct lc898300_vib_cmd *vib_cmd_info = pdata->vib_cmd_info;
	int rc = 0;

	vib_cmd_info->vib_cmd_intensity = val;
	rc = i2c_smbus_write_byte_data(data->client, LC898300_REG_HBPW,
					vib_cmd_info->vib_cmd_intensity);

	if (rc)
		dev_err(data->dev, "Failed to set intensity\n");

	return rc;
}

static struct device_attribute attributes[] = {
	__ATTR(intensity, 0660,
		attr_intensity_show, attr_intensity_store),
};

static int lc898300_create_sysfs_interfaces(struct device *dev)
{
	int i;
	int result;

	for (i = 0; i < ARRAY_SIZE(attributes); i++) {
		result = device_create_file(dev, &attributes[i]);
		if (result) {
			for (; i >= 0; i--)
				device_remove_file(dev, &attributes[i]);
			dev_err(dev, "%s(): Failed to create sysfs I/F\n", __func__);
			return result;
		}
	}

	return result;
}

static int __devinit lc898300_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct lc898300_data *data;
	int rc, rcsfs;

	if (!i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_BYTE_DATA))
		return -EIO;

	if (!client->dev.platform_data)
		return -EIO;

	data = kzalloc(sizeof(struct lc898300_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->client = client;
	data->dev    = &client->dev;
	data->pdata  = client->dev.platform_data;
	data->on = false;
	mutex_init(&data->lock);

	i2c_set_clientdata(client, data);

	rc = lc898300_configure(data, VIB_CMD_PROBE);
	if (rc) {
		dev_err(&client->dev, "Configure failed rc = %d\n", rc);
		goto error;
	}

	hrtimer_init(&data->vib_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	data->vib_timer.function = vibrator_timer_func;

	data->timed_dev.name = data->pdata->name;
	data->timed_dev.get_time = lc898300_vib_get_time;
	data->timed_dev.enable = lc898300_vib_enable;

	rc = timed_output_dev_register(&data->timed_dev);
	if (rc < 0)
		goto error;

	rcsfs = lc898300_create_sysfs_interfaces(&client->dev);
	if (rcsfs) {
		dev_err(&data->client->dev, "%s(): create sysfs failed", __func__);
	}

	return rc;

error:
	i2c_set_clientdata(client, NULL);
	kfree(data);
	return rc;
}

static int __devexit lc898300_remove(struct i2c_client *client)
{
	struct lc898300_data *data = i2c_get_clientdata(client);

	lc898300_configure(data, VIB_CMD_REMOVE);
	hrtimer_cancel(&data->vib_timer);
	timed_output_dev_unregister(&data->timed_dev);
	kfree(data);
	i2c_set_clientdata(client, NULL);

	return 0;
}

static int lc898300_resume(struct device *dev)
{
	struct lc898300_data *data = dev_get_drvdata(dev);
	int rc;
	rc = lc898300_configure(data, VIB_CMD_RESUME);

	return rc;
}

static int lc898300_suspend(struct device *dev)
{
	struct lc898300_data *data = dev_get_drvdata(dev);
	int rc;

	rc = lc898300_configure(data, VIB_CMD_SUSPEND);

	return rc;
}

static const struct dev_pm_ops lc898300_pm_ops = {
	.suspend = lc898300_suspend,
	.resume = lc898300_resume,
};

static struct i2c_driver lc898300_driver = {
	.driver = {
		.name   = LC898300_I2C_NAME,
		.owner = THIS_MODULE,
		.pm = &lc898300_pm_ops,
	},
	.probe  = lc898300_probe,
	.remove = __devexit_p(lc898300_remove),
	.id_table = lc898300_id,
};

static int __init lc898300_init(void)
{
	return i2c_add_driver(&lc898300_driver);
}

static void __exit lc898300_exit(void)
{
	i2c_del_driver(&lc898300_driver);
}

MODULE_AUTHOR("Ardiana Karppinen <ardiana.karppinen@sonyericsson.com>, Stefan Karlsson <stefan3.karlsson@sonyericsson.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LC898300 Linear Vibrator");

module_init(lc898300_init);
module_exit(lc898300_exit);
