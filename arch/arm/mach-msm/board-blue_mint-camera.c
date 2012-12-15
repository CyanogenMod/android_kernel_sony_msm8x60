/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
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
 *
 */

#include <asm/mach-types.h>
#include <linux/gpio.h>
#include <mach/board.h>
#include <mach/msm_bus_board.h>
#include <mach/gpiomux.h>
#ifdef CONFIG_MSM_CAMERA
#include <mach/camera.h>
#include <media/sony_sensor_v4l2.h>
#include <linux/v4l2-mediabus.h>
#endif

#include "devices.h"
#include "board-8960.h"

#ifdef CONFIG_MSM_CAMERA
#if defined(CONFIG_SONY_VPE)
struct resource sony_vpe_resources[] = {
	{
		.start	= 0x05300000,
		.end	= 0x05300000 + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= VPE_IRQ,
		.end	= VPE_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device sony_vpe_device = {
	.name		= "sony_vpe",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(sony_vpe_resources),
	.resource	= sony_vpe_resources,
};
#endif

#if defined(CONFIG_SONY_CAM_MAIN_V4L2) || defined(CONFIG_SONY_CAM_SUB_V4L2)
static struct msm_gpiomux_config msm_camera_2d_configs[] = {
	{
		.gpio		= 1,
		.settings	= {
			[GPIOMUX_ACTIVE]	= &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED]	= &gpio_2ma_no_pull_low,
		},
	},
	{
		.gpio		= 4,
		.settings	= {
			[GPIOMUX_ACTIVE]	= &cam_mclk1,
			[GPIOMUX_SUSPENDED]	= &gpio_2ma_no_pull_low,
		},
	},
	{
		.gpio		= 5,
		.settings	= {
			[GPIOMUX_ACTIVE]	= &cam_mclk0,
			[GPIOMUX_SUSPENDED]	= &gpio_2ma_no_pull_low,
		},
	},
	{
		.gpio		= 18,
		.settings	= {
			[GPIOMUX_ACTIVE]	= &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED]	= &gpio_2ma_no_pull_low,
		},
	},
	{
		.gpio		= 20,
		.settings	= {
			[GPIOMUX_ACTIVE]	= &gsbi4,
			[GPIOMUX_SUSPENDED]	= &gpio_2ma_pull_down_in,
		},
	},
	{
		.gpio		= 21,
		.settings	= {
			[GPIOMUX_ACTIVE]	= &gsbi4,
			[GPIOMUX_SUSPENDED]	= &gpio_2ma_pull_down_in,
		},
	},
	{
		.gpio		= 89,
		.settings	= {
			[GPIOMUX_ACTIVE]	= &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED]	= &gpio_2ma_no_pull_low,
		},
	},
};

static struct msm_bus_vectors msm_camera_init_vectors[] = {
	{
		.src	= MSM_BUS_MASTER_VFE,
		.dst	= MSM_BUS_SLAVE_EBI_CH0,
		.ab	= 0,
		.ib	= 0,
	},
	{
		.src	= MSM_BUS_MASTER_VPE,
		.dst	= MSM_BUS_SLAVE_EBI_CH0,
		.ab	= 0,
		.ib	= 0,
	},
	{
		.src	= MSM_BUS_MASTER_JPEG_ENC,
		.dst	= MSM_BUS_SLAVE_EBI_CH0,
		.ab	= 0,
		.ib	= 0,
	},
	{
		.src	= MSM_BUS_MASTER_JPEG_ENC,
		.dst	= MSM_BUS_SLAVE_MM_IMEM,
		.ab	= 0,
		.ib	= 0,
	},
	{
		.src	= MSM_BUS_MASTER_VFE,
		.dst	= MSM_BUS_SLAVE_MM_IMEM,
		.ab	= 0,
		.ib	= 0,
	},
};

