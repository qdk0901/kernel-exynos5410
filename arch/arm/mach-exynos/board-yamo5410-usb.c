/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/platform_data/dwc3-exynos.h>

#include <plat/ehci.h>
#include <plat/devs.h>
#include <plat/usb-phy.h>
#include <plat/gpio-cfg.h>

#include <mach/ohci.h>
#include <mach/usb3-drd.h>
#include <mach/usb-switch.h>

/* Borad defendent GPIO */
#define YAMO5410_DRD0_IDDET_GPIO	EXYNOS5410_GPX2(4)
#define YAMO5410_DRD0_VBUSDET_GPIO	EXYNOS5410_GPX2(3)
#ifdef CONFIG_MACH_YAMO5410 //AGREEYA
#define YAMO5410_DRD1_IDDET_GPIO	EXYNOS5410_GPD1(1)
#define YAMO5410_DRD1_VBUSDET_GPIO	EXYNOS5410_GPX2(2)
#endif //AGREEYA
#define YAMO5410_USB20_IDDET_GPIO	EXYNOS5410_GPX2(6)
#define YAMO5410_USB20_VBUSDET_GPIO	EXYNOS5410_GPX2(5)

/* Dedicated SFR */
#define UDRD3_0_OVERCUR_U2 		EXYNOS5410_GPK3(0)
#define UDRD3_0_OVERCUR_U3		EXYNOS5410_GPK3(1)
#define UDRD3_0_VBUSCTRL_U2		EXYNOS5410_GPK3(2)
#define UDRD3_0_VBUSCTRL_U3		EXYNOS5410_GPK3(3)
#ifdef CONFIG_MACH_YAMO5410 //AGREEYA
#define UDRD3_1_OVERCUR_U2 		EXYNOS5410_GPK2(4)
#define UDRD3_1_OVERCUR_U3		EXYNOS5410_GPK2(5)
#define UDRD3_1_VBUSCTRL_U2		EXYNOS5410_GPK2(6)
#define UDRD3_1_VBUSCTRL_U3		EXYNOS5410_GPK2(7)
#endif //AGREEYA

static int yamo5410_drd30_vbus_ctrl(struct platform_device *pdev, int on)
{
	int phy_num = pdev->id;
	unsigned gpio;
	int ret = -EINVAL;

	if (phy_num == 0)
		gpio = UDRD3_0_VBUSCTRL_U3; // EXYNOS5410_GPK3(3);
#ifdef CONFIG_MACH_YAMO5410	//AGREEYA
	else if (phy_num == 1)
		gpio = UDRD3_1_VBUSCTRL_U3;
#endif 	
	else
		return ret;

	ret = gpio_request(gpio, "UDRD3_VBUSCTRL_U3");
	if (ret < 0) {
		pr_err("failed to request UDRD3_%d_VBUSCTRL_U3\n",
				phy_num);
		return ret;
	}

	gpio_set_value(gpio, !!on);
	gpio_free(gpio);

	return ret;
}


static int yamo5410_drd30_get_id_state(struct platform_device *pdev)
{
	int phy_num = pdev->id;
	unsigned gpio;

	if (phy_num == 0)
		gpio = YAMO5410_DRD0_IDDET_GPIO;
#ifdef CONFIG_MACH_YAMO5410	//AGREEYA
	else if (phy_num == 1)
		gpio = YAMO5410_DRD1_IDDET_GPIO;
#endif
	else
		return -EINVAL;

	return gpio_get_value(gpio);
}

static bool yamo5410_drd30_get_bsession_valid(struct platform_device *pdev)
{
	int phy_num = pdev->id;
	unsigned gpio;

	if (phy_num == 0)
		gpio = YAMO5410_DRD0_VBUSDET_GPIO;
#ifdef CONFIG_MACH_YAMO5410	//AGREEYA
	else if (phy_num == 1)
		gpio = YAMO5410_DRD1_VBUSDET_GPIO;
#endif //AGREEYA
	else
		/*
		 * If something goes wrong, we return true,
		 * because we don't want switch stop working.
		 */
		return true;

	return !!gpio_get_value(gpio);
}

