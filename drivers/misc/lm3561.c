/* drivers/misc/lm3561.c
 *
 * Copyright (C) 2012 Sony Ericsson Mobile Communications AB.
 * Copyright (C) 2012 Sony Mobile Communications AB.
 *
 * Author: Angela Fox <angela.fox@sonyericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
*/

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/lm3561.h>
#include <linux/kernel.h>
#include <linux/slab.h>

/*
 *************************************************************************
 * - Value declaration
 * - Prototype declaration
 *************************************************************************
 */
/*
 * LM3561 Register address
 */
#define LM3561_REG_ENABLE		0x10
#define LM3561_REG_GPIO			0x20
#define LM3561_REG_TORCH_BRIGHT		0xA0
#define LM3561_REG_FLASH_BRIGHT		0xB0
#define LM3561_REG_FLASH_DURATION	0xC0
#define LM3561_REG_FLAG			0xD0
#define LM3561_REG_CFG_1		0xE0
#define LM3561_REG_CFG_2		0xF0

/*
 * Mask/Value of Enable register
 */
/* LM3561 Enable bits[1:0] */
#define LM3561_ENABLE_EN_MASK			(0x07 << 0)
#define LM3561_ENABLE_EN_SHUTDOWN		(0x00 << 0)
#define LM3561_ENABLE_EN_INDICATOR_MODE		(0x01 << 0)
#define LM3561_ENABLE_EN_TORCH_MODE		(0x02 << 0)
#define LM3561_ENABLE_EN_FLASH_MODE		(0x03 << 0)

/* LM3561 Stobe Enable bit */
#define LM3561_STROBE_MASK			(0x01 << 2)
#define LM3561_STROBE_LEVEL_ON			(0x00 << 2)
#define LM3561_STROBE_EDGE_ON			(0x01 << 2)
#define LM3561_STROBE_EDGE_OFF			(0x00 << 2)

/*
 * Mask/Value of Configuration register 1
 */
/* LM3561 TX2,TX1,NTC,Polarity,STROBE Input Enable */
#define LM3561_CFG1_STROBE_INPUT_MASK		(0x7f)
#define LM3561_CFG1_STROBE_INPUT_DISABLE	(0x00)
#define LM3561_CFG1_STROBE_INPUT_ENABLE		(0x7f)

/*
 * LM3561 Mask of Torch Brightness Register
 */
#define LM3561_TORCH_BRIGHT_MASK		(0x07)

/*
 * LM3561 Mask of Flash Brightness Register
 */
#define LM3561_FLASH_BRIGHT_MASK		(0x0f)

/*
 * LM3561 Mask of Configuration Register 1
 */
#define LM3561_CFG_1_MASK			(0x7f)

/*
 * Mask of Flash Duration register
 */
#define LM3561_FLASH_DURATION_MASK		(0x1f << 0)

/* LM3561 Current Limit bit */
#define LM3561_FLASH_DURATION_CL_MASK		(0x01 << 5)
#define LM3561_FLASH_DURATION_CL_1000MA		(0x00 << 5)
#define LM3561_FLASH_DURATION_CL_1500MA		(0x01 << 5)

/*
 * Mask of GPIO Register
 */
/* LM3561 TX2/GPIO2 pin control bit */
#define LM3561_GPIO_CTRL_MASK			(0x01 << 3)
#define LM3561_GPIO_CTRL_FLASH			(0x00 << 3)

#define STROBE_TRIGGER_SHIFT 2

struct led_limits {
	unsigned long torch_current_min;
	unsigned long torch_current_max;
	unsigned long torch_current_step;
	unsigned long flash_current_min;
	unsigned long flash_current_max;
	unsigned long flash_current_step;
	unsigned long flash_duration_min;
	unsigned long flash_duration_max;
};

/*
 * Min/Max/step torch/flash values and also min/max duration values.
 * TODO! Room for improvement! Some values also defined in macros in:
 * /vendor/semc/hardware/libcameralight/chips/lm3561_flash.c!
 */
const struct led_limits lm3561_limits = {
	18000,
	149600,
	18800,
	36000,
	600000,
	37600,
	32000,
	1024000,
};

