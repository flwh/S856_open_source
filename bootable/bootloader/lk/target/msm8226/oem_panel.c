/* Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of The Linux Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <debug.h>
#include <err.h>
#include <smem.h>
#include <msm_panel.h>
#include <board.h>
#include <mipi_dsi.h>
#include <platform/gpio.h>
#include <pm8x41.h>

#include "include/panel.h"
#include "panel_display.h"

/*---------------------------------------------------------------------------*/
/* GCDB Panel Database                                                       */
/*---------------------------------------------------------------------------*/
#include "include/panel_toshiba_720p_video.h"
#include "include/panel_nt35590_720p_video.h"
#include "include/panel_nt35590_720p_cmd.h"
#include "include/panel_hx8394a_720p_video.h"
#include "include/panel_nt35596_1080p_video.h"
#include "include/panel_nt35521_720p_video.h"
#include "include/panel_ssd2080m_720p_video.h"
#include "include/panel_jdi_1080p_video.h"
#include "include/lenovo_boe_panel_nt35521_720p_video.h"
#include "include/lenovo_tianma_panel_nt35521_720p_video.h"

#define DISPLAY_MAX_PANEL_DETECTION 2

#define SSD2080M_720P_VIDEO_PANEL_ON_DELAY 200

/*---------------------------------------------------------------------------*/
/* static panel selection variable                                           */
/*---------------------------------------------------------------------------*/
enum {
TOSHIBA_720P_VIDEO_PANEL,
NT35590_720P_CMD_PANEL,
NT35590_720P_VIDEO_PANEL,
NT35596_1080P_VIDEO_PANEL,
HX8394A_720P_VIDEO_PANEL,
NT35521_720P_VIDEO_PANEL,
SSD2080M_720P_VIDEO_PANEL,
NT35521_TIANMA_720P_VIDEO_PANEL,
NT35521_BOE_720P_VIDEO_PANEL,
JDI_1080P_VIDEO_PANEL,
UNKNOWN_PANEL
};

enum target_subtype {
	HW_PLATFORM_SUBTYPE_720P = 0,
	HW_PLATFORM_SUBTYPE_SKUAA = 1,
	HW_PLATFORM_SUBTYPE_SKUF = 2,
	HW_PLATFORM_SUBTYPE_1080P = 2,
	HW_PLATFORM_SUBTYPE_SKUAB = 3,
	HW_PLATFORM_SUBTYPE_1080P_EXT_BUCK = 3,
	HW_PLATFORM_SUBTYPE_SKUG = 5,
};

static uint32_t panel_id;

int oem_panel_rotation()
{
	int ret = NO_ERROR;
	switch (panel_id) {
	case TOSHIBA_720P_VIDEO_PANEL:
		ret = mipi_dsi_cmds_tx(toshiba_720p_video_rotation,
				TOSHIBA_720P_VIDEO_ROTATION);
		break;
	case NT35590_720P_CMD_PANEL:
		ret = mipi_dsi_cmds_tx(nt35590_720p_cmd_rotation,
				NT35590_720P_CMD_ROTATION);
		break;
	case NT35590_720P_VIDEO_PANEL:
		ret = mipi_dsi_cmds_tx(nt35590_720p_video_rotation,
				NT35590_720P_VIDEO_ROTATION);
		break;
	}

	return ret;
}

int oem_panel_on()
{
	/* OEM can keep there panel spefic on instructions in this
	function */
	if (panel_id == SSD2080M_720P_VIDEO_PANEL) {
		/* SSD2080M needs extra delay to avoid unexpected artifacts */
		mdelay(SSD2080M_720P_VIDEO_PANEL_ON_DELAY);
	}
	return NO_ERROR;
}

int oem_panel_off()
{
	/* OEM can keep there panel spefic off instructions in this
	function */
	return NO_ERROR;
}

static void mdss_source_pipe_select(struct msm_panel_info *pinfo)
{
	/* Use DMA pipe for splash logo on 8x26 */
	pinfo->use_dma_pipe = 1;
}

