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

#ifndef _MSM_H
#define _MSM_H

#ifdef __KERNEL__

/* Header files */
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/videodev2.h>
#include <linux/pm_qos.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-event.h>
#include <media/v4l2-mediabus.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-msm-mem.h>
#include <media/msm_isp.h>
#include <mach/camera.h>
#include <media/msm_isp.h>
#include <linux/ion.h>
#include <linux/iommu.h>
#include <media/msm_gestures.h>
#include <linux/wakelock.h>

#define MSM_V4L2_DIMENSION_SIZE 96
#define MAX_DEV_NAME_LEN 50

#define ERR_USER_COPY(to) pr_debug("%s(%d): copy %s user\n", \
				__func__, __LINE__, ((to) ? "to" : "from"))
#define ERR_COPY_FROM_USER() ERR_USER_COPY(0)
#define ERR_COPY_TO_USER() ERR_USER_COPY(1)

#define MSM_CSIPHY_DRV_NAME "msm_csiphy"
#define MSM_CSID_DRV_NAME "msm_csid"
#define MSM_CSIC_DRV_NAME "msm_csic"
#define MSM_ISPIF_DRV_NAME "msm_ispif"
#define MSM_VFE_DRV_NAME "msm_vfe"
#define MSM_VPE_DRV_NAME "msm_vpe"
#define MSM_GEMINI_DRV_NAME "msm_gemini"
#define MSM_I2C_MUX_DRV_NAME "msm_cam_i2c_mux"

#define MAX_NUM_CSIPHY_DEV 3
#define MAX_NUM_CSID_DEV 3
#define MAX_NUM_CSIC_DEV 3
#define MAX_NUM_ISPIF_DEV 1
#define MAX_NUM_VFE_DEV 2
#define MAX_NUM_AXI_DEV 2
#define MAX_NUM_VPE_DEV 1

enum msm_cam_subdev_type {
	CSIPHY_DEV,
	CSID_DEV,
	CSIC_DEV,
	ISPIF_DEV,
	VFE_DEV,
	AXI_DEV,
	VPE_DEV,
	SENSOR_DEV,
	ACTUATOR_DEV,
	EEPROM_DEV,
	GESTURE_DEV,
};

/* msm queue management APIs*/

#define msm_dequeue(queue, member) ({	   \
	unsigned long flags;		  \
	struct msm_device_queue *__q = (queue);	 \
	struct msm_queue_cmd *qcmd = 0;	   \
	spin_lock_irqsave(&__q->lock, flags);	 \
	if (!list_empty(&__q->list)) {		\
		__q->len--;		 \
		qcmd = list_first_entry(&__q->list,   \
		struct msm_queue_cmd, member);  \
		list_del_init(&qcmd->member);	 \
	}			 \
	spin_unlock_irqrestore(&__q->lock, flags);  \
	qcmd;			 \
})

#define msm_queue_drain(queue, member) do {	 \
	unsigned long flags;		  \
	struct msm_device_queue *__q = (queue);	 \
	struct msm_queue_cmd *qcmd;	   \
	spin_lock_irqsave(&__q->lock, flags);	 \
	while (!list_empty(&__q->list)) {	 \
		qcmd = list_first_entry(&__q->list,   \
			struct msm_queue_cmd, member);	\
			list_del_init(&qcmd->member);	 \
			free_qcmd(qcmd);		\
	 };			  \
	spin_unlock_irqrestore(&__q->lock, flags);	\
} while (0)

static inline void free_qcmd(struct msm_queue_cmd *qcmd)
{
	if (!qcmd || !atomic_read(&qcmd->on_heap))
		return;
	if (!atomic_sub_return(1, &qcmd->on_heap))
		kfree(qcmd);
}

struct isp_msg_stats {
	uint32_t    id;
	uint32_t    buffer;
	uint32_t    frameCounter;
};

struct msm_free_buf {
	uint8_t num_planes;
	uint32_t ch_paddr[VIDEO_MAX_PLANES];
	uint32_t vb;
};

struct isp_msg_event {
	uint32_t msg_id;
	uint32_t sof_count;
};

