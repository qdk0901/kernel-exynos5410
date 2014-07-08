#include <linux/platform_device.h>
#include <linux/nfc/bcm2079x.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/devs.h>
#include <mach/regs-gpio.h>
#include <plat/iic.h>

#define GPIO_NFC_IRQ        EXYNOS5410_GPX3(1)
#define GPIO_NFC_REG_PU     EXYNOS5410_GPF1(0)
#define GPIO_NFC_WAKE       EXYNOS5410_GPJ1(7)


static unsigned int nfc_gpio_table[][4] = {
	{GPIO_NFC_IRQ, S3C_GPIO_INPUT, 2, S3C_GPIO_PULL_DOWN},
	{GPIO_NFC_REG_PU, S3C_GPIO_OUTPUT, 1, S3C_GPIO_PULL_DOWN},
	{GPIO_NFC_WAKE, S3C_GPIO_OUTPUT, 1, S3C_GPIO_PULL_NONE},
};

static inline void nfc_setup_gpio(void)
{
	int err = 0;
	int array_size = ARRAY_SIZE(nfc_gpio_table);
	u32 i, gpio;
	for (i = 0; i < array_size; i++) {
		gpio = nfc_gpio_table[i][0];

		err = s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(nfc_gpio_table[i][1]));
		if (err < 0)
			pr_err("%s, s3c_gpio_cfgpin gpio(%d) fail(err = %d)\n",
				__func__, i, err);

		err = s3c_gpio_setpull(gpio, nfc_gpio_table[i][3]);
		if (err < 0)
			pr_err("%s, s3c_gpio_setpull gpio(%d) fail(err = %d)\n",
				__func__, i, err);

		if (nfc_gpio_table[i][2] != 2)
			gpio_set_value(gpio, nfc_gpio_table[i][2]);
	}
}

/* NFC */
static struct bcm2079x_platform_data bcm3079x_pdata = {
	.irq_gpio = GPIO_NFC_IRQ,
	.en_gpio = GPIO_NFC_REG_PU,
	.wake_gpio = GPIO_NFC_WAKE,
};

static struct i2c_board_info i2c_dev_nfc[] __initdata = {
	{
		I2C_BOARD_INFO("bcm2079x-i2c", 0x77),
		.platform_data = &bcm3079x_pdata,
		.irq	= IRQ_EINT(25),
	},
};

void __init yamo5410_nfc_init(void)
{
	nfc_setup_gpio();
	s3c_i2c1_set_platdata(NULL);
	i2c_register_board_info(1, i2c_dev_nfc, ARRAY_SIZE(i2c_dev_nfc));
	platform_device_register(&s3c_device_i2c1);
}
