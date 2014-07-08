
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/tty.h>
#include <linux/tty_ldisc.h>
#include <linux/delay.h>
#include <plat/gpio-cfg.h>
#include <linux/gpio.h>

#define N_DMX512 25

static int dmx512_open(struct tty_struct *tty)
{
	if (!tty->ops->write)
		return -EINVAL;

	return 0;
}

static void dmx512_close(struct tty_struct *tty)
{
	return 0;
}

//|<------BREAK----->|<--MAB-->|<--Start Code(0)-->|<-Dimmer1->|<-Dimmer2->|<-Dimmer3->|...|

static ssize_t dmx512_write(struct tty_struct *tty, struct file *file,
		const unsigned char *buf, size_t nr)
{
	unsigned char start_code = 0;
	int c;
	const unsigned char *b = buf;

	//
	int gpio = -1;
	if (tty->index == 2) {
		gpio = EXYNOS5410_GPA1(1);
	} else if (tty->index == 3) {
		gpio = EXYNOS5410_GPA1(5);
	}

	if (gpio != -1) {
		//generate BREAK and MAB signal
		gpio_request(gpio, "GPA1");
		s3c_gpio_cfgpin(gpio, S3C_GPIO_OUTPUT);
		gpio_direction_output(gpio, 0);
		udelay(88);
		gpio_direction_output(gpio, 1);
		udelay(12);
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0x2));
		gpio_free(gpio);
	}


	while (1) {
		c = tty->ops->write(tty, &start_code, 1);
		if (c < 0)
			return -EIO;
		if (c == 1)
			break;
	}

	while (nr > 0) {
		c = tty->ops->write(tty, b, nr);
		if (c < 0)
			return -EIO;
		if (!c)
			break;
		b += c;
		nr -= c;
	}

	return (b - buf);
}

static struct tty_ldisc_ops tty_ldisc_n_dmx512 = {
	.owner = THIS_MODULE,
	.magic = TTY_LDISC_MAGIC,
	.name = "n_dmx512",
	.open = dmx512_open,
	.close = dmx512_close,
	.write = dmx512_write,
};

static int __init n_dmx512_init(void)
{
	int retval;

	retval = tty_register_ldisc(N_DMX512, &tty_ldisc_n_dmx512);
	if (retval < 0)
		pr_err("%s: Registration failed: %d\n", __func__, retval);

	return retval;
}

static void __exit n_dmx512_exit(void)
{
	int retval = tty_unregister_ldisc(N_DMX512);

	if (retval < 0)
		pr_err("%s: Unregistration failed: %d\n", __func__,  retval);
}

module_init(n_dmx512_init);
module_exit(n_dmx512_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Derek Quan");
MODULE_ALIAS_LDISC(N_DMX512);
MODULE_DESCRIPTION("DMX512 ldisc driver");
