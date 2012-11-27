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
	.y	= {1753, 1761, 1770, 1763, 1767},
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
		{4088, 4095, 4095, 4192, 4101},
		{4041, 4051, 4051, 4047, 4055},
		{3998, 4012, 4012, 4009, 4015},
		{3958, 3977, 3978, 3974, 3979},
		{3921, 3945, 3946, 3944, 3947},
		{3873, 3915, 3918, 3916, 3918},
		{3856, 3886, 3891, 3890, 3892},
		{3829, 3853, 3859, 3858, 3863},
		{3805, 3826, 3827, 3825, 3826},
		{3786, 3807, 3808, 3806, 3806},
		{3769, 3793, 3794, 3792, 3791},
		{3754, 3783, 3784, 3782, 3779},
		{3743, 3775, 3776, 3774, 3769},
		{3734, 3769, 3765, 3758, 3747},
		{3724, 3755, 3744, 3734, 3719},
		{3709, 3737, 3727, 3717, 3700},
		{3685, 3710, 3701, 3690, 3674},
		{3649, 3676, 3672, 3665, 3650},
		{3641, 3671, 3668, 3661, 3646},
		{3629, 3665, 3663, 3656, 3641},
		{3618, 3658, 3655, 3647, 3631},
		{3602, 3643, 3635, 3628, 3607},
		{3580, 3611, 3601, 3592, 3571},
		{3546, 3565, 3554, 3546, 3524},
		{3495, 3505, 3494, 3488, 3465},
		{3416, 3427, 3414, 3410, 3385},
		{3284, 3308, 3296, 3292, 3265},
		{3000, 3000, 3000, 3000, 3000}
	},
};

struct pm8921_bms_battery_data  pm8921_battery_data __devinitdata = {
	.fcc			= 1750,
	.fcc_temp_lut		= &fcc_temp,
	.fcc_sf_lut		= &fcc_sf,
	.pc_temp_ocv_lut	= &pc_temp_ocv,
	.pc_sf_lut		= &pc_sf,
};