static struct exynos4_ohci_platdata yamo5410_ohci_pdata __initdata;
static struct s5p_ehci_platdata yamo5410_ehci_pdata __initdata;
static struct dwc3_exynos_data yamo5410_drd_pdata __initdata = {
	.udc_name		= "exynos-ss-udc",
	.xhci_name		= "exynos-xhci",
	.phy_type			= S5P_USB_PHY_DRD,
	.phy_init			= s5p_usb_phy_init,
	.phy_exit			= s5p_usb_phy_exit,
	.phy_crport_ctrl	= exynos5_usb_phy_crport_ctrl,
	.vbus_ctrl			= yamo5410_drd30_vbus_ctrl,
	.get_id_state		= yamo5410_drd30_get_id_state,
	.get_bses_vld		= yamo5410_drd30_get_bsession_valid,
	.irq_flags		= IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
};


static void __init yamo5410_ohci_init(void)
{
	exynos4_ohci_set_platdata(&yamo5410_ohci_pdata);
}

static void __init yamo5410_ehci_init(void)
{
	s5p_ehci_set_platdata(&yamo5410_ehci_pdata);
}

static void __init yamo5410_drd_phy_shutdown(struct platform_device *pdev)
{
	int phy_num = pdev->id;
	struct clk *clk;

	switch (phy_num) {
	case 0:
		clk = clk_get_sys("exynos-dwc3.0", "usbdrd30");
		break;
	case 1:
		clk = clk_get_sys("exynos-dwc3.1", "usbdrd30");
		break;
	default:
		clk = NULL;
		break;
	}

	if (IS_ERR_OR_NULL(clk)) {
		printk(KERN_ERR "failed to get DRD%d phy clock\n", phy_num);
		return;
	}

	if (clk_enable(clk)) {
		printk(KERN_ERR "failed to enable DRD%d clock\n", phy_num);
		return;
	}

	s5p_usb_phy_exit(pdev, S5P_USB_PHY_DRD);

	clk_disable(clk);
}


