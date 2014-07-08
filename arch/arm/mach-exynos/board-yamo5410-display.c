/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/gpio.h>
#include <linux/pwm_backlight.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/clk.h>

#include <video/platform_lcd.h>
#include <video/s5p-dp.h>

#include <plat/cpu.h>
#include <plat/clock.h>
#include <plat/devs.h>
#include <plat/fb.h>
#include <plat/fb-core.h>
#include <plat/regs-fb-v4.h>
#include <plat/dp.h>
#include <plat/backlight.h>
#include <plat/gpio-cfg.h>

#include <mach/map.h>

#ifdef CONFIG_FB_MIPI_DSIM
#include <plat/dsim.h>
#include <plat/mipi_dsi.h>
#endif

#ifdef CONFIG_FB_MIPI_DSIM
#if defined(CONFIG_LCD_MIPI_S6E8AB0)
static void mipi_lcd_set_power(struct plat_lcd_data *pd,
				unsigned int power)
{
	/* reset */
	gpio_request_one(EXYNOS5410_GPX1(5), GPIOF_OUT_INIT_HIGH, "GPX1");

	msleep(20);
	if (power) {
		/* fire nRESET on power up */
		gpio_set_value(EXYNOS5410_GPX1(5), 0);
		msleep(20);
		gpio_set_value(EXYNOS5410_GPX1(5), 1);
		msleep(20);
		gpio_free(EXYNOS5410_GPX1(5));
	} else {
		/* fire nRESET on power off */
		gpio_set_value(EXYNOS5410_GPX1(5), 0);
		msleep(20);
		gpio_set_value(EXYNOS5410_GPX1(5), 1);
		msleep(20);
		gpio_free(EXYNOS5410_GPX1(5));
	}
	msleep(20);
	/* power */
	gpio_request_one(EXYNOS5410_GPX3(0), GPIOF_OUT_INIT_LOW, "GPX3");
	if (power) {
		/* fire nRESET on power up */
		gpio_set_value(EXYNOS5410_GPX3(0), 1);
		gpio_free(EXYNOS5410_GPX3(0));
	} else {
		/* fire nRESET on power off */
		gpio_set_value(EXYNOS5410_GPX3(0), 0);
		gpio_free(EXYNOS5410_GPX3(0));
	}

#ifndef CONFIG_BACKLIGHT_PWM
	/* backlight */
	gpio_request_one(EXYNOS5410_GPB2(0), GPIOF_OUT_INIT_LOW, "GPB2");
	if (power) {
		/* fire nRESET on power up */
		gpio_set_value(EXYNOS5410_GPB2(0), 1);
		gpio_free(EXYNOS5410_GPB2(0));
	} else {
		/* fire nRESET on power off */
		gpio_set_value(EXYNOS5410_GPB2(0), 0);
		gpio_free(EXYNOS5410_GPB2(0));
	}
#endif /* CONFIG_BACKLIGHT_PWM */
}

static struct plat_lcd_data smdk5410_mipi_lcd_data = {
	.set_power	= mipi_lcd_set_power,
};

static struct platform_device smdk5410_mipi_lcd = {
	.name			= "platform-lcd",
	.dev.platform_data	= &smdk5410_mipi_lcd_data,
};

static struct s3c_fb_pd_win smdk5410_fb_win0 = {
	.win_mode = {
		.left_margin	= 0x4,
		.right_margin	= 0x4,
		.upper_margin	= 4,
		.lower_margin	= 4,
		.hsync_len	= 4,
		.vsync_len	= 4,
		.xres		= 1280,
		.yres		= 800,
	},
	.virtual_x		= 1280,
	.virtual_y		= 840 * 2,
	.width			= 223,
	.height			= 125,
	.max_bpp		= 32,
	.default_bpp		= 24,
};

static struct s3c_fb_pd_win smdk5410_fb_win1 = {
	.win_mode = {
		.left_margin	= 0x4,
		.right_margin	= 0x4,
		.upper_margin	= 4,
		.lower_margin	= 4,
		.hsync_len	= 4,
		.vsync_len	= 4,
		.xres		= 1280,
		.yres		= 800,
	},
	.virtual_x		= 1280,
	.virtual_y		= 840 * 2,
	.width			= 223,
	.height			= 125,
	.max_bpp		= 32,
	.default_bpp		= 24,
};

