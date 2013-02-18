/* drivers/video/msm/mipi_s6d6aa0_panels/mipi_sony_acx451akm.c
 *
 * Copyright (C) [2011] Sony Ericsson Mobile Communications AB.
 * Copyright (C) 2012 Sony Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2; as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "../msm_fb.h"
#include "../mipi_dsi.h"
#include "../mipi_dsi_panel.h"

/* Display ON Sequence */
static char exit_sleep[] = {
	0x11
};
static char nvm_reg_access_enable1[] = {
	0xF0, 0x5A, 0x5A
};
static char nvm_reg_access_enable2[] = {
	0xF1, 0x5A, 0x5A
};
static char p_gamma_ctl[] = {
	0xFA,
	/* R */
	0x25, 0x36, 0x0C, 0x01, 0x0A, 0x08, 0x00, 0x00,
	0x03, 0x05, 0x05, 0x16, 0x1D, 0x21, 0x26, 0x2D,
	0x33, 0x38, 0x39, 0x3D, 0x23,
	/* G */
	0x16, 0x38, 0x17, 0x10, 0x1B, 0x1A, 0x12, 0x11,
	0x14, 0x14, 0x11, 0x1E, 0x24, 0x27, 0x2B, 0x30,
	0x33, 0x36, 0x35, 0x37, 0x1D,
	/* B */
	0x1C, 0x38, 0x10, 0x05, 0x0D, 0x0B, 0x03, 0x03,
	0x07, 0x09, 0x08, 0x17, 0x1F, 0x23, 0x29, 0x2D,
	0x2F, 0x32, 0x32, 0x3B, 0x24,
};
static char n_gamma_ctl[] = {
	0xFB,
	/* R */
	0x25, 0x36, 0x0C, 0x01, 0x0A, 0x08, 0x00, 0x00,
	0x03, 0x05, 0x05, 0x16, 0x1D, 0x21, 0x26, 0x2D,
	0x33, 0x38, 0x39, 0x3D, 0x23,
	/* G */
	0x16, 0x38, 0x17, 0x10, 0x1B, 0x1A, 0x12, 0x11,
	0x14, 0x14, 0x11, 0x1E, 0x24, 0x27, 0x2B, 0x30,
	0x33, 0x36, 0x35, 0x37, 0x1D,
	/* B */
	0x1C, 0x38, 0x10, 0x05, 0x0D, 0x0B, 0x03, 0x03,
	0x07, 0x09, 0x08, 0x17, 0x1F, 0x23, 0x29, 0x2D,
	0x2F, 0x32, 0x32, 0x3B, 0x24,
};
static char nvm_reg_access_disable1[] = {
	0xF0, 0xA5, 0xA5
};
static char nvm_reg_access_disable2[] = {
	0xF1, 0xA5, 0xA5
};
static char display_scan_ctrl[] = {
	0x36, 0x80
};
static char display_on[] = {
	0x29
};

/* Display OFF Sequence */
static char display_off[] = {
	0x28
};
static char enter_sleep[] = {
	0x10
};

/* Reading DDB Sequence */
static char read_ddb_start[] = {
	0xA1, 0x00
};

static struct dsi_cmd_desc sony_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 140,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0,
		sizeof(nvm_reg_access_enable1), nvm_reg_access_enable1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0,
		sizeof(nvm_reg_access_enable2), nvm_reg_access_enable2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0,
		sizeof(p_gamma_ctl), p_gamma_ctl},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0,
		sizeof(n_gamma_ctl), n_gamma_ctl},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0,
		sizeof(nvm_reg_access_disable1), nvm_reg_access_disable1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0,
		sizeof(nvm_reg_access_disable2), nvm_reg_access_disable2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(display_scan_ctrl), display_scan_ctrl},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc sony_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 130,
		sizeof(enter_sleep), enter_sleep}
};

static struct dsi_cmd_desc read_ddb_start_cmds[] = {
	{DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(read_ddb_start), read_ddb_start},
};