static void __init __maybe_unused yamo5410_drd0_init(void)
{
	/*
	// Initialize nmos gpio //
	printk(KERN_INFO "USB PVDD_5V disabled for USB device mode \n");
	//5V PWR control, GPY6[3], This pin should output low when device enabled.
	// GPY6CON[3] output to low (5V_PWREN)
	s3c_gpio_cfgpin(EXYNOS5410_GPY6(3), S3C_GPIO_SFN(0x1));
	// GPY6PUD[3] = 0
	s3c_gpio_setpull(EXYNOS5410_GPY6(3), S3C_GPIO_PULL_NONE);
	s5p_gpio_set_drvstr(EXYNOS5410_GPY6(3), S5P_GPIO_DRVSTR_LV1);
	// GPY6DAT[3] = 0
	gpio_set_value(EXYNOS5410_GPY6(3), 0);
	gpio_free(EXYNOS5410_GPY6(3)); */

	/* Initialize DRD0 gpio */
	if (gpio_request(UDRD3_0_OVERCUR_U2, "UDRD3_0_OVERCUR_U2")) {
		pr_err("failed to request UDRD3_0_OVERCUR_U2\n");
	} else {
		s3c_gpio_cfgpin(UDRD3_0_OVERCUR_U2, (0x2 << 0));
		s3c_gpio_setpull(UDRD3_0_OVERCUR_U2, S3C_GPIO_PULL_NONE);
		gpio_free(UDRD3_0_OVERCUR_U2);
	}
	if (gpio_request(UDRD3_0_OVERCUR_U3, "UDRD3_0_OVERCUR_U3")) {
		pr_err("failed to request UDRD3_0_OVERCUR_U3\n");
	} else {
		s3c_gpio_cfgpin(UDRD3_0_OVERCUR_U3, (0x2 << 4));
		s3c_gpio_setpull(UDRD3_0_OVERCUR_U3, S3C_GPIO_PULL_NONE);
		gpio_free(UDRD3_0_OVERCUR_U3);
	}

	if (gpio_request_one(UDRD3_0_VBUSCTRL_U2, GPIOF_OUT_INIT_LOW,
						"UDRD3_0_VBUSCTRL_U2")) {
		pr_err("failed to request UDRD3_0_VBUSCTRL_U2\n");
	} else {
		s3c_gpio_setpull(UDRD3_0_VBUSCTRL_U2, S3C_GPIO_PULL_NONE);
#if defined CONFIG_MACH_YAMO5410
		gpio_set_value(UDRD3_0_VBUSCTRL_U2, 1);
#endif
		gpio_free(UDRD3_0_VBUSCTRL_U2);
	}

	if (gpio_request_one(UDRD3_0_VBUSCTRL_U3, GPIOF_OUT_INIT_LOW,
						 "UDRD3_0_VBUSCTRL_U3")) {
		pr_err("failed to request UDRD3_0_VBUSCTRL_U3\n");
	} else {
		s3c_gpio_setpull(UDRD3_0_VBUSCTRL_U3, S3C_GPIO_PULL_NONE);
#if defined CONFIG_MACH_YAMO5410
		gpio_set_value(UDRD3_0_VBUSCTRL_U3, 1);
#endif
		gpio_free(UDRD3_0_VBUSCTRL_U3);
	}

#if defined(CONFIG_USB_EXYNOS5_USB3_DRD_CH0)
	if (gpio_request_one(YAMO5410_DRD0_IDDET_GPIO, GPIOF_IN, "UDRD3_0_ID")) {
		printk(KERN_ERR "failed to request UDRD3_0_ID\n");
		yamo5410_drd_pdata.id_irq = -1;
	} else {
		s3c_gpio_cfgpin(YAMO5410_DRD0_IDDET_GPIO, S3C_GPIO_SFN(0xF));
#ifdef CONFIG_MACH_YAMO5410		
		s3c_gpio_setpull(YAMO5410_DRD0_IDDET_GPIO, S3C_GPIO_PULL_NONE);
#else
#error not correct machine
#endif 
		gpio_free(YAMO5410_DRD0_IDDET_GPIO);

		yamo5410_drd_pdata.id_irq = gpio_to_irq(YAMO5410_DRD0_IDDET_GPIO);
	}

	if (gpio_request_one(YAMO5410_DRD0_VBUSDET_GPIO, GPIOF_IN, "UDRD3_0_VBUS")) {
		printk(KERN_ERR "failed to request UDRD3_0_VBUS\n");
		yamo5410_drd_pdata.vbus_irq = -1;
	} else {
		s3c_gpio_cfgpin(YAMO5410_DRD0_VBUSDET_GPIO, S3C_GPIO_SFN(0xF));
		s3c_gpio_setpull(YAMO5410_DRD0_VBUSDET_GPIO, S3C_GPIO_PULL_NONE);
		gpio_free(YAMO5410_DRD0_VBUSDET_GPIO);

		/*
		printk(KERN_INFO "USB PVDD_5V enabled for USB host mode \n");
		//5V PWR control, GPY6[3], This pin should output high when host enabled.
		// GPY6CON[3] output low to high (5V_PWREN)
		s3c_gpio_cfgpin(EXYNOS5410_GPY6(3), S3C_GPIO_SFN(0x1));
		// GPY6PUD[3] = 0
		s3c_gpio_setpull(EXYNOS5410_GPY6(3), S3C_GPIO_PULL_NONE);
		s5p_gpio_set_drvstr(EXYNOS5410_GPY6(3), S5P_GPIO_DRVSTR_LV1);
		// GPY6DAT[3] = 1
		gpio_set_value(EXYNOS5410_GPY6(3), 1);
		gpio_free(EXYNOS5410_GPY6(3));*/

		yamo5410_drd_pdata.vbus_irq = gpio_to_irq(YAMO5410_DRD0_VBUSDET_GPIO);
	}

	yamo5410_drd_pdata.quirks = 0;
#if !defined(CONFIG_USB_XHCI_EXYNOS)
	yamo5410_drd_pdata.quirks |= FORCE_RUN_PERIPHERAL;
#endif
#else
	yamo5410_drd_pdata.id_irq = -1;
	yamo5410_drd_pdata.vbus_irq = -1;
	yamo5410_drd_pdata.quirks = DUMMY_DRD;
#endif

	exynos5_usb3_drd0_set_platdata(&yamo5410_drd_pdata);
}

