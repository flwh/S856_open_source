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
#include <smem.h>
#include <err.h>
#include <msm_panel.h>
#include <mipi_dsi.h>
#include <pm8x41.h>
#include <pm8x41_wled.h>
#include <board.h>
#include <mdp5.h>
#include <endian.h>
#include <platform/gpio.h>
#include <platform/clock.h>
#include <platform/iomap.h>
#include <target/display.h>
#include "include/panel.h"
#include "include/display_resource.h"

#define HFPLL_LDO_ID 12

#define GPIO_STATE_LOW 0
#define GPIO_STATE_HIGH 2
#define RESET_GPIO_SEQ_LEN 3

static uint32_t dsi_pll_enable_seq(uint32_t ctl_base)
{
	uint32_t rc = 0;

	mdss_dsi_uniphy_pll_sw_reset(ctl_base);

	writel(0x01, ctl_base + 0x0220); /* GLB CFG */
	mdelay(1);
	writel(0x05, ctl_base + 0x0220); /* GLB CFG */
	mdelay(1);
	writel(0x07, ctl_base + 0x0220); /* GLB CFG */
	mdelay(1);
	writel(0x0f, ctl_base + 0x0220); /* GLB CFG */
	mdelay(1);

	mdss_dsi_uniphy_pll_lock_detect_setting(ctl_base);

	while (!(readl(ctl_base + 0x02c0) & 0x01)) {
		mdss_dsi_uniphy_pll_sw_reset(ctl_base);
		writel(0x01, ctl_base + 0x0220); /* GLB CFG */
		mdelay(1);
		writel(0x05, ctl_base + 0x0220); /* GLB CFG */
		mdelay(1);
		writel(0x07, ctl_base + 0x0220); /* GLB CFG */
		mdelay(1);
		writel(0x05, ctl_base + 0x0220); /* GLB CFG */
		mdelay(1);
		writel(0x07, ctl_base + 0x0220); /* GLB CFG */
		mdelay(1);
		writel(0x0f, ctl_base + 0x0220); /* GLB CFG */
		mdelay(2);
		mdss_dsi_uniphy_pll_lock_detect_setting(ctl_base);
	}
	return rc;
}

int target_backlight_ctrl(struct backlight *bl, uint8_t enable)
{
	struct pm8x41_gpio pwmgpio_param = {
		.direction = PM_GPIO_DIR_OUT,
		.function = PM_GPIO_FUNC_1,
		.vin_sel = 2,	/* VIN_2 */
		.pull = PM_GPIO_PULL_UP_1_5 | PM_GPIO_PULLDOWN_10,
		.output_buffer = PM_GPIO_OUT_CMOS,
		.out_strength = 0x03,
	};

	if (enable) {
		pm8x41_gpio_config(pwm_gpio.pin_id, &pwmgpio_param);

		/* lpg channel 2 */
		pm8x41_lpg_write(PWM_BL_LPG_CHAN_ID, 0x41, 0x33); /* LPG_PWM_SIZE_CLK, */
		pm8x41_lpg_write(PWM_BL_LPG_CHAN_ID, 0x42, 0x01); /* LPG_PWM_FREQ_PREDIV */
		pm8x41_lpg_write(PWM_BL_LPG_CHAN_ID, 0x43, 0x20); /* LPG_PWM_TYPE_CONFIG */
		pm8x41_lpg_write(PWM_BL_LPG_CHAN_ID, 0x44, 0xcc); /* LPG_VALUE_LSB */
		pm8x41_lpg_write(PWM_BL_LPG_CHAN_ID, 0x45, 0x00);  /* LPG_VALUE_MSB */
		pm8x41_lpg_write(PWM_BL_LPG_CHAN_ID, 0x46, 0xe4); /* LPG_ENABLE_CONTROL */
	} else {
		pm8x41_lpg_write(PWM_BL_LPG_CHAN_ID, 0x46, 0x0); /* LPG_ENABLE_CONTROL */
	}

	return NO_ERROR;
}

int target_panel_clock(uint8_t enable, struct msm_panel_info *pinfo)
{
	struct mdss_dsi_pll_config *pll_data;
	uint32_t dual_dsi = pinfo->mipi.dual_dsi;
	dprintf(SPEW, "target_panel_clock\n");

	pll_data = pinfo->mipi.dsi_pll_config;
	if (enable) {
		mdp_gdsc_ctrl(enable);
		mmss_bus_clock_enable();
		mdp_clock_enable();
		mdss_dsi_auto_pll_config(MIPI_DSI0_BASE, pll_data);
		dsi_pll_enable_seq(MIPI_DSI0_BASE);
		if (pinfo->mipi.dual_dsi &&
				!(pinfo->mipi.broadcast)) {
			mdss_dsi_auto_pll_config(MIPI_DSI1_BASE, pll_data);
			dsi_pll_enable_seq(MIPI_DSI1_BASE);
		}
		mmss_dsi_clock_enable(DSI0_PHY_PLL_OUT, dual_dsi,
					pll_data->pclk_m,
					pll_data->pclk_n,
					pll_data->pclk_d);
	} else if(!target_cont_splash_screen()) {
		/* Disable clocks if continuous splash off */
		mmss_dsi_clock_disable(dual_dsi);
		mdp_clock_disable();
		mmss_bus_clock_disable();
		mdp_gdsc_ctrl(enable);
	}

	return NO_ERROR;
}

