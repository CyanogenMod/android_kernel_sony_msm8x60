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
	.y	= {1850, 1850, 1850, 1850, 1850},
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
		{4120, 4120, 4120, 4120, 4120},
		{4071, 4085, 4085, 4079, 4094},
		{4023, 4040, 4040, 4035, 4048},
		{3981, 4001, 4002, 3997, 4008},
		{3942, 3966, 3968, 3964, 3973},
		{3906, 3935, 3937, 3934, 3941},
		{3873, 3906, 3910, 3908, 3913},
		{3843, 3877, 3884, 3883, 3887},
		{3818, 3847, 3854, 3854, 3860},
		{3795, 3820, 3823, 3820, 3823},
		{3776, 3802, 3804, 3802, 3804},
		{3758, 3789, 3791, 3789, 3789},
		{3743, 3780, 3781, 3780, 3778},
		{3732, 3773, 3775, 3772, 3769},
		{3723, 3767, 3765, 3758, 3748},
		{3714, 3754, 3745, 3735, 3720},
		{3701, 3737, 3727, 3717, 3702},
		{3681, 3709, 3700, 3691, 3676},
		{3651, 3673, 3671, 3664, 3651},
		{3643, 3668, 3668, 3661, 3648},
		{3633, 3662, 3663, 3656, 3643},
		{3621, 3653, 3654, 3648, 3632},
		{3606, 3637, 3635, 3627, 3607},
		{3587, 3604, 3597, 3590, 3569},
		{3561, 3557, 3550, 3543, 3522},
		{3522, 3497, 3490, 3485, 3461},
		{3457, 3419, 3412, 3407, 3381},
		{3329, 3301, 3295, 3293, 3261},
		{3000, 3000, 3000, 3000, 3000}
	},
};

struct pm8921_bms_battery_data  pm8921_battery_data __devinitdata = {
	.fcc			= 1840,
	.fcc_temp_lut		= &fcc_temp,
	.fcc_sf_lut		= &fcc_sf,
	.pc_temp_ocv_lut	= &pc_temp_ocv,
	.pc_sf_lut		= &pc_sf,
};