static const struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db[] = {
	/* 720*1280, RGB888, 4 Lane 60 fps video mode */
	{
		/* regulator */
		{0x03, 0x0a, 0x04, 0x00, 0x20},
		/* timing */
		{0x7b, 0x1b, 0x12, 0x00, 0x40, 0x49, 0x17, 0x1e,
		 0x1e, 0x03, 0x04, 0xa0},
		/* phy ctrl */
		{0x5f, 0x00, 0x00, 0x10},
		/* strength */
		{0xff, 0x00, 0x06, 0x00},
		/* pll control */
		{0x01, 0x9e, 0x31, 0xd9, 0x00, 0x50, 0x48, 0x63,
		 0x41, 0x0f, 0x03,
		 0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
	},
};
static const struct mipi_dsi_lane_cfg  lncfg = {
	/* DSI1_DSIPHY_LN_CFG */
	.ln_cfg = {
		{0x80, 0xEF, 0x00},
		{0x80, 0xEF, 0x00},
		{0x80, 0xEF, 0x00},
		{0x80, 0xEF, 0x00}
	},
	/* DSI1_DSIPHY_LN_TEST_DATAPATH */
	.ln_dpath = {0x00, 0x00, 0x00, 0x00},
	/* DSI1_DSIPHY_LN_TEST_STR */
	.ln_str = {
		{0x01, 0x66},
		{0x01, 0x66},
		{0x01, 0x66},
		{0x01, 0x66}
	},
	/* DSI1_DSIPHY_LNCK_CFG */
	.lnck_cfg = {0x40, 0xEF, 0x00},
	/* DSI1_DSIPHY_LNCK_TEST_DATAPATH */
	.lnck_dpath = 0x0,
	/* DSI1_DSIPHY_LNCK_TEST_STR */
	.lnck_str = {0x01, 0x88},
};

static struct msm_panel_info pinfo;

static struct msm_panel_info *get_panel_info(void)
{
	/* should fix porch, pulse widht and so on */
	pinfo.xres = 720;
	pinfo.yres = 1280;
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.lcdc.h_back_porch = 64;
	pinfo.lcdc.h_front_porch = 64;
	pinfo.lcdc.h_pulse_width = 16;
	pinfo.lcdc.v_back_porch = 51;
	pinfo.lcdc.v_front_porch = 51;
	pinfo.lcdc.v_pulse_width = 4;
	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 15;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	pinfo.clk_rate = 431000000;

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.pulse_mode_hsa_he = TRUE;
	pinfo.mipi.hfp_power_stop = FALSE;
	pinfo.mipi.hbp_power_stop = FALSE;
	pinfo.mipi.hsa_power_stop = FALSE;
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
	pinfo.mipi.traffic_mode = DSI_NON_BURST_SYNCH_EVENT;
	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.dlane_swap = 0x01;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.r_sel = 0;
	pinfo.mipi.g_sel = 0;
	pinfo.mipi.b_sel = 0;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.data_lane3 = TRUE;
	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.t_clk_post = 0x04;
	pinfo.mipi.t_clk_pre = 0x1b;
	pinfo.mipi.esc_byte_ratio = 4;
	pinfo.mipi.stream = 0; /* dma_p */
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.force_clk_lane_hs = 1;
	pinfo.mipi.frame_rate  = 60;
	pinfo.mipi.dsi_phy_db =
		(struct mipi_dsi_phy_ctrl *)dsi_video_mode_phy_db;

	return &pinfo;
}

static struct dsi_controller dsi_video_controller_panel = {
	.get_panel_info        = get_panel_info,
	.display_on_cmds       = sony_display_on_cmds,
	.display_off_cmds      = sony_display_off_cmds,
	.read_id_cmds          = read_ddb_start_cmds,
	.display_on_cmds_size  = ARRAY_SIZE(sony_display_on_cmds),
	.display_off_cmds_size = ARRAY_SIZE(sony_display_off_cmds),
};

static char ddb_val_1a[] = {
	0x43, 0x91, 0x1a, 0x01, 0xff
};

static char ddb_val[] = {
	0x43, 0x91, 0xff, 0xff, 0xff
};

static char default_ddb_val[] = {
	0x43, 0x91
};

const struct panel_id sony_acx451akm_panel_id_1a = {
	.name = "mipi_video_sony_wxga_acx451akm_id_1a",
	.pctrl = &dsi_video_controller_panel,
	.id = ddb_val_1a,
	.id_num = ARRAY_SIZE(ddb_val_1a),
	.width = 53,
	.height = 95,
	.plncfg = &lncfg,
};

const struct panel_id sony_acx451akm_panel_id = {
	.name = "mipi_video_sony_wxga_acx451akm",
	.pctrl = &dsi_video_controller_panel,
	.id = ddb_val,
	.id_num = ARRAY_SIZE(ddb_val),
	.width = 53,
	.height = 95,
	.plncfg = &lncfg,
};

const struct panel_id sony_acx451akm_panel_default = {
	.name = "mipi_sony_panel",
	.pctrl = &dsi_video_controller_panel,
	.id = default_ddb_val,
	.id_num = ARRAY_SIZE(default_ddb_val),
	.width = 53,
	.height = 95,
	.plncfg = &lncfg,
};