static struct msm_bus_vectors msm_camera_preview_vectors[] = {
	{
		.src	= MSM_BUS_MASTER_VFE,
		.dst	= MSM_BUS_SLAVE_EBI_CH0,
		.ab	= 139968000,
		.ib	= 559872000,
	},
	{
		.src	= MSM_BUS_MASTER_VPE,
		.dst	= MSM_BUS_SLAVE_EBI_CH0,
		.ab	= 279936000,
		.ib	= 699840000,
	},
	{
		.src	= MSM_BUS_MASTER_JPEG_ENC,
		.dst	= MSM_BUS_SLAVE_EBI_CH0,
		.ab	= 0,
		.ib	= 0,
	},
	{
		.src	= MSM_BUS_MASTER_JPEG_ENC,
		.dst	= MSM_BUS_SLAVE_MM_IMEM,
		.ab	= 0,
		.ib	= 0,
	},
	{
		.src	= MSM_BUS_MASTER_VFE,
		.dst	= MSM_BUS_SLAVE_MM_IMEM,
		.ab	= 0,
		.ib	= 0,
	},
};

static struct msm_bus_vectors msm_camera_video_vectors[] = {
	{
		.src	= MSM_BUS_MASTER_VFE,
		.dst	= MSM_BUS_SLAVE_EBI_CH0,
		.ab	= 121305600,
		.ib	= 485222400,
	},
	{
		.src	= MSM_BUS_MASTER_VPE,
		.dst	= MSM_BUS_SLAVE_EBI_CH0,
		.ab	= 214617600,
		.ib	= 494553600,
	},
	{
		.src	= MSM_BUS_MASTER_JPEG_ENC,
		.dst	= MSM_BUS_SLAVE_EBI_CH0,
		.ab	= 0,
		.ib	= 0,
	},
	{
		.src	= MSM_BUS_MASTER_JPEG_ENC,
		.dst	= MSM_BUS_SLAVE_MM_IMEM,
		.ab	= 0,
		.ib	= 0,
	},
	{
		.src	= MSM_BUS_MASTER_VFE,
		.dst	= MSM_BUS_SLAVE_MM_IMEM,
		.ab	= 0,
		.ib	= 0,
	},
};

static struct msm_bus_vectors msm_camera_snapshot_vectors[] = {
	{
		.src	= MSM_BUS_MASTER_VFE,
		.dst	= MSM_BUS_SLAVE_EBI_CH0,
		.ab	= 295401600,
		.ib	= 1181606400,
	},
	{
		.src	= MSM_BUS_MASTER_VPE,
		.dst	= MSM_BUS_SLAVE_EBI_CH0,
		.ab	= 590803200,
		.ib	= 1477008000,
	},
	{
		.src	= MSM_BUS_MASTER_JPEG_ENC,
		.dst	= MSM_BUS_SLAVE_EBI_CH0,
		.ab	= 590803200,
		.ib	= 590803200,
	},
	{
		.src	= MSM_BUS_MASTER_JPEG_ENC,
		.dst	= MSM_BUS_SLAVE_MM_IMEM,
		.ab	= 43200000,
		.ib	= 69120000,
	},
	{
		.src	= MSM_BUS_MASTER_VFE,
		.dst	= MSM_BUS_SLAVE_MM_IMEM,
		.ab	= 43200000,
		.ib	= 69120000,
	},
};

static struct msm_bus_vectors msm_camera_zsl_vectors[] = {
	{
		.src	= MSM_BUS_MASTER_VFE,
		.dst	= MSM_BUS_SLAVE_EBI_CH0,
		.ab	= 302071680,
		.ib	= 1208286720,
	},
	{
		.src	= MSM_BUS_MASTER_VPE,
		.dst	= MSM_BUS_SLAVE_EBI_CH0,
		.ab	= 0,
		.ib	= 0,
	},
	{
		.src	= MSM_BUS_MASTER_JPEG_ENC,
		.dst	= MSM_BUS_SLAVE_EBI_CH0,
		.ab	= 1521190000,
		.ib	= 1521190000,
	},
	{
		.src	= MSM_BUS_MASTER_JPEG_ENC,
		.dst	= MSM_BUS_SLAVE_MM_IMEM,
		.ab	= 43200000,
		.ib	= 69120000,
	},
	{
		.src	= MSM_BUS_MASTER_VFE,
		.dst	= MSM_BUS_SLAVE_MM_IMEM,
		.ab	= 43200000,
		.ib	= 69120000,
	},
};