static struct s3c_fb_pd_win smdk5410_fb_win2 = {
	.win_mode = {
		.left_margin	= 0x4,
		.right_margin	= 0x4,
		.upper_margin	= 4,
		.lower_margin	= 4,
		.hsync_len	= 4,
		.vsync_len	= 4,
		.xres		= 1280,
		.yres		= 800,
	},
	.virtual_x		= 1280,
	.virtual_y		= 800 * 2,
	.width			= 223,
	.height			= 125,
	.max_bpp		= 32,
	.default_bpp		= 24,
};
#elif defined(CONFIG_LCD_MIPI_S6E8AA0)
static void mipi_lcd_set_power(struct plat_lcd_data *pd,
				unsigned int power)
{
#ifdef CONFIG_S5P_DEV_MIPI_DSIM0
	/* reset */
	gpio_request_one(EXYNOS5410_GPX1(5), GPIOF_OUT_INIT_HIGH, "GPX1");

	msleep(20);
	if (power) {
		/* fire nRESET on power up */
		gpio_set_value(EXYNOS5410_GPX1(5), 0);
		msleep(20);
		gpio_set_value(EXYNOS5410_GPX1(5), 1);
		msleep(20);
		gpio_free(EXYNOS5410_GPX1(5));
	} else {
		/* fire nRESET on power off */
		gpio_set_value(EXYNOS5410_GPX1(5), 0);
		msleep(20);
		gpio_set_value(EXYNOS5410_GPX1(5), 1);
		msleep(20);
		gpio_free(EXYNOS5410_GPX1(5));
	}
	msleep(20);
	/* power */
	gpio_request_one(EXYNOS5410_GPX3(0), GPIOF_OUT_INIT_LOW, "GPX3");
	if (power) {
		/* fire nRESET on power up */
		gpio_set_value(EXYNOS5410_GPX3(0), 1);
		gpio_free(EXYNOS5410_GPX3(0));
	} else {
		/* fire nRESET on power off */
		gpio_set_value(EXYNOS5410_GPX3(0), 0);
		gpio_free(EXYNOS5410_GPX3(0));
	}
#else
	/* reset */
	if (power) {
		gpio_request_one(EXYNOS5410_GPH1(1),
				GPIOF_OUT_INIT_HIGH, "GPH1");
		usleep_range(5000, 6000);
		gpio_set_value(EXYNOS5410_GPH1(1), 0);
		usleep_range(5000, 6000);
		gpio_set_value(EXYNOS5410_GPH1(1), 1);
		usleep_range(5000, 6000);
		gpio_free(EXYNOS5410_GPH1(1));
	} else {
		gpio_request_one(EXYNOS5410_GPH1(1),
				GPIOF_OUT_INIT_LOW, "GPH1");
		usleep_range(5000, 6000);
		gpio_free(EXYNOS5410_GPH1(1));
	}

	/* enable ELVSS and ELVDD */
	if (power) {
		gpio_request_one(EXYNOS5410_GPH1(0),
				GPIOF_OUT_INIT_HIGH, "GPH1");
		usleep_range(5000, 6000);
		gpio_free(EXYNOS5410_GPH1(0));
	} else {
		gpio_request_one(EXYNOS5410_GPH1(0),
				GPIOF_OUT_INIT_LOW, "GPH1");
		usleep_range(5000, 6000);
		gpio_free(EXYNOS5410_GPH1(0));
	}
#endif
}

static struct plat_lcd_data smdk5410_mipi_lcd_data = {
	.set_power	= mipi_lcd_set_power,
};

static struct platform_device smdk5410_mipi_lcd = {
	.name			= "platform-lcd",
	.dev.platform_data	= &smdk5410_mipi_lcd_data,
};

static struct s3c_fb_pd_win smdk5410_fb_win0 = {
	.win_mode = {
		.left_margin	= 10,
		.right_margin	= 10,
		.upper_margin	= 1,
		.lower_margin	= 13,
		.hsync_len	= 5,
		.vsync_len	= 2,
		.xres		= 800,
		.yres		= 1280,
	},
	.virtual_x		= 800,
	.virtual_y		= 1280 * 2,
	.width			= 71,
	.height			= 114,
	.max_bpp		= 32,
	.default_bpp		= 24,
};