struct lm3561_drv_data {
	struct i2c_client *client;
	/* The value except T0-T4 bits of Flash Duration register */
	u8 reg_flash_duration_limit;
	/* The value except StrobeInputEnable bit of Configuration Register1 */
	u8 reg_cfg1;
	int led_nums;
	int torch_current_shift;
	int flash_current_shift;
	int strobe_trigger_shift;
};

static int lm3561_get_reg_data(struct lm3561_drv_data *data,
				u8 addr, u8 *value)
{
	s32 result;

	result = i2c_smbus_read_i2c_block_data(
					data->client,
					addr,
					1,
					value);
	if (result < 0) {
		dev_err(&data->client->dev,
			"%s(): Failed to read register(0x%02x). "
				"errno=%d\n",
				__func__, addr, result);
		return -EIO;
	}

	return 0;
}

static int lm3561_set_reg_data(struct lm3561_drv_data *data,
				u8 addr, u8 mask, u8 value)
{
	u8 current_value;
	s32 result;

	if (mask != 0xFF) {
		result = i2c_smbus_read_i2c_block_data(
						data->client,
						addr,
						1,
						&current_value);
		if (result < 0) {
			dev_err(&data->client->dev,
				"%s(): Failed to read register(0x%02x)"
					". errno=%d\n",
					__func__, addr, result);
			return -EIO;
		}
		value = (current_value & ~mask) | value;
	}

	/* For debug-purpose, get info on what is written to chip */
	dev_dbg(&data->client->dev,
		"%s(): addr:0x%02x, value:0x%02x\n",
		__func__, addr, value);

	result = i2c_smbus_write_i2c_block_data(
					data->client,
					addr,
					1,
					&value);
	if (result < 0) {
		dev_err(&data->client->dev,
			"%s(): Failed to write register(0x%02x). "
				"errno=%d\n",
				__func__, addr, result);
		return -EIO;
	}

	return 0;
}

static int lm3561_set_flash_sync(struct lm3561_drv_data *data,
				enum lm3561_sync_state setting)
{
	if (setting == LM3561_SYNC_ON) {
		return lm3561_set_reg_data(data,
					   LM3561_REG_CFG_1,
					   LM3561_CFG_1_MASK,
					   LM3561_CFG1_STROBE_INPUT_ENABLE);
	} else {
		return lm3561_set_reg_data(data,
					   LM3561_REG_CFG_1,
					   LM3561_CFG_1_MASK,
					   LM3561_CFG1_STROBE_INPUT_DISABLE);
	}
}

static int lm3561_check_status(struct lm3561_drv_data *data, u8 *return_status)
{
	u8 status = 0;
	int error;

	*return_status = 0;

	/* set Tx2/GPIO2 Control as flash interrupt input */
	error = lm3561_set_reg_data(data,
				LM3561_REG_GPIO,
				LM3561_GPIO_CTRL_MASK,
				LM3561_GPIO_CTRL_FLASH);
	if (error)
		return error;

	error = lm3561_get_reg_data(data, LM3561_REG_FLAG, &status);
	if (error)
		return error;

	*return_status &= status;

	return error;
}

static int lm3561_torch_mode(struct lm3561_drv_data *data,
				unsigned setting)
{
	int result;


	if (setting) {
		result = lm3561_set_reg_data(data,
					LM3561_REG_ENABLE,
					LM3561_ENABLE_EN_MASK,
					LM3561_ENABLE_EN_TORCH_MODE);

	} else {
		result = lm3561_set_reg_data(data,
					LM3561_REG_ENABLE,
					LM3561_ENABLE_EN_MASK,
					LM3561_ENABLE_EN_SHUTDOWN);
	}

	return result;
}

static int lm3561_flash_mode(struct lm3561_drv_data *data,
				unsigned setting)
{
	int result;


	if (setting) {
		result = lm3561_set_reg_data(data,
					LM3561_REG_ENABLE,
					LM3561_ENABLE_EN_MASK,
					LM3561_ENABLE_EN_FLASH_MODE);
	} else {
		result = lm3561_set_reg_data(data,
					LM3561_REG_ENABLE,
					LM3561_ENABLE_EN_MASK,
					LM3561_ENABLE_EN_SHUTDOWN);
	}

	return result;
}