struct isp_msg_output {
	uint8_t   output_id;
	struct msm_free_buf buf;
	uint32_t  frameCounter;
};

/* message id for v4l2_subdev_notify*/
enum msm_camera_v4l2_subdev_notify {
	NOTIFY_ISP_MSG_EVT, /* arg = enum ISP_MESSAGE_ID */
	NOTIFY_VFE_MSG_OUT, /* arg = struct isp_msg_output */
	NOTIFY_VFE_MSG_STATS,  /* arg = struct isp_msg_stats */
	NOTIFY_VFE_MSG_COMP_STATS, /* arg = struct msm_stats_buf */
	NOTIFY_VFE_BUF_EVT, /* arg = struct msm_vfe_resp */
	NOTIFY_VPE_MSG_EVT,
	NOTIFY_PCLK_CHANGE, /* arg = pclk */
	NOTIFY_CSIPHY_CFG, /* arg = msm_camera_csiphy_params */
	NOTIFY_CSID_CFG, /* arg = msm_camera_csid_params */
	NOTIFY_CSIC_CFG, /* arg = msm_camera_csic_params */
	NOTIFY_VFE_BUF_FREE_EVT, /* arg = msm_camera_csic_params */
	NOTIFY_VFE_IRQ,
	NOTIFY_AXI_IRQ,
	NOTIFY_GESTURE_EVT, /* arg = v4l2_event */
	NOTIFY_GESTURE_CAM_EVT, /* arg = int */
	NOTIFY_INVALID
};

enum isp_vfe_cmd_id {
	/*
	*Important! Command_ID are arranged in order.
	*Don't change!*/
	ISP_VFE_CMD_ID_STREAM_ON,
	ISP_VFE_CMD_ID_STREAM_OFF,
	ISP_VFE_CMD_ID_FRAME_BUF_RELEASE
};

struct msm_cam_v4l2_device;
struct msm_cam_v4l2_dev_inst;
#define MSM_MAX_IMG_MODE                MSM_V4L2_EXT_CAPTURE_MODE_MAX

enum msm_buffer_state {
	MSM_BUFFER_STATE_UNUSED,
	MSM_BUFFER_STATE_INITIALIZED,
	MSM_BUFFER_STATE_PREPARED,
	MSM_BUFFER_STATE_QUEUED,
	MSM_BUFFER_STATE_RESERVED,
	MSM_BUFFER_STATE_DEQUEUED
};

/* buffer for one video frame */
struct msm_frame_buffer {
	/* common v4l buffer stuff -- must be first */
	struct vb2_buffer         vidbuf;
	struct list_head		  list;
	enum v4l2_mbus_pixelcode  pxlcode;
	enum msm_buffer_state state;
	int active;
};

struct msm_isp_color_fmt {
	char *name;
	int depth;
	int bitsperpxl;
	u32 fourcc;
	enum v4l2_mbus_pixelcode pxlcode;
	enum v4l2_colorspace colorspace;
};

struct msm_mctl_pp_frame_info {
	int user_cmd;
	struct msm_pp_frame src_frame;
	struct msm_pp_frame dest_frame;
	struct msm_mctl_pp_frame_cmd pp_frame_cmd;
};

struct msm_mctl_pp_ctrl {
	int pp_msg_type;
	struct msm_mctl_pp_frame_info *pp_frame_info;

};
struct msm_mctl_pp_info {
	spinlock_t lock;
	uint32_t cnt;
	uint32_t pp_key;
	uint32_t cur_frame_id[MSM_MAX_IMG_MODE];
	struct msm_free_buf div_frame[MSM_MAX_IMG_MODE];
	struct msm_mctl_pp_ctrl pp_ctrl;

};
/* "Media Controller" represents a camera steaming session,
 * which consists of a "sensor" device and an "isp" device
 * (such as VFE, if needed), connected via an "IO" device,
 * (such as IPIF on 8960, or none on 8660) plus other extra
 * sub devices such as VPE and flash.
 */

struct msm_cam_media_controller {

