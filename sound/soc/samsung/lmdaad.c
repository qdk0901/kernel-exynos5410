/*
 *  lmdaad.c
 *
 */

#include <linux/module.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <linux/clk.h>
#include <linux/delay.h>

#include <asm/mach-types.h>
#include <mach/map.h>
#include <asm/io.h>

#include "i2s.h"
#include "i2s-regs.h"

static struct clk *mclk;
static struct clk *xxti_clk;


#ifdef CONFIG_SND_SAMSUNG_I2S_MASTER
static int set_epll_rate(unsigned long rate)
{
	struct clk *fout_epll;

	fout_epll = clk_get(NULL, "fout_epll");
	if (IS_ERR(fout_epll)) {
		printk(KERN_ERR "%s: failed to get fout_epll\n", __func__);
		return PTR_ERR(fout_epll);
	}

	if (rate == clk_get_rate(fout_epll))
		goto out;

	clk_set_rate(fout_epll, rate);
out:
	clk_put(fout_epll);

	return 0;
}
#endif /* CONFIG_SND_SAMSUNG_I2S_MASTER */

#ifndef CONFIG_SND_SAMSUNG_I2S_MASTER
static int smdk_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;

	int ret = 0;

	printk("lmdaad format %d\n", params_format(params));

	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S
					 | SND_SOC_DAIFMT_NB_NF
					 | SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;


	ret = snd_soc_dai_set_sysclk(cpu_dai, SAMSUNG_I2S_OPCLK,
					0, MOD_OPCLK_PCLK);
	if (ret < 0)
		return ret;

	return 0;
}
#else /* CONFIG_SND_SAMSUNG_I2S_MASTER */

static int smdk_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int bfs, psr, rfs, ret;
	unsigned long rclk;
	
	switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_U24:
		case SNDRV_PCM_FORMAT_S24:
			bfs = 48;
			break;
		case SNDRV_PCM_FORMAT_U16_LE:
		case SNDRV_PCM_FORMAT_S16_LE:
			bfs = 32;
			break;
		default:
			printk("error format\n");
			return -EINVAL;
	}

	switch (params_rate(params)) {
		case 16000:
		case 22050:
		case 24000:
		case 32000:
		case 44100:
		case 48000:
		case 88200:
		case 96000:
			if (bfs == 48)
				rfs = 384;
			else
				rfs = 256;
			break;
		case 64000:
			rfs = 384;
			break;
		case 8000:
		case 11025:
		case 12000:
			if (bfs == 48)
				rfs = 768;
			else
				rfs = 512;
			break;
		default:
			printk("error sample rate\n");
			return -EINVAL;
	}

	rclk = params_rate(params) * rfs;

	switch (rclk) {
		case 4096000:
		case 5644800:
		case 6144000:
		case 8467200:
		case 9216000:
			psr = 8;
			break;
		case 8192000:
		case 11289600:
		case 12288000:
		case 16934400:
		case 18432000:
			psr = 4;
			break;
		case 22579200:
		case 24576000:
		case 33868800:
		case 36864000:
			psr = 2;
			break;
		case 67737600:
		case 73728000:
			psr = 1;
			break;
		default:
			printk("Not yet supported!\n");
			return -EINVAL;
	}

	set_epll_rate(rclk * psr);

	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_LEFT_J
					| SND_SOC_DAIFMT_NB_NF
					| SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0) {
		printk("lmdaad set format error\n");
		return ret;
	}

	ret = snd_soc_dai_set_sysclk(cpu_dai, SAMSUNG_I2S_CDCLK,
					rfs, SND_SOC_CLOCK_OUT);
	if (ret < 0) {
		printk("lmdaad set clock error\n");
		return ret;
	}

	ret = snd_soc_dai_set_sysclk(cpu_dai, SAMSUNG_I2S_RCLKSRC_1,
					rfs, SND_SOC_CLOCK_OUT);
	if (ret < 0) {
		printk("lmdaad set clock error\n");
		return ret;
	}

	ret = snd_soc_dai_set_clkdiv(cpu_dai, SAMSUNG_I2S_DIV_BCLK, bfs);
	if (ret < 0) {
		printk("lmdaad set clock div error\n");
		return ret;
	}

	printk("rclk:%d, psr:%d\n", rclk, psr);
	printk("AUDSS SRC %08x\n", __raw_readl(S5P_VA_AUDSS + 0x0));
	printk("AUDSS DIV %08x\n", __raw_readl(S5P_VA_AUDSS + 0x4));
	
	

	return 0;
}

#endif

static struct snd_soc_ops lmdaad_ops = {
	.hw_params = smdk_hw_params,
};


static int smdk_lmdaad_init_paiftx(struct snd_soc_pcm_runtime *rtd)
{
	return 0;
}

enum {
	PLAYBACK = 0,
};

static struct snd_soc_dai_link lmdaad_dai = {
	.name = "lmdaad-machine",
	.stream_name = "lmdaad-stream",
	.cpu_dai_name = "samsung-i2s.0",
	.codec_dai_name = "lmdaad-codec",
	.platform_name = "samsung-audio",
	.codec_name = "lmdaad-codec",
	.ops = &lmdaad_ops,
	.init = smdk_lmdaad_init_paiftx,
};

static struct snd_soc_card lmdaad = {
	.name = "lmdaad-card",
	.owner = THIS_MODULE,
	.dai_link = &lmdaad_dai,
	.num_links = 1,
};

static struct platform_device *smdk_snd_device;

static int __devinit lmdaad_snd_probe(struct platform_device *pdev)
{
	int ret = 0;

	lmdaad.dev = &pdev->dev;
	ret = snd_soc_register_card(&lmdaad);
	if (ret) {
		dev_err(&pdev->dev, "snd_soc_register_card is failed.");
	}
	return ret;
}

static int __devexit lmdaad_snd_remove(struct platform_device *pdev)
{
	snd_soc_unregister_card(&lmdaad);
	return 0;
}


static struct platform_driver lmdaad_snd_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "lmdaad-soc",
		.pm = &snd_soc_pm_ops,
	},
	.probe = lmdaad_snd_probe,
	.remove = __devexit_p(lmdaad_snd_remove),
};

module_platform_driver(lmdaad_snd_driver);

MODULE_AUTHOR("Derek Quan");
MODULE_DESCRIPTION("ALSA SoC LMDAAD");
MODULE_LICENSE("GPL");
