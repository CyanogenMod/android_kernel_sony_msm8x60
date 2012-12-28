/* drivers/media/video/msm/sensors/sony_sensor_sub_v4l2.c
 *
 * Copyright (C) 2012 Sony Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef DEBUG
#define DEBUG
#endif

#define ENABLE_LOGE
#define ENABLE_LOGD
/* #define ENABLE_LOGV */

#include <linux/regulator/consumer.h>
#include <linux/i2c.h>
#include <mach/gpiomux.h>
#include <media/msm_camera.h>
#include <mach/camera.h>
#include <media/sony_sensor_v4l2.h>
#include "msm_sensor.h"
#include "msm_camera_i2c.h"
#include "msm_ispif.h"
#include "msm.h"

#ifdef ENABLE_LOGE
#define LOGE(f, a...)	dev_err(sensor_data.d, "%s: " f, __func__, ##a)
#else
#define LOGE(f, a...)
#endif

#ifdef ENABLE_LOGD
#define LOGD(f, a...)	dev_dbg(sensor_data.d, "%s: " f, __func__, ##a)
#else
#define LOGD(f, a...)
#endif

#ifdef ENABLE_LOGV
#define LOGV(f, a...)	dev_vdbg(sensor_data.d, "%s: " f, __func__, ##a)
#else
#define LOGV(f, a...)
#endif

#define I2C_MAX_DATA_LEN	256
#define SENSOR_NAME_LEN		8
#define SENSOR_ID		1
#define SENSOR_NAME		"sony_sensor_sub"
#define EEPROM_MAX_DATA_LEN	2048

#define SENSOR_ID_MT9M114	0x2481
#define SENSOR_ID_MT9V115	0x2284
#define MODULE_STW01BM0		"STW01BM0"
#define MODULE_APT01BM0		"APT01BM0"
#define MODULE_APT00YP1		"APT00YP1"
#define MODULE_STW00YP1		"STW00YP1"

struct sony_sensor_data {
	bool				probe_done;
	struct device			*d;
	struct msm_sensor_ctrl_t	*s_ctrl;
	uint8_t				buf[I2C_MAX_DATA_LEN];
	uint8_t				eeprom[EEPROM_MAX_DATA_LEN];
	uint16_t			eeprom_len;
	const struct sony_sensor_module *module;
	struct regulator		*cam_vio;
	struct regulator		*cam_vana;
	struct regulator		*cam_vdig;
	struct regulator		*cam_vaf;
};

static struct sony_sensor_data sensor_data;

static DEFINE_MUTEX(sensor_mut);

static struct msm_sensor_ctrl_t sensor_s_ctrl;

static const struct msm_cam_clk_info cam_clk_info[] = {
	{ "cam_clk", 8000000 },
};

static int sony_util_gpio_set(int gpio_pin, int value)
{
	gpio_set_value_cansleep(gpio_pin, value);

	return 0;
}

static int sony_util_vreg_set(struct msm_sensor_ctrl_t *s_ctrl,
			struct sony_sensor_data *data,
			enum sony_sensor_cmd cmd, int level)
{
	int rc = 0;
	struct regulator *vreg;
	struct device *dev = &s_ctrl->sensor_i2c_client->client->dev;

	if (cmd == CAM_VDIG) {
		if (data->cam_vdig) {
			vreg = data->cam_vdig;
		} else {
			vreg = regulator_get(dev, "cam_vdig");
			data->cam_vdig = vreg;
		}
	} else if (cmd == CAM_VIO) {
		if (data->cam_vio) {
			vreg = data->cam_vio;
		} else {
			vreg = regulator_get(dev, "cam_vio");
			data->cam_vio = vreg;
		}
	} else if (cmd == CAM_VANA) {
		if (data->cam_vana) {
			vreg = data->cam_vana;
		} else {
			vreg = regulator_get(dev, "cam_vana");
			data->cam_vana = vreg;
		}
	} else if (cmd == CAM_VAF) {
		if (data->cam_vaf) {
			vreg = data->cam_vaf;
		} else {
			vreg = regulator_get(dev, "cam_vaf");
			data->cam_vaf = vreg;
		}
	} else {
		rc = -EINVAL;
		LOGE("invalid resource\n");
		goto exit;
	}

