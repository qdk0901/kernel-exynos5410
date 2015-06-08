/* linux/arch/arm/mach-exynos/board-yamo5410-audio.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include <linux/delay.h>
#include <mach/irqs.h>
#include <mach/pmu.h>

#ifdef CONFIG_SND_SOC_WM8994
#include <linux/mfd/wm8994/pdata.h>
#include <linux/mfd/wm8994/gpio.h>
#endif

#include <plat/gpio-cfg.h>
#include <plat/devs.h>
#include <plat/iic.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>


#include "board-yamo5410.h"

#ifdef CONFIG_SND_SOC_WM8994
/* PVDD_AUDIO */
static struct regulator_consumer_supply wm8994_pvdd_audio_supplies[] = {
#ifdef CONFIG_MACH_YAMO5410
	REGULATOR_SUPPLY("AVDD2", "10-001a"),
	REGULATOR_SUPPLY("CPVDD", "10-001a"),
	REGULATOR_SUPPLY("DBVDD", "10-001a"),
#else
	REGULATOR_SUPPLY("AVDD2", "10-001a"),
	REGULATOR_SUPPLY("CPVDD", "10-001a"),
	REGULATOR_SUPPLY("DBVDD1", "10-001a"),
	REGULATOR_SUPPLY("DBVDD2", "10-001a"),
	REGULATOR_SUPPLY("DBVDD3", "10-001a"),
#endif
};

static struct regulator_init_data wm8994_pvdd_audio_init_data = {
	.constraints = {
		.always_on = 1,
	},
	.num_consumer_supplies = ARRAY_SIZE(wm8994_pvdd_audio_supplies),
	.consumer_supplies = wm8994_pvdd_audio_supplies,
};

static struct fixed_voltage_config wm8994_pvdd_audio_config = {
	.supply_name = "PVDD_AUDIO",
	.microvolts = 1800000,
	.gpio = -EINVAL,
	.init_data = &wm8994_pvdd_audio_init_data,
};

static struct platform_device wm8994_pvdd_audio_device = {
	.name = "reg-fixed-voltage",
	.id = 0,
	.dev = {
		.platform_data	= &wm8994_pvdd_audio_config,
	},
};

/* VBAT */
static struct regulator_consumer_supply wm8994_vbat_supplies[] = {
	REGULATOR_SUPPLY("SPKVDD1", "10-001a"),
	REGULATOR_SUPPLY("SPKVDD2", "10-001a"),
};

static struct regulator_init_data wm8994_vbat_init_data = {
	.constraints = {
		.always_on = 1,
	},
	.num_consumer_supplies = ARRAY_SIZE(wm8994_vbat_supplies),
	.consumer_supplies = wm8994_vbat_supplies,
};

static struct fixed_voltage_config wm8994_vbat_config = {
	.supply_name = "VBAT",
	.microvolts = 5000000,
	.gpio = -EINVAL,
	.init_data = &wm8994_vbat_init_data,
};

static struct platform_device wm8994_vbat_device = {
	.name = "reg-fixed-voltage",
	.id = 1,
	.dev = {
		.platform_data = &wm8994_vbat_config,
	},
};

/* WM8994 LDO1 */
static struct regulator_consumer_supply wm8994_ldo1_supplies[] = {
	REGULATOR_SUPPLY("AVDD1", "10-001a"),
};

static struct regulator_init_data wm8994_ldo1_init_data = {
	.constraints = {
		.name = "WM8994 LDO1",
        .valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = ARRAY_SIZE(wm8994_ldo1_supplies),
	.consumer_supplies = wm8994_ldo1_supplies,
};

/* WM8994 LDO2 */
static struct regulator_consumer_supply wm8994_ldo2_supplies[] = {
	REGULATOR_SUPPLY("DCVDD", "10-001a"),
};

static struct regulator_init_data wm8994_ldo2_init_data = {
	.constraints = {
		.name = "WM8994 LDO2",
		.always_on = 1,     /* Practically status is changed by LDO1 */
	},
	.num_consumer_supplies = ARRAY_SIZE(wm8994_ldo2_supplies),
	.consumer_supplies = wm8994_ldo2_supplies,
};

static struct wm8994_pdata wm8994_platform_data = {
#ifdef CONFIG_MACH_YAMO5410
	.gpio_defaults[2] = 0x0100, /* GPIO3 BCLK2   out */
	.gpio_defaults[3] = 0x0100, /* GPIO4 LRCLK2  out */
	.gpio_defaults[4] = 0x8100, /* GPIO5  DACDAT2 in  */
	.gpio_defaults[6] = 0x0100,	/* GPIO7  ADCDAT2 out */

#else
	/* GPIO1 - IRQ output */
	.gpio_defaults[0] = WM8994_GP_FN_IRQ,
	/* If the i2s0 and i2s2 is enabled simultaneously */
	.gpio_defaults[7] = 0x8100,     /* GPIO8  DACDAT3 in */
	.gpio_defaults[8] = 0x0100,     /* GPIO9  ADCDAT3 out */
	.gpio_defaults[9] = 0x0100,     /* GPIO10 LRCLK3  out */
	.gpio_defaults[10] = 0x0100,    /* GPIO11 BCLK3   out */
#endif /*CONFIG_MACH_YAMO5410*/
	.ldo[0] = {
	 .enable = EXYNOS5410_GPG1(0),  /* CODEC_LDO_EN */
	 .init_data = &wm8994_ldo1_init_data
	},
	.ldo[1] = {
	    .enable = 0,
        .init_data = &wm8994_ldo2_init_data
    },

