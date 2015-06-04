/*
 * lmdaad.c
 *
 */

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/soc.h>

/*
 * Note this is a simple chip with no configuration interface, sample rate is
 * determined automatically by examining the Master clock and Bit clock ratios
 */
#define LMDAAD_RATES  (SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_44100)


static struct snd_soc_dai_driver lmdaad_dai[] = {
	{
		.name = "lmdaad-hifi-playback",
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = LMDAAD_RATES,
			.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
		},
	},
};

static struct snd_soc_codec_driver soc_codec_dev_lmdaad;

static __devinit int lmdaad_probe(struct platform_device *pdev)
{
	return snd_soc_register_codec(&pdev->dev,
			&soc_codec_dev_lmdaad, lmdaad_dai, 1);
}

static int __devexit lmdaad_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

static struct platform_driver lmdaad_codec_driver = {
	.driver = {
			.name = "lmdaad-codec",
			.owner = THIS_MODULE,
	},

	.probe = lmdaad_probe,
	.remove = __devexit_p(lmdaad_remove),
};

module_platform_driver(lmdaad_codec_driver);

MODULE_DESCRIPTION("ASoC LMDAAD driver");
MODULE_AUTHOR("Derek Quan");
MODULE_LICENSE("GPL");
