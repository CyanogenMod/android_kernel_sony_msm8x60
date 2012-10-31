/* arch/arm/mach-msm/board-semc_blue_hayabusa.c
 *
 * Copyright (C) 2012 Sony Ericsson Mobile Communications AB.
 * Copyright (C) 2012 Sony Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <asm/mach-types.h>
#include <linux/clearpad.h>
#include <linux/gpio.h>
#include <linux/gpio_event.h>
#include <linux/gpio_keys.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/leds-as3676.h>
#include <linux/mpu.h>
#include <linux/platform_device.h>
#include <linux/input/pmic8xxx-keypad.h>
#include <linux/mfd/core.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/regulator/gpio-regulator.h>
#include <mach/board.h>
#include <mach/gpiomux.h>
#include <mach/irqs.h>
#include <mach/rpm-regulator.h>
#ifdef CONFIG_MSM_CAMERA
#include <mach/msm_bus_board.h>
#include <mach/camera.h>
#include <media/semc_sensor_v4l2.h>
#include <linux/v4l2-mediabus.h>
#include "devices.h"
#endif

#include "board-8960.h"
#include "charger-semc_blue.h"
#include "gyro-semc_common.h"
#include "pm8921-gpio-mpp-blue.h"
#include "board-semc_blue-vibrator.h"
#include "board-semc_blue-usb.h"

/* Section: Vibrator */
struct pm8xxx_vibrator_platform_data pm8xxx_vibrator_pdata = {
	.initial_vibrate_ms = 0,
	.max_timeout_ms = 15000,
	.level_mV = 3100,
};

/* Section: Touch */
struct synaptics_pointer_data pointer_data_0x19 = {
	.offset_x = 0,
	.offset_y = 49,
};

struct synaptics_pointer_data pointer_data_0x1A = {
	.offset_x = 0,
	.offset_y = 55,
};

struct synaptics_funcarea clearpad_funcarea_array[] = {
	{
		{ 0, 0, 719, 1279 }, { 0, 0, 719, 1299 },
		SYN_FUNCAREA_POINTER, NULL
	},
	{ .func = SYN_FUNCAREA_END }
};

struct synaptics_funcarea clearpad_funcarea_array_0x19[] = {
	{
		{ 0, 0, 719, 36 }, { 0, 0, 719, 36 },
		SYN_FUNCAREA_INSENSIBLE, NULL
	},
	{
		{ 0, 49, 719, 1328 }, { 0, 37, 719, 1332 },
		SYN_FUNCAREA_POINTER, &pointer_data_0x19
	},
	{ .func = SYN_FUNCAREA_END }
};

struct synaptics_funcarea clearpad_funcarea_array_0x1A[] = {
	{
		{ 0, 0, 719, 42 }, { 0, 0, 719, 42 },
		SYN_FUNCAREA_INSENSIBLE, NULL
	},
	{
		{ 0, 55, 719, 1334 }, { 0, 43, 719, 1336 },
		SYN_FUNCAREA_POINTER, &pointer_data_0x1A
	},
	{ .func = SYN_FUNCAREA_END }
};

struct synaptics_funcarea *clearpad_funcarea_get(u8 module_id, u8 rev)
{
	struct synaptics_funcarea *funcarea = NULL;

	pr_info("%s: module_id=0x%02x rev=0x%02x\n", __func__, module_id, rev);
	switch (module_id) {
	case 0x19:
		funcarea = clearpad_funcarea_array_0x19;
		break;
	case 0x1A:
	case 0xFF:
		funcarea = clearpad_funcarea_array_0x1A;
		break;
	default:
		funcarea = clearpad_funcarea_array;
		break;
	}

	return funcarea;
}

/* Section: Charging */
static int pm8921_therm_mitigation[] = {
	1525,
	825,
	475,
	325,
};

#define MAX_VOLTAGE_MV		4200

struct pm8921_charger_platform_data pm8921_chg_pdata __devinitdata = {
	.safety_time		= 512,
	.ttrkl_time		= 64,
	.update_time		= 30000,
	.max_voltage		= MAX_VOLTAGE_MV,
	.min_voltage		= 3200,
	.resume_voltage_delta	= 60,
	.resume_soc		= 99,
	.term_current		= 70,
	.cold_temp		= 5,
	.cool_temp		= 10,
	.warm_temp		= 45,
	.hot_temp		= 55,
	.hysterisis_temp	= 3,
	.temp_check_period	= 1,
	.safe_current		= 1525,
	.max_bat_chg_current	= 1525,
	.cool_bat_chg_current	= 1525,
	.warm_bat_chg_current	= 325,
	.cool_bat_voltage	= 4200,
	.warm_bat_voltage	= 4000,
	.thermal_mitigation	= pm8921_therm_mitigation,
	.thermal_levels		= ARRAY_SIZE(pm8921_therm_mitigation),
	.cold_thr		= PM_SMBC_BATT_TEMP_COLD_THR__HIGH,
	.hot_thr		= PM_SMBC_BATT_TEMP_HOT_THR__HIGH,
	.rconn_mohm		= 18,
};

struct pm8921_bms_platform_data pm8921_bms_pdata __devinitdata = {
	.battery_data		= &pm8921_battery_data,
	.r_sense		= 10,
	.i_test			= 1000,
	.v_failure		= 3200,
	.calib_delay_ms		= 600000,
	.max_voltage_uv         = MAX_VOLTAGE_MV * 1000,
	.default_rbatt_mohms	= 170,
	.rconn_mohm		= 30,
	.enable_fcc_learning	= 1,
};

/* Section: Gyro */
#ifdef CONFIG_SENSORS_MPU3050

#define GYRO_ORIENTATION {  0,  1,  0,  -1,  0,  0,  0,  0,  1 }
#define ACCEL_ORIENTATION { 1, 0,  0,  0,  1,  0,  0,  0,  1 }
#define COMPASS_ORIENTATION {  1, 0,  0,  0,  1,  0,  0,  0, 1 }
#define PRESSURE_ORIENTATION {  1,  0,  0,  0,  1,  0,  0,  0,  1 }

struct mpu3050_platform_data mpu_data = {
	.int_config  = BIT_INT_ANYRD_2CLEAR,
	.orientation = GYRO_ORIENTATION,
	.accel = {
		 .get_slave_descr = get_accel_slave_descr,
		 .adapt_num   = 12,
		 .bus         = EXT_SLAVE_BUS_SECONDARY,
		 .address     = (0x30 >> 1),
		 .orientation = ACCEL_ORIENTATION,
		 .bypass_state = mpu3050_bypassmode,
		 .check_sleep_status = check_bma250_sleep_state,
		 .vote_sleep_status = vote_bma250_sleep_state,
	 },
	.compass = {
		 .get_slave_descr = get_compass_slave_descr,
		 .adapt_num   = 12,
		 .bus         = EXT_SLAVE_BUS_PRIMARY,
		 .address     = (0x18 >> 1),
		 .orientation = COMPASS_ORIENTATION,
	 },
	.pressure = {
		 .get_slave_descr = NULL,
		 .adapt_num   = 0,
		 .bus         = EXT_SLAVE_BUS_INVALID,
		 .address     = 0,
		 .orientation = PRESSURE_ORIENTATION,
	 },
	.setup   = mpu3050_gpio_setup,
	.hw_config  = mpu3050_hw_config,
	.power_mode = mpu3050_power_mode,
};
#endif

/* Section: Keypad */
#define SINGLE_IRQ_RESOURCE(_name, _irq) \
{ \
	.name	= _name, \
	.start	= _irq, \
	.end	= _irq, \
	.flags	= IORESOURCE_IRQ, \
}

static const unsigned int keymap[] = {
	KEY(1, 0, KEY_CAMERA_FOCUS),
	KEY(1, 1, KEY_CAMERA_SNAPSHOT),
};

static struct matrix_keymap_data keymap_data = {
	.keymap_size    = ARRAY_SIZE(keymap),
	.keymap         = keymap,
};

static struct pm8xxx_keypad_platform_data keypad_data = {
	.input_name             = "keypad_8960",
	.input_phys_device      = "keypad_8960/input0",
	.num_rows               = 2,
	.num_cols               = 5,
	.rows_gpio_start	= PM8921_GPIO_PM_TO_SYS(9),
	.cols_gpio_start	= PM8921_GPIO_PM_TO_SYS(1),
	.debounce_ms            = 15,
	.scan_delay_ms          = 32,
	.row_hold_ns            = 91500,
	.wakeup                 = 1,
	.keymap_data            = &keymap_data,
};

