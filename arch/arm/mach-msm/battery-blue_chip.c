/* copyright (c) 2011, code aurora forum. all rights reserved.
 * Copyright (C) 2012 Sony Ericsson Mobile Communications AB.
 * Copyright (C) 2012 Sony Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/mfd/pm8xxx/pm8921-bms.h>

static struct single_row_lut fcc_temp = {
	.x	= {-20, 0, 25, 40, 65},
	.y	= {1470, 1515, 1520, 1530, 1540},
	.cols	= 5,
};

static struct single_row_lut fcc_sf = {
	.x	= {100, 200, 300, 400, 500},
	.y	= {100, 100, 100, 100, 100},
	.cols	= 5,
};

static struct sf_lut pc_sf = {
	.rows		= 10,
	.cols		= 5,
	.row_entries		= {100, 200, 300, 400, 500},
	.percent	= {100, 90, 80, 70, 60, 50, 40, 30, 20, 10},
	.sf		= {
		{100, 100, 100, 100, 100},
		{100, 100, 100, 100, 100},
		{100, 100, 100, 100, 100},
		{100, 100, 100, 100, 100},
		{100, 100, 100, 100, 100},
		{100, 100, 100, 100, 100},
		{100, 100, 100, 100, 100},
		{100, 100, 100, 100, 100},
		{100, 100, 100, 100, 100},
		{100, 100, 100, 100, 100}
	},
};

static struct pc_temp_ocv_lut  pc_temp_ocv = {
	.rows		= 29,
	.cols		= 5,
	.temp		= {-20, 0, 25, 40, 65},
	.percent	= {100, 95, 90, 85, 80, 75, 70, 65, 60, 55,
		50, 45, 40, 35, 30, 25, 20, 15, 10, 9,
		8, 7, 6, 5, 4, 3, 2, 1, 0
	},
	.ocv		= {
		{4160, 4160, 4160, 4160, 4160},
		{4110, 4110, 4110, 4110, 4110},
		{4065, 4065, 4065, 4065, 4065},
		{4025, 4025, 4025, 4025, 4025},
		{3995, 3995, 3995, 3995, 3995},
		{3962, 3962, 3962, 3962, 3962},
		{3938, 3938, 3938, 3938, 3938},
		{3910, 3910, 3910, 3910, 3910},
		{3880, 3880, 3880, 3880, 3880},
		{3846, 3846, 3846, 3846, 3846},
		{3828, 3828, 3828, 3828, 3828},
		{3812, 3812, 3812, 3812, 3812},
		{3802, 3802, 3802, 3802, 3802},
		{3796, 3796, 3796, 3796, 3796},
		{3788, 3788, 3788, 3788, 3788},
		{3776, 3776, 3776, 3776, 3776},
		{3760, 3760, 3760, 3760, 3760},
		{3730, 3730, 3730, 3730, 3730},
		{3705, 3705, 3705, 3705, 3705},
		{3700, 3700, 3700, 3700, 3700},
		{3680, 3680, 3680, 3680, 3680},
		{3660, 3660, 3660, 3660, 3660},
		{3640, 3640, 3640, 3640, 3640},
		{3600, 3600, 3600, 3600, 3600},
		{3550, 3550, 3550, 3550, 3550},
		{3500, 3500, 3500, 3500, 3500},
		{3450, 3450, 3450, 3450, 3450},
		{3300, 3300, 3300, 3300, 3300},
		{3000, 3000, 3000, 3000, 3000}
	},
};

static struct double_row_lut rbatt_temp_soc = {
	.rows		= 13,
	.cols		= 5,
	.in_1		= {-10, 0, 10, 20, 40},
	.in_2		= {
		{INT_MAX, INT_MAX, INT_MAX, INT_MAX, INT_MAX},
		{INT_MAX, INT_MAX, INT_MAX, INT_MAX, 100},
		{INT_MAX, INT_MAX, INT_MAX, 100, 0},
		{INT_MAX, INT_MAX, 100, 96, INT_MIN},
		{INT_MAX, 100, 96, 70, INT_MIN},
		{INT_MAX, 96, 86, 30, INT_MIN},
		{100, 92, 60, 20, INT_MIN},
		{96, 88, 40, 0, INT_MIN},
		{92, 84, 30, INT_MIN, INT_MIN},
		{88, 60, 20, INT_MIN, INT_MIN},
		{84, 30, -60, INT_MIN, INT_MIN},
		{80, -30, INT_MIN, INT_MIN, INT_MIN},
		{0, INT_MIN, INT_MIN, INT_MIN, INT_MIN},
	},
	.out		= {150, 160, 170, 180, 200, 240, 280,
		320, 360, 400, 500, 600, 700
	},
};

struct pm8921_bms_battery_data  pm8921_battery_data __devinitdata = {
	.fcc			= 1520,
	.fcc_temp_lut		= &fcc_temp,
	.fcc_sf_lut		= &fcc_sf,
	.pc_temp_ocv_lut	= &pc_temp_ocv,
	.pc_sf_lut		= &pc_sf,
	.rbatt_temp_soc_lut	= &rbatt_temp_soc,
};