static struct msm_bus_paths msm_camera_bus_client_config[] = {
	{
		ARRAY_SIZE(msm_camera_init_vectors),
		msm_camera_init_vectors,
	},
	{
		ARRAY_SIZE(msm_camera_preview_vectors),
		msm_camera_preview_vectors,
	},
	{
		ARRAY_SIZE(msm_camera_video_vectors),
		msm_camera_video_vectors,
	},
	{
		ARRAY_SIZE(msm_camera_snapshot_vectors),
		msm_camera_snapshot_vectors,
	},
	{
		ARRAY_SIZE(msm_camera_zsl_vectors),
		msm_camera_zsl_vectors,
	},
};

static struct msm_bus_scale_pdata msm_camera_bus_client_pdata = {
	msm_camera_bus_client_config,
	ARRAY_SIZE(msm_camera_bus_client_config),
	.name = "msm_camera",
};

static struct msm_camera_device_platform_data msm_camera_csi_device_data[] = {
	{
		.csid_core		= 0,
		.is_csiphy		= 1,
		.is_csid		= 1,
		.is_ispif		= 1,
		.is_vpe			= 0,
		.cam_bus_scale_table	= &msm_camera_bus_client_pdata,
	},
	{
		.csid_core		= 1,
		.is_csiphy		= 1,
		.is_csid		= 1,
		.is_ispif		= 1,
		.is_vpe			= 0,
		.cam_bus_scale_table	= &msm_camera_bus_client_pdata,
	},
        {
               .ioclk.mclk_clk_rate = 24000000,
               .ioclk.vfe_clk_rate  = 228570000,
               .csid_core = 2,
               .cam_bus_scale_table = &msm_camera_bus_client_pdata,
        },
};

static struct camera_vreg_t msm_camera_back_cam_vreg[] = {
	{ "cam_vana", REG_LDO, 2600000, 3000000, 85600 },
	{ "cam_vio", REG_VS, 0, 0, 0 },
	{ "cam_vdig", REG_LDO, 1200000, 1200000, 105000 },
};

static struct camera_vreg_t msm_camera_front_cam_vreg[] = {
	{ "cam_vana", REG_LDO, 2600000, 3000000, 85600 },
	{ "cam_vio", REG_VS, 0, 0, 0 },
};

static struct gpio msm_camera_common_cam_gpio[] = {
	{ 4, GPIOF_DIR_IN, "CAMIF_MCLK" },
	{ 5, GPIOF_DIR_IN, "CAMIF_MCLK" },
	{ 20, GPIOF_DIR_IN, "CAMIF_I2C_DATA" },
	{ 21, GPIOF_DIR_IN, "CAMIF_I2C_CLK" },
	{ 89, GPIOF_DIR_OUT, "CAM_AF" },
};

static struct gpio msm_camera_front_cam_gpio[] = {
	{ 18, GPIOF_DIR_OUT, "CAM_RESET" },
};

static struct gpio msm_camera_back_cam_gpio[] = {
	{ 1, GPIOF_DIR_OUT, "CAM_RESET" },
};

static struct msm_gpio_set_tbl msm_camera_front_cam_gpio_set_tbl[] = {
	{ 18, GPIOF_OUT_INIT_LOW, 1 },
	{ 18, GPIOF_OUT_INIT_HIGH, 1000 },
};

static struct msm_gpio_set_tbl msm_camera_back_cam_gpio_set_tbl[] = {
	{ 1, GPIOF_OUT_INIT_LOW, 1 },
	{ 1, GPIOF_OUT_INIT_HIGH, 1000 },
};

static struct msm_camera_gpio_conf msm_camera_front_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl		= msm_camera_2d_configs,
	.cam_gpiomux_conf_tbl_size	= ARRAY_SIZE(msm_camera_2d_configs),
	.cam_gpio_common_tbl		= msm_camera_common_cam_gpio,
	.cam_gpio_common_tbl_size
		= ARRAY_SIZE(msm_camera_common_cam_gpio),
	.cam_gpio_req_tbl		= msm_camera_front_cam_gpio,
	.cam_gpio_req_tbl_size		= ARRAY_SIZE(msm_camera_front_cam_gpio),
	.cam_gpio_set_tbl		= msm_camera_front_cam_gpio_set_tbl,
	.cam_gpio_set_tbl_size
		= ARRAY_SIZE(msm_camera_front_cam_gpio_set_tbl),
};