struct pm8xxx_keypad_platform_data *get_keypad_data(void)
{
	return &keypad_data;
}

static struct gpio_keys_button gpio_keys_buttons[] = {
	{
		.code = KEY_VOLUMEDOWN,
		.gpio = PM8921_GPIO_PM_TO_SYS(20),
		.active_low = 1,
		.desc = "volume down",
		.type = EV_KEY,
		.wakeup = 1,
		.debounce_interval = 10,
	},
	{
		.code = KEY_VOLUMEUP,
		.gpio = PM8921_GPIO_PM_TO_SYS(21),
		.active_low = 1,
		.desc = "volume up",
		.type = EV_KEY,
		.wakeup = 1,
		.debounce_interval = 10,
	},
};

static struct gpio_keys_platform_data gpio_keys_pdata = {
	.buttons = gpio_keys_buttons,
	.nbuttons = 2,
};

static struct platform_device gpio_keys_device = {
	.name = "gpio-keys",
	.dev = { .platform_data = &gpio_keys_pdata },
};

static int __init input_devices_init(void)
{
	platform_device_register(&gpio_keys_device);
	return 0;
}

static void __exit input_devices_exit(void)
{
}

module_init(input_devices_init);
module_exit(input_devices_exit);

/* Section: Leds */
#if defined(CONFIG_LEDS_AS3676)

struct as3676_platform_data as3676_platform_data = {
	.step_up_vtuning = 20,	/* 0 .. 31 uA on DCDC_FB */
	.audio_speed_down = 1,	/* 0..3 resp. 0, 200, 400, 800ms */
	.audio_speed_up = 4,	/* 0..7 resp. 0, 50, 100, 150,
					200,250,400, 800ms */
	.audio_agc_ctrl = 1,	/* 0 .. 7: 0==no AGC, 7 very aggressive*/
	.audio_gain = 7,	/* 0..7: -12, -6,  0, 6
					12, 18, 24, 30 dB */
	.audio_source = 2,	/* 0..3: 0=curr33, 1=DCDC_FB
					2=GPIO1,  3=GPIO2 */
	.step_up_lowcur = true,
	.reset_on_i2c_shutdown = true,
	.caps_mounted_on_dcdc_feedback = 1,
	.cp_control = 0x10,
	.leds[0] = {
		.name = "lcd-backlight_1",
		.on_charge_pump = 0,
		.max_current_uA = 19950,
		.startup_current_uA = 19950,
	},
	.leds[1] = {
		.name = "lcd-backlight_2",
		.on_charge_pump = 0,
		.max_current_uA = 19950,
		.startup_current_uA = 19950,
	},
	.leds[2] = {
		.name = "led3-not-connected",
		.on_charge_pump = 0,
		.max_current_uA = 0,
	},
	.leds[3] = {
		.name = "logo-backlight_1",
		.on_charge_pump = 1,
		.max_current_uA = 3000,
	},
	.leds[4] = {
		.name = "logo-backlight_2",
		.on_charge_pump = 1,
		.max_current_uA = 3000,
	},
	.leds[5] = {
		.name = "led6-not-connected",
		.on_charge_pump = 1,
		.max_current_uA = 0,
	},
	.leds[6] = {
		.name = "pwr-red",
		.on_charge_pump = 1,
		.max_current_uA = 2000,
	},
	.leds[7] = {
		.name = "pwr-green",
		.on_charge_pump = 1,
		.max_current_uA = 3300,
	},
	.leds[8] = {
		.name = "pwr-blue",
		.on_charge_pump = 1,
		.max_current_uA = 2000,
	},
	.leds[9] = {
		.name = "led10-not-connected",
		.on_charge_pump = 1,
		.max_current_uA = 0,
	},
	.leds[10] = {
		.name = "led11-not-connected",
		.on_charge_pump = 1,
		.max_current_uA = 0,
	},
	.leds[11] = {
		.name = "led12-not-connected",
		.on_charge_pump = 1,
		.max_current_uA = 0,
	},
	.leds[12] = {
		.name = "led13-not-connected",
		.on_charge_pump = 1,
		.max_current_uA = 0,
	},
};

#define VBUS_BIT 0x04
static int __init startup_rgb(char *str)
{
	int vbus;
	if (get_option(&str, &vbus)) {
		if (vbus & VBUS_BIT)
			as3676_platform_data.leds[6].startup_current_uA = 2000;
		return 0;
	}
	return -EINVAL;
}

early_param("startup", startup_rgb);
#endif

/* Section: PMIC GPIO */
static struct pm8xxx_gpio_init pm8921_gpios[] __initdata = {

	/* KYPD_SNS(GPIO_1-5) and KYPD_DRV(GPIO_9,10) is set by PM8XXX keypad
	   driver. */

	/* SPKR_RIGHT_EN (GPIO_19) is set by Audio driver */

	/* UIM1_RST_CONN (GPIO_27),
	   UIM1_CLK_MSM  (GPIO_29) and
	   UIM1_CLK_CONN (GPIO_30) is set by UIM driver in AMSS */

	/* CLK_MSM_FWD (GPIO_39) is set by QCT code */

	/* NC */
	PM8XXX_GPIO_DISABLE(6),
	PM8XXX_GPIO_DISABLE(7),
	PM8XXX_GPIO_DISABLE(8),
	PM8XXX_GPIO_DISABLE(11),
	PM8XXX_GPIO_DISABLE(12),
	PM8XXX_GPIO_DISABLE(13),
	PM8XXX_GPIO_DISABLE(14),
	PM8XXX_GPIO_DISABLE(15),
	PM8XXX_GPIO_DISABLE(16),
	PM8XXX_GPIO_DISABLE(17),
	PM8XXX_GPIO_DISABLE(18),
	PM8XXX_GPIO_DISABLE(23),
	PM8XXX_GPIO_DISABLE(24),
	PM8XXX_GPIO_DISABLE(25),
	PM8XXX_GPIO_DISABLE(28),
	PM8XXX_GPIO_DISABLE(35),
	PM8XXX_GPIO_DISABLE(37),
	PM8XXX_GPIO_DISABLE(41),
	PM8XXX_GPIO_DISABLE(44),

	PM8XXX_GPIO_INPUT(20,	PM_GPIO_PULL_UP_30),	/* VOLUME_DOWN_KEY */
	PM8XXX_GPIO_INPUT(21,	PM_GPIO_PULL_UP_30),	/* VOLUME_UP_KEY */
	PM8XXX_GPIO_OUTPUT(22,	0),			/* RF_ID_EN */
	PM8XXX_GPIO_INPUT(26,	PM_GPIO_PULL_DN),	/* SD_CARD_DET_N */
#if defined(CONFIG_SEMC_FELICA_SUPPORT) && !defined(CONFIG_NFC_PN544)
	/* FELICA_LOCK */
	PM8XXX_GPIO_INIT(31, PM_GPIO_DIR_IN, PM_GPIO_OUT_BUF_CMOS, 0, \
				PM_GPIO_PULL_NO, PM_GPIO_VIN_L17, \
				PM_GPIO_STRENGTH_NO, \
				PM_GPIO_FUNC_NORMAL, 0, 0),
	/* FELICA_FF */
	PM8XXX_GPIO_INIT(32, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, 0, \
				PM_GPIO_PULL_NO, PM_GPIO_VIN_L17, \
				PM_GPIO_STRENGTH_LOW, \
				PM_GPIO_FUNC_NORMAL, 0, 0),
	/* FELICA_PON */
	PM8XXX_GPIO_INIT(33, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, 0, \
				PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
				PM_GPIO_STRENGTH_LOW, \
				PM_GPIO_FUNC_NORMAL, 0, 0),
#elif !defined(CONFIG_SEMC_FELICA_SUPPORT) && defined(CONFIG_NFC_PN544)
	PM8XXX_GPIO_DISABLE(31),
	PM8XXX_GPIO_DISABLE(32),
	/* NFC_EN */
	PM8XXX_GPIO_INIT(33, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_OPEN_DRAIN, 0, \
				PM_GPIO_PULL_NO, PM_GPIO_VIN_VPH, \
				PM_GPIO_STRENGTH_LOW, \
				PM_GPIO_FUNC_NORMAL, 0, 0),
#else
	PM8XXX_GPIO_DISABLE(31),
	PM8XXX_GPIO_DISABLE(32),
	PM8XXX_GPIO_DISABLE(33),
#endif
	/* WCD9310_RESET_N */
	PM8XXX_GPIO_INIT(34, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, 1, \
				PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
				PM_GPIO_STRENGTH_MED, \
				PM_GPIO_FUNC_NORMAL, 0, 0),
	PM8XXX_GPIO_INPUT(36,	PM_GPIO_PULL_UP_30),	/* SIM_DET_N */
	PM8XXX_GPIO_INPUT(38,	PM_GPIO_PULL_NO),	/* PLUG_DET */
	/* For CDB compatible  */
	PM8XXX_GPIO_INIT(40, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, 1, \
				PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
				PM_GPIO_STRENGTH_LOW, \
				PM_GPIO_FUNC_NORMAL, 0, 0),
	/* OTG_OVP_CNTL */
	PM8XXX_GPIO_INIT(42, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, 1, \
				PM_GPIO_PULL_NO, PM_GPIO_VIN_VPH, \
				PM_GPIO_STRENGTH_LOW, \
				PM_GPIO_FUNC_NORMAL, 0, 0),