static int lm3561_get_torch_current(struct lm3561_drv_data *data,
				    unsigned long *get_current)
{
	u8 leds = 1, reg_current;
	int result;

	result = lm3561_get_reg_data(data,
				     LM3561_REG_TORCH_BRIGHT,
				     &reg_current);
	if (result)
		return result;

	*get_current = ((reg_current & LM3561_TORCH_BRIGHT_MASK)
		* lm3561_limits.torch_current_step * leds
		+ lm3561_limits.torch_current_min * leds);

	return result;
}

static int lm3561_set_torch_current(struct lm3561_drv_data *data,
				    unsigned long request_current)
{
	u8 current_bits_value;
	int leds = 1;

	if ((request_current < lm3561_limits.torch_current_min)  ||
	    (request_current > lm3561_limits.torch_current_max)) {
		dev_err(&data->client->dev,
			"%s(): Value (%luuA) should be from %luuA to %luuA\n",
			__func__, request_current,
			lm3561_limits.torch_current_min,
			lm3561_limits.torch_current_max);

		if (request_current < lm3561_limits.torch_current_min)
			request_current = lm3561_limits.torch_current_min;
		else if (request_current > lm3561_limits.torch_current_max)
			request_current = lm3561_limits.torch_current_max;

		dev_err(&data->client->dev,
			"%s(): Value is now set to %luuA\n",
			__func__, request_current);
	}



	/* Convert current value to register value (Round-down fraction) */
	current_bits_value =
		(request_current - lm3561_limits.torch_current_min * leds)
		/ (lm3561_limits.torch_current_step * leds);

	current_bits_value = (current_bits_value << data->torch_current_shift)
		| current_bits_value;

	return lm3561_set_reg_data(data,
				LM3561_REG_TORCH_BRIGHT,
				LM3561_TORCH_BRIGHT_MASK,
				current_bits_value);
}

static int lm3561_get_flash_current(struct lm3561_drv_data *data,
				    unsigned long *get_current)
{
	u8 leds = 1, reg_current;
	int result;

	result = lm3561_get_reg_data(data,
				     LM3561_REG_FLASH_BRIGHT,
				     &reg_current);
	if (result)
		return result;

	*get_current = ((reg_current & LM3561_FLASH_BRIGHT_MASK)
		* lm3561_limits.flash_current_step * leds
		+ lm3561_limits.flash_current_min * leds);

	return result;
}

static int lm3561_set_flash_current(struct lm3561_drv_data *data,
				    unsigned long flash_current)
{
	u8 current_bits_value;
	int leds = 1;

	if ((flash_current < lm3561_limits.flash_current_min) ||
	    (flash_current > lm3561_limits.flash_current_max)) {
		dev_err(&data->client->dev,
			"%s(): Value (%luuA) should be from %luuA to %luuA.\n",
			__func__, flash_current,
			lm3561_limits.flash_current_min,
			lm3561_limits.flash_current_max);

		if (flash_current < lm3561_limits.flash_current_min)
			flash_current = lm3561_limits.flash_current_min;
		else if (flash_current > lm3561_limits.flash_current_max)
			flash_current = lm3561_limits.flash_current_max;

		dev_err(&data->client->dev,
			"%s(): Value is now set to %luuA\n",
			__func__, flash_current);
	}
	/* Convert current value to register value (Round-down fraction) */
	current_bits_value =
		(flash_current - lm3561_limits.flash_current_min * leds)
		/ (lm3561_limits.flash_current_step * leds);

	current_bits_value = (current_bits_value << data->flash_current_shift)
		| current_bits_value;

	return lm3561_set_reg_data(data,
				     LM3561_REG_FLASH_BRIGHT,
				     LM3561_FLASH_BRIGHT_MASK,
				     current_bits_value);
}

static int lm3561_get_flash_duration(struct lm3561_drv_data *data,
				    unsigned long *flash_duration)
{
	u8 reg_duration;
	int result;

	result = lm3561_get_reg_data(data,
				     LM3561_REG_FLASH_DURATION,
				     &reg_duration);
	if (result != 0)
		return result;

	*flash_duration = ((reg_duration & LM3561_FLASH_DURATION_MASK) + 1)
		* lm3561_limits.flash_duration_min;

	return result;

}