static struct s3c_fb_pd_win smdk5410_fb_win1 = {
	.win_mode = {
		.left_margin	= 10,
		.right_margin	= 10,
		.upper_margin	= 1,
		.lower_margin	= 13,
		.hsync_len	= 5,
		.vsync_len	= 2,
		.xres		= 800,
		.yres		= 1280,
	},
	.virtual_x		= 800,
	.virtual_y		= 1280 * 2,
	.width			= 71,
	.height			= 114,
	.max_bpp		= 32,
	.default_bpp		= 24,
};

static struct s3c_fb_pd_win smdk5410_fb_win2 = {
	.win_mode = {
		.left_margin	= 10,
		.right_margin	= 10,
		.upper_margin	= 1,
		.lower_margin	= 13,
		.hsync_len	= 5,
		.vsync_len	= 2,
		.xres		= 800,
		.yres		= 1280,
	},
	.virtual_x		= 800,
	.virtual_y		= 1280 * 2,
	.width			= 71,
	.height			= 114,
	.max_bpp		= 32,
	.default_bpp		= 24,
};
#elif defined(CONFIG_LCD_MIPI_TC358764)
static void mipi_lcd_set_power(struct plat_lcd_data *pd,
				unsigned int power)
{
	printk("s5p-mipi-dsim %s\n", __func__);
	//MIPI2LVDS_RST
		/* reset */
	gpio_request_one(EXYNOS5410_GPK0(3), GPIOF_OUT_INIT_HIGH, "GPK03");
	msleep(20);
	if (power) {
		msleep(20);
		/* fire nRESET on power up */
		gpio_set_value(EXYNOS5410_GPK0(3), 0);
		msleep(20);
		gpio_set_value(EXYNOS5410_GPK0(3), 1);
		msleep(20);
		gpio_free(EXYNOS5410_GPK0(3));
	} else {
		/* fire nRESET on power off */
		gpio_set_value(EXYNOS5410_GPK0(3), 0);
		msleep(20);
		gpio_set_value(EXYNOS5410_GPK0(3), 1);
		msleep(20);
		gpio_free(EXYNOS5410_GPK0(3));
	}
	msleep(20);

	//qdk++S

	gpio_request_one(EXYNOS5410_GPK0(0), GPIOF_OUT_INIT_LOW, "GPK0");
	if (power) {
		gpio_set_value(EXYNOS5410_GPK0(0), 1);
		gpio_free(EXYNOS5410_GPK0(0));
	} else {
		gpio_set_value(EXYNOS5410_GPK0(0), 0);
		gpio_free(EXYNOS5410_GPK0(0));
	}	

	//LCD_BL_EN
	
	gpio_request_one(EXYNOS5410_GPJ0(4), GPIOF_OUT_INIT_LOW, "GPJ0");
	if (power) {
		gpio_set_value(EXYNOS5410_GPJ0(4), 1);
		gpio_free(EXYNOS5410_GPJ0(4));
	} else {
		gpio_set_value(EXYNOS5410_GPJ0(4), 0);
		gpio_free(EXYNOS5410_GPJ0(4));
	}	
	//qdk++E
	return;
}

static struct plat_lcd_data smdk5410_mipi_lcd_data = {
	.set_power	= mipi_lcd_set_power,
};

static struct platform_device smdk5410_mipi_lcd = {
	.name			= "platform-lcd",
	.dev.platform_data	= &smdk5410_mipi_lcd_data,
};

static struct s3c_fb_pd_win smdk5410_fb_win0 = {
	.win_mode = {
		.left_margin	= 30,
		.right_margin	= 30,
		.upper_margin	= 10,
		.lower_margin	= 10,
		.hsync_len	= 20,
		.vsync_len	= 10,
		.xres		= 1680,
		.yres		= 1050,
	},
	.virtual_x		= 1680,
	.virtual_y		= 1050 * 2,
	.width			= 223,
	.height			= 125,
	.max_bpp		= 32,
	.default_bpp		= 24,
};

static struct s3c_fb_pd_win smdk5410_fb_win1 = {
	.win_mode = {
		.left_margin	= 30,
		.right_margin	= 30,
		.upper_margin	= 10,
		.lower_margin	= 10,
		.hsync_len	= 20,
		.vsync_len	= 10,
		.xres		= 1680,
		.yres		= 1050,
	},
	.virtual_x		= 1680,
	.virtual_y		= 1050 * 2,
	.width			= 223,
	.height			= 125,
	.max_bpp		= 32,
	.default_bpp		= 24,
};