	/* MLCD_RST_N (GPIO_43) is set by LCD driver */
};

/* Initial PM8921 MPP configurations */
static struct pm8xxx_mpp_init pm8921_mpps[] __initdata = {
	/* External 5V regulator enable; shared by HDMI and USB_OTG switches. */
	PM8XXX_MPP_INIT(7, D_INPUT, PM8921_MPP_DIG_LEVEL_VPH, DIN_TO_INT),
	PM8XXX_MPP_INIT(PM8XXX_AMUX_MPP_8, A_INPUT, PM8XXX_MPP_AIN_AMUX_CH8,
								DOUT_CTRL_LOW),
	PM8XXX_MPP_INIT(4, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW),

};

void __init pm8921_gpio_mpp_init(void)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(pm8921_gpios); i++) {
		rc = pm8xxx_gpio_config(pm8921_gpios[i].gpio,
					&pm8921_gpios[i].config);
		if (rc) {
			pr_err("%s: pm8xxx_gpio_config: rc=%d\n", __func__, rc);
			break;
		}
	}

	for (i = 0; i < ARRAY_SIZE(pm8921_mpps); i++) {
		rc = pm8xxx_mpp_config(pm8921_mpps[i].mpp,
					&pm8921_mpps[i].config);
		if (rc) {
			pr_err("%s: pm8xxx_mpp_config: rc=%d\n", __func__, rc);
			break;
		}
	}
}

/* Section: Regulators */
#define VREG_CONSUMERS(_id) \
	static struct regulator_consumer_supply vreg_consumers_##_id[]

/*
 * Consumer specific regulator names:
 *			 regulator name		consumer dev_name
 */
