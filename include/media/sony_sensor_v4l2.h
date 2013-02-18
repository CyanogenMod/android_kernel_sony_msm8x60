/* include/media/sony_sensor_v4l2.h
 *
 * Copyright (C) 2012 Sony Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef __LINUX_SONY_SENSOR_V4L2_H
#define __LINUX_SONY_SENSOR_V4L2_H

#ifdef __KERNEL__

enum sony_sensor_cmd {
	CAM_VDIG,
	CAM_VIO,
	CAM_VANA,
	CAM_VAF,
	GPIO_AF,
	GPIO_RESET,
	CAM_CLK,
	I2C_WRITE,
	EXIT,
};

struct sony_sensor_seq {
	enum sony_sensor_cmd	cmd;
	int			val;
	int			wait;
};

struct sony_sensor_module {
	char				*name;
	const struct sony_sensor_seq	*seq_on;
	const struct sony_sensor_seq	*seq_off;
};

struct sony_sensor_info {
	uint16_t			i2c_addr;
	uint16_t			eeprom_addr;
	int				eeprom_type;
	int				gpio_af;
	int				subdev_code;
	const struct sony_sensor_module	*modules;
	int				modules_num;
};

extern const struct sony_sensor_info sensor_info[];

#endif
#endif
