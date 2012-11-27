/*
 * Bosh BMA 2XX. Digital, triaxial acceleration sensor.
 *
 * Copyright (C) 2010 Sony Ericsson Mobile Communications AB.
 * Copyright (C) 2012 Sony Mobile Communications AB.
 *
 * Authors: Marcus Bauer <marcus.bauer@sonyericsson.com>
 *          Stefan Karlsson <stefan3.karlsson@sonyericssom.com>
 *
 * NOTE: This file has been created by Sony Ericsson Mobile Communications AB.
 *       This file contains code from: bma150.c
 *       The orginal bma150.c header is included below:
 *
 * Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef LINUX_BMA2XX_MODULE_H
#define LINUX_BMA2XX_MODULE_H

#include <linux/device.h>

#define BMA2XX_NAME                      "bma2xx"

#define BMA2XX_CHIP_ID_REG               0x00
#define BMA2XX_CHIP_ID_BMA250            0x03
#define BMA2XX_CHIP_ID_BMA255            0xfa

#define BMA2XX_VERSION_REG               0x01
#define BMA2XX_X_AXIS_LSB_REG            0x02
#define BMA2XX_Y_AXIS_LSB_REG            0x04
#define BMA2XX_Z_AXIS_LSB_REG            0x06

#define BMA2XX_RANGE_REG                 0x0F
#define BMA2XX_RANGE_MASK                0x0F
#define BMA2XX_RANGE_16G                 0x0C
#define BMA2XX_RANGE_8G                  0x08
#define BMA2XX_RANGE_4G                  0x05
#define BMA2XX_RANGE_2G                  0x03

#define BMA2XX_BW_SEL_REG                0x10
#define BMA2XX_BW_7_81HZ                 0x08
#define BMA2XX_BW_15_63HZ                0x09
#define BMA2XX_BW_31_25HZ                0x0A
#define BMA2XX_BW_62_50HZ                0x0B
#define BMA2XX_BW_125HZ                  0x0C
#define BMA2XX_BW_250HZ                  0x0D
#define BMA2XX_BW_500HZ                  0x0E
#define BMA2XX_BW_1000HZ                 0x0F

#define BMA2XX_MODE_CTRL_REG             0x11
#define BMA2XX_MODE_NOSLEEP              0x00
#define BMA2XX_MODE_SLEEP_0_5MS          0x4A
#define BMA2XX_MODE_SLEEP_1MS            0x4C
#define BMA2XX_MODE_SLEEP_2MS            0x4E
#define BMA2XX_MODE_SLEEP_4MS            0x50
#define BMA2XX_MODE_SLEEP_6MS            0x52
#define BMA2XX_MODE_SLEEP_10MS           0x54
#define BMA2XX_MODE_SLEEP_25MS           0x56
#define BMA2XX_MODE_SLEEP_50MS           0x58
#define BMA2XX_MODE_SLEEP_100MS          0x5A
#define BMA2XX_MODE_SLEEP_500MS          0x5C
#define BMA2XX_MODE_SLEEP_1000MS         0x5E
#define BMA2XX_MODE_SUSPEND              0x80
#define BMA2XX_MODE_LOWPOWER             0x40
#define BMA2XX_MODE_DEEPSUSPEND          0x20

#define BMA2XX_RESET_REG                 0x14
#define BMA2XX_RESET                     0xB6

#define BMA2XX_INT_ENABLE1_REG           0x16
#define BMA2XX_INT_FLAT                  0x80
#define BMA2XX_INT_ORIENT                0x40
#define BMA2XX_INT_S_TAP                 0x20
#define BMA2XX_INT_D_TAP                 0x10
#define BMA2XX_INT_SLOPE_Z               0x04
#define BMA2XX_INT_SLOPE_Y               0x02
#define BMA2XX_INT_SLOPE_X               0x01

#define BMA2XX_INT_ENABLE2_REG           0x17
#define BMA2XX_INT_NEW_DATA              0x10
#define BMA2XX_INT_LOW_G                 0x08
#define BMA2XX_INT_HIGH_G_Z              0x04
#define BMA2XX_INT_HIGH_G_Y              0x02
#define BMA2XX_INT_HIGH_G_X              0x01

#define BMA2XX_INT_PIN1_REG              0x19
#define BMA2XX_INT_PIN1_FLAT             0x80
#define BMA2XX_INT_PIN1_ORIENT           0x40
#define BMA2XX_INT_PIN1_S_TAP            0x20
#define BMA2XX_INT_PIN1_D_TAP            0x10
#define BMA2XX_INT_PIN1_SLOPE            0x04
#define BMA2XX_INT_PIN1_HIGH_G           0x02
#define BMA2XX_INT_PIN1_LOW_G            0x01

#define BMA2XX_INT_NEW_DATA_REG          0x1A
#define BMA2XX_INT_PIN2                  0x80
#define BMA2XX_INT_PIN1                  0x01

#define BMA2XX_INT_PIN2_REG              0x1B
#define BMA2XX_INT_PIN2_FLAT             0x80
#define BMA2XX_INT_PIN2_ORIENT           0x40
#define BMA2XX_INT_PIN2_S_TAP            0x20
#define BMA2XX_INT_PIN2_D_TAP            0x10
#define BMA2XX_INT_PIN2_SLOPE            0x04
#define BMA2XX_INT_PIN2_HIGH_G           0x02
#define BMA2XX_INT_PIN2_LOW_G            0x01

#define BMA2XX_INT_CTRL_REG              0x21
#define BMA2XX_INT_RESET                 0x80
#define BMA2XX_INT_MODE_LATCHED          0x0F

#define BMA2XX_SLOPE_DUR                 0x27
#define BMA2XX_SLOPE_THR                 0x28

#define BMA2XX_TAP_SAMP_TH               0x2B
#define BMA2XX_SAMPLE_2                  0x00
#define BMA2XX_SAMPLE_4                  0x40
#define BMA2XX_SAMPLE_8                  0x80
#define BMA2XX_SAMPLE_16                 0xC0
#define BMA2XX_THRESHOLD                 0x01

/**
 * struct bma2xx_platform_data - data to set up bma2xx driver
 *
 * @setup: optional callback to activate the driver.
 * @teardown: optional callback to invalidate the driver.
 * @power_init: optional callback to init power management
 * @power_destroy: optional callback to destroy power management
 * @power_up: optional callback to turn power on, it is assumed
 *            that Vdd and/or VddIO is turned on. Note that this
 *            must include sleeping to wait for capacitors to get
 *            changed and curcuit wakeup (BMA255 3ms, BMA250 2ms)
 *            You shouldn't sleep more than say 25ms for driver
 *            to work.
 * @power_down: optional callback to turn power off, it is assumed
 *             that Vdd and/or VddIO is turned off
 *
**/

struct bma2xx_platform_data {
	int (*setup)(struct device *);
	void (*teardown)(struct device *);
	int (*power_up)(struct device *);
	int (*power_down)(struct device *);
	int range;
};

#endif /* LINUX_BMA2XX_MODULE_H */