VREG_CONSUMERS(L1) = {
	REGULATOR_SUPPLY("8921_l1",		NULL),
};
VREG_CONSUMERS(L2) = {
	REGULATOR_SUPPLY("8921_l2",		NULL),
	REGULATOR_SUPPLY("dsi_vdda",		"mipi_dsi.1"),
	REGULATOR_SUPPLY("mipi_csi_vdd",	"msm_camera_imx074.0"),
	REGULATOR_SUPPLY("mipi_csi_vdd",	"msm_camera_ov2720.0"),
#if defined(CONFIG_SEMC_CAM_MAIN_V4L2) || defined(CONFIG_SEMC_CAM_SUB_V4L2)
	REGULATOR_SUPPLY("mipi_csi_vdd", "msm_csid.0"),
	REGULATOR_SUPPLY("mipi_csi_vdd", "msm_csid.1"),
#endif
};
VREG_CONSUMERS(L3) = {
	REGULATOR_SUPPLY("8921_l3",		NULL),
	REGULATOR_SUPPLY("HSUSB_3p3",		"msm_otg"),
};
VREG_CONSUMERS(L4) = {
	REGULATOR_SUPPLY("8921_l4",		NULL),
	REGULATOR_SUPPLY("HSUSB_1p8",		"msm_otg"),
	REGULATOR_SUPPLY("iris_vddxo",		"wcnss_wlan.0"),
};
VREG_CONSUMERS(L5) = {
	REGULATOR_SUPPLY("8921_l5",		NULL),
	REGULATOR_SUPPLY("sdc_vdd",		"msm_sdcc.1"),
};
VREG_CONSUMERS(L6) = {
	REGULATOR_SUPPLY("8921_l6",		NULL),
	REGULATOR_SUPPLY("sdc_vdd",		"msm_sdcc.3"),
};
VREG_CONSUMERS(L7) = {
	REGULATOR_SUPPLY("8921_l7",		NULL),
	REGULATOR_SUPPLY("sdc_vddp",		"msm_sdcc.3"),
};
VREG_CONSUMERS(L8) = {
	REGULATOR_SUPPLY("8921_l8",		NULL),
	REGULATOR_SUPPLY("dsi_vci",		"mipi_dsi.1"),
};
VREG_CONSUMERS(L9) = {
	REGULATOR_SUPPLY("8921_l9",		NULL),
	REGULATOR_SUPPLY("prox_vdd",		NULL),
	REGULATOR_SUPPLY("acc_vdd",		NULL),
	REGULATOR_SUPPLY("mag_vdd",		NULL),
	REGULATOR_SUPPLY("gyro_vdd",		NULL),
};
VREG_CONSUMERS(L10) = {
	REGULATOR_SUPPLY("8921_l10",		NULL),
	REGULATOR_SUPPLY("iris_vddpa",		"wcnss_wlan.0"),

};
VREG_CONSUMERS(L11) = {
	REGULATOR_SUPPLY("8921_l11",		NULL),
	REGULATOR_SUPPLY("cam_vana",		"msm_camera_imx074.0"),
	REGULATOR_SUPPLY("cam_vana",		"msm_camera_ov2720.0"),
#if defined(CONFIG_SEMC_CAM_MAIN_V4L2) || defined(CONFIG_SEMC_CAM_SUB_V4L2)
	REGULATOR_SUPPLY("cam_vana",		"4-001a"),
	REGULATOR_SUPPLY("cam_vana",		"4-0048"),
#endif
};
VREG_CONSUMERS(L12) = {
	REGULATOR_SUPPLY("8921_l12",		NULL),
	REGULATOR_SUPPLY("cam_vdig",		"msm_camera_imx074.0"),
	REGULATOR_SUPPLY("cam_vdig",		"msm_camera_ov2720.0"),
#if defined(CONFIG_SEMC_CAM_MAIN_V4L2) || defined(CONFIG_SEMC_CAM_SUB_V4L2)
	REGULATOR_SUPPLY("cam_vdig",		"4-001a"),
#endif
};
VREG_CONSUMERS(L14) = {
	REGULATOR_SUPPLY("8921_l14",		NULL),
	REGULATOR_SUPPLY("pa_therm",		"pm8xxx-adc"),
};
VREG_CONSUMERS(L15) = {
	REGULATOR_SUPPLY("8921_l15",		NULL),
};
VREG_CONSUMERS(L16) = {
	REGULATOR_SUPPLY("8921_l16",		NULL),
	REGULATOR_SUPPLY("cam_vaf",		"msm_camera_imx074.0"),
	REGULATOR_SUPPLY("cam_vaf",		"msm_camera_ov2720.0"),
#if defined(CONFIG_SEMC_CAM_MAIN_V4L2) || defined(CONFIG_SEMC_CAM_SUB_V4L2)
	REGULATOR_SUPPLY("cam_vaf",		"4-001a"),
#endif
};
VREG_CONSUMERS(L17) = {
	REGULATOR_SUPPLY("8921_l17",		NULL),
};
VREG_CONSUMERS(L18) = {
	REGULATOR_SUPPLY("8921_l18",		NULL),
};
VREG_CONSUMERS(L21) = {
	REGULATOR_SUPPLY("8921_l21",		NULL),
};
VREG_CONSUMERS(L22) = {
	REGULATOR_SUPPLY("8921_l22",		NULL),
};
VREG_CONSUMERS(L23) = {
	REGULATOR_SUPPLY("8921_l23",		NULL),
	REGULATOR_SUPPLY("hdmi_avdd",		"hdmi_msm.0"),
	REGULATOR_SUPPLY("pll_vdd",		"pil_riva"),
	REGULATOR_SUPPLY("pll_vdd",		"pil_qdsp6v4.1"),
	REGULATOR_SUPPLY("pll_vdd",		"pil_qdsp6v4.2"),
};
VREG_CONSUMERS(L24) = {
	REGULATOR_SUPPLY("8921_l24",		NULL),
	REGULATOR_SUPPLY("riva_vddmx",		"wcnss_wlan.0"),
};
VREG_CONSUMERS(L25) = {
	REGULATOR_SUPPLY("8921_l25",		NULL),
	REGULATOR_SUPPLY("VDDD_CDC_D",		"tabla-slim"),
	REGULATOR_SUPPLY("CDC_VDDA_A_1P2V",	"tabla-slim"),
	REGULATOR_SUPPLY("VDDD_CDC_D",		"tabla2x-slim"),
	REGULATOR_SUPPLY("CDC_VDDA_A_1P2V",	"tabla2x-slim"),
};
VREG_CONSUMERS(L26) = {
	REGULATOR_SUPPLY("8921_l26",		NULL),
	REGULATOR_SUPPLY("core_vdd",		"pil_qdsp6v4.0"),
};
VREG_CONSUMERS(L27) = {
	REGULATOR_SUPPLY("8921_l27",		NULL),
	REGULATOR_SUPPLY("core_vdd",		"pil_qdsp6v4.2"),
};
VREG_CONSUMERS(L28) = {
	REGULATOR_SUPPLY("8921_l28",		NULL),
	REGULATOR_SUPPLY("core_vdd",		"pil_qdsp6v4.1"),
};
VREG_CONSUMERS(L29) = {
	REGULATOR_SUPPLY("8921_l29",		NULL),
	REGULATOR_SUPPLY("dsi_vddio",		"mipi_dsi.1"),
};
VREG_CONSUMERS(S1) = {
	REGULATOR_SUPPLY("8921_s1",		NULL),
};
VREG_CONSUMERS(S2) = {
	REGULATOR_SUPPLY("8921_s2",		NULL),
	REGULATOR_SUPPLY("iris_vddrfa",		"wcnss_wlan.0"),

};
VREG_CONSUMERS(S3) = {
	REGULATOR_SUPPLY("8921_s3",		NULL),
	REGULATOR_SUPPLY("HSUSB_VDDCX",		"msm_otg"),
	REGULATOR_SUPPLY("riva_vddcx",		"wcnss_wlan.0"),
};
VREG_CONSUMERS(S4) = {
	REGULATOR_SUPPLY("8921_s4",		NULL),
	REGULATOR_SUPPLY("sdc_vccq",		"msm_sdcc.1"),
	REGULATOR_SUPPLY("sdc_vdd",		"msm_sdcc.2"),
	REGULATOR_SUPPLY("sdc_vddp",            "msm_sdcc.4"),
	REGULATOR_SUPPLY("riva_vddpx",		"wcnss_wlan.0"),
	REGULATOR_SUPPLY("hdmi_vcc",		"hdmi_msm.0"),
	REGULATOR_SUPPLY("VDDIO_CDC",		"tabla-slim"),
	REGULATOR_SUPPLY("CDC_VDD_CP",		"tabla-slim"),
	REGULATOR_SUPPLY("CDC_VDDA_TX",		"tabla-slim"),
	REGULATOR_SUPPLY("CDC_VDDA_RX",		"tabla-slim"),
	REGULATOR_SUPPLY("VDDIO_CDC",		"tabla2x-slim"),
	REGULATOR_SUPPLY("CDC_VDD_CP",		"tabla2x-slim"),
	REGULATOR_SUPPLY("CDC_VDDA_TX",		"tabla2x-slim"),
	REGULATOR_SUPPLY("CDC_VDDA_RX",		"tabla2x-slim"),
	REGULATOR_SUPPLY("EXT_HUB_VDDIO",	"msm_hsic_host"),
};
VREG_CONSUMERS(S5) = {
	REGULATOR_SUPPLY("8921_s5",		NULL),
	REGULATOR_SUPPLY("krait0",		NULL),
};
VREG_CONSUMERS(S6) = {
	REGULATOR_SUPPLY("8921_s6",		NULL),
	REGULATOR_SUPPLY("krait1",		NULL),
};
VREG_CONSUMERS(S7) = {
	REGULATOR_SUPPLY("8921_s7",		NULL),
};
VREG_CONSUMERS(S8) = {
	REGULATOR_SUPPLY("8921_s8",		NULL),
};
VREG_CONSUMERS(LVS1) = {
	REGULATOR_SUPPLY("8921_lvs1",		NULL),
	REGULATOR_SUPPLY("iris_vddio",		"wcnss_wlan.0"),
};
VREG_CONSUMERS(LVS2) = {
	REGULATOR_SUPPLY("8921_lvs2",		NULL),
	REGULATOR_SUPPLY("iris_vdddig",		"wcnss_wlan.0"),
};
VREG_CONSUMERS(LVS3) = {
	REGULATOR_SUPPLY("8921_lvs3",		NULL),
};
VREG_CONSUMERS(LVS4) = {
	REGULATOR_SUPPLY("8921_lvs4",		NULL),
};
VREG_CONSUMERS(LVS5) = {
	REGULATOR_SUPPLY("8921_lvs5",		NULL),
	REGULATOR_SUPPLY("cam_vio",		"msm_camera_imx074.0"),
	REGULATOR_SUPPLY("cam_vio",		"msm_camera_ov2720.0"),
#if defined(CONFIG_SEMC_CAM_MAIN_V4L2) || defined(CONFIG_SEMC_CAM_SUB_V4L2)
	REGULATOR_SUPPLY("cam_vio",		"4-001a"),
	REGULATOR_SUPPLY("cam_vio",		"4-0048"),
#endif
};
VREG_CONSUMERS(LVS6) = {
	REGULATOR_SUPPLY("8921_lvs6",		NULL),
};
VREG_CONSUMERS(LVS7) = {
	REGULATOR_SUPPLY("8921_lvs7",		NULL),
};
VREG_CONSUMERS(USB_OTG) = {
	REGULATOR_SUPPLY("8921_usb_otg",	NULL),
};
VREG_CONSUMERS(HDMI_MVS) = {
	REGULATOR_SUPPLY("8921_hdmi_mvs",	NULL),
};

#define PM8XXX_VREG_INIT(_id, _name, _min_uV, _max_uV, _modes, _ops, \
			 _apply_uV, _pull_down, _always_on, _supply_regulator, \
			 _system_uA, _enable_time, _reg_id) \
	{ \
		.init_data = { \
			.constraints = { \
				.valid_modes_mask	= _modes, \
				.valid_ops_mask		= _ops, \
				.min_uV			= _min_uV, \
				.max_uV			= _max_uV, \
				.input_uV		= _max_uV, \
				.apply_uV		= _apply_uV, \
				.always_on		= _always_on, \
				.name			= _name, \
			}, \
			.num_consumer_supplies	= \
					ARRAY_SIZE(vreg_consumers_##_id), \
			.consumer_supplies	= vreg_consumers_##_id, \
			.supply_regulator	= _supply_regulator, \
		}, \
		.id			= _reg_id, \
		.pull_down_enable	= _pull_down, \
		.system_uA		= _system_uA, \
		.enable_time		= _enable_time, \
	}

#define PM8XXX_LDO(_id, _name, _always_on, _pull_down, _min_uV, _max_uV, \
		_enable_time, _supply_regulator, _system_uA, _reg_id) \
	PM8XXX_VREG_INIT(_id, _name, _min_uV, _max_uV, REGULATOR_MODE_NORMAL \
		| REGULATOR_MODE_IDLE, REGULATOR_CHANGE_VOLTAGE | \
		REGULATOR_CHANGE_STATUS | REGULATOR_CHANGE_MODE | \
		REGULATOR_CHANGE_DRMS, 0, _pull_down, _always_on, \
		_supply_regulator, _system_uA, _enable_time, _reg_id)