	int (*mctl_open)(struct msm_cam_media_controller *p_mctl,
					 const char *const apps_id);
	int (*mctl_cb)(void);
	int (*mctl_cmd)(struct msm_cam_media_controller *p_mctl,
					unsigned int cmd, unsigned long arg);
	int (*mctl_release)(struct msm_cam_media_controller *p_mctl);
	int (*mctl_buf_init)(struct msm_cam_v4l2_dev_inst *pcam);
	int (*mctl_vbqueue_init)(struct msm_cam_v4l2_dev_inst *pcam,
				struct vb2_queue *q, enum v4l2_buf_type type);
	int (*mctl_ufmt_init)(struct msm_cam_media_controller *p_mctl);

	/* the following reflect the HW topology information*/
	struct v4l2_subdev *sensor_sdev; /* sensor sub device */
	struct v4l2_subdev *act_sdev; /* actuator sub device */
	struct v4l2_subdev *csiphy_sdev; /*csiphy sub device*/
	struct v4l2_subdev *csid_sdev; /*csid sub device*/
	struct v4l2_subdev *csic_sdev; /*csid sub device*/
	struct v4l2_subdev *ispif_sdev; /* ispif sub device */
	struct v4l2_subdev *gemini_sdev; /* gemini sub device */
	struct v4l2_subdev *vpe_sdev; /* vpe sub device */
	struct v4l2_subdev *axi_sdev; /* axi sub device */
	struct v4l2_subdev *eeprom_sdev; /* eeprom sub device */

	struct msm_isp_ops *isp_sdev;    /* isp sub device : camif/VFE */
	struct msm_cam_config_dev *config_device;

	/*mctl session control information*/
	uint8_t opencnt; /*mctl ref count*/
	const char *apps_id; /*ID for app that open this session*/
	struct mutex lock;
	struct pm_qos_request idle_pm_qos; /*avoid low power mode when active*/
	struct pm_qos_request pm_qos_req_list;
	struct wake_lock suspend_lock;
	struct msm_mctl_pp_info pp_info;
	struct msm_mctl_stats_t stats_info; /*stats pmem info*/
	uint32_t vfe_output_mode; /* VFE output mode */
	struct ion_client *client;
	struct kref refcount;

	/*pcam ptr*/
	struct msm_cam_v4l2_device *pcam_ptr;

	/*sensor info*/
	struct msm_camera_sensor_info *sdata;

	/*IOMMU mapped IMEM addresses*/
	uint32_t ping_imem_y;
	uint32_t ping_imem_cbcr;
	uint32_t pong_imem_y;
	uint32_t pong_imem_cbcr;
};

/* abstract camera device represents a VFE and connected sensor */
struct msm_isp_ops {
	char *config_dev_name;

	/*int (*isp_init)(struct msm_cam_v4l2_device *pcam);*/
	int (*isp_open)(struct v4l2_subdev *sd,
		struct msm_cam_media_controller *mctl);
	int (*isp_config)(struct msm_cam_media_controller *pmctl,
		 unsigned int cmd, unsigned long arg);
	int (*isp_notify)(struct v4l2_subdev *sd,
		unsigned int notification, void *arg);
	void (*isp_release)(struct msm_cam_media_controller *mctl,
		struct v4l2_subdev *sd);
	int (*isp_pp_cmd)(struct msm_cam_media_controller *pmctl,
		 struct msm_mctl_pp_cmd, void *data);

	/* vfe subdevice */
	struct v4l2_subdev *sd;
	struct v4l2_subdev *sd_vpe;
};

struct msm_isp_buf_info {
	int type;
	unsigned long buffer;
	int fd;
};
struct msm_cam_buf_offset {
	uint32_t addr_offset;
	uint32_t data_offset;
};

#define MSM_DEV_INST_MAX                    16
struct msm_cam_v4l2_dev_inst {
	struct v4l2_fh  eventHandle;
	struct vb2_queue vid_bufq;
	spinlock_t vq_irqlock;
	struct list_head free_vq;
	struct v4l2_format vid_fmt;
	/* sensor pixel code*/
	enum v4l2_mbus_pixelcode sensor_pxlcode;
	struct msm_cam_v4l2_device *pcam;
	int my_index;
	int image_mode;
	int path;
	int buf_count;
	/* buffer offsets, if any */
	struct msm_cam_buf_offset **buf_offset;
	struct v4l2_crop crop;
	int streamon;
	struct msm_mem_map_info mem_map;
	int is_mem_map_inst;
	struct img_plane_info plane_info;
	int vbqueue_initialized;
	struct mutex inst_lock;
};

