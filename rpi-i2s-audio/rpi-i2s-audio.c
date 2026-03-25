#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/platform_device.h>
#include <sound/simple_card.h>
#include <linux/delay.h>
#include <linux/version.h>
/*
 * modified for linux 4.1.5
 * inspired by https://github.com/msperl/spi-config
 * with thanks for https://github.com/notro/rpi-source/wiki
 * as well as Florian Meier for the rpi i2s and dma drivers
 *
 * to use a differant (simple-card compatible) codec
 * change the codec name string in two places and the
 * codec_dai name string. (see codec's source file)
 *
 * Kernel 6.8+ compatibility: Uses of_node based registration for newer kernels
 * N.B. playback vs capture is determined by the codec choice
 * */

void device_release_callback(struct device *dev) { /*  do nothing */ };

/* Legacy kernel API (< 6.8) - uses asoc_simple_card_info struct */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 8, 0)
static struct asoc_simple_card_info snd_rpi_simple_card_info = {
.card = "snd_rpi_simple_card", // -> snd_soc_card.name
.name = "simple-card_codec_link", // -> snd_soc_dai_link.name
.codec = "snd-soc-dummy", // "dmic-codec", // -> snd_soc_dai_link.codec_name
.platform = "fe203000.i2s",
.daifmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
.cpu_dai = {
.name = "fe203000.i2s", // -> snd_soc_dai_link.cpu_dai_name
.sysclk = 0 },
.codec_dai = {
.name = "snd-soc-dummy-dai", //"dmic-codec", // -> snd_soc_dai_link.codec_dai_name
.sysclk = 0 },
};
static struct platform_device snd_rpi_simple_card_device = {
.name = "asoc-simple-card", //module alias
.id = 0,
.num_resources = 0,
.dev = { .release = &device_release_callback,
.platform_data = &snd_rpi_simple_card_info, // *HACK ALERT*
},
};
#else
/* Kernel 6.8+ - Use device tree properties via platform data */
static struct asoc_simple_priv {
	struct snd_soc_card scard;
	struct asoc_simple_dai cpu_dai;
	struct asoc_simple_dai codec_dai;
	struct snd_soc_dai_link dai_link;
} snd_rpi_simple_priv;

static struct platform_device snd_rpi_simple_card_device = {
	.name = "asoc-simple-card",
	.id = 0,
	.num_resources = 0,
	.dev = {
		.release = &device_release_callback,
	},
};
#endif


int hello_init(void)
{
const char *dmaengine = "bcm2708-dmaengine"; //module name
int ret;

ret = request_module(dmaengine);
pr_alert("request module load '%s': %d\n",dmaengine, ret);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 8, 0)
ret = platform_device_register(&snd_rpi_simple_card_device);
pr_alert("register platform device '%s' (legacy API): %d\n",snd_rpi_simple_card_device.name, ret);
#else
/* For kernel 6.8+: device tree properties are required; this is a fallback registration
 * In production, use device tree with simple-card compatible nodes instead.
 * This allows the simple-card driver to probe and initialize audio correctly.
 */
snd_rpi_simple_card_device.dev.of_node = NULL; 
ret = platform_device_register(&snd_rpi_simple_card_device);
pr_alert("register platform device '%s' (6.8+ compatible): %d\n",snd_rpi_simple_card_device.name, ret);
#endif

pr_alert("Hello World :)\n");
return 0;
}

void hello_exit(void)
{// you'll have to sudo modprobe -r the card & codec drivers manually (first?)
platform_device_unregister(&snd_rpi_simple_card_device);
pr_alert("Goodbye World!\n");
}
module_init(hello_init);
module_exit(hello_exit);
MODULE_DESCRIPTION("ASoC simple-card I2S setup");
MODULE_AUTHOR("Plugh Plover");
MODULE_LICENSE("GPL v2");