static int lm3561_set_flash_duration(struct lm3561_drv_data *data,
					unsigned long flash_duration)
{
	u8 duration_bits_value;

	if ((flash_duration < lm3561_limits.flash_duration_min) ||
	    (flash_duration > lm3561_limits.flash_duration_max)) {
		dev_err(&data->client->dev,
			"%s(): Value (%luus) should be from %luus to %luus\n",
			__func__, flash_duration,
			lm3561_limits.flash_duration_min,
			lm3561_limits.flash_duration_max);

		if (flash_duration < lm3561_limits.flash_duration_min)
			flash_duration = lm3561_limits.flash_duration_min;
		else if (flash_duration > lm3561_limits.flash_duration_max)
			flash_duration = lm3561_limits.flash_duration_max;

		dev_err(&data->client->dev,
			"%s(): Value is now set to %luus\n",
			__func__, flash_duration);
	}

	/* Convert duration value to register value. (Round-up fraction) */
	duration_bits_value =
		(flash_duration - 1) / lm3561_limits.flash_duration_min;

	return lm3561_set_reg_data(data,
				LM3561_REG_FLASH_DURATION,
				LM3561_FLASH_DURATION_MASK,
				duration_bits_value);
}

static int lm3561_init_enable_register(struct lm3561_drv_data *data,
					struct lm3561_platform_data *pdata)
{
	int result;
	u8 value = 0;

	if (pdata->strobe_trigger)
		value |= (1 << STROBE_TRIGGER_SHIFT);

	result = lm3561_set_reg_data(data,
				     LM3561_REG_ENABLE,
				     LM3561_ENABLE_EN_MASK
				     | (1 << STROBE_TRIGGER_SHIFT),
				     value);
	return result;
}

static int lm3561_init_cfg1_register(struct lm3561_drv_data *data,
					struct lm3561_platform_data *pdata)
{
	int result;

	result = lm3561_set_reg_data(data,
				LM3561_REG_CFG_1,
				LM3561_CFG_1_MASK,
				LM3561_CFG1_STROBE_INPUT_ENABLE);
	if (result)
		return result;

	result = lm3561_set_flash_sync(data, pdata->flash_sync);

	return result;
}

static int lm3561_chip_init(struct lm3561_drv_data *data,
				struct lm3561_platform_data *pdata)
{
	int result;

	result = lm3561_init_enable_register(data, pdata);
	if (result)
		return result;

	result =  lm3561_set_reg_data(data,
				LM3561_REG_FLASH_DURATION,
				LM3561_FLASH_DURATION_CL_MASK,
				data->reg_flash_duration_limit);
	if (result)
		return result;

	result = lm3561_init_cfg1_register(data, pdata);
	if (result)
		return result;

	return result;
}

/*
 ****************************************************************************
 * - Sysfs operations
 ****************************************************************************
 */

static ssize_t attr_torch_enable_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct lm3561_drv_data *data = dev_get_drvdata(dev);
	u8 value;
	int result;

	result = lm3561_get_reg_data(data, LM3561_REG_ENABLE, &value);

	if (result)
		return result;

	/* Get torch enable */
	value &= LM3561_ENABLE_EN_MASK;
	value = (value == LM3561_ENABLE_EN_TORCH_MODE) ? 1 : 0;

	return snprintf(buf, PAGE_SIZE, "%u\n", value);
}

static ssize_t attr_torch_enable_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct lm3561_drv_data *data = dev_get_drvdata(dev);
	unsigned long enable;
	int result;

	result = strict_strtoul(buf, 10, &enable);
	if (result) {
		dev_err(&data->client->dev,
			"%s(): strtoul failed, result=%d\n",
				__func__, result);
		return -EINVAL;
	}

	if (1 < enable) {
		dev_err(&data->client->dev,
			"%s(): 1 < enable, enable=%lu\n",
				__func__, enable);
		return -EINVAL;
	}

	result = lm3561_torch_mode(data, (unsigned)enable);

	if (result)
		return result;

	return size;
}