#define PM8XXX_NLDO1200(_id, _name, _always_on, _pull_down, _min_uV, \
		_max_uV, _enable_time, _supply_regulator, _system_uA, _reg_id) \
	PM8XXX_VREG_INIT(_id, _name, _min_uV, _max_uV, REGULATOR_MODE_NORMAL \
		| REGULATOR_MODE_IDLE, REGULATOR_CHANGE_VOLTAGE | \
		REGULATOR_CHANGE_STATUS | REGULATOR_CHANGE_MODE | \
		REGULATOR_CHANGE_DRMS, 0, _pull_down, _always_on, \
		_supply_regulator, _system_uA, _enable_time, _reg_id)

#define PM8XXX_SMPS(_id, _name, _always_on, _pull_down, _min_uV, _max_uV, \
		_enable_time, _supply_regulator, _system_uA, _reg_id) \
	PM8XXX_VREG_INIT(_id, _name, _min_uV, _max_uV, REGULATOR_MODE_NORMAL \
		| REGULATOR_MODE_IDLE, REGULATOR_CHANGE_VOLTAGE | \
		REGULATOR_CHANGE_STATUS | REGULATOR_CHANGE_MODE | \
		REGULATOR_CHANGE_DRMS, 0, _pull_down, _always_on, \
		_supply_regulator, _system_uA, _enable_time, _reg_id)

#define PM8XXX_FTSMPS(_id, _name, _always_on, _pull_down, _min_uV, _max_uV, \
		_enable_time, _supply_regulator, _system_uA, _reg_id) \
	PM8XXX_VREG_INIT(_id, _name, _min_uV, _max_uV, REGULATOR_MODE_NORMAL, \
		REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS \
		| REGULATOR_CHANGE_MODE, 0, _pull_down, _always_on, \
		_supply_regulator, _system_uA, _enable_time, _reg_id)

#define PM8XXX_VS(_id, _name, _always_on, _pull_down, _enable_time, \
		_supply_regulator, _reg_id) \
	PM8XXX_VREG_INIT(_id, _name, 0, 0, 0, REGULATOR_CHANGE_STATUS, 0, \
		_pull_down, _always_on, _supply_regulator, 0, _enable_time, \
		_reg_id)

#define PM8XXX_VS300(_id, _name, _always_on, _pull_down, _enable_time, \
		_supply_regulator, _reg_id) \
	PM8XXX_VREG_INIT(_id, _name, 0, 0, 0, REGULATOR_CHANGE_STATUS, 0, \
		_pull_down, _always_on, _supply_regulator, 0, _enable_time, \
		_reg_id)

#define PM8XXX_NCP(_id, _name, _always_on, _min_uV, _max_uV, _enable_time, \
		_supply_regulator, _reg_id) \
	PM8XXX_VREG_INIT(_id, _name, _min_uV, _max_uV, 0, \
		REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS, 0, 0, \
		_always_on, _supply_regulator, 0, _enable_time, _reg_id)

/* Pin control initialization */
#define PM8XXX_PC(_id, _name, _always_on, _pin_fn, _pin_ctrl, \
		  _supply_regulator, _reg_id) \
	{ \
		.init_data = { \
			.constraints = { \
				.valid_ops_mask	= REGULATOR_CHANGE_STATUS, \
				.always_on	= _always_on, \
				.name		= _name, \
			}, \
			.num_consumer_supplies	= \
					ARRAY_SIZE(vreg_consumers_##_id##_PC), \
			.consumer_supplies	= vreg_consumers_##_id##_PC, \
			.supply_regulator  = _supply_regulator, \
		}, \
		.id		= _reg_id, \
		.pin_fn		= PM8XXX_VREG_PIN_FN_##_pin_fn, \
		.pin_ctrl	= _pin_ctrl, \
	}

#define GPIO_VREG(_id, _reg_name, _gpio_label, _gpio, _supply_regulator) \
	[GPIO_VREG_ID_##_id] = { \
		.init_data = { \
			.constraints = { \
				.valid_ops_mask	= REGULATOR_CHANGE_STATUS, \
			}, \
			.num_consumer_supplies	= \
					ARRAY_SIZE(vreg_consumers_##_id), \
			.consumer_supplies	= vreg_consumers_##_id, \
			.supply_regulator	= _supply_regulator, \
		}, \
		.regulator_name = _reg_name, \
		.gpio_label	= _gpio_label, \
		.gpio		= _gpio, \
	}


#define SAW_VREG_INIT(_id, _name, _min_uV, _max_uV) \
	{ \
		.constraints = { \
			.name		= _name, \
			.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE, \
			.min_uV		= _min_uV, \
			.max_uV		= _max_uV, \
		}, \
		.num_consumer_supplies	= ARRAY_SIZE(vreg_consumers_##_id), \
		.consumer_supplies	= vreg_consumers_##_id, \
	}

#define RPM_INIT(_id, _min_uV, _max_uV, _modes, _ops, _apply_uV, _default_uV, \
		 _peak_uA, _avg_uA, _pull_down, _pin_ctrl, _freq, _pin_fn, \
		 _force_mode, _power_mode, _state, _sleep_selectable, \
		 _always_on, _supply_regulator, _system_uA) \
	{ \
		.init_data = { \
			.constraints = { \
				.valid_modes_mask	= _modes, \
				.valid_ops_mask		= _ops, \
				.min_uV			= _min_uV, \
				.max_uV			= _max_uV, \
				.input_uV		= _min_uV, \
				.apply_uV		= _apply_uV, \
				.always_on		= _always_on, \
			}, \
			.num_consumer_supplies	= \
					ARRAY_SIZE(vreg_consumers_##_id), \
			.consumer_supplies	= vreg_consumers_##_id, \
			.supply_regulator	= _supply_regulator, \
		}, \
		.id			= RPM_VREG_ID_PM8921_##_id, \
		.default_uV		= _default_uV, \
		.peak_uA		= _peak_uA, \
		.avg_uA			= _avg_uA, \
		.pull_down_enable	= _pull_down, \
		.pin_ctrl		= _pin_ctrl, \
		.freq			= RPM_VREG_FREQ_##_freq, \
		.pin_fn			= _pin_fn, \
		.force_mode		= _force_mode, \
		.power_mode		= _power_mode, \
		.state			= _state, \
		.sleep_selectable	= _sleep_selectable, \
		.system_uA		= _system_uA, \
	}

#define RPM_LDO(_id, _always_on, _pd, _sleep_selectable, _min_uV, _max_uV, \
		_supply_regulator, _system_uA, _init_peak_uA) \
	RPM_INIT(_id, _min_uV, _max_uV, REGULATOR_MODE_NORMAL \
		 | REGULATOR_MODE_IDLE, REGULATOR_CHANGE_VOLTAGE \
		 | REGULATOR_CHANGE_STATUS | REGULATOR_CHANGE_MODE \
		 | REGULATOR_CHANGE_DRMS, 0, _max_uV, _init_peak_uA, 0, _pd, \
		 RPM_VREG_PIN_CTRL_NONE, NONE, RPM_VREG_PIN_FN_8960_NONE, \
		 RPM_VREG_FORCE_MODE_8960_NONE, RPM_VREG_POWER_MODE_8960_PWM, \
		 RPM_VREG_STATE_OFF, _sleep_selectable, _always_on, \
		 _supply_regulator, _system_uA)

#define RPM_SMPS(_id, _always_on, _pd, _sleep_selectable, _min_uV, _max_uV, \
		 _supply_regulator, _system_uA, _freq) \
	RPM_INIT(_id, _min_uV, _max_uV, REGULATOR_MODE_NORMAL \
		 | REGULATOR_MODE_IDLE, REGULATOR_CHANGE_VOLTAGE \
		 | REGULATOR_CHANGE_STATUS | REGULATOR_CHANGE_MODE \
		 | REGULATOR_CHANGE_DRMS, 0, _max_uV, _system_uA, 0, _pd, \
		 RPM_VREG_PIN_CTRL_NONE, _freq, RPM_VREG_PIN_FN_8960_NONE, \
		 RPM_VREG_FORCE_MODE_8960_NONE, RPM_VREG_POWER_MODE_8960_PWM, \
		 RPM_VREG_STATE_OFF, _sleep_selectable, _always_on, \
		 _supply_regulator, _system_uA)

#define RPM_VS(_id, _always_on, _pd, _sleep_selectable, _supply_regulator) \
	RPM_INIT(_id, 0, 0, 0, REGULATOR_CHANGE_STATUS, 0, 0, 1000, 1000, _pd, \
		 RPM_VREG_PIN_CTRL_NONE, NONE, RPM_VREG_PIN_FN_8960_NONE, \
		 RPM_VREG_FORCE_MODE_8960_NONE, RPM_VREG_POWER_MODE_8960_PWM, \
		 RPM_VREG_STATE_OFF, _sleep_selectable, _always_on, \
		 _supply_regulator, 0)

#define RPM_NCP(_id, _always_on, _sleep_selectable, _min_uV, _max_uV, \
		_supply_regulator, _freq) \
	RPM_INIT(_id, _min_uV, _max_uV, 0, REGULATOR_CHANGE_VOLTAGE \
		 | REGULATOR_CHANGE_STATUS, 0, _max_uV, 1000, 1000, 0, \
		 RPM_VREG_PIN_CTRL_NONE, _freq, RPM_VREG_PIN_FN_8960_NONE, \
		 RPM_VREG_FORCE_MODE_8960_NONE, RPM_VREG_POWER_MODE_8960_PWM, \
		 RPM_VREG_STATE_OFF, _sleep_selectable, _always_on, \
		 _supply_regulator, 0)