struct msm_cam_mctl_node {
	/* MCTL V4l2 device */
	struct v4l2_device v4l2_dev;
	struct video_device *pvdev;
	struct msm_cam_v4l2_dev_inst *dev_inst[MSM_DEV_INST_MAX];
	struct msm_cam_v4l2_dev_inst *dev_inst_map[MSM_MAX_IMG_MODE];
	struct mutex dev_lock;
	int active;
	int use_count;
};

/* abstract camera device for each sensor successfully probed*/
struct msm_cam_v4l2_device {

	/* device node information */
	int vnode_id;
	struct v4l2_device v4l2_dev; /* V4l2 device */
	struct video_device *pvdev; /* registered as /dev/video*/
	struct msm_cam_mctl_node mctl_node; /* node for buffer management */
	struct media_device media_dev; /* node to get video node info*/

	/* device session information */
	int use_count;
	struct mutex vid_lock;
	uint32_t server_queue_idx;
	uint32_t mctl_handle;
	struct msm_cam_v4l2_dev_inst *dev_inst[MSM_DEV_INST_MAX];
	struct msm_cam_v4l2_dev_inst *dev_inst_map[MSM_MAX_IMG_MODE];
	int op_mode;

	/* v4l2 format support */
	struct msm_isp_color_fmt *usr_fmts;
	int num_fmts;

	struct v4l2_subdev *sensor_sdev; /* sensor sub device */
	struct v4l2_subdev *act_sdev; /* actuator sub device */
	struct v4l2_subdev *eeprom_sdev; /* actuator sub device */
	struct msm_camera_sensor_info *sdata;
};

static inline struct msm_cam_v4l2_device *to_pcam(
	struct v4l2_device *v4l2_dev)
{
	return container_of(v4l2_dev, struct msm_cam_v4l2_device, v4l2_dev);
}

/*pseudo v4l2 device and v4l2 event queue
  for server and config cdevs*/
struct v4l2_queue_util {
	struct video_device *pvdev;
	struct v4l2_fh  eventHandle;
};

/* abstract config device for all sensor successfully probed*/
struct msm_cam_config_dev {
	struct cdev config_cdev;
	struct v4l2_queue_util config_stat_event_queue;
	int use_count;
	/*struct msm_isp_ops* isp_subdev;*/
	struct msm_cam_media_controller *p_mctl;
	struct msm_mem_map_info mem_map;
};

/* 2 for camera, 1 for gesture */
#define MAX_NUM_ACTIVE_CAMERA 3

struct msm_cam_server_queue {
	uint32_t queue_active;
	struct msm_device_queue ctrl_q;
	struct msm_device_queue eventData_q;
	uint8_t *ctrl_data;
	uint32_t evt_id;
};

struct msm_cam_server_mctl_inst {
	struct msm_cam_media_controller mctl;
	uint32_t handle;
};

/* abstract camera server device for all sensor successfully probed*/
struct msm_cam_server_dev {

	/* config node device*/
	struct platform_device *server_pdev;
	/* server node v4l2 device */
	struct v4l2_device v4l2_dev;
	struct video_device *video_dev;
	struct media_device media_dev;

	/* info of sensors successfully probed*/
	struct msm_camera_info camera_info;
	/* info of configs successfully created*/
	struct msm_cam_config_dev_info config_info;
	/* active working camera device - only one allowed at this time*/
	struct msm_cam_v4l2_device *pcam_active;
	/* number of camera devices opened*/
	atomic_t number_pcam_active;
	struct v4l2_queue_util server_command_queue;

	/* This queue used by the config thread to send responses back to the
	 * control thread.  It is accessed only from a process context.
	 */
	struct msm_cam_server_queue server_queue[MAX_NUM_ACTIVE_CAMERA];
	uint32_t server_evt_id;

