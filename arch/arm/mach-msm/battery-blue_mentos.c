/* copyright (c) 2011, code aurora forum. all rights reserved.
 * Copyright (C) 2012 Sony Ericsson Mobile Communications AB.
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
	.y	= {1800, 1810, 1830, 1840, 1840},
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
		{4118, 4118, 4118, 4118, 4118},
		{4071, 4071, 4071, 4071, 4071},
		{4031, 4031, 4031, 4031, 4031},
		{3996, 3996, 3996, 3996, 3996},
		{3964, 3964, 3964, 3964, 3964},
		{3933, 3933, 3933, 3933, 3933},
		{3900, 3900, 3900, 3900, 3900},
		{3868, 3868, 3868, 3868, 3868},
		{3843, 3843, 3843, 3843, 3843},
		{3822, 3822, 3822, 3822, 3822},
		{3807, 3807, 3807, 3807, 3807},
		{3795, 3795, 3795, 3795, 3795},
		{3786, 3786, 3786, 3786, 3786},
		{3776, 3776, 3776, 3776, 3776},
		{3762, 3762, 3762, 3762, 3762},
		{3740, 3740, 3740, 3740, 3740},
		{3715, 3715, 3715, 3715, 3715},
		{3685, 3685, 3685, 3685, 3685},
		{3673, 3673, 3673, 3673, 3673},
		{3657, 3657, 3657, 3657, 3657},
		{3637, 3637, 3637, 3637, 3637},
		{3616, 3616, 3616, 3616, 3616},
		{3595, 3595, 3595, 3595, 3595},
		{3573, 3573, 3573, 3573, 3573},
		{3552, 3552, 3552, 3552, 3552},
		{3508, 3508, 3508, 3508, 3508},
		{3330, 3330, 3330, 3330, 3330},
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
		{INT_MAX, INT_MAX, 100, 5, INT_MIN},
		{INT_MAX, INT_MAX, 15, 0, INT_MIN},
		{INT_MAX, INT_MAX, 5, INT_MIN, INT_MIN},
		{INT_MAX, 100, 2, INT_MIN, INT_MIN},
		{INT_MAX, 50, 0, INT_MIN, INT_MIN},
		{INT_MAX, 4, INT_MIN, INT_MIN, INT_MIN},
		{100, 0, INT_MIN, INT_MIN, INT_MIN},
		{12, INT_MIN, INT_MIN, INT_MIN, INT_MIN},
		{0, INT_MIN, INT_MIN, INT_MIN, INT_MIN},
		{INT_MIN, INT_MIN, INT_MIN, INT_MIN, INT_MIN},
	},
	.out		= {90, 100, 120, 140, 160, 180, 200,
		240, 320, 360, 420, 500, 540
	},
};

struct pm8921_bms_battery_data  pm8921_battery_data __devinitdata = {
	.fcc			= 1850,
	.fcc_temp_lut		= &fcc_temp,
	.fcc_sf_lut		= &fcc_sf,
	.pc_temp_ocv_lut	= &pc_temp_ocv,
	.pc_sf_lut		= &pc_sf,
	.rbatt_temp_soc_lut	= &rbatt_temp_soc,
};
