/* copyright (c) 2011, code aurora forum. all rights reserved.
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
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
	.y	= {1450, 1455, 1460, 1465, 1470},
	.cols	= 5,
};

static struct single_row_lut fcc_sf = {
	.x	= {100, 200, 300, 400, 500},
	.y	= {99, 98, 95, 93, 92},
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
		{4175, 4175, 4175, 4175, 4175},
		{4101, 4101, 4101, 4101, 4101},
		{4050, 4050, 4050, 4050, 4050},
		{3997, 3997, 3997, 3997, 3997},
		{3965, 3965, 3965, 3965, 3965},
		{3930, 3930, 3930, 3930, 3930},
		{3897, 3897, 3897, 3897, 3897},
		{3867, 3867, 3867, 3867, 3867},
		{3841, 3841, 3841, 3841, 3841},
		{3818, 3818, 3818, 3818, 3818},
		{3799, 3799, 3799, 3799, 3799},
		{3782, 3782, 3782, 3782, 3782},
		{3768, 3768, 3768, 3768, 3768},
		{3755, 3755, 3755, 3755, 3755},
		{3743, 3743, 3743, 3743, 3743},
		{3729, 3729, 3729, 3729, 3729},
		{3713, 3713, 3713, 3713, 3713},
		{3691, 3691, 3691, 3691, 3691},
		{3651, 3651, 3651, 3651, 3651},
		{3650, 3650, 3650, 3650, 3650},
		{3649, 3649, 3649, 3649, 3649},
		{3642, 3642, 3642, 3642, 3642},
		{3626, 3626, 3626, 3626, 3626},
		{3589, 3589, 3589, 3589, 3589},
		{3556, 3556, 3556, 3556, 3556},
		{3494, 3494, 3494, 3494, 3494},
		{3417, 3417, 3417, 3417, 3417},
		{3303, 3303, 3303, 3303, 3303},
		{3000, 3000, 3000, 3000, 3000}
	},
};

struct pm8921_bms_battery_data  pm8921_battery_data __devinitdata = {
	.fcc			= 1500,
	.fcc_temp_lut		= &fcc_temp,
	.fcc_sf_lut		= &fcc_sf,
	.pc_temp_ocv_lut	= &pc_temp_ocv,
	.pc_sf_lut		= &pc_sf,
};