static ssize_t attr_torch_current_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct lm3561_drv_data *data = dev_get_drvdata(dev);
	int result;
	unsigned long torch_current;

	result = lm3561_get_torch_current(data, &torch_current);

	if (result != 0)
		return result;

	return snprintf(buf, PAGE_SIZE, "%lu\n", torch_current);
}

static ssize_t attr_torch_current_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct lm3561_drv_data *data = dev_get_drvdata(dev);
	unsigned long torch_current;
	int result;

	result = strict_strtoul(buf, 10, &torch_current);
	if (result) {
		dev_err(&data->client->dev,
			"%s(): strtoul failed, result=%d\n",
				__func__, result);
		return -EINVAL;
	}

	result = lm3561_set_torch_current(data, torch_current);

	if (result)
		return result;

	return size;
}

static ssize_t attr_flash_enable_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct lm3561_drv_data *data = dev_get_drvdata(dev);
	u8 value;
	int result;

	result = lm3561_get_reg_data(data, LM3561_REG_ENABLE, &value);

	if (result)
		return result;

	/* Get flash enable */
	value &= LM3561_ENABLE_EN_MASK;
	value = (value == LM3561_ENABLE_EN_FLASH_MODE) ? 1 : 0;

	return snprintf(buf, PAGE_SIZE, "%u\n", value);
}

static ssize_t attr_flash_enable_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct lm3561_drv_data *data = dev_get_drvdata(dev);
	unsigned long enable;
	int result;

	result = strict_strtoul(buf, 10, &enable);
	if (result) {
		dev_err(&data->client->dev,
			"%s(): strtoul failed, result=%d\n",
				__func__, result);
		return -EINVAL;
	}

	if (1 < enable) {
		dev_err(&data->client->dev,
			"%s(): 1 < enable, enable=%d\n",
				__func__, enable);
		return -EINVAL;
	}

	result = lm3561_flash_mode(data, (unsigned)enable);

	if (result)
		return result;

	return size;
}

static ssize_t attr_flash_current_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct lm3561_drv_data *data = dev_get_drvdata(dev);

	int result;
	unsigned long flash_current;

	result = lm3561_get_flash_current(data, &flash_current);

	if (result != 0)
		return result;

	return snprintf(buf, PAGE_SIZE, "%lu\n", flash_current);
}

static ssize_t attr_flash_current_store(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t size)
{
	struct lm3561_drv_data *data = dev_get_drvdata(dev);
	unsigned long flash_current;
	int result = 0;

	result = strict_strtoul(buf, 10, &flash_current);
	if (result) {
		dev_err(&data->client->dev,
			"%s(): strtoul failed, result=%d\n",
				__func__, result);
		return -EINVAL;
	}

	result = lm3561_set_flash_current(data, flash_current);

	if (result)
		return result;

	return size;
}

static ssize_t attr_flash_duration_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct lm3561_drv_data *data = dev_get_drvdata(dev);
	int result;
	unsigned long flash_duration;

	result = lm3561_get_flash_duration(data, &flash_duration);
	if (result != 0)
		return result;

	return snprintf(buf, PAGE_SIZE, "%lu\n", flash_duration);
}

static ssize_t attr_flash_duration_store(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t size)
{
	struct lm3561_drv_data *data = dev_get_drvdata(dev);
	unsigned long flash_duration;
	int result;

	result = strict_strtoul(buf, 10, &flash_duration);
	if (result) {
		dev_err(&data->client->dev,
			"%s(): strtoul failed, result=%d\n",
				__func__, result);
		return -EINVAL;
	}

	result = lm3561_set_flash_duration(data, flash_duration);

	if (result)
		return result;

	return size;
}

static ssize_t attr_flash_sync_enable_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct lm3561_drv_data *data = dev_get_drvdata(dev);
	int result;
	u8 reg_cfg1;

	result = lm3561_get_reg_data(data, LM3561_REG_CFG_1, &reg_cfg1);
	if (result)
		return result;
	reg_cfg1 &= LM3561_CFG1_STROBE_INPUT_MASK;

	return snprintf(buf, PAGE_SIZE, "%hu\n", reg_cfg1 ? 1 : 0);
}