static void __init __maybe_unused yamo5410_drd1_init(void)
{
	/* Initialize DRD1 gpio */
	if (gpio_request(UDRD3_1_OVERCUR_U2, "UDRD3_1_OVERCUR_U2")) {
		pr_err("failed to request UDRD3_1_OVERCUR_U2\n");
	} else {
		s3c_gpio_cfgpin(UDRD3_1_OVERCUR_U2, (0x2 << 16));
		s3c_gpio_setpull(UDRD3_1_OVERCUR_U2, S3C_GPIO_PULL_NONE);
		gpio_free(UDRD3_1_OVERCUR_U2);
	}

	if (gpio_request(UDRD3_1_OVERCUR_U3, "UDRD3_1_OVERCUR_U3")) {
		pr_err("failed to request UDRD3_1_OVERCUR_U3\n");
	} else {
		s3c_gpio_cfgpin(UDRD3_1_OVERCUR_U3, (0x2 << 20));
		s3c_gpio_setpull(UDRD3_1_OVERCUR_U3, S3C_GPIO_PULL_NONE);
		gpio_free(UDRD3_1_OVERCUR_U3);
	}

	if (gpio_request_one(UDRD3_1_VBUSCTRL_U2, GPIOF_OUT_INIT_LOW,
				"UDRD3_1_VBUSCTRL_U2")) {
		pr_err("failed to request UDRD3_1_VBUSCTRL_U2\n");
	} else {
		s3c_gpio_setpull(UDRD3_1_VBUSCTRL_U2, S3C_GPIO_PULL_NONE);
#ifdef CONFIG_MACH_YAMO5410
		gpio_set_value(UDRD3_1_VBUSCTRL_U2, 1);
#endif
		gpio_free(UDRD3_1_VBUSCTRL_U2);
	}

	if (gpio_request_one(UDRD3_1_VBUSCTRL_U3, GPIOF_OUT_INIT_LOW,
				"UDRD3_1_VBUSCTRL_U3")) {
		pr_err("failed to request UDRD3_1_VBUSCTRL_U3\n");
	} else {
		s3c_gpio_setpull(UDRD3_1_VBUSCTRL_U3, S3C_GPIO_PULL_NONE);
#ifdef CONFIG_MACH_YAMO5410
		gpio_set_value(UDRD3_1_VBUSCTRL_U3, 1);
#endif
		gpio_free(UDRD3_1_VBUSCTRL_U3);
	}

	if (gpio_request_one(YAMO5410_DRD1_IDDET_GPIO, GPIOF_IN, "UDRD3_1_ID")) {
		printk(KERN_ERR "failed to request UDRD3_1_ID\n");
		yamo5410_drd_pdata.id_irq = -1;
	} else {
		s3c_gpio_cfgpin(YAMO5410_DRD1_IDDET_GPIO, S3C_GPIO_SFN(0xF));
		s3c_gpio_setpull(YAMO5410_DRD1_IDDET_GPIO, S3C_GPIO_PULL_DOWN);
		gpio_free(YAMO5410_DRD1_IDDET_GPIO);

		yamo5410_drd_pdata.id_irq = gpio_to_irq(YAMO5410_DRD1_IDDET_GPIO);
	}

	if (gpio_request_one(YAMO5410_DRD1_VBUSDET_GPIO, GPIOF_IN, "UDRD3_1_VBUS")) {
		printk(KERN_ERR "failed to request UDRD3_1_VBUS\n");
		yamo5410_drd_pdata.vbus_irq = -1;
	} else {
		s3c_gpio_cfgpin(YAMO5410_DRD1_VBUSDET_GPIO, S3C_GPIO_SFN(0xF));
		s3c_gpio_setpull(YAMO5410_DRD1_VBUSDET_GPIO, S3C_GPIO_PULL_NONE);
		gpio_free(YAMO5410_DRD1_VBUSDET_GPIO);

		yamo5410_drd_pdata.vbus_irq = gpio_to_irq(YAMO5410_DRD1_VBUSDET_GPIO);
	}
#ifdef CONFIG_MACH_YAMO5410 //AGREEYA
	yamo5410_drd_pdata.quirks = 0;
#if !defined(CONFIG_USB_XHCI_EXYNOS)
	yamo5410_drd_pdata.quirks |= FORCE_RUN_PERIPHERAL;
#endif
#else
	yamo5410_drd_pdata.id_irq = -1;
	yamo5410_drd_pdata.vbus_irq = -1;
	yamo5410_drd_pdata.quirks = DUMMY_DRD;
#endif
	exynos5_usb3_drd1_set_platdata(&yamo5410_drd_pdata);
}


