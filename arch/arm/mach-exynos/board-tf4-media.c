/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/clkdev.h>
#include <media/exynos_gscaler.h>
#include <media/exynos_flite.h>
#include <media/m5mols.h>
#include <linux/spi/spi.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/iic.h>
#include <plat/tv-core.h>
#include <plat/mipi_csis.h>
#include <plat/gpio-cfg.h>
#include <plat/fimg2d.h>
#include <plat/jpeg.h>
#include <plat/s3c64xx-spi.h>
#include <mach/spi-clocks.h>

#include <mach/hs-iic.h>
#include <mach/exynos-tv.h>
#include <mach/exynos-mfc.h>
#include <mach/exynos-scaler.h>

#include <media/exynos_fimc_is.h>
#include <media/exynos_fimc_is_sensor.h>
#include "board-tf4.h"

#ifdef CONFIG_VIDEO_EXYNOS_FIMC_LITE
/* 1 MIPI Cameras */
#ifdef CONFIG_VIDEO_M5MOLS
static struct m5mols_platform_data m5mols_platdata = {
#ifdef CONFIG_CSI_C
	.gpio_rst = EXYNOS5410_GPX1(2), /* ISP_RESET */
#endif
#ifdef CONFIG_CSI_D
	.gpio_rst = EXYNOS5410_GPX1(0), /* ISP_RESET */
#endif
#ifdef CONFIG_CSI_E
	.gpio_rst = EXYNOS5410_GPX1(1), /* ISP_RESET */
#endif
	.enable_rst = true, /* positive reset */
	.irq = IRQ_EINT(22),
};

static struct i2c_board_info hs_i2c_devs1[] __initdata = {
	{
		I2C_BOARD_INFO("M5MOLS", 0x1F),
		.platform_data = &m5mols_platdata,
	},
};
#endif

/* For S5K6B2 vision sensor */
#ifdef CONFIG_VIDEO_S5K6B2
#ifdef CONFIG_VISION_MODE
static struct exynos5_sensor_gpio_info gpio_vision_sensor_smdk5410 = {
	.cfg = {
		/* 2M AVDD_28V */
		/* 2M DVDD_18V */
		/* 2M VIS_STBY */
		/* 2M MCLK */
		{
			/* MIPI-CSI2 */
			.pin_type = PIN_GPIO,
			.pin = EXYNOS5410_GPM7(7),
			.name = "GPIO_VT_CAM_MCLK",
			.value = (0x2<<28),
			.act = GPIO_PULL_NONE,
			.flite_id = FLITE_ID_C,
			.count = 0,
		},
	}
};

static struct exynos_fimc_is_sensor_platform_data s5k6b2_platdata = {
#ifdef CONFIG_S5K6B2_CSI_C
	.gpio_rst = EXYNOS5410_GPX1(2), /* VT_CAM_RESET */
#endif
#ifdef CONFIG_S5K6B2_CSI_D
	.gpio_rst = EXYNOS5410_GPX1(0), /* VT_CAM_RESET */
#endif
#ifdef CONFIG_S5K6B2_CSI_E
	.gpio_rst = EXYNOS5410_GPX1(1), /* VT_CAM_RESET */
#endif
	.enable_rst = true, /* positive reset */
	.gpio_info = &gpio_vision_sensor_smdk5410,
	.clk_on = exynos5_fimc_is_sensor_clk_on,
	.clk_off = exynos5_fimc_is_sensor_clk_off,
};

static struct i2c_board_info hs_i2c_devs1[] __initdata = {
	{
		I2C_BOARD_INFO("S5K6B2", 0x35),
		.platform_data = &s5k6b2_platdata,
	},
};
#endif
#endif

#endif /* CONFIG_VIDEO_EXYNOS_FIMC_LITE */

#ifdef CONFIG_VIDEO_EXYNOS_MIPI_CSIS
static struct regulator_consumer_supply mipi_csi_fixed_voltage_supplies[] = {
	REGULATOR_SUPPLY("mipi_csi", "s5p-mipi-csis.0"),
	REGULATOR_SUPPLY("mipi_csi", "s5p-mipi-csis.1"),
	REGULATOR_SUPPLY("mipi_csi", "s5p-mipi-csis.2"),
};

