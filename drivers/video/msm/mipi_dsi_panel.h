/* drivers/video/msm/mipi_dsi_panel.h
 *
 * Copyright (C) 2010 Sony Ericsson Mobile Communications AB.
 * Copyright (C) 2012 Sony Mobile Communications AB.
 * Author: Yosuke Hatanaka <yosuke.hatanaka@sonyericsson.com>
 * Copyright (C) 2012 Sony Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */


#ifndef MIPI_DSI_PANEL_H
#define MIPI_DSI_PANEL_H

#include <linux/types.h>
#include "mipi_dsi.h"

#define ONE_FRAME_TRANSMIT_WAIT_MS 20

struct dsi_controller {
	struct msm_panel_info *(*get_panel_info) (void);
	struct dsi_cmd_desc *display_init_cmds;
	struct dsi_cmd_desc *display_on_eco_cmds;
	struct dsi_cmd_desc *display_on_cmds;
	struct dsi_cmd_desc *display_off_cmds;
	struct dsi_cmd_desc *read_id_cmds;
	struct dsi_cmd_desc *eco_mode_gamma_cmds;
	struct dsi_cmd_desc *normal_gamma_cmds;
	int display_init_cmds_size;
	int display_on_eco_cmds_size;
	int display_on_cmds_size;
	int display_off_cmds_size;
	int eco_mode_gamma_cmds_size;
	int normal_gamma_cmds_size;
};

struct mipi_dsi_lane_cfg {
	uint32_t	ln_cfg[4][3];
	uint32_t	ln_dpath[4];
	uint32_t	ln_str[4][2];
	uint32_t	lnck_cfg[3];
	uint32_t	lnck_dpath;
	uint32_t	lnck_str[2];
};

struct panel_id {
	const char			*name;
	struct dsi_controller	*pctrl;
	const u32			width;	/* in mm */
	const u32			height;	/* in mm */
	const char			*id;
	const int			id_num;
	const struct mipi_dsi_lane_cfg	*plncfg;
};

struct mipi_dsi_data {
	struct dsi_buf tx_buf;
	struct dsi_buf rx_buf;
	struct msm_fb_panel_data panel_data;
	const struct panel_id *panel;
	const struct panel_id **default_panels;
	const struct panel_id **panels;
	int (*lcd_power)(int on);
	int (*lcd_reset)(int on);
	int (*eco_mode_switch)(struct msm_fb_data_type *mfd);
#ifdef CONFIG_DEBUG_FS
	struct dentry *panel_driver_ic_dir;
	char *debug_buf;
#endif
	int panel_detecting;
	int eco_mode_on;
};

void mipi_dsi_set_default_panel(struct mipi_dsi_data *dsi_data);
struct msm_panel_info *mipi_dsi_detect_panel(
	struct msm_fb_data_type *mfd);
int __devinit mipi_dsi_need_detect_panel(
	const struct panel_id **panels);
int mipi_dsi_update_panel(struct platform_device *pdev);
int mipi_dsi_eco_mode_switch(struct msm_fb_data_type *mfd);
void mipi_dsi_update_lane_cfg(const struct mipi_dsi_lane_cfg *plncfg);

int eco_mode_sysfs_register(struct device *dev);

#ifdef CONFIG_DEBUG_FS
void mipi_dsi_debugfs_init(struct platform_device *pdev,
	const char *sub_name);
void mipi_dsi_debugfs_exit(struct platform_device *pdev);
#endif /* CONFIG_DEBUG_FS */

#endif /* MIPI_DSI_PANEL_H */