	struct msm_cam_server_mctl_inst mctl[MAX_NUM_ACTIVE_CAMERA];
	uint32_t mctl_handle_cnt;

	int use_count;
	/* all the registered ISP subdevice*/
	struct msm_isp_ops *isp_subdev[MSM_MAX_CAMERA_CONFIGS];
	/* info of MCTL nodes successfully probed*/
	struct msm_mctl_node_info mctl_node_info;
	struct mutex server_lock;
	struct mutex server_queue_lock;
	/*v4l2 subdevs*/
	struct v4l2_subdev *csiphy_device[MAX_NUM_CSIPHY_DEV];
	struct v4l2_subdev *csid_device[MAX_NUM_CSID_DEV];
	struct v4l2_subdev *csic_device[MAX_NUM_CSIC_DEV];
	struct v4l2_subdev *ispif_device;
	struct v4l2_subdev *vfe_device[MAX_NUM_VFE_DEV];
	struct v4l2_subdev *axi_device[MAX_NUM_AXI_DEV];
	struct v4l2_subdev *vpe_device[MAX_NUM_VPE_DEV];
	struct v4l2_subdev *gesture_device;
};

/* camera server related functions */


/* ISP related functions */
void msm_isp_vfe_dev_init(struct v4l2_subdev *vd);
/*
int msm_isp_register(struct msm_cam_v4l2_device *pcam);
*/
int msm_isp_register(struct msm_cam_server_dev *psvr);
void msm_isp_unregister(struct msm_cam_server_dev *psvr);
int msm_sensor_register(struct v4l2_subdev *);
int msm_isp_init_module(int g_num_config_nodes);

int msm_mctl_init(struct msm_cam_v4l2_device *pcam);
int msm_mctl_free(struct msm_cam_v4l2_device *pcam);
int msm_mctl_buf_init(struct msm_cam_v4l2_device *pcam);
int msm_mctl_init_user_formats(struct msm_cam_v4l2_device *pcam);
int msm_mctl_buf_done(struct msm_cam_media_controller *pmctl,
			int msg_type, struct msm_free_buf *buf,
			uint32_t frame_id);
int msm_mctl_buf_done_pp(struct msm_cam_media_controller *pmctl,
	int msg_type, struct msm_free_buf *frame, int dirty, int node_type);
int msm_mctl_reserve_free_buf(struct msm_cam_media_controller *pmctl,
				struct msm_cam_v4l2_dev_inst *pcam_inst,
				int path, struct msm_free_buf *free_buf);
int msm_mctl_release_free_buf(struct msm_cam_media_controller *pmctl,
				struct msm_cam_v4l2_dev_inst *pcam_inst,
				int path, struct msm_free_buf *free_buf);
/*Memory(PMEM) functions*/
int msm_register_pmem(struct hlist_head *ptype, void __user *arg,
				struct ion_client *client);
int msm_pmem_table_del(struct hlist_head *ptype, void __user *arg,
				struct ion_client *client);
int msm_pmem_region_get_phy_addr(struct hlist_head *ptype,
	struct msm_mem_map_info *mem_map, int32_t *phyaddr);
uint8_t msm_pmem_region_lookup(struct hlist_head *ptype,
	int pmem_type, struct msm_pmem_region *reg, uint8_t maxcount);
uint8_t msm_pmem_region_lookup_2(struct hlist_head *ptype,
					int pmem_type,
					struct msm_pmem_region *reg,
					uint8_t maxcount);
unsigned long msm_pmem_stats_vtop_lookup(
				struct msm_cam_media_controller *mctl,
				unsigned long buffer,
				int fd);
unsigned long msm_pmem_stats_ptov_lookup(
	struct msm_cam_media_controller *mctl,
	unsigned long addr, int *fd);

int msm_vfe_subdev_init(struct v4l2_subdev *sd,
			struct msm_cam_media_controller *mctl);
void msm_vfe_subdev_release(struct v4l2_subdev *sd);

int msm_isp_subdev_ioctl(struct v4l2_subdev *sd,
	struct msm_vfe_cfg_cmd *cfgcmd, void *data);
int msm_vpe_subdev_init(struct v4l2_subdev *sd,
			struct msm_cam_media_controller *mctl);