/* Pull DISP_RST_N high to get panel out of reset */
int target_panel_reset(uint8_t enable, struct panel_reset_sequence *resetseq,
					struct msm_panel_info *pinfo)
{
	uint32_t i = 0;

	if (enable) {
		gpio_tlmm_config(reset_gpio.pin_id, 0,
				reset_gpio.pin_direction, reset_gpio.pin_pull,
				reset_gpio.pin_strength, reset_gpio.pin_state);

		gpio_tlmm_config(enable_gpio.pin_id, 0,
			enable_gpio.pin_direction, enable_gpio.pin_pull,
			enable_gpio.pin_strength, enable_gpio.pin_state);

		gpio_tlmm_config(bkl_gpio.pin_id, 0,
			bkl_gpio.pin_direction, bkl_gpio.pin_pull,
			bkl_gpio.pin_strength, bkl_gpio.pin_state);

		gpio_set(enable_gpio.pin_id, 2);
		gpio_set(bkl_gpio.pin_id, 2);
		/* reset */
		for (i = 0; i < RESET_GPIO_SEQ_LEN; i++) {
			if (resetseq->pin_state[i] == GPIO_STATE_LOW)
				gpio_set(reset_gpio.pin_id, GPIO_STATE_LOW);
			else
				gpio_set(reset_gpio.pin_id, GPIO_STATE_HIGH);
			mdelay(resetseq->sleep[i]);
		}
	} else {
		gpio_set(reset_gpio.pin_id, 0);
		gpio_set(enable_gpio.pin_id, 0);
		gpio_set(bkl_gpio.pin_id, 0);
	}

	return NO_ERROR;
}

int target_ldo_ctrl(uint8_t enable)
{
	uint32_t ldocounter = 0;
	uint32_t pm8x41_ldo_base = 0x13F00;

	while (ldocounter < TOTAL_LDO_DEFINED) {
		struct pm8x41_ldo ldo_entry = LDO((pm8x41_ldo_base +
			0x100 * ldo_entry_array[ldocounter].ldo_id),
			ldo_entry_array[ldocounter].ldo_type);

		dprintf(SPEW, "Setting %s\n",
				ldo_entry_array[ldocounter].ldo_id);

		/* Set voltage during power on */
		if (enable) {
			pm8x41_ldo_set_voltage(&ldo_entry,
					ldo_entry_array[ldocounter].ldo_voltage);
			pm8x41_ldo_control(&ldo_entry, enable);
		} else if(ldo_entry_array[ldocounter].ldo_id != HFPLL_LDO_ID) {
			pm8x41_ldo_control(&ldo_entry, enable);
		}
		ldocounter++;
	}

	return NO_ERROR;
}

int target_display_pre_on()
{
	writel(0x000000FA, MDP_QOS_REMAPPER_CLASS_0);
	writel(0x00000055, MDP_QOS_REMAPPER_CLASS_1);
	writel(0xC0000CCD, MDP_CLK_CTRL0);
	writel(0xD0000CCC, MDP_CLK_CTRL1);
	writel(0x00CCCCCC, MDP_CLK_CTRL2);
	writel(0x000000CC, MDP_CLK_CTRL6);
	writel(0x0CCCC0C0, MDP_CLK_CTRL3);
	writel(0xCCCCC0C0, MDP_CLK_CTRL4);
	writel(0xCCCCC0C0, MDP_CLK_CTRL5);
	writel(0x00CCC000, MDP_CLK_CTRL7);

	writel(0x00000001, VBIF_VBIF_DDR_FORCE_CLK_ON);
	writel(0x00080808, VBIF_VBIF_IN_RD_LIM_CONF0);
	writel(0x08000808, VBIF_VBIF_IN_RD_LIM_CONF1);
	writel(0x00080808, VBIF_VBIF_IN_RD_LIM_CONF2);
	writel(0x00000808, VBIF_VBIF_IN_RD_LIM_CONF3);
	writel(0x10000000, VBIF_VBIF_IN_WR_LIM_CONF0);
	writel(0x00100000, VBIF_VBIF_IN_WR_LIM_CONF1);
	writel(0x10000000, VBIF_VBIF_IN_WR_LIM_CONF2);
	writel(0x00000000, VBIF_VBIF_IN_WR_LIM_CONF3);
	writel(0x00013fff, VBIF_VBIF_ABIT_SHORT);
	writel(0x000000A4, VBIF_VBIF_ABIT_SHORT_CONF);
	writel(0x00003FFF, VBIF_VBIF_GATE_OFF_WRREQ_EN);
	writel(0x00000003, VBIF_VBIF_DDR_RND_RBN_QOS_ARB);

	return NO_ERROR;
}

void display_init(void)
{
	uint32_t ret = 0;
	ret = gcdb_display_init(MDP_REV_50, MIPI_FB_ADDR);
	if (ret) {
		msm_display_off();
	}
}

void display_shutdown(void)
{
	gcdb_display_shutdown();
}