	level *= 1000;
	if (level >= 0) {
		if (level > 0)
			rc = regulator_set_voltage(vreg, level, level);
		if (rc == 0) {
			rc = regulator_enable(vreg);
			if (rc < 0)
				regulator_disable(vreg);
		}
		if (rc < 0)
			regulator_put(vreg);
	} else {
		rc = regulator_disable(vreg);
		regulator_put(vreg);
	}

	if (rc < 0 || level < 0) {
		if (vreg == data->cam_vdig)
			data->cam_vdig = NULL;
		else if (vreg == data->cam_vio)
			data->cam_vio = NULL;
		else if (vreg == data->cam_vana)
			data->cam_vana = NULL;
		else if (vreg == data->cam_vaf)
			data->cam_vaf = NULL;
	}

exit:
	if (rc < 0)
		LOGE("error happened (%d)\n", rc);
	return rc;
}

static int sony_util_mclk_set(struct msm_sensor_ctrl_t *s_ctrl,
			uint32_t *mclk, int value)
{
	int rc;

	if (value >= 0) {
		if (value > 0 && mclk)
			*mclk = value;

		rc = msm_cam_clk_enable(
			&s_ctrl->sensor_i2c_client->client->dev,
			(struct msm_cam_clk_info *)cam_clk_info,
			&s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 1);
	} else {
		rc = msm_cam_clk_enable(
			&s_ctrl->sensor_i2c_client->client->dev,
			(struct msm_cam_clk_info *)cam_clk_info,
			&s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 0);
	}

	if (rc < 0)
		LOGE("error happened (%d)\n", rc);

	return rc;
}

static int sony_cam_i2c_read(struct msm_sensor_ctrl_t *s_ctrl,
			uint8_t slave_addr, uint32_t addr,
			uint8_t type, uint16_t len, uint8_t *data)
{
	int rc;
	uint8_t tmp[SENSOR_I2C_ADDR_4BYTE];
	struct i2c_msg msgs[] = {
		{
			.addr	= slave_addr,
			.flags	= 0,
			.len	= type,
			.buf	= tmp,
		},
		{
			.addr	= slave_addr,
			.flags	= I2C_M_RD,
			.len	= len,
			.buf	= sensor_data.buf,
		},
	};
	struct i2c_msg *pmsg = msgs;
	uint8_t msglen = 2;

	LOGV("slave:0x%04x addr:0x%04x, type:0x%02x, len:0x%02x\n",
			slave_addr, addr, type, len);

	if (len > I2C_MAX_DATA_LEN) {
		rc = -EPERM;
		LOGE("invalid parameter\n");
		goto exit;
	}

	switch (type) {
	case SENSOR_I2C_ADDR_0BYTE:
		pmsg = &msgs[1];
		msglen = 1;
		break;
	case SENSOR_I2C_ADDR_1BYTE:
		pmsg->buf[0] = (uint8_t)((addr & 0xFF) >> 0);
		break;
	case SENSOR_I2C_ADDR_2BYTE:
		pmsg->buf[0] = (uint8_t)((addr & 0xFF00) >> 8);
		pmsg->buf[1] = (uint8_t)((addr & 0x00FF) >> 0);
		break;
	case SENSOR_I2C_ADDR_4BYTE:
		pmsg->buf[0] = (uint8_t)((addr & 0xFF000000) >> 24);
		pmsg->buf[1] = (uint8_t)((addr & 0x00FF0000) >> 16);
		pmsg->buf[2] = (uint8_t)((addr & 0x0000FF00) >> 8);
		pmsg->buf[3] = (uint8_t)((addr & 0x000000FF) >> 0);
		break;
	default:
		rc = -EINVAL;
		LOGE("invalid case\n");
		goto exit;
	}

	rc = i2c_transfer(s_ctrl->sensor_i2c_client->client->adapter,
				pmsg, msglen);
	if (rc < 0 || rc != msglen) {
		rc = -EIO;
		LOGE("i2c transfer failed\n");
		goto exit;
	}
	memcpy(data, sensor_data.buf, len);
	rc = 0;

exit:
	return rc;
}