static void init_panel_data(struct panel_struct *panelstruct,
			struct msm_panel_info *pinfo,
			struct mdss_dsi_phy_ctrl *phy_db)
{
	switch (panel_id) {
	case TOSHIBA_720P_VIDEO_PANEL:
		panelstruct->paneldata    = &toshiba_720p_video_panel_data;
		panelstruct->panelres     = &toshiba_720p_video_panel_res;
		panelstruct->color        = &toshiba_720p_video_color;
		panelstruct->videopanel   = &toshiba_720p_video_video_panel;
		panelstruct->commandpanel = &toshiba_720p_video_command_panel;
		panelstruct->state        = &toshiba_720p_video_state;
		panelstruct->laneconfig   = &toshiba_720p_video_lane_config;
		panelstruct->paneltiminginfo
					 = &toshiba_720p_video_timing_info;
		panelstruct->panelresetseq
					 = &toshiba_720p_video_panel_reset_seq;
		panelstruct->backlightinfo = &toshiba_720p_video_backlight;
		pinfo->mipi.panel_cmds
					= toshiba_720p_video_on_command;
		pinfo->mipi.num_of_panel_cmds
					= TOSHIBA_720P_VIDEO_ON_COMMAND;
		memcpy(phy_db->timing,
			toshiba_720p_video_timings, TIMING_SIZE);
		break;
	case NT35590_720P_VIDEO_PANEL:
		panelstruct->paneldata    = &nt35590_720p_video_panel_data;
		panelstruct->panelres     = &nt35590_720p_video_panel_res;
		panelstruct->color        = &nt35590_720p_video_color;
		panelstruct->videopanel   = &nt35590_720p_video_video_panel;
		panelstruct->commandpanel = &nt35590_720p_video_command_panel;
		panelstruct->state        = &nt35590_720p_video_state;
		panelstruct->laneconfig   = &nt35590_720p_video_lane_config;
		panelstruct->paneltiminginfo
					 = &nt35590_720p_video_timing_info;
		panelstruct->panelresetseq
					 = &nt35590_720p_video_panel_reset_seq;
		panelstruct->backlightinfo = &nt35590_720p_video_backlight;
		pinfo->mipi.panel_cmds
					= nt35590_720p_video_on_command;
		pinfo->mipi.num_of_panel_cmds
					= NT35590_720P_VIDEO_ON_COMMAND;
		memcpy(phy_db->timing,
				nt35590_720p_video_timings, TIMING_SIZE);
		break;
	case NT35521_720P_VIDEO_PANEL:
		panelstruct->paneldata    = &nt35521_720p_video_panel_data;
		panelstruct->panelres     = &nt35521_720p_video_panel_res;
		panelstruct->color        = &nt35521_720p_video_color;
		panelstruct->videopanel   = &nt35521_720p_video_video_panel;
		panelstruct->commandpanel = &nt35521_720p_video_command_panel;
		panelstruct->state        = &nt35521_720p_video_state;
		panelstruct->laneconfig   = &nt35521_720p_video_lane_config;
		panelstruct->paneltiminginfo
					 = &nt35521_720p_video_timing_info;
		panelstruct->panelresetseq
					 = &nt35521_720p_video_panel_reset_seq;
		panelstruct->backlightinfo = &nt35521_720p_video_backlight;
		pinfo->mipi.panel_cmds
					= nt35521_720p_video_on_command;
		pinfo->mipi.num_of_panel_cmds
					= NT35521_720P_VIDEO_ON_COMMAND;
		memcpy(phy_db->timing,
				nt35521_720p_video_timings, TIMING_SIZE);
		break;
	case SSD2080M_720P_VIDEO_PANEL:
		panelstruct->paneldata    = &ssd2080m_720p_video_panel_data;
		panelstruct->panelres     = &ssd2080m_720p_video_panel_res;
		panelstruct->color        = &ssd2080m_720p_video_color;
		panelstruct->videopanel   = &ssd2080m_720p_video_video_panel;
		panelstruct->commandpanel = &ssd2080m_720p_video_command_panel;
		panelstruct->state        = &ssd2080m_720p_video_state;
		panelstruct->laneconfig   = &ssd2080m_720p_video_lane_config;
		panelstruct->paneltiminginfo
					 = &ssd2080m_720p_video_timing_info;
		panelstruct->panelresetseq
					 = &ssd2080m_720p_video_panel_reset_seq;
		panelstruct->backlightinfo = &ssd2080m_720p_video_backlight;
		pinfo->mipi.panel_cmds
					= ssd2080m_720p_video_on_command;
		pinfo->mipi.num_of_panel_cmds
					= SSD2080M_720P_VIDEO_ON_COMMAND;
		memcpy(phy_db->timing,
				ssd2080m_720p_video_timings, TIMING_SIZE);
		break;
	case NT35521_BOE_720P_VIDEO_PANEL:
		panelstruct->paneldata    = &nt35521_boe_720p_video_panel_data;
		panelstruct->panelres     = &nt35521_boe_720p_video_panel_res;
		panelstruct->color        = &nt35521_boe_720p_video_color;
		panelstruct->videopanel   = &nt35521_boe_720p_video_video_panel;
		panelstruct->commandpanel = &nt35521_boe_720p_video_command_panel;
		panelstruct->state        = &nt35521_boe_720p_video_state;
		panelstruct->laneconfig   = &nt35521_boe_720p_video_lane_config;
		panelstruct->paneltiminginfo
					 = &nt35521_boe_720p_video_timing_info;
		panelstruct->panelresetseq
					 = &nt35521_boe_720p_video_panel_reset_seq;
		panelstruct->backlightinfo = &nt35521_boe_720p_video_backlight;
		pinfo->mipi.panel_cmds
					= nt35521_boe_720p_video_on_command;
		pinfo->mipi.num_of_panel_cmds
					= BOE_NT35521_720P_VIDEO_ON_COMMAND;
		memcpy(phy_db->timing,
				nt35521_boe_720p_video_timings, TIMING_SIZE);
		break;
	case NT35521_TIANMA_720P_VIDEO_PANEL:
		panelstruct->paneldata    = &nt35521_tianma_720p_video_panel_data;
		panelstruct->panelres     = &nt35521_tianma_720p_video_panel_res;
		panelstruct->color        = &nt35521_tianma_720p_video_color;
		panelstruct->videopanel   = &nt35521_tianma_720p_video_video_panel;
		panelstruct->commandpanel = &nt35521_tianma_720p_video_command_panel;
		panelstruct->state        = &nt35521_tianma_720p_video_state;
		panelstruct->laneconfig   = &nt35521_tianma_720p_video_lane_config;
		panelstruct->paneltiminginfo
					 = &nt35521_tianma_720p_video_timing_info;
		panelstruct->panelresetseq
					 = &nt35521_tianma_720p_video_panel_reset_seq;
		panelstruct->backlightinfo = &nt35521_tianma_720p_video_backlight;
		pinfo->mipi.panel_cmds
					= nt35521_tianma_720p_video_on_command;
		pinfo->mipi.num_of_panel_cmds
					= TIANMA_NT35521_720P_VIDEO_ON_COMMAND;
		memcpy(phy_db->timing,
				nt35521_tianma_720p_video_timings, TIMING_SIZE);
		break;
	case HX8394A_720P_VIDEO_PANEL:
		panelstruct->paneldata    = &hx8394a_720p_video_panel_data;
		panelstruct->panelres     = &hx8394a_720p_video_panel_res;
		panelstruct->color        = &hx8394a_720p_video_color;
		panelstruct->videopanel   = &hx8394a_720p_video_video_panel;
		panelstruct->commandpanel = &hx8394a_720p_video_command_panel;
		panelstruct->state        = &hx8394a_720p_video_state;
		panelstruct->laneconfig   = &hx8394a_720p_video_lane_config;
		panelstruct->paneltiminginfo
					 = &hx8394a_720p_video_timing_info;
		panelstruct->panelresetseq
					 = &hx8394a_720p_video_panel_reset_seq;
		panelstruct->backlightinfo = &hx8394a_720p_video_backlight;
		pinfo->mipi.panel_cmds
					= hx8394a_720p_video_on_command;
		pinfo->mipi.num_of_panel_cmds
					= HX8394A_720P_VIDEO_ON_COMMAND;
		memcpy(phy_db->timing,
				hx8394a_720p_video_timings, TIMING_SIZE);
		pinfo->mipi.signature = HX8394A_720P_VIDEO_SIGNATURE;
		break;

	case NT35590_720P_CMD_PANEL:
		panelstruct->paneldata    = &nt35590_720p_cmd_panel_data;
		panelstruct->panelres     = &nt35590_720p_cmd_panel_res;
		panelstruct->color        = &nt35590_720p_cmd_color;
		panelstruct->videopanel   = &nt35590_720p_cmd_video_panel;
		panelstruct->commandpanel = &nt35590_720p_cmd_command_panel;
		panelstruct->state        = &nt35590_720p_cmd_state;
		panelstruct->laneconfig   = &nt35590_720p_cmd_lane_config;
		panelstruct->paneltiminginfo = &nt35590_720p_cmd_timing_info;
		panelstruct->panelresetseq
					= &nt35590_720p_cmd_panel_reset_seq;
		panelstruct->backlightinfo = &nt35590_720p_cmd_backlight;
		pinfo->mipi.panel_cmds
					= nt35590_720p_cmd_on_command;
		pinfo->mipi.num_of_panel_cmds
					= NT35590_720P_CMD_ON_COMMAND;
		memcpy(phy_db->timing,
				nt35590_720p_cmd_timings, TIMING_SIZE);
		break;
	case NT35596_1080P_VIDEO_PANEL:
		panelstruct->paneldata    = &nt35596_1080p_video_panel_data;
		panelstruct->panelres     = &nt35596_1080p_video_panel_res;
		panelstruct->color        = &nt35596_1080p_video_color;
		panelstruct->videopanel   = &nt35596_1080p_video_video_panel;
		panelstruct->commandpanel = &nt35596_1080p_video_command_panel;
		panelstruct->state        = &nt35596_1080p_video_state;
		panelstruct->laneconfig   = &nt35596_1080p_video_lane_config;
		panelstruct->paneltiminginfo
					= &nt35596_1080p_video_timing_info;
		panelstruct->panelresetseq
					= &nt35596_1080p_video_panel_reset_seq;
		panelstruct->backlightinfo
					= &nt35596_1080p_video_backlight;
		pinfo->mipi.panel_cmds
					= nt35596_1080p_video_on_command;
		pinfo->mipi.num_of_panel_cmds
					= NT35596_1080P_VIDEO_ON_COMMAND;
		memcpy(phy_db->timing,
				nt35596_1080p_video_timings, TIMING_SIZE);
		pinfo->mipi.signature = NT35596_1080P_VIDEO_SIGNATURE;
		break;
	case JDI_1080P_VIDEO_PANEL:
		panelstruct->paneldata    = &jdi_1080p_video_panel_data;
		panelstruct->paneldata->panel_with_enable_gpio = 1;
		panelstruct->panelres     = &jdi_1080p_video_panel_res;
		panelstruct->color        = &jdi_1080p_video_color;
		panelstruct->videopanel   = &jdi_1080p_video_video_panel;
		panelstruct->commandpanel = &jdi_1080p_video_command_panel;
		panelstruct->state        = &jdi_1080p_video_state;
		panelstruct->laneconfig   = &jdi_1080p_video_lane_config;
		panelstruct->paneltiminginfo
			= &jdi_1080p_video_timing_info;
		panelstruct->panelresetseq
					 = &jdi_1080p_video_panel_reset_seq;
		panelstruct->backlightinfo = &jdi_1080p_video_backlight;
		pinfo->mipi.panel_cmds
			= jdi_1080p_video_on_command;
		pinfo->mipi.num_of_panel_cmds
			= JDI_1080P_VIDEO_ON_COMMAND;
		memcpy(phy_db->timing,
			jdi_1080p_video_timings, TIMING_SIZE);
		break;
        case UNKNOWN_PANEL:
                memset(panelstruct, 0, sizeof(struct panel_struct));
                memset(pinfo->mipi.panel_cmds, 0, sizeof(struct mipi_dsi_cmd));
                pinfo->mipi.num_of_panel_cmds = 0;
                memset(phy_db->timing, 0, TIMING_SIZE);
                pinfo->mipi.signature = 0;
                break;
	}
}