static struct msm_camera_gpio_conf msm_camera_back_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl		= msm_camera_2d_configs,
	.cam_gpiomux_conf_tbl_size	= ARRAY_SIZE(msm_camera_2d_configs),
	.cam_gpio_common_tbl		= msm_camera_common_cam_gpio,
	.cam_gpio_common_tbl_size
		= ARRAY_SIZE(msm_camera_common_cam_gpio),
	.cam_gpio_req_tbl		= msm_camera_back_cam_gpio,
	.cam_gpio_req_tbl_size		= ARRAY_SIZE(msm_camera_back_cam_gpio),
	.cam_gpio_set_tbl		= msm_camera_back_cam_gpio_set_tbl,
	.cam_gpio_set_tbl_size
		= ARRAY_SIZE(msm_camera_back_cam_gpio_set_tbl),
};

static struct msm_camera_sensor_flash_data msm_camera_flash_none = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
	.flash_src	= NULL,
};
#endif

#ifdef CONFIG_SONY_CAM_MAIN_V4L2
static struct msm_camera_sensor_platform_info
		msm_camera_sony_sensor_main_platform_info = {
	.mount_angle		= 90,
	.sensor_reset		= 1,
	.cam_vreg		= msm_camera_back_cam_vreg,
	.num_vreg		= ARRAY_SIZE(msm_camera_back_cam_vreg),
	.gpio_conf		= &msm_camera_back_cam_gpio_conf,
};

static struct msm_camera_sensor_info
		msm_camera_sony_sensor_main_sensor_info = {
	.sensor_name		= "sony_sensor_main",
	.sensor_reset		= 1,
	.pdata			= &msm_camera_csi_device_data[0],
	.flash_data		= &msm_camera_flash_none,
	.sensor_platform_info	= &msm_camera_sony_sensor_main_platform_info,
	.csi_if			= 1,
	.camera_type		= BACK_CAMERA_2D,
};
#endif

#ifdef CONFIG_SONY_CAM_SUB_V4L2
static struct msm_camera_sensor_platform_info
		msm_camera_sony_sensor_sub_platform_info = {
	.mount_angle		= 90,
	.sensor_reset		= 18,
	.cam_vreg		= msm_camera_front_cam_vreg,
	.num_vreg		= ARRAY_SIZE(msm_camera_front_cam_vreg),
	.gpio_conf		= &msm_camera_front_cam_gpio_conf,
};

static struct msm_camera_sensor_info
		msm_camera_sony_sensor_sub_sensor_info = {
	.sensor_name		= "sony_sensor_sub",
	.sensor_reset		= 18,
	.pdata			= &msm_camera_csi_device_data[1],
	.flash_data		= &msm_camera_flash_none,
	.sensor_platform_info	= &msm_camera_sony_sensor_sub_platform_info,
	.csi_if			= 1,
	.camera_type		= FRONT_CAMERA_2D,
};
#endif

void __init msm8960_init_cam(void)
{
#if defined(CONFIG_SONY_CAM_MAIN_V4L2) || defined(CONFIG_SONY_CAM_SUB_V4L2)
	msm_gpiomux_install(msm_camera_2d_configs,
			ARRAY_SIZE(msm_camera_2d_configs));

	platform_device_register(&msm8960_device_csiphy0);
	platform_device_register(&msm8960_device_csiphy1);
	platform_device_register(&msm8960_device_csiphy2);
	platform_device_register(&msm8960_device_csid0);
	platform_device_register(&msm8960_device_csid1);
	platform_device_register(&msm8960_device_csid2);
	platform_device_register(&msm8960_device_ispif);
	platform_device_register(&msm8960_device_vfe);
#if defined(CONFIG_SONY_VPE)
	platform_device_register(&sony_vpe_device);
#else
	platform_device_register(&msm8960_device_vpe);
#endif
#endif
}