static int sony_cam_i2c_write(struct msm_sensor_ctrl_t *s_ctrl,
	uint8_t slave_addr, uint32_t addr, uint8_t type,
	uint16_t len, uint8_t *data)
{
	int rc;

	struct i2c_msg msgs[] = {
		{
			.addr	= slave_addr,
			.flags	= 0,
			.len	= len + type,
			.buf	= sensor_data.buf,
		},
	};

	struct i2c_msg *pmsg = msgs;
	uint8_t msglen = 1;
	LOGV("addr:0x%04x, type:0x%02x, len:0x%02x\n", addr, type, len);

	if (len + type > I2C_MAX_DATA_LEN) {
		rc = -EPERM;
		LOGE("invalid parameter\n");
		goto exit;
	}

	switch (type) {
	case SENSOR_I2C_ADDR_1BYTE:
		pmsg->buf[0] = (uint8_t)((addr & 0xFF) >> 0);
		break;
	case SENSOR_I2C_ADDR_2BYTE:
		pmsg->buf[0] = (uint8_t)((addr & 0xFF00) >> 8);
		pmsg->buf[1] = (uint8_t)((addr & 0x00FF) >> 0);
		break;
	case SENSOR_I2C_ADDR_4BYTE:
		pmsg->buf[0] = (uint8_t)((addr & 0xFF000000) >> 24);
		pmsg->buf[1] = (uint8_t)((addr & 0x00FF0000) >> 16);
		pmsg->buf[2] = (uint8_t)((addr & 0x0000FF00) >> 8);
		pmsg->buf[3] = (uint8_t)((addr & 0x000000FF) >> 0);
		break;
	default:
		rc = -EINVAL;
		LOGE("invalid case\n");
		goto exit;
	}

	memcpy(sensor_data.buf + type, data, len);
	rc = i2c_transfer(s_ctrl->sensor_i2c_client->client->adapter,
			pmsg, msglen);
	if (rc < 0 || rc != msglen) {
		rc = -EIO;
		LOGE("i2c transfer failed\n");
		goto exit;
	}
	rc = 0;
exit:
	return rc;
}

static int sony_util_power_ctrl(struct msm_sensor_ctrl_t *s_ctrl,
		struct sony_sensor_data *data, bool on)
{
	int rc = 0;
	const struct sony_sensor_module *mod = data->probe_done ?
		data->module : sensor_info[SENSOR_ID].modules;
	const struct sony_sensor_seq *seq = on ? mod->seq_on : mod->seq_off;

	while (seq->cmd != EXIT) {
		uint8_t iodt = 0x00;
		switch (seq->cmd) {
		case GPIO_RESET:
			rc = sony_util_gpio_set(s_ctrl->sensordata->
				sensor_platform_info->sensor_reset,
				seq->val);
			break;
		case GPIO_AF:
			rc = sony_util_gpio_set(sensor_info[SENSOR_ID].gpio_af,
				seq->val);
			break;
		case CAM_VDIG:
		case CAM_VIO:
		case CAM_VANA:
		case CAM_VAF:
			rc = sony_util_vreg_set(s_ctrl,
				data, seq->cmd, seq->val);
			break;
		case CAM_CLK:
			rc = sony_util_mclk_set(s_ctrl,
				(uint32_t *)&s_ctrl->clk_rate, seq->val);
			break;
		case I2C_WRITE:
			rc = sony_cam_i2c_write(s_ctrl,
				s_ctrl->sensor_i2c_addr,
				seq->val,
				SENSOR_I2C_ADDR_2BYTE,
				1,
				&iodt);
			break;
		default:
			goto exit;
		}
		mdelay(seq->wait);
		if (rc < 0 && on)
			goto exit;
		seq++;
	}
exit:
	return rc;
}