static struct s3c_fb_pd_win smdk5410_fb_win2 = {
	.win_mode = {
		.left_margin	= 30,
		.right_margin	= 30,
		.upper_margin	= 10,
		.lower_margin	= 10,
		.hsync_len	= 20,
		.vsync_len	= 10,
		.xres		= 1680,
		.yres		= 1050,
	},
	.virtual_x		= 1680,
	.virtual_y		= 1050 * 2,
	.width			= 223,
	.height			= 125,
	.max_bpp		= 32,
	.default_bpp		= 24,
};
static struct s3c_fb_pd_win smdk5410_fb_win3 = {
	.win_mode = {
		.left_margin	= 30,
		.right_margin	= 30,
		.upper_margin	= 10,
		.lower_margin	= 10,
		.hsync_len	= 20,
		.vsync_len	= 10,
		.xres		= 1680,
		.yres		= 1050,
	},
	.virtual_x		= 1680,
	.virtual_y		= 1050 * 2,
	.width			= 223,
	.height			= 125,
	.max_bpp		= 32,
	.default_bpp		= 24,
};
static struct s3c_fb_pd_win smdk5410_fb_win4 = {
	.win_mode = {
		.left_margin	= 30,
		.right_margin	= 30,
		.upper_margin	= 10,
		.lower_margin	= 10,
		.hsync_len	= 20,
		.vsync_len	= 10,
		.xres		= 1680,
		.yres		= 1050,
	},
	.virtual_x		= 1680,
	.virtual_y		= 1050 * 2,
	.width			= 223,
	.height			= 125,
	.max_bpp		= 32,
	.default_bpp		= 24,
};
#endif

#elif defined(CONFIG_S5P_DP)
static void dp_lcd_set_power(struct plat_lcd_data *pd,
				unsigned int power)
{
	printk("## %s, %d\n", __func__, __LINE__);

#ifndef CONFIG_BACKLIGHT_PWM
	/* LCD_PWM_IN_2.8V: LCD_B_PWM, GPB2_0 */
	gpio_request(EXYNOS5410_GPB2(0), "GPB2");
#endif

	gpio_request(EXYNOS5410_GPJ0(0), "GPJ0");
	gpio_direction_output(EXYNOS5410_GPJ0(0), power);
	msleep(90);

#ifndef CONFIG_BACKLIGHT_PWM
	/* LCD_PWM_IN_2.8V: LCD_B_PWM, GPB2_0 */
	gpio_direction_output(EXYNOS5410_GPB2(0), power);

	gpio_free(EXYNOS5410_GPB2(0));
#endif

	gpio_free(EXYNOS5410_GPJ0(0));
}

static struct plat_lcd_data smdk5410_dp_lcd_data = {
	.set_power	= dp_lcd_set_power,
};

static struct platform_device smdk5410_dp_lcd = {
	.name	= "platform-lcd",
	.dev	= {
		.parent		= &s5p_device_fimd1.dev,
		.platform_data	= &smdk5410_dp_lcd_data,
	},
};

static struct s3c_fb_pd_win smdk5410_fb_win0 = {
	.win_mode = {
		.left_margin	= 80,
		.right_margin	= 48,
		.upper_margin	= 37,
		.lower_margin	= 3,
		.hsync_len	= 32,
		.vsync_len	= 6,
		.xres		= 2560,
		.yres		= 1600,
	},
	.virtual_x		= 2560,
	.virtual_y		= 1640 * 2,
	.max_bpp		= 32,
	.default_bpp		= 24,
};

static struct s3c_fb_pd_win smdk5410_fb_win1 = {
	.win_mode = {
		.left_margin	= 80,
		.right_margin	= 48,
		.upper_margin	= 37,
		.lower_margin	= 3,
		.hsync_len	= 32,
		.vsync_len	= 6,
		.xres		= 2560,
		.yres		= 1600,
	},
	.virtual_x		= 2560,
	.virtual_y		= 1640 * 2,
	.max_bpp		= 32,
	.default_bpp		= 24,
};

static struct s3c_fb_pd_win smdk5410_fb_win2 = {
	.win_mode = {
		.left_margin	= 80,
		.right_margin	= 48,
		.upper_margin	= 37,
		.lower_margin	= 3,
		.hsync_len	= 32,
		.vsync_len	= 6,
		.xres		= 2560,
		.yres		= 1600,
	},
	.virtual_x		= 2560,
	.virtual_y		= 1600 * 2,
	.max_bpp		= 32,
	.default_bpp		= 24,
};

