/* Copyright (c) 2012 Code Aurora Forum. All rights reserved.
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
#include <linux/i2c.h>
#include <linux/mfd/pmic8901.h>
#include <mach/board.h>
#include <mach/board-msm8660.h>
#include <mach/gpiomux.h>
#include <mach/msm_bus_board.h>
#ifdef CONFIG_MSM_CAMERA
#include <mach/camera.h>
#include <media/sony_sensor_v4l2.h>
#include <linux/v4l2-mediabus.h>
#endif
#include "devices-msm8x60.h"
#include "devices.h"

#define GPIO_EXT_CAMIF_PWR_EN1 (PM8901_MPP_BASE + PM8901_MPPS + 13)
#define GPIO_WEB_CAMIF_STANDBY1 (PM8901_MPP_BASE + PM8901_MPPS + 60)

#if !defined(CONFIG_SONY_CAM_MAIN_V4L2) && !defined(CONFIG_SONY_CAM_SUB_V4L2)

#ifdef CONFIG_MSM_CAMERA_FLASH
#define VFE_CAMIF_TIMER1_GPIO 29
#define VFE_CAMIF_TIMER2_GPIO 30
#define VFE_CAMIF_TIMER3_GPIO_INT 31
#define FUSION_VFE_CAMIF_TIMER1_GPIO 42


#endif
#endif
static struct msm_bus_vectors cam_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_preview_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
#if defined(CONFIG_SONY_CAM_MAIN_V4L2) || defined(CONFIG_SONY_CAM_SUB_V4L2)
		.ab  = 484614900,
		.ib  = 2100000000,
#else
		.ab  = 283115520,
		.ib  = 452984832,
#endif
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
#if defined(CONFIG_SONY_CAM_MAIN_V4L2) || defined(CONFIG_SONY_CAM_SUB_V4L2)
		.ab  = 149299200,
		.ib  = 2640000000U,
#else
		.ab  = 0,
		.ib  = 0,
#endif
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
#if defined(CONFIG_SONY_CAM_MAIN_V4L2) || defined(CONFIG_SONY_CAM_SUB_V4L2)
		.ab  = 223948800,
		.ib  = 335923200,
#else
		.ab  = 0,
		.ib  = 0,
#endif
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_video_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
#if defined(CONFIG_SONY_CAM_MAIN_V4L2) || defined(CONFIG_SONY_CAM_SUB_V4L2)
		.ab  = 0,
		.ib  = 0,
#else
		.ab  = 283115520,
		.ib  = 452984832,
#endif
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
#if defined(CONFIG_SONY_CAM_MAIN_V4L2) || defined(CONFIG_SONY_CAM_SUB_V4L2)
		.ab  = 656132400,
		.ib  = 2100000000,
#else
		.ab  = 283115520,
		.ib  = 452984832,
#endif
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
#if defined(CONFIG_SONY_CAM_MAIN_V4L2) || defined(CONFIG_SONY_CAM_SUB_V4L2)
		.ab  = 215654400,
		.ib  = 2640000000U,
#else
		.ab  = 319610880,
		.ib  = 511377408,
#endif
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
#if defined(CONFIG_SONY_CAM_MAIN_V4L2) || defined(CONFIG_SONY_CAM_SUB_V4L2)
		.ab  = 323481600,
		.ib  = 485222400,
#else
		.ab  = 0,
		.ib  = 0,
#endif
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_snapshot_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
#if defined(CONFIG_SONY_CAM_MAIN_V4L2) || defined(CONFIG_SONY_CAM_SUB_V4L2)
		.ab  = 0,
		.ib  = 0,
#else
		.ab  = 566231040,
		.ib  = 905969664,
#endif
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
#if defined(CONFIG_SONY_CAM_MAIN_V4L2) || defined(CONFIG_SONY_CAM_SUB_V4L2)
		.ab  = 660234240,
		.ib  = 1056374784,
#else
		.ab  = 69984000,
		.ib  = 111974400,
#endif
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
#if defined(CONFIG_SONY_CAM_MAIN_V4L2) || defined(CONFIG_SONY_CAM_SUB_V4L2)
		.ab  = 79902720,
		.ib  = 127844352,
#else
		.ab  = 0,
		.ib  = 0,
#endif
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
#if defined(CONFIG_SONY_CAM_MAIN_V4L2) || defined(CONFIG_SONY_CAM_SUB_V4L2)
		.ab  = 1042495488,
		.ib  = 1667992781,
#else
		.ab  = 0,
		.ib  = 0,
#endif
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 320864256,
		.ib  = 513382810,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 320864256,
		.ib  = 513382810,
	},
};

static struct msm_bus_vectors cam_zsl_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 566231040,
		.ib  = 905969664,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 706199040,
		.ib  = 1129918464,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 320864256,
		.ib  = 513382810,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 320864256,
		.ib  = 513382810,
	},
};

static struct msm_bus_vectors cam_stereo_video_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 212336640,
		.ib  = 339738624,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 25090560,
		.ib  = 40144896,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 239708160,
		.ib  = 383533056,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 79902720,
		.ib  = 127844352,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_stereo_snapshot_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 300902400,
		.ib  = 481443840,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 230307840,
		.ib  = 368492544,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 245113344,
		.ib  = 392181351,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 106536960,
		.ib  = 170459136,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 106536960,
		.ib  = 170459136,
	},
};

static struct msm_bus_paths cam_bus_client_config[] = {
	{
		ARRAY_SIZE(cam_init_vectors),
		cam_init_vectors,
	},
	{
		ARRAY_SIZE(cam_preview_vectors),
		cam_preview_vectors,
	},
	{
		ARRAY_SIZE(cam_video_vectors),
		cam_video_vectors,
	},
	{
		ARRAY_SIZE(cam_snapshot_vectors),
		cam_snapshot_vectors,
	},
	{
		ARRAY_SIZE(cam_zsl_vectors),
		cam_zsl_vectors,
	},
	{
		ARRAY_SIZE(cam_stereo_video_vectors),
		cam_stereo_video_vectors,
	},
	{
		ARRAY_SIZE(cam_stereo_snapshot_vectors),
		cam_stereo_snapshot_vectors,
	},
};

static struct msm_bus_scale_pdata cam_bus_client_pdata = {
		cam_bus_client_config,
		ARRAY_SIZE(cam_bus_client_config),
		.name = "msm_camera",
};

static struct msm_camera_device_platform_data msm_camera_csi_device_data[] = {
	{
		.ioclk.mclk_clk_rate    = 8000000,
		.ioclk.vfe_clk_rate     = 266667000,
		.csid_core = 0,
#if defined(CONFIG_SEMC_VPE)
		.is_vpe			= 0,
#else
		.is_vpe    = 1,
#endif
		.cam_bus_scale_table = &cam_bus_client_pdata,
	},
	{
		.ioclk.mclk_clk_rate    = 8000000,
		.ioclk.vfe_clk_rate     = 228570000,
		.csid_core = 1,
#if defined(CONFIG_SEMC_VPE)
		.is_vpe			= 0,
#else
		.is_vpe    = 1,
#endif
		.cam_bus_scale_table = &cam_bus_client_pdata,
	},
};

#define VREG_L1			"8058_l1"
#define VREG_LVS0		"8058_lvs0"
#define VREG_L15		"8058_l15"
#define VREG_L9			"8058_l9"

static struct camera_vreg_t msm_8x60_back_cam_vreg[] = {
	{ VREG_L15, REG_LDO, 2600000, 3000000, 85600 },
	{ VREG_LVS0, REG_VS, 0, 0, 0 },
	{ VREG_L1, REG_LDO, 1200000, 1200000, 105000 },
	{ VREG_L9, REG_LDO, 2600000, 3000000, 300000 },
};

static struct camera_vreg_t msm_8x60_front_cam_vreg[] = {
	{ VREG_L15, REG_LDO, 2600000, 3000000, 85600 },
	{ VREG_LVS0, REG_VS, 0, 0, 0 },
};

static struct gpio msm8x60_common_cam_gpio[] = {
	{32, GPIOF_DIR_IN, "CAMIF_MCLK"},
	{47, GPIOF_DIR_IN, "CAMIF_I2C_DATA"},
	{48, GPIOF_DIR_IN, "CAMIF_I2C_CLK"},
	{105, GPIOF_DIR_IN, "STANDBY"},

};

static struct gpio msm8x60_front_cam_gpio[] = {
	{25, GPIOF_DIR_OUT, "CAM_RESET"},
};

static struct gpio msm8x60_back_cam_gpio[] = {
	{106, GPIOF_DIR_OUT, "CAM_RESET"},
};

static struct msm_gpio_set_tbl msm8x60_front_cam_gpio_set_tbl[] = {
	{25, GPIOF_OUT_INIT_LOW, 1000},
	{25, GPIOF_OUT_INIT_HIGH, 4000},

};
static struct msm_gpio_set_tbl msm8x60_back_cam_gpio_set_tbl[] = {
#if 0
	{GPIO_EXT_CAMIF_PWR_EN1, GPIOF_OUT_INIT_LOW, 10000},
	{GPIO_EXT_CAMIF_PWR_EN1, GPIOF_OUT_INIT_HIGH, 5000},
#endif
	{106, GPIOF_OUT_INIT_LOW, 1000},
	{106, GPIOF_OUT_INIT_HIGH, 4000},
};

static struct msm_camera_gpio_conf msm_8x60_front_cam_gpio_conf = {
	.cam_gpio_common_tbl = msm8x60_common_cam_gpio,
	.cam_gpio_common_tbl_size = ARRAY_SIZE(msm8x60_common_cam_gpio),
	.cam_gpio_req_tbl = msm8x60_front_cam_gpio,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(msm8x60_front_cam_gpio),
	.cam_gpio_set_tbl = msm8x60_front_cam_gpio_set_tbl,
	.cam_gpio_set_tbl_size = ARRAY_SIZE(msm8x60_front_cam_gpio_set_tbl),
};

static struct msm_camera_gpio_conf msm_8x60_back_cam_gpio_conf = {
	.cam_gpio_common_tbl = msm8x60_common_cam_gpio,
	.cam_gpio_common_tbl_size = ARRAY_SIZE(msm8x60_common_cam_gpio),
	.cam_gpio_req_tbl = msm8x60_back_cam_gpio,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(msm8x60_back_cam_gpio),
	.cam_gpio_set_tbl = msm8x60_back_cam_gpio_set_tbl,
	.cam_gpio_set_tbl_size = ARRAY_SIZE(msm8x60_back_cam_gpio_set_tbl),
};

#if defined(CONFIG_SONY_CAM_MAIN_V4L2) || defined(CONFIG_SONY_CAM_SUB_V4L2)
static struct msm_camera_sensor_flash_data msm_camera_flash_none = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
	.flash_src	= NULL,
};

static struct msm_camera_csi_lane_params msm_camera_csi_lane_params = {
	.csi_lane_assign	= 0xE4,
	.csi_lane_mask		= 0xF,
};
#endif

#ifdef CONFIG_SONY_CAM_MAIN_V4L2
static struct msm_camera_sensor_platform_info
		msm_camera_sony_sensor_main_platform_info = {
	.mount_angle		= 90,
	.sensor_reset		= 106,
	.cam_vreg		= msm_8x60_back_cam_vreg,
	.num_vreg		= ARRAY_SIZE(msm_8x60_back_cam_vreg),
	.gpio_conf		= &msm_8x60_back_cam_gpio_conf,
	.csi_lane_params	= &msm_camera_csi_lane_params,
};

static struct msm_camera_sensor_info
		msm_camera_sony_sensor_main_sensor_info = {
	.sensor_name		= "sony_sensor_main",
	.sensor_reset		= 106,
	.sensor_pwd			= 106,
	.vcm_pwd			= 106,
	.vcm_enable			= 106,
	.mclk				= 32,
	.pdata			= &msm_camera_csi_device_data[0],
	.flash_data		= &msm_camera_flash_none,
	.sensor_platform_info	= &msm_camera_sony_sensor_main_platform_info,
	.csi_if			= 1,
	.camera_type		= BACK_CAMERA_2D,
	.csi_params = {
			.data_format    = CSI_10BIT,
			.lane_cnt       = 4,
			.lane_assign    = 0xe4,
			.settle_cnt     = 25,
			.dpcm_scheme    = 0,
	},
};
#else
#endif

#ifdef CONFIG_SONY_CAM_SUB_V4L2
static struct msm_camera_sensor_platform_info
		msm_camera_sony_sensor_sub_platform_info = {
	.mount_angle		= 90,
	.sensor_reset		= 25,
	.cam_vreg		= msm_8x60_front_cam_vreg,
	.num_vreg		= ARRAY_SIZE(msm_8x60_front_cam_vreg),
	.gpio_conf		= &msm_8x60_front_cam_gpio_conf,
	.csi_lane_params	= &msm_camera_csi_lane_params,
};

static struct msm_camera_sensor_info
		msm_camera_sony_sensor_sub_sensor_info = {
	.sensor_name		= "sony_sensor_sub",
	.sensor_reset		= 25,
	.sensor_pwd			= 106,
	.vcm_pwd			= 106,
	.vcm_enable			= 106,
	.pdata			= &msm_camera_csi_device_data[1],
	.flash_data		= &msm_camera_flash_none,
	.sensor_platform_info	= &msm_camera_sony_sensor_sub_platform_info,
	.csi_if			= 1,
	.camera_type		= FRONT_CAMERA_2D,
	.csi_params = {
			.data_format    = CSI_10BIT,
			.lane_cnt       = 1,
			.lane_assign    = 0xe4,
			.settle_cnt     = 34,
			.dpcm_scheme    = 0,
	},
};
#else
#endif

static struct platform_device msm_camera_server = {
	.name = "msm_cam_server",
	.id = 0,
};


#if defined(CONFIG_SEMC_VPE)
static struct resource semc_vpe_resources[] = {
	{
		.name = "semc_vpe.0",
		.start  = 0x05300000,
		.end    = 0x05300000 + SZ_1M - 1,
		.flags  = IORESOURCE_MEM,
	},
	{
		.name = "semc_vpe.0",
		.start  = INT_VPE,
		.end    = INT_VPE,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device semc_vpe_device = {
	.name           = "semc_vpe",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(semc_vpe_resources),
	.resource       = semc_vpe_resources,
};
#endif


void __init msm8x60_init_cam(void)
{
	platform_device_register(&msm_camera_server);
	platform_device_register(&msm_device_csic0);
	platform_device_register(&msm_device_csic1);
	platform_device_register(&msm_device_vfe);
#if defined(CONFIG_SEMC_VPE)
	platform_device_register(&semc_vpe_device);
#else
	platform_device_register(&msm_device_vpe);
#endif
}

#ifdef CONFIG_I2C
static struct i2c_board_info msm8x60_camera_i2c_boardinfo[] = {
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

struct msm_camera_board_info msm8x60_camera_board_info = {
	.board_info = msm8x60_camera_i2c_boardinfo,
	.num_i2c_board_info = ARRAY_SIZE(msm8x60_camera_i2c_boardinfo),
};
#endif

#if defined(CONFIG_SONY_CAM_MAIN_V4L2) || defined(CONFIG_SONY_CAM_SUB_V4L2)
static const struct sony_sensor_seq sensor_main_power_off[] = {
	{ I2C_WRITE, 0x0100, 100 },
	{ CAM_CLK, -1, 1 },
	{ GPIO_RESET, 0, 1 },
	{ CAM_VAF, -1, 0 },
	{ CAM_VANA, -1, 1 },
	{ CAM_VIO, -1, 1 },
	{ CAM_VDIG, -1, 15 },
	{ EXIT, 0, 0 },
};

static const struct sony_sensor_seq sensor_main_power_on[] = {
	{ CAM_VDIG, 1200, 1 },
	{ CAM_VIO, 0, 1 },
	{ CAM_VANA, 2800, 0 },
	{ CAM_VAF, 2750, 1 },
	{ GPIO_RESET, 1, 9 },
	{ CAM_CLK, 0, 1 },
	{ EXIT, 0, 0 },
};

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
	{ CAM_CLK, 0, 200 },
	{ GPIO_RESET, 1, 200 },
	{ EXIT, 0, 0 },
};

static const struct sony_sensor_module sensor_main_modules[] = {
	{ "GENERIC",  sensor_main_power_on, sensor_main_power_off },
	{ "SOI13BS0", sensor_main_power_on, sensor_main_power_off },
	{ "KMO13BS0", sensor_main_power_on, sensor_main_power_off },
};

static const struct sony_sensor_module sensor_sub_modules[] = {
	{ "GENERIC",  sensor_sub_power_on, sensor_sub_power_off },
	{ "APT00YP1", sensor_sub_power_on, sensor_sub_power_off },
	{ "APT01BM0", sensor_sub_power_on, sensor_sub_power_off },
	{ "STW00YP1", sensor_sub_power_on, sensor_sub_power_off },
	{ "STW01BM0", sensor_sub_power_on, sensor_sub_power_off },
};

const struct sony_sensor_info sensor_info[] = {
	{
		.i2c_addr	= 0x1A,
		.eeprom_addr	= 0x50,
		.eeprom_type	= 0,
		.gpio_af	= 0,
		.subdev_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.modules	= sensor_main_modules,
		.modules_num	= ARRAY_SIZE(sensor_main_modules),
	},
	{
		.i2c_addr	= 0x48,
		.eeprom_addr	= 0x48,
		.eeprom_type	= 1,
		.gpio_af	= 0,
		.subdev_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.modules	= sensor_sub_modules,
		.modules_num	= ARRAY_SIZE(sensor_sub_modules),
	},
};
#endif