static int sony_eeprom_load(struct i2c_adapter *adapter)
{
	int rc = 0;
	uint16_t i;
	uint16_t len;
	uint8_t *d = sensor_data.eeprom;

	LOGV("load eeprom\n");

	/* load eeprom */
	if (sensor_info[SENSOR_ID].eeprom_type == 0) {
		for (i = 0; i < EEPROM_MAX_DATA_LEN; i += I2C_MAX_DATA_LEN) {
			uint8_t slave_addr = sensor_info[SENSOR_ID].eeprom_addr
						+ ((i & 0x0700) >> 8);
			uint32_t offset = i & 0x00FF;

			rc = sony_cam_i2c_read(&sensor_s_ctrl, slave_addr,
				offset, SENSOR_I2C_ADDR_1BYTE,
				I2C_MAX_DATA_LEN, d + i);
			if (rc < 0) {
				LOGE("eeprom i2c read fail %d\n", rc);
				sensor_data.eeprom_len = 0;
				goto exit;
			}
		}
		len = i;
	} else {
		len = 16;
		rc = sony_cam_i2c_read(&sensor_s_ctrl,
					sensor_info[SENSOR_ID].eeprom_addr,
					0x0000, SENSOR_I2C_ADDR_2BYTE, 2, d);
		if (rc < 0) {
			LOGE("i2c read faile\n");
			goto exit;
		}

		if (((uint16_t)d[0] << 8 | d[1]) == SENSOR_ID_MT9M114) {
			d[0] = 0x00;
			d[1] = 0x55;
			rc = sony_cam_i2c_write(&sensor_s_ctrl,
				sensor_info[SENSOR_ID].eeprom_addr,
				0x3052, SENSOR_I2C_ADDR_2BYTE, 2, d);
			if (rc < 0)
				goto exit;

			d[0] = 0x00;
			d[1] = 0x10;
			rc = sony_cam_i2c_write(&sensor_s_ctrl,
				sensor_info[SENSOR_ID].eeprom_addr,
				0x3050, SENSOR_I2C_ADDR_2BYTE, 2, d);
			if (rc < 0)
				goto exit;

			i = 0;
			do {
				msleep(20);
				i++;
				rc = sony_cam_i2c_read(&sensor_s_ctrl,
					sensor_info[SENSOR_ID].eeprom_addr,
					0x3050, SENSOR_I2C_ADDR_2BYTE, 2, d);
				if (rc < 0)
					goto exit;
			} while (!(d[1] & 0x20) && i < 100);

			if (i >= 100) {
				rc = -ENODEV;
				goto exit;
			}

			rc = sony_cam_i2c_read(&sensor_s_ctrl,
				sensor_info[SENSOR_ID].eeprom_addr,
				0x313A, SENSOR_I2C_ADDR_2BYTE, 2, d + 8);
			if (rc < 0)
				goto exit;

			rc = sony_cam_i2c_read(&sensor_s_ctrl,
				sensor_info[SENSOR_ID].eeprom_addr,
				0x313C, SENSOR_I2C_ADDR_2BYTE, 2, d + 10);
			if (rc < 0)
				goto exit;

			if ((d[8] & 0xC0) == 0x00)
				memcpy(d, MODULE_STW01BM0, SENSOR_NAME_LEN);
			else
				memcpy(d, MODULE_APT01BM0, SENSOR_NAME_LEN);
		} else if (((uint16_t)d[0] << 8 | d[1]) == SENSOR_ID_MT9V115) {
			d[0] = 0x00;
			d[1] = 0x00;
			rc = sony_cam_i2c_write(&sensor_s_ctrl,
				sensor_info[SENSOR_ID].eeprom_addr,
				0x0018, SENSOR_I2C_ADDR_2BYTE, 2, d);
			if (rc < 0)
				goto exit;

			i = 0;
			do {
				msleep(20);
				i++;
				rc = sony_cam_i2c_read(&sensor_s_ctrl,
					sensor_info[SENSOR_ID].eeprom_addr,
					0x0018, SENSOR_I2C_ADDR_2BYTE, 2, d);
				if (rc < 0)
					goto exit;
			} while ((d[1] & 0x40) && i < 100);

			if (i >= 100) {
				rc = -ENODEV;
				goto exit;
			}

			rc = sony_cam_i2c_read(&sensor_s_ctrl,
					sensor_info[SENSOR_ID].eeprom_addr,
					0x001A, SENSOR_I2C_ADDR_2BYTE, 2, d);
			if (rc < 0)
				goto exit;

			if ((d[0] & 0xF0) == 0xB0)
				memcpy(d, MODULE_APT00YP1, 8);
			else if ((d[0] & 0xF0) == 0xE0 || (d[0] & 0xF0) == 0x00)
				memcpy(d, MODULE_STW00YP1, 8);
			else {
				LOGE("%s Module ID is irregular\n", __func__);
				memcpy(d, MODULE_APT00YP1, 8);
			}
		} else {
			LOGE("%s Wrong sub camera module.\n", __func__);
		}
	}

	/* identify sensor module */
	for (i = 1; i < sensor_info[SENSOR_ID].modules_num; i++) {
		if (!strncmp(sensor_info[SENSOR_ID].modules[i].name,
				sensor_data.eeprom,
				SENSOR_NAME_LEN)) {
			sensor_data.module = &sensor_info[SENSOR_ID].modules[i];
			LOGD("detected sensor module name\n");
			break;
		}
	}

	if (sensor_data.module) {
		sensor_s_ctrl.sensor_i2c_addr = sensor_info[SENSOR_ID].i2c_addr;
		sensor_data.eeprom_len = len;
	} else {
		rc = -ENODEV;
		LOGD("unsupported sensor module %d\n", rc);
		goto exit;
	}

	LOGD("MODULE NAME: len=%d, name=%c%c%c%c%c%c%c%c\n",
			len, d[0], d[1], d[2], d[3],
			d[4], d[5], d[6], d[7]);
exit:
	return rc;
}