static struct s3c_fb_pd_win smdk5410_fb_win3 = {
	.win_mode = {
		.left_margin	= 80,
		.right_margin	= 48,
		.upper_margin	= 37,
		.lower_margin	= 3,
		.hsync_len	= 32,
		.vsync_len	= 6,
		.xres		= 2560,
		.yres		= 1600,
	},
	.virtual_x		= 2560,
	.virtual_y		= 1600 * 2,
	.max_bpp		= 32,
	.default_bpp		= 24,
};

static struct s3c_fb_pd_win smdk5410_fb_win4 = {
	.win_mode = {
		.left_margin	= 80,
		.right_margin	= 48,
		.upper_margin	= 37,
		.lower_margin	= 3,
		.hsync_len	= 32,
		.vsync_len	= 6,
		.xres		= 2560,
		.yres		= 1600,
	},
	.virtual_x		= 2560,
	.virtual_y		= 1600 * 2,
	.max_bpp		= 32,
	.default_bpp		= 24,
};
#endif  /* CONFIG_S5P_DP */

static void exynos_fimd_gpio_setup_24bpp(void)
{
	unsigned int reg = 0;

#if defined(CONFIG_S5P_DP)
	/* Set Hotplug detect for DP */
	gpio_request(EXYNOS5410_GPX0(7), "GPX0");
	s3c_gpio_cfgpin(EXYNOS5410_GPX0(7), S3C_GPIO_SFN(3));
#endif

	/*
	 * Set DISP1BLK_CFG register for Display path selection
	 *
	 * FIMD of DISP1_BLK Bypass selection : DISP1BLK_CFG[15]
	 * ---------------------
	 *  0 | MIE/MDNIE
	 *  1 | FIMD : selected
	 */
#if defined(CONFIG_S5P_DEV_FIMD0)
	reg = __raw_readl(S3C_VA_SYS + 0x0210);
	reg &= ~(1 << 15);	/* To save other reset values */
	reg |= (1 << 15);
	__raw_writel(reg, S3C_VA_SYS + 0x0210);
#else
	reg = __raw_readl(S3C_VA_SYS + 0x0214);
	reg &= ~(1 << 15);	/* To save other reset values */
	reg |= (1 << 15);
	__raw_writel(reg, S3C_VA_SYS + 0x0214);
#endif
#if defined(CONFIG_S5P_DP)
	/* Reference clcok selection for DPTX_PHY: PAD_OSC_IN */
	reg = __raw_readl(S3C_VA_SYS + 0x04d4);
	reg &= ~(1 << 0);
	__raw_writel(reg, S3C_VA_SYS + 0x04d4);

	/* DPTX_PHY: XXTI */
	reg = __raw_readl(S3C_VA_SYS + 0x04d8);
	reg &= ~(1 << 3);
	__raw_writel(reg, S3C_VA_SYS + 0x04d8);
#endif
}

#ifdef CONFIG_S5P_DEV_FIMD0
static struct s3c_fb_platdata smdk5410_lcd0_pdata __initdata = {
	.win[0]		= &smdk5410_fb_win0,
	.win[1]		= &smdk5410_fb_win1,
	.win[2]		= &smdk5410_fb_win2,
	.default_win	= 2,
	.vidcon0	= VIDCON0_VIDOUT_RGB | VIDCON0_PNRMODE_RGB,
	.vidcon1	= VIDCON1_INV_VCLK,
	.setup_gpio	= exynos_fimd_gpio_setup_24bpp,
	.ip_version	= EXYNOS5_813,
};
#else
static struct s3c_fb_platdata smdk5410_lcd1_pdata __initdata = {
	.win[0]		= &smdk5410_fb_win0,
	.win[1]		= &smdk5410_fb_win1,
	.win[2]		= &smdk5410_fb_win2,
	.win[3]		= &smdk5410_fb_win3,
	.win[4]		= &smdk5410_fb_win4,
	.default_win	= 0,
	.vidcon0	= VIDCON0_VIDOUT_RGB | VIDCON0_PNRMODE_RGB,
#if defined(CONFIG_S5P_DP)
	.vidcon1	= 0,
#else
	.vidcon1	= VIDCON1_INV_VCLK,
#endif
	.setup_gpio	= exynos_fimd_gpio_setup_24bpp,
	.ip_version	= EXYNOS5_813,
};
#endif