/* Pin control initialization */
#define RPM_PC_INIT(_id, _always_on, _pin_fn, _pin_ctrl, _supply_regulator) \
	{ \
		.init_data = { \
			.constraints = { \
				.valid_ops_mask	= REGULATOR_CHANGE_STATUS, \
				.always_on	= _always_on, \
			}, \
			.num_consumer_supplies	= \
					ARRAY_SIZE(vreg_consumers_##_id##_PC), \
			.consumer_supplies	= vreg_consumers_##_id##_PC, \
			.supply_regulator	= _supply_regulator, \
		}, \
		.id	  = RPM_VREG_ID_PM8921_##_id##_PC, \
		.pin_fn	  = RPM_VREG_PIN_FN_8960_##_pin_fn, \
		.pin_ctrl = _pin_ctrl, \
	}

/* SAW regulator constraints */
struct regulator_init_data msm_saw_regulator_pdata_s5 =
	/*	      ID  vreg_name	       min_uV   max_uV */
	SAW_VREG_INIT(S5, "8921_s5",	       850000, 1300000);
struct regulator_init_data msm_saw_regulator_pdata_s6 =
	SAW_VREG_INIT(S6, "8921_s6",	       850000, 1300000);

/* PM8921 regulator constraints */
struct pm8xxx_regulator_platform_data
msm_pm8921_regulator_pdata[] __devinitdata = {
	/*
	 *			  ID a_on pd min_uV   max_uV   en_t  supply
	 *	system_uA
	 */
	PM8XXX_NLDO1200(L26, "8921_l26", 0, 1, 1050000, 1050000, 200, "8921_s7",
		0, 1),
	PM8XXX_NLDO1200(L27, "8921_l27", 0, 1, 1050000, 1050000, 200, "8921_s7",
		0, 2),
	PM8XXX_NLDO1200(L28, "8921_l28", 0, 1, 1050000, 1050000, 200, "8921_s7",
		0, 3),
	PM8XXX_LDO(L29,      "8921_l29", 0, 1, 1800000, 1800000, 200, "8921_s8",
		0, 4),

	/*	     ID        name      always_on pd en_t supply reg_ID */
	PM8XXX_VS300(USB_OTG,  "8921_usb_otg",  0, 1, 0,   NULL,  5),
	PM8XXX_VS300(HDMI_MVS, "8921_hdmi_mvs", 0, 1, 0,   NULL,  6),
};

static struct rpm_regulator_init_data
msm_rpm_regulator_init_data[] __devinitdata = {
	/*	 ID    a_on pd ss min_uV   max_uV  supply sys_uA freq */
	RPM_SMPS(S1,	 1, 1, 0, 1225000, 1225000, NULL, 100000, 3p20),
	RPM_SMPS(S2,	 0, 1, 0, 1300000, 1300000, NULL, 0,	  1p60),
	RPM_SMPS(S3,	 0, 1, 1,  500000, 1150000, NULL, 100000, 4p80),
	RPM_SMPS(S4,	 1, 1, 0, 1800000, 1800000, NULL, 100000, 1p60),
	RPM_SMPS(S7,	 0, 1, 0, 1150000, 1150000, NULL, 100000, 3p20),
	RPM_SMPS(S8,	 1, 1, 1, 2050000, 2050000, NULL, 100000, 1p60),

	/*	ID     a_on pd ss min_uV   max_uV  supply  sys_uA init_ip */
	RPM_LDO(L1,	 1, 1, 0, 1050000, 1050000, "8921_s4", 0, 10000),
	RPM_LDO(L2,	 0, 1, 0, 1200000, 1200000, "8921_s4", 0, 0),
	RPM_LDO(L3,	 0, 1, 0, 3075000, 3075000, NULL,      0, 0),
	RPM_LDO(L4,	 1, 1, 0, 1800000, 1800000, NULL,      10000, 10000),
	RPM_LDO(L5,	 0, 1, 0, 2950000, 2950000, NULL,      0, 0),
	RPM_LDO(L6,	 1, 1, 0, 2950000, 2950000, NULL,      0, 0),
	RPM_LDO(L7,	 1, 1, 0, 1850000, 2950000, NULL,      10000, 10000),
	RPM_LDO(L8,	 0, 1, 0, 2800000, 3000000, NULL,      0, 0),
	RPM_LDO(L9,	 0, 1, 0, 2850000, 2850000, NULL,      0, 0),
	RPM_LDO(L10,	 0, 1, 0, 3000000, 3000000, NULL,      0, 0),
	RPM_LDO(L11,	 0, 1, 0, 2600000, 3000000, NULL,      0, 0),
	RPM_LDO(L12,	 0, 1, 0, 1200000, 1200000, "8921_s4", 0, 0),
	RPM_LDO(L14,	 0, 1, 0, 1800000, 1800000, NULL,      0, 0),
	RPM_LDO(L15,	 0, 1, 0, 1800000, 2950000, NULL,      0, 0),
	RPM_LDO(L16,	 0, 1, 0, 2600000, 3000000, NULL,      0, 0),
	RPM_LDO(L17,	 0, 1, 0, 1800000, 3000000, NULL,      0, 0),
	RPM_LDO(L18,	 0, 1, 0, 1200000, 1200000, "8921_s4", 0, 0),
	RPM_LDO(L21,	 0, 1, 0, 1900000, 1900000, "8921_s8", 0, 0),
	RPM_LDO(L22,	 0, 1, 0, 2750000, 2750000, NULL,      0, 0),
	RPM_LDO(L23,	 1, 1, 1, 1800000, 1800000, "8921_s8", 10000, 10000),
	RPM_LDO(L24,	 0, 1, 1,  750000, 1150000, "8921_s1", 10000, 10000),
	RPM_LDO(L25,	 1, 1, 0, 1225000, 1250000, "8921_s1", 10000, 10000),

	/*	ID     a_on pd ss		    supply */
	RPM_VS(LVS1,	 0, 1, 0,		    "8921_s4"),
	RPM_VS(LVS2,	 0, 1, 0,		    "8921_s1"),
	RPM_VS(LVS3,	 0, 1, 0,		    "8921_s4"),
	RPM_VS(LVS4,	 0, 1, 0,		    "8921_s4"),
	RPM_VS(LVS5,	 0, 1, 0,		    "8921_s4"),
	RPM_VS(LVS6,	 0, 1, 0,		    "8921_s4"),
	RPM_VS(LVS7,	 0, 1, 0,		    "8921_s4"),
};

int msm_pm8921_regulator_pdata_len __devinitdata =
	ARRAY_SIZE(msm_pm8921_regulator_pdata);

struct rpm_regulator_platform_data msm_rpm_regulator_pdata __devinitdata = {
	.init_data = msm_rpm_regulator_init_data,
	.num_regulators = ARRAY_SIZE(msm_rpm_regulator_init_data),
	.version		= RPM_VREG_VERSION_8960,
	.vreg_id_vdd_mem	= RPM_VREG_ID_PM8921_L24,
	.vreg_id_vdd_dig	= RPM_VREG_ID_PM8921_S3,
};

/* Section: MSM GPIO */
static struct gpiomux_setting unused_gpio = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir  = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting gpio_2ma_no_pull_low = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir  = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting gpio_2ma_no_pull_in = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir  = GPIOMUX_IN,
};