    .irq_base = IRQ_BOARD_START + 0x10, /* After SEC PMIC */

    /* Support external capacitors */
    .jd_ext_cap = 1,

    /* Regulated mode at highest output voltage */
    .micbias = {0x2f, 0x27},

    .micd_lvl_sel = 0xff,

    .ldo_ena_always_driven = 1,
};

/* I2C - GPIO */
static struct i2c_gpio_platform_data i2c_wm1811_platdata = {
	.sda_pin = EXYNOS5410_GPA1(2),	/* AUDIO_SDA */
	.scl_pin = EXYNOS5410_GPA1(3),	/* AUDIO_SCL */
	.udelay = 2,
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.scl_is_output_only = 0,
};

static struct platform_device s3c_device_i2c_wm1811 = {
	.name = "i2c-gpio",
	.id = 10,
	.dev.platform_data = &i2c_wm1811_platdata,
};

static struct i2c_board_info i2c_devs_wm8994[] __initdata = {
	{
                I2C_BOARD_INFO("wm8994", (0x34 >> 1)),  /* 0x1a */
		.platform_data  = &wm8994_platform_data,
		.irq = IRQ_EINT(11),    /* CODEC_INT */
	},
};

static struct platform_device yamo5410_wm8994 = {
	.name = "yamo5410-wm8994",
	.id = -1,
};

#endif  // CONFIG_SND_SOC_WM8994

#ifdef CONFIG_SND_SOC_SMDK_LMDAAD
/*
	// use these four pins for dsp control
	EXYNOS5410_GPA1(2),
	EXYNOS5410_GPA1(3),
	EXYNOS5410_GPB2(2),
	EXYNOS5410_GPB2(3),
*/
#if 1
#define CS EXYNOS5410_GPA1(2)
#define SCK EXYNOS5410_GPA1(3)
#define MOSI EXYNOS5410_GPB2(2)
#define MISO EXYNOS5410_GPB2(3)
#else
#define CS EXYNOS5410_GPX0(6)//SPI_GPIO_NO_CHIPSELECT
#define SCK EXYNOS5410_GPB1(4) 
#define MOSI EXYNOS5410_GPB1(3)
#define MISO SPI_GPIO_NO_MISO
#endif

static struct spi_gpio_platform_data lm_gpio_spi = {
	.sck			= SCK,
	.mosi			= MOSI,
	.miso			= MISO,
	.num_chipselect = 1,
};

static struct platform_device lm_gpio_spi_device = {
	.name			= "spi_gpio",
	.id			= 0,
	.dev.platform_data	= &lm_gpio_spi,
};

static struct spi_board_info lm_spi_devices[] __initdata = {
        {
                .modalias               = "spidev",
                .max_speed_hz           = 1000000,
                .bus_num                = 0,
                .chip_select            = 0,
		.mode = SPI_MODE_3,
                .controller_data        = (void *) CS,
        },
};

static void __init lm_init_spi(void)
{
        spi_register_board_info(lm_spi_devices,
                                ARRAY_SIZE(lm_spi_devices));
        platform_device_register(&lm_gpio_spi_device);
}

static struct platform_device lmdaad_soc = {
	.name = "lmdaad-soc",
	.id = -1,
};

static struct platform_device lmdaad_codec = {
	.name = "lmdaad-codec",
	.id = -1,
};
#endif

static struct platform_device *yamo5410_audio_devices[] __initdata = {
#ifdef CONFIG_SND_SAMSUNG_I2S
	&exynos5_device_i2s0,
#endif
#ifdef CONFIG_SND_SAMSUNG_PCM
	&exynos5_device_pcm0,
#endif
#ifdef CONFIG_SND_SOC_SAMSUNG_YAMO5410_MODEM
	&exynos5_device_pcm1,
#endif
#ifdef CONFIG_SND_SAMSUNG_SPDIF
	&exynos5_device_spdif,
#endif
#if defined(CONFIG_SND_SAMSUNG_RP) || defined(CONFIG_SND_SAMSUNG_ALP)
	&exynos5_device_srp,
#endif
#ifdef CONFIG_SND_SOC_WM8994
	&s3c_device_i2c_wm1811,
	&wm8994_pvdd_audio_device,
	&wm8994_vbat_device,
	&yamo5410_wm8994,
#endif
#ifdef CONFIG_SND_SOC_SMDK_LMDAAD
	&lmdaad_codec,
	&lmdaad_soc,
#endif
	&samsung_asoc_dma,
	&samsung_asoc_idma,
};

void __init exynos5_yamo5410_audio_init(void)
{
#ifdef CONFIG_SND_SOC_WM8994
	i2c_register_board_info(10, i2c_devs_wm8994, ARRAY_SIZE(i2c_devs_wm8994));
#endif

#ifdef CONFIG_SND_SOC_SMDK_LMDAAD
	lm_init_spi();
#endif
	platform_add_devices(yamo5410_audio_devices,
			ARRAY_SIZE(yamo5410_audio_devices));
}