#ifdef CONFIG_FB_MIPI_DSIM
#if defined(CONFIG_LCD_MIPI_S6E8AB0)
static struct mipi_dsim_config dsim_info = {
	.e_interface	= DSIM_VIDEO,
	.e_pixel_format	= DSIM_24BPP_888,
	/* main frame fifo auto flush at VSYNC pulse */
	.auto_flush	= false,
	.eot_disable	= false,
	.auto_vertical_cnt = true,
	.hse = false,
	.hfp = false,
	.hbp = false,
	.hsa = false,

	.e_no_data_lane	= DSIM_DATA_LANE_4,
	.e_byte_clk	= DSIM_PLL_OUT_DIV8,
	.e_burst_mode	= DSIM_BURST,

	.p = 2,
	.m = 57,
	.s = 1,

	/* D-PHY PLL stable time spec :min = 200usec ~ max 400usec */
	.pll_stable_time = 500,

	.esc_clk = 20 * 1000000,	/* escape clk : 10MHz */

	/* stop state holding counter after bta change count 0 ~ 0xfff */
	.stop_holding_cnt = 0x0fff,
	.bta_timeout = 0xff,		/* bta timeout 0 ~ 0xff */
	.rx_timeout = 0xffff,		/* lp rx timeout 0 ~ 0xffff */

	.dsim_ddi_pd = &s6e8ab0_mipi_lcd_driver,
};

static struct mipi_dsim_lcd_config dsim_lcd_info = {
	.rgb_timing.left_margin		= 0xa,
	.rgb_timing.right_margin	= 0xa,
	.rgb_timing.upper_margin	= 80,
	.rgb_timing.lower_margin	= 48,
	.rgb_timing.hsync_len		= 5,
	.rgb_timing.vsync_len		= 32,
	.cpu_timing.cs_setup		= 0,
	.cpu_timing.wr_setup		= 1,
	.cpu_timing.wr_act		= 0,
	.cpu_timing.wr_hold		= 0,
	.lcd_size.width			= 1280,
	.lcd_size.height		= 800,
};

#elif defined(CONFIG_LCD_MIPI_S6E8AA0)
static struct mipi_dsim_config dsim_info = {
	.e_interface	= DSIM_VIDEO,
	.e_pixel_format = DSIM_24BPP_888,
	/* main frame fifo auto flush at VSYNC pulse */
	.auto_flush	= false,
	.eot_disable	= false,
	.auto_vertical_cnt = true,
	.hse = false,
	.hfp = false,
	.hbp = false,
	.hsa = false,

	.e_no_data_lane = DSIM_DATA_LANE_4,
	.e_byte_clk	= DSIM_PLL_OUT_DIV8,
	.e_burst_mode	= DSIM_BURST,

	.p = 4,
	.m = 80,
	.s = 2,

	/* D-PHY PLL stable time spec :min = 200usec ~ max 400usec */
	.pll_stable_time = 500,

	.esc_clk = 7 * 1000000, /* escape clk : 7MHz */

	/* stop state holding counter after bta change count 0 ~ 0xfff */
	.stop_holding_cnt = 0x0fff,
	.bta_timeout = 0xff,		/* bta timeout 0 ~ 0xff */
	.rx_timeout = 0xffff,		/* lp rx timeout 0 ~ 0xffff */

	.dsim_ddi_pd = &s6e8aa0_mipi_lcd_driver,
};

static struct mipi_dsim_lcd_config dsim_lcd_info = {
	.rgb_timing.left_margin		= 10,
	.rgb_timing.right_margin	= 10,
	.rgb_timing.upper_margin	= 1,
	.rgb_timing.lower_margin	= 13,
	.rgb_timing.hsync_len		= 2,
	.rgb_timing.vsync_len		= 2,
	.cpu_timing.cs_setup		= 0,
	.cpu_timing.wr_setup		= 1,
	.cpu_timing.wr_act		= 0,
	.cpu_timing.wr_hold		= 0,
	.lcd_size.width			= 800,
	.lcd_size.height		= 1280,
};

#elif defined(CONFIG_LCD_MIPI_TC358764)
static struct mipi_dsim_config dsim_info = {
	.e_interface	= DSIM_VIDEO,
	.e_pixel_format	= DSIM_24BPP_888,
	/* main frame fifo auto flush at VSYNC pulse */
	.auto_flush	= false,
	.eot_disable	= false,
	.auto_vertical_cnt = false,
	.hse = false,
	.hfp = false,
	.hbp = false,
	.hsa = false,