static void sony_sensor_start_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
}

static void sony_sensor_stop_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
}

static void sony_sensor_group_hold_on(struct msm_sensor_ctrl_t *s_ctrl)
{
}

static void sony_sensor_group_hold_off(struct msm_sensor_ctrl_t *s_ctrl)
{
}

static int32_t sony_sensor_set_fps(struct msm_sensor_ctrl_t *s_ctrl,
						struct fps_cfg *fps)
{
	return 0;
}

static int32_t sony_sensor_write_exp_gain1(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line)
{
	return 0;
}

static int32_t sony_sensor_match_id(struct msm_sensor_ctrl_t *s_ctrl)
{
	return 0;
}

static int32_t sony_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res)
{
	int32_t rc = 0;

	s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);

	if (update_type == MSM_SENSOR_REG_INIT) {
		s_ctrl->curr_csi_params = NULL;
	} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_PCLK_CHANGE,
			&s_ctrl->msm_sensor_reg->
			output_settings[res].op_pixel_clk);
		s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
	}

	return rc;
}

static int32_t sony_sensor_set_sensor_mode(struct msm_sensor_ctrl_t *s_ctrl,
	int mode, int res)
{
	int32_t rc = 0;

	LOGV("%s: mode[%d] res[%d]\n", __func__, mode, res);

	if (s_ctrl->func_tbl->sensor_setting
		(s_ctrl, MSM_SENSOR_UPDATE_PERIODIC, res) < 0)
		return rc;
	s_ctrl->curr_res = res;

	return rc;
}

static int32_t sony_sensor_mode_init(struct msm_sensor_ctrl_t *s_ctrl,
			int mode, struct sensor_init_cfg *init_info)
{
	int32_t rc = 0;
	s_ctrl->fps_divider = Q10;

	LOGV("%s: %d\n", __func__, __LINE__);

	s_ctrl->curr_res = MSM_SENSOR_INVALID_RES;
	s_ctrl->cam_mode = mode;

	rc = s_ctrl->func_tbl->sensor_setting(s_ctrl,
		MSM_SENSOR_REG_INIT, 0);

	return rc;
}

static int32_t sony_sensor_get_output_info(struct msm_sensor_ctrl_t *s_ctrl,
		struct sensor_output_info_t *sensor_output_info)
{
	return 0;
}

