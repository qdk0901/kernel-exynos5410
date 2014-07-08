/*
 * Vibrator Driver for TF4
 *
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/irq.h>
#include <linux/wakelock.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <plat/gpio-cfg.h>
#include <linux/i2c.h>
//#include <mach/regs-pmu5.h>
#include <asm/io.h>
#include <plat/map-base.h>
//#include <plat/map-s5p.h>


#define VIB_TEST	0

static void exynos_vib_off(void)
{
#if VIB_TEST
	printk("exynos_vib_off\n");
#endif
	/* MOTOR_PWM: GPB2_1 */
	gpio_request(EXYNOS5410_GPB2(1),"GPB2");
	
	if(samsung_rev() > EXYNOS5410_REV_1_0)
    gpio_direction_output(EXYNOS5410_GPB2(1),0);
  else
    gpio_direction_output(EXYNOS5410_GPB2(1),1);
   
	gpio_free(EXYNOS5410_GPB2(1));
}

static void exynos_vib_on(int value)
{
#if VIB_TEST
	printk("exynos_vib_on\n");
#endif
	/* MOTOR_PWM: GPB2_1 */

	gpio_request(EXYNOS5410_GPB2(1),"GPB2");

	if(samsung_rev() > EXYNOS5410_REV_1_0)
		gpio_direction_output(EXYNOS5410_GPB2(1),1);
	else
		gpio_direction_output(EXYNOS5410_GPB2(1),0);

	gpio_free(EXYNOS5410_GPB2(1));

	mdelay(value*3);
	exynos_vib_off();
}

int strtoint(const char *str,int len)
{
	int result = 0;
	int i = 0;
	char c;
	while(i <= len-1)
	{   
		c = str[i++];
		if(c<'0' || c>'9')
			break;
		result = 10*result + c - '0';
	}
	return result;
}

static ssize_t exynos_vib_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return printk("write a number in to vibrate\n");
}

static ssize_t exynos_vib_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int retval = count;
	int value;

	exynos_vib_off( );
	value = strtoint(buf,count);
	
#if VIB_TEST
	printk("count:%d, buf:%s",count,buf);
	printk("inv:%d ms\n",value);
#endif
	
	if(value)
	{
		exynos_vib_on(value);		
	}
	
	return retval;
}

/*
 * sysfs: /sys/devices/platform/exynos-vib
 * usage: echo ms > /sys/devices/platform/exynos-vib/vib
 */
static DEVICE_ATTR(vib, 0666, exynos_vib_show, exynos_vib_store);

static int exynos_vib_probe(struct platform_device *pdev)
{
	int err;
	err = device_create_file(&pdev->dev, &dev_attr_vib);

	if(err)
	{
		printk(KERN_ERR "failed to create attr file\n");
		return err;
	}

	printk("vibrator probe success\n");

	return 0;
}

static int exynos_vib_remove(struct platform_device *pdev)
{
	exynos_vib_off();
	return 0;
}

#ifdef CONFIG_PM
static int exynos_vib_suspend(struct platform_device *pdev, pm_message_t state)
{
	exynos_vib_off();
	return 0;
}

static int exynos_vib_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define exynos_vib_suspend NULL
#define exynos_vib_resume  NULL
#endif

static struct platform_driver exynos_vib_driver = {
	.driver = {
		.name	= "exynos-vib",
		.owner	= THIS_MODULE,
	},
	.probe		= exynos_vib_probe,
	.remove		= __devexit_p(exynos_vib_remove),
	.suspend	= exynos_vib_suspend,
	.resume 	= exynos_vib_resume,
};

static int __init exynos_vib_init(void)
{
	printk("TF4 VIBRATOR\n");
	return platform_driver_register(&exynos_vib_driver);
}

static void __exit exynos_vib_exit(void)
{
	printk("%s()\n",__FUNCTION__);
	platform_driver_unregister(&exynos_vib_driver);
}

module_init(exynos_vib_init);
module_exit(exynos_vib_exit);

MODULE_DESCRIPTION("TF4 Vibrator Driver");
MODULE_LICENSE("GPL");