	.e_no_data_lane	= DSIM_DATA_LANE_4,
	.e_byte_clk	= DSIM_PLL_OUT_DIV8,
	.e_burst_mode	= DSIM_BURST,


#if 1
	/*
	 * ===========================================
	 * |    P    |    M    |    S    |    MHz    |
	 * -------------------------------------------
	 * |    3    |   100   |    3    |    100    |
	 * |    3    |   100   |    2    |    200    |
	 * |    3    |    63   |    1    |    252    |
	 * |    4    |   100   |    1    |    300    |
	 * |    4    |   110   |    1    |    330    |
	 * |   12    |   350   |    1    |    350    |
	 * |    3    |   100   |    1    |    400    |
	 * |    4    |   150   |    1    |    450    |
	 * |    6    |   118   |    1    |    472    |
	 * |	3    |   120   |    1    |    480    |
	 * |   12    |   250   |    0    |    500    |
	 * |    4    |   100   |    0    |    600    |
	 * |    3    |    81   |    0    |    648    |
	 * |    3    |    88   |    0    |    704    |
	 * |    3    |    90   |    0    |    720    |
	 * |    3    |   100   |    0    |    800    |
	 * |   12    |   425   |    0    |    850    |
	 * |    4    |   150   |    0    |    900    |
	 * |   12    |   475   |    0    |    950    |
	 * |    6    |   250   |    0    |   1000    |
	 * -------------------------------------------
	 */


	/*
	 * pms could be calculated as the following.
	 * M * 24 / P * 2 ^ S = MHz
	 */
	// 24 * 100 / 3 / 8
	#if 0
	.p = 6,
	.m = 70,
	.s = 0,
	#else
	.p = 6,
	.m = 100,
	.s = 0,
	#endif

	/* D-PHY PLL stable time spec :min = 200usec ~ max 400usec */
	.pll_stable_time = 500,		//500

	.esc_clk = 0.4 * 1000000,	//0.4	/* escape clk : 10MHz */
#else
	.p = 2, 	
	.m = 35,
	.s = 2,

	/* D-PHY PLL stable time spec :min = 200usec ~ max 400usec */
	.pll_stable_time = 300,		//500

	.esc_clk = 0.4 * 1000000,	//0.4	/* escape clk : 10MHz */
#endif

	/* stop state holding counter after bta change count 0 ~ 0xfff */
	.stop_holding_cnt	= 0xfff, 	//0x0f
	.bta_timeout		= 0xff,		/* bta timeout 0 ~ 0xff */
	.rx_timeout		= 0xffff,	/* lp rx timeout 0 ~ 0xffff */

	.dsim_ddi_pd = &tc358764_mipi_lcd_driver,
};

static struct mipi_dsim_lcd_config dsim_lcd_info = {
	.rgb_timing.left_margin		= 0xA,
	.rgb_timing.right_margin	= 0xA,
	.rgb_timing.upper_margin	= 0xA,
	.rgb_timing.lower_margin	= 0xA,
	.rgb_timing.hsync_len		= 0xA,
	.rgb_timing.vsync_len		= 0xA,
	.cpu_timing.cs_setup		= 0,
	.cpu_timing.wr_setup		= 1,
	.cpu_timing.wr_act		= 0,
	.cpu_timing.wr_hold		= 0,
	.lcd_size.width			= 1680,
	.lcd_size.height		= 1050,
};
#endif

static struct s5p_platform_mipi_dsim dsim_platform_data = {
#ifdef CONFIG_S5P_DEV_MIPI_DSIM0
	.clk_name		= "dsim0",
#else
	.clk_name		= "dsim1",
#endif
	.dsim_config		= &dsim_info,
	.dsim_lcd_config	= &dsim_lcd_info,

	.part_reset		= s5p_dsim_part_reset,
	.init_d_phy		= s5p_dsim_init_d_phy,
	.get_fb_frame_done	= NULL,
	.trigger		= NULL,

	/*
	 * The stable time of needing to write data on SFR
	 * when the mipi mode becomes LP mode.
	 */
	.delay_for_stabilization = 600,
};
#endif  /* CONFIG_FB_MIPI_DSIM */

#ifdef CONFIG_S5P_DP
static struct video_info smdk5410_dp_config = {
	.name			= "WQXGA(2560x1600) LCD, for SMDK TEST",