uint32_t oem_panel_max_auto_detect_panels()
{
        return target_panel_auto_detect_enabled() ?
                        DISPLAY_MAX_PANEL_DETECTION : 0;
}

uint8_t get_lcd_id()
{
	uint8_t mpp_status; 
	struct pm8x41_mpp mpp; 
	mpp.base = 0xA600; 
	mpp.mode = MPP_HIGH; 
	mpp.vin = MPP_VIN3; 
	pm8x41_config_input_mpp(&mpp); 
	pm8x41_mpp_get_under_input(mpp.base, &mpp_status);
	return mpp_status;
}

static uint32_t auto_pan_loop = 0;

bool oem_panel_select(struct panel_struct *panelstruct,
			struct msm_panel_info *pinfo,
			struct mdss_dsi_phy_ctrl *phy_db)
{
	uint32_t hw_id = board_hardware_id();
	uint32_t target_id = board_target_id();
	uint32_t nt35590_panel_id = NT35590_720P_VIDEO_PANEL;
	uint32_t hw_subtype = board_hardware_subtype();
	uint8_t pin_status = 2;
	bool ret = true;

#if DISPLAY_TYPE_CMD_MODE
	nt35590_panel_id = NT35590_720P_CMD_PANEL;
#endif

	switch (hw_id) {
	case HW_PLATFORM_QRD:
		if (hw_subtype == HW_PLATFORM_SUBTYPE_SKUF) {
			panel_id = NT35521_720P_VIDEO_PANEL;
		} else if (hw_subtype == HW_PLATFORM_SUBTYPE_SKUG) {
			gpio_tlmm_config(52, 1, GPIO_INPUT, GPIO_PULL_UP, GPIO_2MA, GPIO_ENABLE);
			mdelay(10);
			pin_status = get_lcd_id();
			if( pin_status == 1){
				panel_id = NT35521_TIANMA_720P_VIDEO_PANEL;
				dprintf(INFO, "oem_panel_select NT35521_TIANMA_720P_VIDEO_PANEL, IDpin level=%x\n", pin_status);
			}
			else{
				panel_id = NT35521_BOE_720P_VIDEO_PANEL;
				dprintf(INFO, "oem_panel_select NT35521_BOE_720P_VIDEO_PANEL, IDpin level=%x\n", pin_status);
			}
		} else {
			if (((target_id >> 16) & 0xFF) == 0x1 || ((target_id >> 16) & 0xFF) == 0x3) //EVT || PVT
				panel_id = nt35590_panel_id;
			else if (((target_id >> 16) & 0xFF) == 0x2) { //DVT
				panel_id = HX8394A_720P_VIDEO_PANEL;
				switch (auto_pan_loop) {
					case 0:
						panel_id = HX8394A_720P_VIDEO_PANEL;
						break;
					case 1:
						panel_id = NT35596_1080P_VIDEO_PANEL;
						break;
					default:
						panel_id = UNKNOWN_PANEL;
						ret = false;
						dprintf(CRITICAL, "Unknown panel\n");
						return ret;
				}
				auto_pan_loop++;
			}
			else {
				dprintf(CRITICAL, "Not supported device, target_id=%x\n"
									, target_id);
				return false;
			}
		}
		break;
	case HW_PLATFORM_MTP:
	case HW_PLATFORM_SURF:
		if ((hw_subtype == HW_PLATFORM_SUBTYPE_1080P) ||
			(hw_subtype == HW_PLATFORM_SUBTYPE_1080P_EXT_BUCK))
			panel_id = JDI_1080P_VIDEO_PANEL;
		else
			panel_id = nt35590_panel_id;
		break;
	default:
		dprintf(CRITICAL, "Display not enabled for %d HW type\n"
								, hw_id);
		return false;
	}

	init_panel_data(panelstruct, pinfo, phy_db);
	mdss_source_pipe_select(pinfo);

	return ret;
}