static int sony_sensor_config(struct msm_sensor_ctrl_t *s_ctrl,
			void __user *argp)
{
	int rc = 0;
	struct sensor_cfg_data cdata;
	uint8_t slave_addr;
	uint32_t addr;
	uint8_t type;
	uint8_t length;
	uint8_t data[I2C_MAX_DATA_LEN];
	uint16_t rom_offset;
	uint16_t rom_len;

	mutex_lock(s_ctrl->msm_sensor_mutex);

	if (copy_from_user(&cdata, (void *)argp, sizeof(cdata))) {
		rc = -EFAULT;
		LOGE("copy failed\n");
		goto exit;
	}

	switch (cdata.cfgtype) {
	case CFG_SET_WRITE_CMD:
		slave_addr = cdata.cfg.i2c_io.slave_addr;
		addr = cdata.cfg.i2c_io.address;
		type = cdata.cfg.i2c_io.address_type;
		length = cdata.cfg.i2c_io.length;
		if (!length || length > I2C_MAX_DATA_LEN) {
			rc = -EPERM;
			LOGE("parameter failed\n");
			goto exit;
		}
		memset(data, 0, sizeof(data));
		if (copy_from_user(data, cdata.cfg.i2c_io.data, length)) {
			rc = -EFAULT;
			LOGE("copy failed\n");
			goto exit;
		}
		rc = sony_cam_i2c_write(s_ctrl,
			slave_addr, addr, type, length, data);
		if (rc < 0) {
			LOGE("i2c write failed\n");
			goto exit;
		}
		break;
	case CFG_SET_READ_CMD:
		slave_addr = cdata.cfg.i2c_io.slave_addr;
		addr = cdata.cfg.i2c_io.address;
		type = cdata.cfg.i2c_io.address_type;
		length = cdata.cfg.i2c_io.length;
		if (!length || length > I2C_MAX_DATA_LEN) {
			rc = -EPERM;
			LOGE("parameter failed\n");
			goto exit;
		}
		memset(data, 0, sizeof(data));
		rc = sony_cam_i2c_read(s_ctrl,
			slave_addr, addr, type, length, data);
		if (rc < 0) {
			LOGE("i2c read failed\n");
			goto exit;
		}
		if (copy_to_user(cdata.cfg.i2c_io.data, data, length)) {
			rc = -EFAULT;
			LOGE("copy failed\n");
			goto exit;
		}
		break;
	case CFG_SET_GPIO_CTRL:
		if (cdata.cfg.gpio_ctrl.gpio == SENSOR_GPIO_CTRL_RESET) {
			rc = sony_util_gpio_set(
				s_ctrl->sensordata->sensor_reset,
				cdata.cfg.gpio_ctrl.value);
		} else
			rc = -EINVAL;
		if (rc < 0) {
			LOGE("gpio control failed\n");
			goto exit;
		}
		break;
	case CFG_SET_CSI_CTRL:
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_CSID_CFG,
			&cdata.cfg.csi_ctrl.csid_params);
		mb();
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_CSIPHY_CFG,
			&cdata.cfg.csi_ctrl.csiphy_params);
		mb();
		break;
	case CFG_GET_ROM:
		rom_offset = cdata.cfg.rom_in.address;
		rom_len = cdata.cfg.rom_in.length;
		if (!rom_len || rom_offset >= EEPROM_MAX_DATA_LEN) {
			rc = -EPERM;
			LOGE("parameter failed\n");
			goto exit;
		}
		if (rom_offset + rom_len > EEPROM_MAX_DATA_LEN)
			rom_len = EEPROM_MAX_DATA_LEN - rom_offset;
		if (copy_to_user(cdata.cfg.rom_in.data,
			sensor_data.eeprom + rom_offset, rom_len)) {
			rc = -EFAULT;
			LOGE("copy failed\n");
			goto exit;
		}
		break;
	default:
		LOGV("generic command %d\n", cdata.cfgtype);
		mutex_unlock(s_ctrl->msm_sensor_mutex);
		rc = msm_sensor_config(s_ctrl, argp);
		mutex_lock(s_ctrl->msm_sensor_mutex);
		if (rc < 0)
			LOGE("generic command failed\n");
		goto exit;
	}

	if (copy_to_user((void *)argp, &cdata, sizeof(cdata))) {
		rc = -EFAULT;
		LOGE("copy failed\n");
		goto exit;
	}

exit:
	mutex_unlock(s_ctrl->msm_sensor_mutex);

	if (rc < 0)
		LOGE("error happened (%d)\n", rc);

	return rc;
}

static int sony_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;

	LOGV("%s: %d\n", __func__, __LINE__);

	rc = msm_camera_request_gpio_table(data, 1);
	if (rc < 0)
		goto exit;

	rc = sony_util_power_ctrl(s_ctrl, &sensor_data, true);
	if (rc < 0)
		LOGE("power_up fail\n");
exit:
	return rc;
}