static struct gpiomux_setting gpio_2ma_pull_up_in = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir  = GPIOMUX_IN,
};

static struct gpiomux_setting gpio_2ma_pull_down_in = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir  = GPIOMUX_IN,
};

static struct gpiomux_setting cdc_mclk = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting slimbus = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

static struct gpiomux_setting gsbi3 = {
	.func = GPIOMUX_FUNC_1,
	.drv  = GPIOMUX_DRV_4MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi4 = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_4MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi10 = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_4MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi12 = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_4MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting cam_mclk0 = {
	.func = GPIOMUX_FUNC_1,
	.drv  = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting cam_mclk1 = {
	.func = GPIOMUX_FUNC_2,
	.drv  = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting hdmi_suspend_1_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir  = GPIOMUX_IN,
};

static struct gpiomux_setting hdmi_active_1_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting hdmi_suspend_2_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting hdmi_active_2_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting debug_uart_tx = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_4MA,
	.pull = GPIOMUX_PULL_NONE
};

static struct gpiomux_setting debug_uart_rx = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP
};

static struct msm_gpiomux_config semc_blue_all_cfgs[] __initdata = {
	{ /* NC */
		.gpio = 0,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* MCAM_RST_N */
		.gpio = 1,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* FLASH_DR_RST_N */
		.gpio = 2,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* FLASH_TRG */
		.gpio = 3,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_pull_down_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in,
		},
	},
	{ /* CAM_MCLK1 */
		.gpio = 4,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_mclk1,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* CAM_MCLK0 */
		.gpio = 5,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_mclk0,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* Reserved (PON_VOLTAGE_SEL) */
		.gpio = 6,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 7,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 8,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 9,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* ACCEL_INT */
		.gpio = 10,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in,
		},
	},
	{ /* TP_INT */
		.gpio = 11,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_in,
		},
	},
	{ /* HW_ID0 */
		.gpio = 12,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_in,
		},
	},
	{ /* HW_ID1 */
		.gpio = 13,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_in,
		},
	},
	{ /* HW_ID2 */
		.gpio = 14,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_in,
		},
	},
	{ /* HW_ID3 */
		.gpio = 15,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_in,
		},
	},
	{ /* I2C_DATA_TP */
		.gpio = 16,
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi3,
			[GPIOMUX_SUSPENDED] = &gsbi3,
		},
	},
	{ /* I2C_CLK_TP */
		.gpio = 17,
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi3,
			[GPIOMUX_SUSPENDED] = &gsbi3,
		},
	},
	{ /* CHAT_CAM_RST_N */
		.gpio = 18,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
#if defined(CONFIG_SEMC_FELICA_SUPPORT) && !defined(CONFIG_NFC_PN544)
	{ /* FELICIA_RFS */
		.gpio = 19,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_in,
		},
	},