static ssize_t attr_flash_sync_enable_store(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t size)
{
	struct lm3561_drv_data *data = dev_get_drvdata(dev);
	unsigned long enable;
	int result;

	result = strict_strtoul(buf, 10, &enable);
	if (result) {
		dev_err(&data->client->dev,
			"%s(): strtoul failed, result=%d\n",
				__func__, result);
		return -EINVAL;
	}

	if (1 < enable) {
		dev_err(&data->client->dev,
			"%s(): 1 < enable, enable=%lu\n",
				__func__, enable);
		return -EINVAL;
	}

	result = lm3561_set_flash_sync(data,
			       enable ? LM3561_SYNC_ON : LM3561_SYNC_OFF);

	if (result)
		return result;

	return size;
}

static ssize_t attr_status_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lm3561_drv_data *data = dev_get_drvdata(dev);
	int result;
	u8 status;

	result = lm3561_check_status(data, &status);

	if (result)
		return result;

	return snprintf(buf, PAGE_SIZE, "%d\n", status);

}
static struct device_attribute attributes[] = {
	__ATTR(torch_enable, 0660,
		attr_torch_enable_show, attr_torch_enable_store),
	__ATTR(torch_current, 0660,
		attr_torch_current_show, attr_torch_current_store),
	__ATTR(flash_enable, 0660,
		attr_flash_enable_show, attr_flash_enable_store),
	__ATTR(flash_current, 0660,
		attr_flash_current_show, attr_flash_current_store),
	__ATTR(flash_duration, 0660,
		attr_flash_duration_show, attr_flash_duration_store),
	__ATTR(flash_synchronization, 0660,
		attr_flash_sync_enable_show, attr_flash_sync_enable_store),
	__ATTR(status, 0440, attr_status_show, NULL),
};

static int lm3561_create_sysfs_interfaces(struct device *dev)
{
	int i;
	int result;

	for (i = 0; i < ARRAY_SIZE(attributes); i++) {
		result = device_create_file(dev, &attributes[i]);
		if (result) {
			for (; i >= 0; i--)
				device_remove_file(dev, &attributes[i]);
			dev_err(dev, "%s(): Failed to create sysfs I/F\n",
					__func__);
			return result;
		}
	}

	return result;
}

static void remove_sysfs_interfaces(struct device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		device_remove_file(dev, &attributes[i]);
}

/*
 ****************************************************************************
 * - Device operation such as;
 *   probe, init/exit, remove
 ****************************************************************************
 */
static int __devinit lm3561_probe(struct i2c_client *client,
	  const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct lm3561_platform_data *pdata = client->dev.platform_data;
	struct lm3561_drv_data *data;
	int result;

	dev_info(&client->dev, "%s\n", __func__);

	if (!pdata) {
		dev_err(&data->client->dev,
			"%s(): failed during init",
				__func__);
		return -EINVAL;
	}

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_I2C_BLOCK)) {
		dev_err(&data->client->dev,
			"%s(): failed during i2c_check_functionality",
				__func__);
		return -EIO;
	}

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		dev_err(&data->client->dev,
			"%s(): failed during kzalloc",
				__func__);
		return -ENOMEM;
	}

	dev_set_drvdata(&client->dev, data);

	data->client = client;

	result = pdata->hw_enable();
	if (result) {
		dev_err(&client->dev,
			"%s: Failed to HW Enable.\n", __func__);
		goto err_setup;
	}

	data->led_nums = 1;
	data->torch_current_shift = 0;
	data->flash_current_shift = 0;
	if (pdata->current_limit >= 1500000) {
		data->reg_flash_duration_limit =
			LM3561_FLASH_DURATION_CL_1500MA;
	} else if (pdata->current_limit >= 1000000) {
		data->reg_flash_duration_limit =
			LM3561_FLASH_DURATION_CL_1000MA;
	} else {
		/* current_limit > 1500000uA || current_limit < 1000000uA */
		dev_err(&data->client->dev,
			"%s(): current_limit(%luuA) is invalid\n",
			__func__, pdata->current_limit);
		result = -EINVAL;
		goto err_chip_init;
	}

	result = lm3561_chip_init(data, pdata);
	if (result) {
		dev_err(&data->client->dev,
			"%s(): chip init failed",
				__func__);
		goto err_chip_init;
	}

	result = lm3561_create_sysfs_interfaces(&client->dev);
	if (result) {
		dev_err(&data->client->dev,
			"%s(): create sysfs failed",
				__func__);
		goto err_chip_init;
	}

	dev_info(&data->client->dev, "%s: loaded\n", __func__);

	return 0;