static int sony_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;

	rc = sony_util_power_ctrl(s_ctrl, &sensor_data, false);
	if (rc < 0)
		LOGE("power_down fail\n");

	rc = msm_camera_request_gpio_table(data, 0);

	return rc;
}

static int32_t sony_sensor_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	struct msm_sensor_ctrl_t *s_ctrl;

	LOGV("%s_probe called\n", client->name);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		LOGE("i2c_check_functionality failed\n");
		rc = -EFAULT;
		return rc;
	}

	s_ctrl = (struct msm_sensor_ctrl_t *)(id->driver_data);

	if (s_ctrl->sensor_i2c_client != NULL) {
		s_ctrl->sensor_i2c_client->client = client;
		if (s_ctrl->sensor_i2c_addr != 0)
			s_ctrl->sensor_i2c_client->client->addr =
				s_ctrl->sensor_i2c_addr;
	} else {
		rc = -EFAULT;
		return rc;
	}

	memset(&sensor_data, 0, sizeof(sensor_data));
	sensor_data.d = &s_ctrl->sensor_i2c_client->client->dev;
	s_ctrl->sensordata = client->dev.platform_data;

	rc = s_ctrl->func_tbl->sensor_power_up(s_ctrl);
	if (rc < 0)
		goto probe_fail;

	rc = sony_eeprom_load(s_ctrl->sensor_i2c_client->client->adapter);
	if (rc < 0) {
		LOGE("eeprom load fail\n");
		goto probe_fail;
	}

	s_ctrl->func_tbl->sensor_power_down(s_ctrl);

	s_ctrl->sensor_v4l2_subdev_info->code =
		sensor_info[SENSOR_ID].subdev_code;

	snprintf(s_ctrl->sensor_v4l2_subdev.name,
		sizeof(s_ctrl->sensor_v4l2_subdev.name), "%s", id->name);
	v4l2_i2c_subdev_init(&s_ctrl->sensor_v4l2_subdev, client,
		s_ctrl->sensor_v4l2_subdev_ops);

	msm_sensor_register(&s_ctrl->sensor_v4l2_subdev);
	sensor_data.probe_done = true;
	LOGD("probe ok\n");
	return rc;

probe_fail:
	LOGE("%s_i2c_probe failed\n", client->name);
	s_ctrl->func_tbl->sensor_power_down(s_ctrl);
	return rc;
}