#elif !defined(CONFIG_SEMC_FELICA_SUPPORT) && defined(CONFIG_NFC_PN544)
	{ /* NFC_DWLD_EN */
		.gpio = 19,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
#else
	{ /* NC */
		.gpio = 19,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
#endif
	{ /* I2C_DATA_CAM */
		.gpio = 20,
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi4,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in,
		},
	},
	{ /* I2C_CLK_CAM */
		.gpio = 21,
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi4,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in,
		},
	},
	{ /* NC */
		.gpio = 22,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 23,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 24,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 25,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},

	/* NOT CONFIGURED: 26-31 */

	{ /* NC */
		.gpio = 32,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 33,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* UART_TX_DFMS */
		.gpio = 34,
		.settings = {
			[GPIOMUX_ACTIVE] = &debug_uart_tx,
			[GPIOMUX_SUSPENDED] = &debug_uart_tx,
		},
	},
	{ /* UART_RX_DTMS */
		.gpio = 35,
		.settings = {
			[GPIOMUX_ACTIVE] = &debug_uart_rx,
			[GPIOMUX_SUSPENDED] = &debug_uart_rx,
		},
	},
	{ /* LCD_DCDC_EN */
		.gpio = 36,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* OTG_OVRCUR_DET_N */
		.gpio = 37,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_in,
		},
	},
	{ /* NC */
		.gpio = 38,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 39,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 40,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 41,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 42,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 43,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* I2C_DATA_SENS */
		.gpio = 44,
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi12,
			[GPIOMUX_SUSPENDED] = &gsbi12,
		},
	},
	{ /* I2C_CLK_SENS */
		.gpio = 45,
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi12,
			[GPIOMUX_SUSPENDED] = &gsbi12,
		},
	},
	{ /* DEBUG_GPIO0 */
		.gpio = 46,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* DEBUG_GPIO1 */
		.gpio = 47,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* GYRO_FSYNC */
		.gpio = 48,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* PROX_INT_N */
		.gpio = 49,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_pull_up_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_up_in,
		},
	},
	{ /* NC */
		.gpio = 50,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* USB_OTG_EN */
		.gpio = 51,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* NC */
		.gpio = 52,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 53,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 54,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 55,
		.settings = { [GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in, },
	},
	{ /* NC */
		.gpio = 56,
		.settings = { [GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in, },
	},

	/* 57 not configured */

	{ /* NC */
		.gpio = 58,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* SLIMBUS1_MCLK */
		.gpio = 59,
		.settings = {
			[GPIOMUX_SUSPENDED] = &cdc_mclk,
		},
	},
	{ /* SLIMBUS1_CLK */
		.gpio = 60,
		.settings = {
			[GPIOMUX_SUSPENDED] = &slimbus,
		},
	},
	{ /* SLIMBUS1_DATA */
		.gpio = 61,
		.settings = {
			[GPIOMUX_SUSPENDED] = &slimbus,
		},
	},

	/* 62 not configured */

	{ /* NC */
		.gpio = 63,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* MHL_RST_N */
		.gpio = 64,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* MHL_INT */
		.gpio = 65,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_pull_up_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_up_in,
		},
	},
	{ /* LCD_ID */
		.gpio = 66,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_in,
		},
	},
	{ /* NC */
		.gpio = 67,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 68,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* GYRO_INT_N */
		.gpio = 69,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in,
		},
	},
	{ /* COMPASS_INT */
		.gpio = 70,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_pull_down_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in,
		},
	},
	{ /* UART_TX_FELICA */
		.gpio = 71,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* UART_RX_FELICA */
		.gpio = 72,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_pull_down_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in,
		},
	},
	{ /* I2C_DATA_PERI */
		.gpio = 73,
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi10,
			[GPIOMUX_SUSPENDED] = &gsbi10,
		},
	},
	{ /* I2C_CLK_PERI */
		.gpio = 74,
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi10,
			[GPIOMUX_SUSPENDED] = &gsbi10,
		},
	},
	{ /* NC */
		.gpio = 75,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 76,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 77,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 78,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 79,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 80,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 81,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 82,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},

	/* 83 not configured */

	/* 84-88 configured in separate struct below */

	{ /* NC */
		.gpio = 89,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 90,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 91,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},

	/* 92 not configured */

	{ /* NC */
		.gpio = 93,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 94,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 95,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 96,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 97,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 98,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 99,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* HDMI_DDCSCL */
		.gpio = 100,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_1_cfg,
		},
	},
	{ /* HDMI_DDCSCA */
		.gpio = 101,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_1_cfg,
		},
	},
	{ /* HDMI_HPDHDMI */
		.gpio = 102,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_2_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_2_cfg,
		},
	},

	/* 103-105 not configured */

	{ /* NFC_IRQ/FELICA_INT */
		.gpio = 106,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_in,
		},
	},
	{ /* SW_SERVICE */
		.gpio = 107,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_pull_up_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_up_in,
		},
	},

	/* 108 not configured */

	{ /* NC */
		.gpio = 109,
		.settings = { [GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in, },
	},
	{ /* NC */
		.gpio = 110,
		.settings = { [GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in, },
	},

	/* 111-119 not configured */

	{ /* NC */
		.gpio = 120,
		.settings = { [GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in, },
	},
	{ /* NC */
		.gpio = 121,
		.settings = { [GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in, },
	},

	/* 122-123 not configured */

	{ /* NC */
		.gpio = 124,
		.settings = { [GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in, },
	},

	/* 125-128 not configured */

	{ /* NC */
		.gpio = 129,
		.settings = { [GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in, },
	},

	/* 130-132 not configured */

	{ /* NC */
		.gpio = 133,
		.settings = { [GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in, },
	},

	/* 134-137 not configured */

	{ /* NC */
		.gpio = 138,
		.settings = { [GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in, },
	},

	/* 139 not configured */

	{ /* NC */
		.gpio = 140,
		.settings = { [GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in, },
	},
	{ /* NC */
		.gpio = 141,
		.settings = { [GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in, },
	},

	/* 142-148 not configured */

	{ /* NC */
		.gpio = 149,
		.settings = { [GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in, },
	},
	{ /* NC */
		.gpio = 150,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 151,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
};


static struct gpiomux_setting wcnss_5wire_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting wcnss_5wire_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv  = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};

struct msm_gpiomux_config wcnss_5wire_interface[] = {
	{
		.gpio = 84,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 85,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 86,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 87,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 88,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
};

const size_t wcnss_5wire_interface_size = ARRAY_SIZE(wcnss_5wire_interface);

void __init gpiomux_device_install(void)
{
	msm_gpiomux_install(semc_blue_all_cfgs,
			ARRAY_SIZE(semc_blue_all_cfgs));

	msm_gpiomux_install(wcnss_5wire_interface,
			ARRAY_SIZE(wcnss_5wire_interface));
}

#ifdef CONFIG_MSM_CAMERA
#if defined(CONFIG_SEMC_VPE)
struct resource semc_vpe_resources[] = {
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

struct platform_device semc_vpe_device = {
	.name		= "semc_vpe",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(semc_vpe_resources),
	.resource	= semc_vpe_resources,
};
#endif

#if defined(CONFIG_SEMC_CAM_MAIN_V4L2) || defined(CONFIG_SEMC_CAM_SUB_V4L2)
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
	{ "cam_vaf", REG_LDO, 2600000, 3000000, 300000 },
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

#ifdef CONFIG_SEMC_CAM_MAIN_V4L2
static struct msm_camera_sensor_platform_info
		msm_camera_semc_sensor_main_platform_info = {
	.mount_angle		= 90,
	.sensor_reset		= 1,
	.cam_vreg		= msm_camera_back_cam_vreg,
	.num_vreg		= ARRAY_SIZE(msm_camera_back_cam_vreg),
	.gpio_conf		= &msm_camera_back_cam_gpio_conf,
};

static struct msm_camera_sensor_info
		msm_camera_semc_sensor_main_sensor_info = {
	.sensor_name		= "semc_sensor_main",
	.sensor_reset		= 1,
	.pdata			= &msm_camera_csi_device_data[0],
	.flash_data		= &msm_camera_flash_none,
	.sensor_platform_info	= &msm_camera_semc_sensor_main_platform_info,
	.csi_if			= 1,
	.camera_type		= BACK_CAMERA_2D,
};
#endif

#ifdef CONFIG_SEMC_CAM_SUB_V4L2
static struct msm_camera_sensor_platform_info
		msm_camera_semc_sensor_sub_platform_info = {
	.mount_angle		= 90,
	.sensor_reset		= 18,
	.cam_vreg		= msm_camera_front_cam_vreg,
	.num_vreg		= ARRAY_SIZE(msm_camera_front_cam_vreg),
	.gpio_conf		= &msm_camera_front_cam_gpio_conf,
};

static struct msm_camera_sensor_info
		msm_camera_semc_sensor_sub_sensor_info = {
	.sensor_name		= "semc_sensor_sub",
	.sensor_reset		= 18,
	.pdata			= &msm_camera_csi_device_data[1],
	.flash_data		= &msm_camera_flash_none,
	.sensor_platform_info	= &msm_camera_semc_sensor_sub_platform_info,
	.csi_if			= 1,
	.camera_type		= FRONT_CAMERA_2D,
};
#endif

void __init msm8960_init_cam(void)
{
#if defined(CONFIG_SEMC_CAM_MAIN_V4L2) || defined(CONFIG_SEMC_CAM_SUB_V4L2)
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
#if defined(CONFIG_SEMC_VPE)
	platform_device_register(&semc_vpe_device);
#else
	platform_device_register(&msm8960_device_vpe);
#endif
#endif
}

#if defined(CONFIG_SEMC_CAM_MAIN_V4L2) || defined(CONFIG_SEMC_CAM_SUB_V4L2)
static struct i2c_board_info msm8960_camera_i2c_boardinfo[] = {
#ifdef CONFIG_SEMC_CAM_MAIN_V4L2
	{
		I2C_BOARD_INFO("semc_sensor_main", 0x1A),
		.platform_data = &msm_camera_semc_sensor_main_sensor_info,
	},
#endif
#ifdef CONFIG_SEMC_CAM_SUB_V4L2
	{
		I2C_BOARD_INFO("semc_sensor_sub", 0x48),
		.platform_data = &msm_camera_semc_sensor_sub_sensor_info,
	},
#endif
};

struct msm_camera_board_info msm8960_camera_board_info = {
	.board_info = msm8960_camera_i2c_boardinfo,
	.num_i2c_board_info = ARRAY_SIZE(msm8960_camera_i2c_boardinfo),
};

static const struct semc_sensor_seq sensor_main_power_off[] = {
	{ CAM_CLK, -1, 1 },
	{ GPIO_RESET, 0, 1 },
	{ CAM_VAF, -1, 0 },
	{ CAM_VANA, -1, 1 },
	{ CAM_VIO, -1, 1 },
	{ CAM_VDIG, -1, 15 },
	{ EXIT, 0, 0 },
};

static const struct semc_sensor_seq sensor_main_power_on[] = {
	{ CAM_VDIG, 1200, 1 },
	{ CAM_VIO, 0, 1 },
	{ CAM_VANA, 2800, 0 },
	{ CAM_VAF, 2700, 1 },
	{ GPIO_RESET, 1, 9 },
	{ CAM_CLK, 0, 1 },
	{ EXIT, 0, 0 },
};

const struct semc_sensor_module sensor_main_modules[] = {
	{ "GENERIC",  0x1A, sensor_main_power_on, sensor_main_power_off },
	{ "SOI13BS0", 0x1A, sensor_main_power_on, sensor_main_power_off },
	{ "SOI13BS1", 0x1A, sensor_main_power_on, sensor_main_power_off },
	{ "KMO13BS0", 0x1A, sensor_main_power_on, sensor_main_power_off },
};
const int semc_sensor_main_modules_len = ARRAY_SIZE(sensor_main_modules);

static const struct semc_sensor_seq sensor_sub_power_off[] = {
	{ GPIO_RESET, 0, 1 },
	{ CAM_CLK, -1, 1 },
	{ CAM_VANA, -1, 0 },
	{ CAM_VIO, -1, 0 },
	{ EXIT, 0, 0 },
};

static const struct semc_sensor_seq sensor_sub_power_on[] = {
	{ CAM_VIO, 0, 1 },
	{ CAM_VANA, 2800, 1 },
	{ CAM_CLK, 0, 1 },
	{ GPIO_RESET, 1, 200 },
	{ EXIT, 0, 0 },
};

const struct semc_sensor_module sensor_sub_modules[] = {
	{ "GENERIC",  0x48, sensor_sub_power_on, sensor_sub_power_off },
	{ "STW01BM0", 0x48, sensor_sub_power_on, sensor_sub_power_off },
	{ "APT01BM0", 0x48, sensor_sub_power_on, sensor_sub_power_off },
};
const int semc_sensor_sub_modules_len = ARRAY_SIZE(sensor_sub_modules);
const int semc_sensor_gpio_af_power_pin = 89;
const int semc_sensor_main_eeprom_addr = 0x50;
const int semc_sensor_sub_eeprom_addr = 0x48;
const int semc_sensor_main_subdev_code = V4L2_MBUS_FMT_SBGGR10_1X10;
const int semc_sensor_sub_subdev_code = V4L2_MBUS_FMT_SBGGR10_1X10;

#endif

#endif

int semc_usb_phy_init_seq[] = {
	0x39, 0x81, /* PARAMETER_OVERRIDE_B */
	0x24, 0x82, /* PARAMETER_OVERRIDE_C */
	-1
};