	.h_sync_polarity	= 0,
	.v_sync_polarity	= 0,
	.interlaced		= 0,

	.color_space		= COLOR_RGB,
	.dynamic_range		= VESA,
	.ycbcr_coeff		= COLOR_YCBCR601,
	.color_depth		= COLOR_8,

	.link_rate		= LINK_RATE_2_70GBPS,
	.lane_count		= LANE_COUNT4,
};

static void s5p_dp_backlight_on(void)
{
	printk("## %s, %d\n", __func__, __LINE__);
	/* EDP_BL_EN: GPK1_0 */
	gpio_request(EXYNOS5410_GPK1(0), "GPK1");
	gpio_direction_output(EXYNOS5410_GPK1(0), 1);
	usleep_range(20000, 21000);

	gpio_free(EXYNOS5410_GPK1(0));
	
}

static void s5p_dp_backlight_off(void)
{
	printk("s5p_dp_backlight_off\n");
	/* EDP_BL_EN: GPK1_0 */
	gpio_request(EXYNOS5410_GPK1(0), "GPK1");

	gpio_direction_output(EXYNOS5410_GPK1(0), 0);
	usleep_range(20000, 21000);

	gpio_free(EXYNOS5410_GPK1(0));
}

static struct s5p_dp_platdata smdk5410_dp_data __initdata = {
	.video_info	= &smdk5410_dp_config,
	.phy_init	= s5p_dp_phy_init,
	.phy_exit	= s5p_dp_phy_exit,
	.backlight_on	= s5p_dp_backlight_on,
	.backlight_off	= s5p_dp_backlight_off,
};
#endif  /* CONFIG_S5P_DP */

static struct platform_device *smdk5410_display_devices[] __initdata = {
#ifdef CONFIG_FB_MIPI_DSIM
	&smdk5410_mipi_lcd,
#ifdef CONFIG_S5P_DEV_MIPI_DSIM0
	&s5p_device_mipi_dsim0,
#else
	&s5p_device_mipi_dsim1,
#endif
#endif
#ifdef CONFIG_S5P_DEV_FIMD0
	&s5p_device_fimd0,
#else
	&s5p_device_fimd1,
#endif
#ifdef CONFIG_S5P_DP
	&s5p_device_dp,
	&smdk5410_dp_lcd,
#endif
};

/* LCD Backlight data */
static struct samsung_bl_gpio_info smdk5410_bl_gpio_info = {
	.no = EXYNOS5410_GPB2(0),
	.func = S3C_GPIO_SFN(2),
};

static struct platform_pwm_backlight_data smdk5410_bl_data = {
	.pwm_id = 0,
	.max_brightness = 255,
	.dft_brightness = 30,
	.pwm_period_ns = 40000, 
};

void __init exynos5_yamo5410_display_init(void)
{
#ifdef CONFIG_FB_MIPI_DSIM
#ifdef CONFIG_S5P_DEV_MIPI_DSIM0
	s5p_dsim0_set_platdata(&dsim_platform_data);
#else
	s5p_dsim1_set_platdata(&dsim_platform_data);
#endif
#endif
	
#ifdef CONFIG_S5P_DP
	s5p_dp_set_platdata(&smdk5410_dp_data);
#endif

#ifdef CONFIG_S5P_DEV_FIMD0
	s5p_fimd0_set_platdata(&smdk5410_lcd0_pdata);
#else
	s5p_fimd1_set_platdata(&smdk5410_lcd1_pdata);
#endif

	samsung_bl_set(&smdk5410_bl_gpio_info, &smdk5410_bl_data);

	platform_add_devices(smdk5410_display_devices,
			ARRAY_SIZE(smdk5410_display_devices));

#ifdef CONFIG_S5P_DP
	exynos5_fimd1_setup_clock(&s5p_device_fimd1.dev,
			"sclk_fimd", "mout_mpll_bpll", 300 * MHZ);
#endif

#ifdef CONFIG_FB_MIPI_DSIM
#if defined(CONFIG_S5P_DEV_FIMD0)
	exynos5_fimd0_setup_clock(&s5p_device_fimd0.dev,
			"sclk_fimd", "mout_mpll_bpll", 800 * MHZ);

#else
	exynos5_fimd1_setup_clock(&s5p_device_fimd1.dev,
			"sclk_fimd", "mout_mpll_bpll", 67 * MHZ);

#endif
#endif
}