#if defined(CONFIG_SONY_CAM_MAIN_V4L2) || defined(CONFIG_SONY_CAM_SUB_V4L2)
static struct i2c_board_info msm8960_camera_i2c_boardinfo[] = {
#ifdef CONFIG_SONY_CAM_MAIN_V4L2
	{
		I2C_BOARD_INFO("sony_sensor_main", 0x1A),
		.platform_data = &msm_camera_sony_sensor_main_sensor_info,
	},
#endif
#ifdef CONFIG_SONY_CAM_SUB_V4L2
	{
		I2C_BOARD_INFO("sony_sensor_sub", 0x48),
		.platform_data = &msm_camera_sony_sensor_sub_sensor_info,
	},
#endif
};

struct msm_camera_board_info msm8960_camera_board_info = {
	.board_info = msm8960_camera_i2c_boardinfo,
	.num_i2c_board_info = ARRAY_SIZE(msm8960_camera_i2c_boardinfo),
};

static const struct sony_sensor_seq sensor_main_power_off[] = {
	{ CAM_CLK, -1, 1 },
	{ GPIO_RESET, 0, 1 },
	{ GPIO_AF, -1, 0 },
	{ CAM_VANA, -1, 1 },
	{ CAM_VIO, -1, 1 },
	{ CAM_VDIG, -1, 15 },
	{ EXIT, 0, 0 },
};

static const struct sony_sensor_seq sensor_main_power_on[] = {
	{ CAM_VDIG, 1200, 1 },
	{ CAM_VIO, 0, 1 },
	{ CAM_VANA, 2800, 0 },
	{ GPIO_AF, 1, 1 },
	{ GPIO_RESET, 1, 9 },
	{ CAM_CLK, 0, 1 },
	{ EXIT, 0, 0 },
};

const struct sony_sensor_module sensor_main_modules[] = {
	{ "GENERIC",  0x1A, sensor_main_power_on, sensor_main_power_off },
	{ "SOI13BS0", 0x1A, sensor_main_power_on, sensor_main_power_off },
	{ "SOI13BS1", 0x1A, sensor_main_power_on, sensor_main_power_off },
	{ "KMO13BS0", 0x1A, sensor_main_power_on, sensor_main_power_off },
};
const int sony_sensor_main_modules_len = ARRAY_SIZE(sensor_main_modules);

static const struct sony_sensor_seq sensor_sub_power_off[] = {
	{ GPIO_RESET, 0, 1 },
	{ CAM_CLK, -1, 1 },
	{ CAM_VANA, -1, 0 },
	{ CAM_VIO, -1, 0 },
	{ EXIT, 0, 0 },
};

static const struct sony_sensor_seq sensor_sub_power_on[] = {
	{ CAM_VIO, 0, 1 },
	{ CAM_VANA, 2800, 1 },
	{ CAM_CLK, 0, 1 },
	{ GPIO_RESET, 1, 200 },
	{ EXIT, 0, 0 },
};

const struct sony_sensor_module sensor_sub_modules[] = {
	{ "GENERIC",  0x48, sensor_sub_power_on, sensor_sub_power_off },
	{ "STW01BM0", 0x48, sensor_sub_power_on, sensor_sub_power_off },
	{ "APT01BM0", 0x48, sensor_sub_power_on, sensor_sub_power_off },
};


const struct sony_sensor_info sensor_info[] = {
	{
		.i2c_addr	= 0x1A,
		.eeprom_addr	= 0x50,
		.eeprom_type	= 0,
		.gpio_af	= 89,
		.subdev_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.modules	= sensor_main_modules,
		.modules_num	= ARRAY_SIZE(sensor_main_modules),
	},
	{
		.i2c_addr	= 0x48,
		.eeprom_addr	= 0x48,
		.eeprom_type	= 1,
		.gpio_af	= 0,
		.subdev_code	= V4L2_MBUS_FMT_YUYV8_2X8,
		.modules	= sensor_sub_modules,
		.modules_num	= ARRAY_SIZE(sensor_sub_modules),
	},
};

#endif
#endif