static struct regulator_init_data mipi_csi_fixed_voltage_init_data = {
	.constraints = {
		.always_on = 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(mipi_csi_fixed_voltage_supplies),
	.consumer_supplies	= mipi_csi_fixed_voltage_supplies,
};

static struct fixed_voltage_config mipi_csi_fixed_voltage_config = {
	.supply_name	= "DC_5V",
	.microvolts	= 5000000,
	.gpio		= -EINVAL,
	.init_data	= &mipi_csi_fixed_voltage_init_data,
};

static struct platform_device mipi_csi_fixed_voltage = {
	.name		= "reg-fixed-voltage",
	.id		= 3,
	.dev		= {
		.platform_data	= &mipi_csi_fixed_voltage_config,
	},
};
#endif

#ifdef CONFIG_VIDEO_M5MOLS
static struct regulator_consumer_supply m5mols_fixed_voltage_supplies[] = {
	REGULATOR_SUPPLY("core", NULL),
	REGULATOR_SUPPLY("dig_18", NULL),
	REGULATOR_SUPPLY("d_sensor", NULL),
	REGULATOR_SUPPLY("dig_28", NULL),
	REGULATOR_SUPPLY("a_sensor", NULL),
	REGULATOR_SUPPLY("dig_12", NULL),
};

static struct regulator_init_data m5mols_fixed_voltage_init_data = {
	.constraints = {
		.always_on = 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(m5mols_fixed_voltage_supplies),
	.consumer_supplies	= m5mols_fixed_voltage_supplies,
};

static struct fixed_voltage_config m5mols_fixed_voltage_config = {
	.supply_name	= "CAM_SENSOR",
	.microvolts	= 1800000,
	.gpio		= -EINVAL,
	.init_data	= &m5mols_fixed_voltage_init_data,
};

static struct platform_device m5mols_fixed_voltage = {
	.name		= "reg-fixed-voltage",
	.id		= 4,
	.dev		= {
		.platform_data	= &m5mols_fixed_voltage_config,
	},
};
#endif

#ifdef CONFIG_EXYNOS_MEDIA_DEVICE
struct platform_device exynos_device_md0 = {
	.name = "exynos-mdev",
	.id = 0,
};

struct platform_device exynos_device_md1 = {
	.name = "exynos-mdev",
	.id = 1,
};

struct platform_device exynos_device_md2 = {
	.name = "exynos-mdev",
	.id = 2,
};
#endif
#ifdef CONFIG_VIDEO_EXYNOS5_FIMC_IS
static struct exynos5_platform_fimc_is exynos5_fimc_is_data;
#endif

#ifdef CONFIG_VIDEO_EXYNOS_HDMI_CEC
static struct s5p_platform_cec hdmi_cec_data __initdata = {
};
#endif

#if defined(CONFIG_VIDEO_EXYNOS_TV) && defined(CONFIG_VIDEO_EXYNOS_MIXER)
	static struct s5p_mxr_platdata mxr_platdata __initdata = {
	};
#endif

#if defined(CONFIG_VIDEO_EXYNOS_TV) && defined(CONFIG_VIDEO_EXYNOS_HDMI)
static struct s5p_hdmi_platdata hdmi_platdata __initdata = {
};
#endif

#ifdef CONFIG_VIDEO_EXYNOS_TV
static struct i2c_board_info i2c_devs0[] __initdata = {
	{
		I2C_BOARD_INFO("exynos_hdcp", (0x74 >> 1)),
	},
	{
		I2C_BOARD_INFO("exynos_edid", (0xA0 >> 1)),
	},
};
#endif

#ifdef CONFIG_VIDEO_EXYNOS_MFC
#ifdef CONFIG_ARM_EXYNOS5410_BUS_DEVFREQ
static struct s5p_mfc_qos tf4_mfc_qos_table[] = {
	[0] = {
		.thrd_mb	= 0,
		.freq_int	= 160000,
#ifdef CONFIG_S5P_DP
		.freq_mif	= 400000,
#else
		.freq_mif	= 200000,
#endif

#ifdef CONFIG_ARM_EXYNOS_IKS_CPUFREQ
		.freq_cpu	= 0,
#else
		.freq_cpu	= 0,
		.freq_kfc	= 0,
#endif
	},
	[1] = {
		.thrd_mb	= 161568,
		.freq_int	= 200000,
		.freq_mif	= 400000,
#ifdef CONFIG_ARM_EXYNOS_IKS_CPUFREQ
		.freq_cpu	= 0,
#else
		.freq_cpu	= 0,
		.freq_kfc	= 0,
#endif
	},
	[2] = {
		.thrd_mb	= 244800,
		.freq_int	= 267000,
		.freq_mif	= 400000,
#ifdef CONFIG_ARM_EXYNOS_IKS_CPUFREQ
		.freq_cpu	= 400000,
#else
		.freq_cpu	= 0,
		.freq_kfc	= 400000,
#endif
	},
	[3] = {
		.thrd_mb	= 328032,
		.freq_int	= 400000,
		.freq_mif	= 400000,
#ifdef CONFIG_ARM_EXYNOS_IKS_CPUFREQ
		.freq_cpu	= 400000,
#else
		.freq_cpu	= 0,
		.freq_kfc	= 400000,
#endif
	},
};
#endif

static struct s5p_mfc_platdata tf4_mfc_pd = {
	.ip_ver		= IP_VER_MFC_5A_1,
	.clock_rate	= 333 * MHZ,
#ifdef CONFIG_ARM_EXYNOS5410_BUS_DEVFREQ
	.num_qos_steps	= ARRAY_SIZE(tf4_mfc_qos_table),
	.qos_table	= tf4_mfc_qos_table,
#endif
};
#endif

#ifdef CONFIG_VIDEO_EXYNOS_FIMG2D
static struct fimg2d_platdata fimg2d_data __initdata = {
	.ip_ver		= IP_VER_G2D_5A,
	.hw_ver		= 0x42,
	.gate_clkname	= "fimg2d",
};
#endif

static struct platform_device *tf4_media_devices[] __initdata = {
#if defined (CONFIG_CSI_D) || defined (CONFIG_S5K6B2_CSI_D)
	&exynos5_device_hs_i2c1,
#endif
#if defined (CONFIG_CSI_E) || defined (CONFIG_S5K6B2_CSI_E)
	&exynos5_device_hs_i2c2,
#endif
#ifdef CONFIG_EXYNOS_MEDIA_DEVICE
	&exynos_device_md0,
	&exynos_device_md1,
	&exynos_device_md2,
#endif
	&exynos5_device_scaler0,
	&exynos5_device_rotator,
#ifdef CONFIG_VIDEO_EXYNOS_GSCALER
	&exynos5_device_gsc0,
	&exynos5_device_gsc1,
	&exynos5_device_gsc2,
	&exynos5_device_gsc3,
#endif
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_LITE
	&exynos_device_flite0,
	&exynos_device_flite1,
	&exynos_device_flite2,
#endif
#ifdef CONFIG_VIDEO_EXYNOS_MIPI_CSIS
	&s5p_device_mipi_csis0,
	&s5p_device_mipi_csis1,
	&s5p_device_mipi_csis2,
	&mipi_csi_fixed_voltage,
#endif

#ifdef CONFIG_VIDEO_M5MOLS
	&m5mols_fixed_voltage,
#endif
#ifdef CONFIG_VIDEO_EXYNOS5_FIMC_IS
	&exynos5_device_fimc_is,
#endif
#ifdef CONFIG_VIDEO_EXYNOS_TV
#ifdef CONFIG_VIDEO_EXYNOS_HDMI
	&s5p_device_hdmi,
#endif
#ifdef CONFIG_VIDEO_EXYNOS_HDMIPHY
	&s5p_device_i2c_hdmiphy,
#endif
#ifdef CONFIG_VIDEO_EXYNOS_MIXER
	&s5p_device_mixer,
#endif
#ifdef CONFIG_VIDEO_EXYNOS_HDMI_CEC
	&s5p_device_cec,
#endif
	&s3c_device_i2c0,
#endif
#ifdef CONFIG_VIDEO_EXYNOS_MFC
	&s5p_device_mfc,
#endif
#ifdef CONFIG_VIDEO_EXYNOS_FIMG2D
	&s5p_device_fimg2d,
#endif
#ifdef CONFIG_VIDEO_EXYNOS_JPEG
	&s5p_device_jpeg,
#endif
#ifdef CONFIG_VIDEO_EXYNOS_JPEG_HX
	&exynos5_device_jpeg_hx,
#endif
};
#ifdef CONFIG_VIDEO_EXYNOS5_FIMC_IS
static struct exynos5_sensor_gpio_info gpio_tf4 = {
	.cfg = {
		/* Rear Camera setting-----------------------*/
		/* 8M MCLK */
		{
			/* MIPI-CSI0 */
			.pin_type = PIN_GPIO,
			.pin = EXYNOS5410_GPM7(5),
			.name = "GPIO_CAM_MCLK",
			.value = (0x2 << 20),
			.act = GPIO_PULL_NONE,
			.flite_id = FLITE_ID_A,
			.count = 0,
		},
		/* 8M reset */
		{
			/* MIPI-CSI0 */
			.pin_type = PIN_GPIO,
			.pin = EXYNOS5410_GPF1(4),
			.name = "GPIO_MAIN_CAM_RESET",
			.value = (1),
			.act = GPIO_RESET,
			.flite_id = FLITE_ID_A,
			.count = 0,
		},
		/* 8M I2C */
		{
			.pin_type = PIN_GPIO,
			.pin = EXYNOS5410_GPF0(0),
			.name = "GPIO_MAIN_CAM_SDA_18V",
			.value = (0x2 << 0),
			.act = GPIO_PULL_NONE,
			.flite_id = FLITE_ID_A,
			.count = 0,
		},
		{
			.pin_type = PIN_GPIO,
			.pin = EXYNOS5410_GPF0(1),
			.name = "GPIO_MAIN_CAM_SCL_18V",
			.value = (0x2 << 4),
			.act = GPIO_PULL_NONE,
			.flite_id = FLITE_ID_A,
			.count = 0,
		},
        //camera flash driver     
        {
            .pin_type = PIN_GPIO,
            .pin = EXYNOS5410_GPF1(0),
            .name = "GPIO_FLASH_TORCH",
            .value = (0x3 << 0),
            .act = GPIO_PULL_NONE,
            .flite_id = FLITE_ID_A,
            .count = 0,
        },
        {
            .pin_type = PIN_GPIO,
            .pin = EXYNOS5410_GPF1(1),
            .name = "GPIO_FLASH_STROBE",
            .value = (0x3 << 4),
            .act = GPIO_PULL_NONE,
            .flite_id = FLITE_ID_A,
            .count = 0,
        },                  
        /* Flash I2C */
        {
            .pin_type = PIN_GPIO,
            .pin = EXYNOS5410_GPF0(4),
            .name = "GPIO_VT_CAM_SDA_18V",
            .value = (0x2 << 16),
            .act = GPIO_PULL_NONE,
            .flite_id = FLITE_ID_A,
            .count = 0,
        },
        {
            .pin_type = PIN_GPIO,
            .pin = EXYNOS5410_GPF0(5),
            .name = "GPIO_VT_CAM_SCL_18V",
            .value = (0x2 << 20),
            .act = GPIO_PULL_NONE,
            .flite_id = FLITE_ID_A,
            .count = 0,
        },
		
		/* Front Camera setting-----------------------*/
		/* 2M reset */
		{
			/* MIPI-CSI2 */
			.pin_type = PIN_GPIO,
			.pin = EXYNOS5410_GPE0(1),
			.name = "GPIO_CAM_VT_nRST",
			.value = (1),
			.act = GPIO_RESET,
			.flite_id = FLITE_ID_C,
			.count = 0,
		},
		#if 0 // EVT use MIPI-CSI1
		/* 2M MCLK */
		{
			/* MIPI-CSI1 */
			.pin_type = PIN_GPIO,
			.pin = EXYNOS5410_GPM7(6),
			.name = "GPIO_VT_CAM_MCLK",
			.value = (0x2<<24),
			.act = GPIO_PULL_NONE,
			.flite_id = FLITE_ID_B,
			.count = 0,
		},
		/* 2M I2C */
		{
			.pin_type = PIN_GPIO,
			.pin = EXYNOS5410_GPF0(2),
			.name = "GPIO_VT_CAM_SDA_18V",
			.value = (0x2 << 8),
			.act = GPIO_PULL_NONE,
			.flite_id = FLITE_ID_B,
			.count = 0,
		},
		{
			.pin_type = PIN_GPIO,
			.pin = EXYNOS5410_GPF0(3),
			.name = "GPIO_VT_CAM_SCL_18V",
			.value = (0x2 << 12),
			.act = GPIO_PULL_NONE,
			.flite_id = FLITE_ID_B,
			.count = 0,
		},
		#else // DVT use MIPI-CSI2 
		
		/* 2M MCLK */
		{
			/* MIPI-CSI2 */
			.pin_type = PIN_GPIO,
			.pin = EXYNOS5410_GPM7(7),
			.name = "GPIO_VT_CAM_MCLK",
			.value = (0x2 << 28),
			.act = GPIO_PULL_NONE,
			.flite_id = FLITE_ID_C,
			.count = 0,
		},
		/* 2M I2C */
		{
			.pin_type = PIN_GPIO,
			.pin = EXYNOS5410_GPF0(4),
			.name = "GPIO_VT_CAM_SDA_18V",
			.value = (0x2 << 16),
			.act = GPIO_PULL_NONE,
			.flite_id = FLITE_ID_C,
			.count = 0,
		},
		{
			.pin_type = PIN_GPIO,
			.pin = EXYNOS5410_GPF0(5),
			.name = "GPIO_VT_CAM_SCL_18V",
			.value = (0x2 << 20),
			.act = GPIO_PULL_NONE,
			.flite_id = FLITE_ID_C,
			.count = 0,
		},
		#endif	
		/* ETC */
		/* Host use spi controller in image subsystem */
		/*
		{
			.pin_type = PIN_GPIO,
			.pin = EXYNOS5410_GPF1(0),
			.name = "GPIO_CAM_SPI0_SCLK",
			.value = (0x2 << 0),
			.act = GPIO_PULL_NONE,
		},
		*/
		/* chip select is controlled by gpio output */
		/*
		{
			.pin_type = PIN_GPIO,
			.pin = EXYNOS5410_GPF1(1),
			.name = "GPIO_CAM_SPI0_SSN",
			.value = (0x1 << 4),
			.act = GPIO_PULL_NONE,
		},
		*/
/* this pin is used for ISP UART log
		{
			.pin_type = PIN_GPIO,
			.pin = EXYNOS5410_GPE0(6),
			.name = "GPIO_USBSEL_AP_CP",
			.value = (0x1 << 24),
			.act = GPIO_PULL_NONE,
			.flite_id = FLITE_ID_A,
			.count = 0,
		},
*/		
		{
			.pin_type = PIN_GPIO,
			.pin = EXYNOS5410_GPE0(7),
			.name = "GPIO_TXD_UART_ISP",
			.value = (0x3 << 28),
			.act = GPIO_PULL_NONE,
			.flite_id = FLITE_ID_A,
			.count = 0,
		},
		{
			.pin_type = PIN_GPIO,
			.pin = EXYNOS5410_GPE1(0),
			.name = "GPIO_nCTS_UART_ISP",
			.value = (0x3 << 0),
			.act = GPIO_PULL_NONE,
			.flite_id = FLITE_ID_A,
			.count = 0,
		},
		{
			.pin_type = PIN_GPIO,
			.pin = EXYNOS5410_GPE1(1),
			.name = "GPIO_RXD_UART_ISP",
			.value = (0x3 << 4),
			.act = GPIO_PULL_NONE,
			.flite_id = FLITE_ID_A,
			.count = 0,
		},
	}
};

#endif

#ifdef CONFIG_VIDEO_EXYNOS_FIMC_LITE
static void __init smdk5410_camera_gpio_cfg(void)
{
#ifdef CONFIG_VIDEO_M5MOLS
#ifdef CONFIG_CSI_D
	gpio_request(EXYNOS5410_GPM7(6), "GPM7");
	s3c_gpio_cfgpin(EXYNOS5410_GPM7(6), S3C_GPIO_SFN(0x2));
	s3c_gpio_setpull(EXYNOS5410_GPM7(6), S3C_GPIO_PULL_NONE);
	gpio_free(EXYNOS5410_GPM7(6));
#endif
#ifdef CONFIG_CSI_E
	gpio_request(EXYNOS5410_GPM7(7), "GPM7");
	s3c_gpio_cfgpin(EXYNOS5410_GPM7(7), S3C_GPIO_SFN(0x2));
	s3c_gpio_setpull(EXYNOS5410_GPM7(7), S3C_GPIO_PULL_NONE);
	gpio_free(EXYNOS5410_GPM7(7));
#endif
	s3c_gpio_cfgpin(EXYNOS5410_GPX3(5), S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(EXYNOS5410_GPX3(5), S3C_GPIO_PULL_NONE);
#endif
}
#endif

#if defined(CONFIG_VIDEO_EXYNOS_GSCALER) && defined(CONFIG_VIDEO_EXYNOS_FIMC_LITE)
#if defined(CONFIG_VIDEO_M5MOLS)
static struct exynos_isp_info m5mols = {
	.board_info	= &hs_i2c_devs1,
	.cam_srclk_name	= "sclk_isp_sensor",
	.clk_frequency  = 24000000UL,
	.bus_type	= CAM_TYPE_MIPI,
	.cam_clk_src_name = "dout_aclk_333_432_gscl",
	.cam_clk_name	= "aclk_333_432_gscl",
#ifdef CONFIG_CSI_C
	.camif_clk_name	= "gscl_flite0",
	.i2c_bus_num	= 4,
	.cam_port	= CAM_PORT_A, /* A-Port : 0 */
#endif
#ifdef CONFIG_CSI_D
	.camif_clk_name	= "gscl_flite1",
	.i2c_bus_num	= 5,
	.cam_port	= CAM_PORT_B, /* B-Port : 1 */
#endif
#ifdef CONFIG_CSI_E
	.camif_clk_name	= "gscl_flite2",
	.i2c_bus_num	= 6,
	.cam_port	= CAM_PORT_C, /* C-Port : 2 */
#endif
	.flags		= CAM_CLK_INV_PCLK | CAM_CLK_INV_VSYNC,
	.csi_data_align = 32,
};
/* This is for platdata of fimc-lite */
static struct s3c_platform_camera flite_m5mo = {
	.type		= CAM_TYPE_MIPI,
	.use_isp	= true,
	.inv_pclk	= 1,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,
};
#endif

#ifdef CONFIG_VIDEO_S5K6B2
#ifdef CONFIG_VISION_MODE
static struct exynos_isp_info s5k6b2 = {
	.board_info	= hs_i2c_devs1,
	.cam_srclk_name	= "sclk_isp_sensor",
	.clk_frequency	= 24000000UL,
	.bus_type	= CAM_TYPE_MIPI,
	.cam_clk_src_name	= "dout_aclk_333_432_gscl",
	.cam_clk_name	= "aclk_333_432_gscl",
#ifdef CONFIG_S5K6B2_CSI_C
	.camif_clk_name	= "gscl_flite0",
	.i2c_bus_num	= 4,
	.cam_port	= CAM_PORT_A, /* A-Port : 0 */
#endif
#ifdef CONFIG_S5K6B2_CSI_D
	.camif_clk_name	= "gscl_flite1",
	.i2c_bus_num	= 5,
	.cam_port	= CAM_PORT_B, /* B-Port : 1 */
#endif
#ifdef CONFIG_S5K6B2_CSI_E
	.camif_clk_name	= "gscl_flite2",
	.i2c_bus_num	= 6,
	.cam_port	= CAM_PORT_C, /* C-Port : 2 */
#endif
	.flags		= 0,
	.csi_data_align	= 24,
};

/* This is for platdata of fimc-lite */
static struct s3c_platform_camera flite_s5k6b2 = {
	.type		= CAM_TYPE_MIPI,
	.use_isp	= false,
	.inv_pclk	= 0,
	.inv_vsync	= 0,
	.inv_href	= 0,
	.inv_hsync	= 0,
};
#endif
#endif

static void __set_mipi_csi_config(struct s5p_platform_mipi_csis *data,
					u8 alignment)
{
	data->alignment = alignment;
}

static void __set_gsc_camera_config(struct exynos_platform_gscaler *data,
					u32 active_index, u32 preview,
					u32 camcording, u32 max_cam)
{
	data->active_cam_index = active_index;
	data->cam_preview = preview;
	data->cam_camcording = camcording;
	data->num_clients = max_cam;
}

static void __set_flite_camera_config(struct exynos_platform_flite *data,
					u32 active_index, u32 max_cam)
{
	data->active_cam_index = active_index;
	data->num_clients = max_cam;
}

static void __init smdk5410_set_camera_platdata(void)
{
	int gsc_cam_index = 0;
	int flite0_cam_index = 0;
	int flite1_cam_index = 0;
	int flite2_cam_index = 0;
#if defined(CONFIG_VIDEO_M5MOLS)
	exynos_gsc1_default_data.isp_info[gsc_cam_index++] = &m5mols;
#if defined(CONFIG_CSI_C)
	exynos_flite0_default_data.cam[flite0_cam_index] = &flite_m5mo;
	exynos_flite0_default_data.isp_info[flite0_cam_index] = &m5mols;
	flite0_cam_index++;
#endif
#if defined(CONFIG_CSI_D)
	exynos_flite1_default_data.cam[flite1_cam_index] = &flite_m5mo;
	exynos_flite1_default_data.isp_info[flite1_cam_index] = &m5mols;
	flite1_cam_index++;
#endif
#if defined(CONFIG_CSI_E)
	exynos_flite2_default_data.cam[flite2_cam_index] = &flite_m5mo;
	exynos_flite2_default_data.isp_info[flite2_cam_index] = &m5mols;
	flite2_cam_index++;
#endif
#endif

#ifdef CONFIG_VIDEO_S5K6B2
#ifdef CONFIG_VISION_MODE
	exynos_gsc1_default_data.isp_info[gsc_cam_index++] = &s5k6b2;
#if defined(CONFIG_S5K6B2_CSI_C)
	exynos_flite0_default_data.cam[flite0_cam_index] = &flite_s5k6b2;
	exynos_flite0_default_data.isp_info[flite0_cam_index] = &s5k6b2;
	flite0_cam_index++;
#endif
#if defined(CONFIG_S5K6B2_CSI_D)
	exynos_flite1_default_data.cam[flite1_cam_index] = &flite_s5k6b2;
	exynos_flite1_default_data.isp_info[flite1_cam_index] = &s5k6b2;
	flite1_cam_index++;
#endif
#if defined(CONFIG_S5K6B2_CSI_E)
	exynos_flite2_default_data.cam[flite2_cam_index] = &flite_s5k6b2;
	exynos_flite2_default_data.isp_info[flite2_cam_index] = &s5k6b2;
	flite2_cam_index++;
#endif
#endif
#endif

	/* flite platdata register */
	__set_flite_camera_config(&exynos_flite0_default_data, 0, flite0_cam_index);
	__set_flite_camera_config(&exynos_flite1_default_data, 0, flite1_cam_index);
	__set_flite_camera_config(&exynos_flite2_default_data, 0, flite2_cam_index);

	/* gscaler platdata register */
	/* GSC-0 */
	__set_gsc_camera_config(&exynos_gsc1_default_data, 0, 1, 0, gsc_cam_index);

	/* GSC-1 */
	/* GSC-2 */
	/* GSC-3 */
}
#endif /* CONFIG_VIDEO_EXYNOS_GSCALER */

void __init exynos5_tf4_media_init(void)
{
#if defined (CONFIG_CSI_D) || defined (CONFIG_S5K6B2_CSI_D)
	exynos5_hs_i2c1_set_platdata(NULL);
#endif
#if defined (CONFIG_CSI_E) || defined (CONFIG_S5K6B2_CSI_E)
	exynos5_hs_i2c2_set_platdata(NULL);
#endif
#ifdef CONFIG_VIDEO_EXYNOS_MFC
	s5p_mfc_set_platdata(&tf4_mfc_pd);

	dev_set_name(&s5p_device_mfc.dev, "s3c-mfc");
	clk_add_alias("mfc", "s5p-mfc-v6", "mfc", &s5p_device_mfc.dev);
	s5p_mfc_setname(&s5p_device_mfc, "s5p-mfc-v6");
#endif

	platform_add_devices(tf4_media_devices,
			ARRAY_SIZE(tf4_media_devices));

	s3c_set_platdata(&exynos5410_scaler_pd, sizeof(exynos5410_scaler_pd),
			&exynos5_device_scaler0);

#ifdef CONFIG_VIDEO_S5K6B2
#if defined(CONFIG_S5K6B2_CSI_C)
	__set_mipi_csi_config(&s5p_mipi_csis0_default_data, s5k6b2.csi_data_align);
#elif defined(CONFIG_S5K6B2_CSI_D)
	__set_mipi_csi_config(&s5p_mipi_csis1_default_data, s5k6b2.csi_data_align);
#elif defined(CONFIG_S5K6B2_CSI_E)
	__set_mipi_csi_config(&s5p_mipi_csis2_default_data, s5k6b2.csi_data_align);
#endif
#endif

#ifdef CONFIG_VIDEO_EXYNOS_MIPI_CSIS
	s3c_set_platdata(&s5p_mipi_csis0_default_data,
			sizeof(s5p_mipi_csis0_default_data), &s5p_device_mipi_csis0);
	s3c_set_platdata(&s5p_mipi_csis1_default_data,
			sizeof(s5p_mipi_csis1_default_data), &s5p_device_mipi_csis1);
	s3c_set_platdata(&s5p_mipi_csis2_default_data,
			sizeof(s5p_mipi_csis2_default_data), &s5p_device_mipi_csis2);
#endif
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_LITE
	smdk5410_camera_gpio_cfg();
	smdk5410_set_camera_platdata();
	s3c_set_platdata(&exynos_flite0_default_data,
			sizeof(exynos_flite0_default_data), &exynos_device_flite0);
	s3c_set_platdata(&exynos_flite1_default_data,
			sizeof(exynos_flite1_default_data), &exynos_device_flite1);
	s3c_set_platdata(&exynos_flite2_default_data,
			sizeof(exynos_flite2_default_data), &exynos_device_flite2);
#endif

#if defined(CONFIG_VIDEO_EXYNOS_TV)
	dev_set_name(&s5p_device_hdmi.dev, "exynos5-hdmi");
	mxr_platdata.ip_ver = IP_VER_TV_5A_1;
	hdmi_platdata.ip_ver = IP_VER_TV_5A_1;

	s5p_tv_setup();
/* Below should be enabled after power domain is available */
#if 0
	s5p_device_hdmi.dev.parent = &exynos5_device_pd[PD_DISP1].dev;
	s5p_device_mixer.dev.parent = &exynos5_device_pd[PD_DISP1].dev;
#endif
#ifdef CONFIG_VIDEO_EXYNOS_HDMI_CEC
	s5p_hdmi_cec_set_platdata(&hdmi_cec_data);
#endif
	s3c_set_platdata(&mxr_platdata, sizeof(mxr_platdata), &s5p_device_mixer);
	s5p_hdmi_set_platdata(&hdmi_platdata);
	s3c_i2c0_set_platdata(NULL);
	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
#endif
#ifdef CONFIG_VIDEO_EXYNOS_GSCALER
	exynos5_gsc_set_ip_ver(IP_VER_GSC_5A);

	s3c_set_platdata(&exynos_gsc0_default_data, sizeof(exynos_gsc0_default_data),
			&exynos5_device_gsc0);
	s3c_set_platdata(&exynos_gsc1_default_data, sizeof(exynos_gsc1_default_data),
			&exynos5_device_gsc1);
	s3c_set_platdata(&exynos_gsc2_default_data, sizeof(exynos_gsc2_default_data),
			&exynos5_device_gsc2);
	s3c_set_platdata(&exynos_gsc3_default_data, sizeof(exynos_gsc3_default_data),
			&exynos5_device_gsc3);
#endif
#ifdef CONFIG_VIDEO_EXYNOS5_FIMC_IS
	dev_set_name(&exynos5_device_fimc_is.dev, "s5p-mipi-csis.0");
	clk_add_alias("gscl_wrap0", FIMC_IS_MODULE_NAME, "gscl_wrap0", &exynos5_device_fimc_is.dev);
	dev_set_name(&exynos5_device_fimc_is.dev, "s5p-mipi-csis.1");
	clk_add_alias("gscl_wrap1", FIMC_IS_MODULE_NAME, "gscl_wrap1", &exynos5_device_fimc_is.dev);
	dev_set_name(&exynos5_device_fimc_is.dev, "s5p-mipi-csis.2");
	clk_add_alias("gscl_wrap2", FIMC_IS_MODULE_NAME, "gscl_wrap2", &exynos5_device_fimc_is.dev);

	dev_set_name(&exynos5_device_fimc_is.dev, FIMC_IS_MODULE_NAME);

	exynos5_fimc_is_data.gpio_info = &gpio_tf4;

	exynos5_fimc_is_set_platdata(&exynos5_fimc_is_data);
#endif
#ifdef CONFIG_VIDEO_EXYNOS_FIMG2D
	s5p_fimg2d_set_platdata(&fimg2d_data);
#endif
#ifdef CONFIG_VIDEO_EXYNOS_JPEG
	exynos5_jpeg_fimp_setup_clock(&s5p_device_jpeg.dev, 166500000);
#endif
#ifdef CONFIG_VIDEO_EXYNOS_JPEG_HX
	exynos5_jpeg_hx_setup_clock(&exynos5_device_jpeg_hx.dev, 300000000);
#endif
}