int msm_gemini_subdev_init(struct v4l2_subdev *gemini_sd);
void msm_vpe_subdev_release(void);
void msm_gemini_subdev_release(struct v4l2_subdev *gemini_sd);
int msm_mctl_is_pp_msg_type(struct msm_cam_media_controller *p_mctl,
	int msg_type);
int msm_mctl_do_pp(struct msm_cam_media_controller *p_mctl,
			int msg_type, uint32_t y_phy, uint32_t frame_id);
int msm_mctl_pp_ioctl(struct msm_cam_media_controller *p_mctl,
			unsigned int cmd, unsigned long arg);
int msm_mctl_pp_notify(struct msm_cam_media_controller *pmctl,
			struct msm_mctl_pp_frame_info *pp_frame_info);
int msm_mctl_img_mode_to_inst_index(struct msm_cam_media_controller *pmctl,
					int out_type, int node_type);
struct msm_frame_buffer *msm_mctl_buf_find(
	struct msm_cam_media_controller *pmctl,
	struct msm_cam_v4l2_dev_inst *pcam_inst, int del_buf,
	int msg_type, struct msm_free_buf *fbuf);
void msm_mctl_gettimeofday(struct timeval *tv);
struct msm_frame_buffer *msm_mctl_get_free_buf(
		struct msm_cam_media_controller *pmctl,
		int msg_type);
int msm_mctl_put_free_buf(
		struct msm_cam_media_controller *pmctl,
		int msg_type, struct msm_frame_buffer *buf);
int msm_mctl_check_pp(struct msm_cam_media_controller *p_mctl,
		int msg_type, int *pp_divert_type, int *pp_type);
int msm_mctl_do_pp_divert(
	struct msm_cam_media_controller *p_mctl,
	int msg_type, struct msm_free_buf *fbuf,
	uint32_t frame_id, int pp_type);
int msm_mctl_buf_del(struct msm_cam_media_controller *pmctl,
	int msg_type,
	struct msm_frame_buffer *my_buf);
int msm_mctl_pp_release_free_frame(
	struct msm_cam_media_controller *p_mctl,
	void __user *arg);
int msm_mctl_pp_reserve_free_frame(
	struct msm_cam_media_controller *p_mctl,
	void __user *arg);
int msm_mctl_set_pp_key(struct msm_cam_media_controller *p_mctl,
				void __user *arg);
int msm_mctl_pp_done(
	struct msm_cam_media_controller *p_mctl,
	void __user *arg);
int msm_mctl_pp_divert_done(
	struct msm_cam_media_controller *p_mctl,
	void __user *arg);
int msm_setup_v4l2_event_queue(struct v4l2_fh *eventHandle,
					struct video_device *pvdev);
int msm_setup_mctl_node(struct msm_cam_v4l2_device *pcam);
struct msm_cam_v4l2_dev_inst *msm_mctl_get_pcam_inst(
		struct msm_cam_media_controller *pmctl,
		int image_mode);
int msm_mctl_buf_return_buf(struct msm_cam_media_controller *pmctl,
			int image_mode, struct msm_frame_buffer *buf);
int msm_mctl_pp_mctl_divert_done(struct msm_cam_media_controller *p_mctl,
					void __user *arg);
void msm_release_ion_client(struct kref *ref);
int msm_cam_register_subdev_node(struct v4l2_subdev *sd,
			enum msm_cam_subdev_type sdev_type, uint8_t index);
uint32_t msm_camera_get_mctl_handle(void);
struct msm_cam_media_controller *msm_camera_get_mctl(uint32_t handle);
void msm_camera_free_mctl(uint32_t handle);
int msm_server_open_client(int *p_qidx);
int msm_server_send_ctrl(struct msm_ctrl_cmd *out, int ctrl_id);
int msm_server_close_client(int idx);
int msm_cam_server_open_mctl_session(struct msm_cam_v4l2_device *pcam,
	int *p_active);
int msm_cam_server_close_mctl_session(struct msm_cam_v4l2_device *pcam);
#endif /* __KERNEL__ */

#endif /* _MSM_H */