static struct s5p_usbswitch_platdata yamo5410_usbswitch_pdata __initdata;

static void __init yamo5410_usbswitch_init(void)
{
	struct s5p_usbswitch_platdata *pdata = &yamo5410_usbswitch_pdata;
	int err;

#if defined(CONFIG_USB_EHCI_S5P) || defined(CONFIG_USB_OHCI_EXYNOS)
		pdata->gpio_host_detect = YAMO5410_USB20_IDDET_GPIO; // EXYNOS5410_GPX2(6);
		err = gpio_request_one(pdata->gpio_host_detect, GPIOF_IN,
			"HOST_DETECT");
		if (err) {
			pr_err("failed to request host gpio\n");
			return;
		}

		s3c_gpio_cfgpin(pdata->gpio_host_detect, S3C_GPIO_SFN(0xF));
		/* Fix YAMO5410 hardware
			YAMO5410 have no external pull up 
		*/
#ifdef CONFIG_MACH_YAMO5410		
		s3c_gpio_setpull(pdata->gpio_host_detect, S3C_GPIO_PULL_UP);
#elif defined(CONFIG_MACH_SMDK5410)
		s3c_gpio_setpull(pdata->gpio_host_detect, S3C_GPIO_PULL_NONE);
#else
#error not correct MACH
#endif
		gpio_free(pdata->gpio_host_detect);

		pdata->gpio_host_vbus = YAMO5410_USB20_VBUSDET_GPIO;
		err = gpio_request_one(pdata->gpio_host_vbus,
			GPIOF_OUT_INIT_LOW,
			"HOST_VBUS_CONTROL");
		if (err) {
			pr_err("failed to request host_vbus gpio\n");
			return;
		}

		s3c_gpio_setpull(pdata->gpio_host_vbus, S3C_GPIO_PULL_NONE);
		gpio_free(pdata->gpio_host_vbus);
#endif

	s5p_usbswitch_set_platdata(pdata);
}

static struct platform_device *yamo5410_usb_devices[] __initdata = {
	&s5p_device_ehci,
	&exynos4_device_ohci,
	&exynos5_device_usb3_drd0,
#ifdef CONFIG_MACH_YAMO5410 //AGREEYA
	&exynos5_device_usb3_drd1,
#endif //AGREEYA
};

void __init exynos5_yamo5410_usb_init(void)
{
	yamo5410_ohci_init();
	yamo5410_ehci_init();

	if (soc_is_exynos5410() && samsung_rev() == 0)
		yamo5410_drd_pdata.quirks |= EXYNOS_PHY20_NO_SUSPEND;

	/*
	 * Shutdown DRD PHYs to reduce power consumption.
	 * Later, DRD driver will turn on only the PHY it needs.
	 */
	yamo5410_drd_phy_shutdown(&exynos5_device_usb3_drd0);
	yamo5410_drd_phy_shutdown(&exynos5_device_usb3_drd1);
	yamo5410_drd0_init();
#ifdef CONFIG_MACH_YAMO5410 //AGREEYA
	yamo5410_drd1_init();
#endif //AGREEYA
#ifdef CONFIG_USB_EXYNOS_SWITCH
	yamo5410_usbswitch_init();
#endif
	platform_add_devices(yamo5410_usb_devices,
			ARRAY_SIZE(yamo5410_usb_devices));

}