static struct v4l2_subdev_info sony_sensor_subdev_info[] = {
	{
		.code		= 0x00,
		.colorspace	= V4L2_COLORSPACE_JPEG,
		.fmt		= 1,
		.order		= 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_sensor_output_info_t sony_sensor_dimensions[] = {
	{
		.x_output		= 0x0000,
		.y_output		= 0x0000,
		.line_length_pclk	= 0x0000,
		.frame_length_lines	= 0x0000,
		.vt_pixel_clk		= 228570000,
		.op_pixel_clk		= 228570000,
		.binning_factor		= 1,
	},
	{
		.x_output		= 0x0000,
		.y_output		= 0x0000,
		.line_length_pclk	= 0x0000,
		.frame_length_lines	= 0x0000,
		.vt_pixel_clk		= 228570000,
		.op_pixel_clk		= 228570000,
		.binning_factor		= 2,
	},
	{
		.x_output		= 0x0000,
		.y_output		= 0x0000,
		.line_length_pclk	= 0x0000,
		.frame_length_lines	= 0x0000,
		.vt_pixel_clk		= 228570000,
		.op_pixel_clk		= 228570000,
		.binning_factor		= 3,
	},
	{
		.x_output		= 0x0000,
		.y_output		= 0x0000,
		.line_length_pclk	= 0x0000,
		.frame_length_lines	= 0x0000,
		.vt_pixel_clk		= 228570000,
		.op_pixel_clk		= 228570000,
		.binning_factor		= 4,
	},
	{
		.x_output		= 0x0000,
		.y_output		= 0x0000,
		.line_length_pclk	= 0x0000,
		.frame_length_lines	= 0x0000,
		.vt_pixel_clk		= 228570000,
		.op_pixel_clk		= 228570000,
		.binning_factor		= 5,
	},
	{
		.x_output		= 0x0000,
		.y_output		= 0x0000,
		.line_length_pclk	= 0x0000,
		.frame_length_lines	= 0x0000,
		.vt_pixel_clk		= 228570000,
		.op_pixel_clk		= 228570000,
		.binning_factor		= 6,
	},
};

static const struct i2c_device_id sony_sensor_i2c_id[] = {
	{ SENSOR_NAME, (kernel_ulong_t)&sensor_s_ctrl },
	{ }
};

static struct i2c_driver sony_sensor_i2c_driver = {
	.id_table	= sony_sensor_i2c_id,
	.probe		= sony_sensor_i2c_probe,
	.driver = {
		.name	= SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client sony_sensor_i2c_client = {
	.addr_type	= MSM_CAMERA_I2C_WORD_ADDR,
};

static int __init msm_sensor_init_module(void)
{
	return i2c_add_driver(&sony_sensor_i2c_driver);
}

static struct v4l2_subdev_core_ops sony_sensor_subdev_core_ops = {
	.ioctl		= msm_sensor_subdev_ioctl,
	.s_power	= msm_sensor_power,
};

static struct v4l2_subdev_video_ops sony_sensor_subdev_video_ops = {
	.enum_mbus_fmt	= msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops sony_sensor_subdev_ops = {
	.core	= &sony_sensor_subdev_core_ops,
	.video	= &sony_sensor_subdev_video_ops,
};

static struct msm_sensor_fn_t sony_sensor_func_tbl = {
	.sensor_start_stream		= sony_sensor_start_stream,
	.sensor_stop_stream		= sony_sensor_stop_stream,
	.sensor_group_hold_on		= sony_sensor_group_hold_on,
	.sensor_group_hold_off		= sony_sensor_group_hold_off,
	.sensor_set_fps			= sony_sensor_set_fps,
	.sensor_write_exp_gain		= sony_sensor_write_exp_gain1,
	.sensor_write_snapshot_exp_gain = sony_sensor_write_exp_gain1,
	.sensor_setting			= sony_sensor_setting,
	.sensor_set_sensor_mode		= sony_sensor_set_sensor_mode,
	.sensor_mode_init		= sony_sensor_mode_init,
	.sensor_get_output_info		= sony_sensor_get_output_info,
	.sensor_config			= sony_sensor_config,
	.sensor_power_up		= sony_sensor_power_up,
	.sensor_power_down		= sony_sensor_power_down,
	.sensor_match_id		= sony_sensor_match_id,
	.sensor_get_csi_params		= msm_sensor_get_csi_params,
};

static struct msm_sensor_reg_t sony_sensor_regs = {
	.default_data_type		= MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf		= NULL,
	.start_stream_conf_size		= 0,
	.stop_stream_conf		= NULL,
	.stop_stream_conf_size		= 0,
	.group_hold_on_conf		= NULL,
	.group_hold_on_conf_size	= 0,
	.group_hold_off_conf		= NULL,
	.group_hold_off_conf_size	= 0,
	.init_settings			= NULL,
	.init_size			= 0,
	.mode_settings			= NULL,
	.output_settings		= sony_sensor_dimensions,
	.num_conf			= 0,
};

static struct msm_sensor_ctrl_t sensor_s_ctrl = {
	.msm_sensor_reg			= &sony_sensor_regs,
	.sensor_i2c_client		= &sony_sensor_i2c_client,
	.sensor_i2c_addr		= 0x00,
	.sensor_output_reg_addr		= NULL,
	.sensor_id_info			= NULL,
	.sensor_exp_gain_info		= NULL,
	.cam_mode			= MSM_SENSOR_MODE_INVALID,
	.csi_params			= NULL,
	.msm_sensor_mutex		= &sensor_mut,
	.sensor_i2c_driver		= &sony_sensor_i2c_driver,
	.sensor_v4l2_subdev_info	= sony_sensor_subdev_info,
	.sensor_v4l2_subdev_info_size	= ARRAY_SIZE(sony_sensor_subdev_info),
	.sensor_v4l2_subdev_ops		= &sony_sensor_subdev_ops,
	.func_tbl			= &sony_sensor_func_tbl,
	.clk_rate			= 8000000,
};

late_initcall(msm_sensor_init_module);

MODULE_DESCRIPTION("SONY V4L2 sub camera sensor driver");
MODULE_LICENSE("GPL v2");