err_chip_init:
	pdata->hw_disable();
err_setup:
	dev_set_drvdata(&client->dev, NULL);
	kfree(data);
	dev_err(&client->dev,
		"%s: failed with code %d.\n", __func__, result);

	return result;
}

static int __devexit lm3561_remove(struct i2c_client *client)
{
	struct lm3561_drv_data *data = dev_get_drvdata(&client->dev);
	struct lm3561_platform_data *pdata = client->dev.platform_data;

	remove_sysfs_interfaces(&client->dev);
	pdata->hw_disable();
	kfree(data);
	return 0;
}

#ifdef CONFIG_SUSPEND
static int lm3561_suspend(struct device *dev)
{
	struct lm3561_drv_data *data = dev_get_drvdata(dev);
	struct lm3561_platform_data *pdata = data->client->dev.platform_data;
	int result;

	/* Shutdown in LM3561, bits 1-0 = 0.
	   Strobe is level triggered, bit 2 = 0 */
	result = lm3561_set_reg_data(data,
				     LM3561_REG_ENABLE,
				     LM3561_ENABLE_EN_MASK, 0x00);
	if (result) {
		dev_err(dev, "%s:set_reg_data error\n", __func__);
		goto exit_suspend;
	}

	result = pdata->hw_disable();
	if (result) {
		dev_err(dev, "%s: Failed to HW Disable.\n", __func__);
		goto exit_suspend;
	}

	dev_info(&data->client->dev,
		 "%s: Suspending LM3561 driver.\n", __func__);

exit_suspend:
	return result;
}

static int lm3561_resume(struct device *dev)
{
	struct lm3561_drv_data *data = dev_get_drvdata(dev);
	struct lm3561_platform_data *pdata = data->client->dev.platform_data;
	int result;

	result = pdata->hw_enable();
	if (result) {
		dev_err(dev, "%s: Failed to HW Enable.\n", __func__);
		goto exit_resume;
	}

	dev_info(dev, "%s: Reinit lm3561 chip.\n", __func__);
	result = lm3561_chip_init(data, pdata);
	if (result) {
		dev_err(dev, "%s:chip init error\n", __func__);
		goto exit_resume;
	}

	dev_info(&data->client->dev,
		 "%s: Resuming LM3561 driver.\n", __func__);

exit_resume:
	return result;
}
#else
#define lm3561_suspend NULL
#define lm3561_resume NULL
#endif

static const struct dev_pm_ops lm3561_pm = {
	.suspend = lm3561_suspend,
	.resume = lm3561_resume,
};

static const struct i2c_device_id lm3561_id[] = {
	{ LM3561_DRV_NAME, 0 },
};
MODULE_DEVICE_TABLE(i2c, lm3561_id);

static struct i2c_driver lm3561_driver = {
	.driver = {
		.name	= LM3561_DRV_NAME,
		.owner	= THIS_MODULE,
		.pm = &lm3561_pm,
	},
	.probe	= lm3561_probe,
	.remove	= __devexit_p(lm3561_remove),
	.id_table = lm3561_id,
};

static int __init lm3561_init(void)
{
	return i2c_add_driver(&lm3561_driver);
}

static void __exit lm3561_exit(void)
{
	i2c_del_driver(&lm3561_driver);
}

module_init(lm3561_init);
module_exit(lm3561_exit);

MODULE_AUTHOR("Angela Fox <angela.fox@sonyericsson.com>");
MODULE_DESCRIPTION("LM3561 I2C LED driver");
MODULE_LICENSE("GPL");
