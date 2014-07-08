/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>

#include <media/rc-map.h>

static struct rc_map_table samsung_necx[] = {
	{ 0x70702, KEY_POWER},		/* power */
	{ 0x7070f, KEY_MUTE},		/* mute */
	{ 0x70704, KEY_1},
	{ 0x70705, KEY_2},
	{ 0x70706, KEY_3},
	{ 0x70708, KEY_4},
	{ 0x70709, KEY_5},
	{ 0x7070a, KEY_6},
	{ 0x7070c, KEY_7},
	{ 0x7070d, KEY_8},
	{ 0x7070e, KEY_9},
	{ 0x70711, KEY_0},
	{ 0x70712, KEY_CHANNELUP},
	{ 0x70710, KEY_CHANNELDOWN},
	{ 0x70707, KEY_VOLUMEUP},
	{ 0x7070b, KEY_VOLUMEDOWN},
	{ 0x70760, KEY_UP},
	{ 0x70768, KEY_ENTER},
	{ 0x70761, KEY_DOWN},
	{ 0x70765, KEY_LEFT},
	{ 0x70762, KEY_RIGHT},
	{ 0x7072d, KEY_EXIT},
	{ 0x70749, KEY_RECORD},
	{ 0x70747, KEY_PLAY},
	{ 0x70746, KEY_STOP},
	{ 0x70745, KEY_REWIND},
	{ 0x70748, KEY_FORWARD},
	{ 0x7074a, KEY_PAUSE},
	{ 0x70703, KEY_SLEEP},
	{ 0x7076c, KEY_RED},
	{ 0x70714, KEY_GREEN},
	{ 0x70715, KEY_YELLOW},
	{ 0x70716, KEY_BLUE},
	{ 0x70758, KEY_BACK},
	{ 0x7071a, KEY_MENU},
	{ 0x7076b, KEY_LIST},
	{ 0x70701, KEY_TV2},
	{ 0x7071f, KEY_INFO},
	{ 0x7071b, KEY_TV},
	{ 0x7078b, KEY_AUX},
	{ 0x7078c, KEY_MEDIA},

	// Philips format
	{ 0x00, KEY_NUMERIC_0 },
	{ 0x01, KEY_NUMERIC_1 },
	{ 0x02, KEY_NUMERIC_2 },
	{ 0x03, KEY_NUMERIC_3 },
	{ 0x04, KEY_NUMERIC_4 },
	{ 0x05, KEY_NUMERIC_5 },
	{ 0x06, KEY_NUMERIC_6 },
	{ 0x07, KEY_NUMERIC_7 },
	{ 0x08, KEY_NUMERIC_8 },
	{ 0x09, KEY_NUMERIC_9 },
	{ 0xF4, KEY_SOUND },
	{ 0xF3, KEY_SCREEN },	/* Picture */

	{ 0x10, KEY_VOLUMEUP },
	{ 0x11, KEY_VOLUMEDOWN },
	{ 0x0d, KEY_MUTE },
	{ 0x20, KEY_CHANNELUP },
	{ 0x21, KEY_CHANNELDOWN },
	{ 0x0A, KEY_BACK },
	{ 0x0f, KEY_INFO },
	{ 0x5c, KEY_OK },
	{ 0x58, KEY_UP },
	{ 0x59, KEY_DOWN },
	{ 0x5a, KEY_LEFT },
	{ 0x5b, KEY_RIGHT },
	{ 0xcc, KEY_PAUSE },
	{ 0x6d, KEY_PVR },	/* Demo */
	{ 0x40, KEY_EXIT },
	{ 0x6e, KEY_PROG1 },	/* Scenea */
	{ 0x6f, KEY_MODE },	/* Dual */
	{ 0x70, KEY_SLEEP },
	{ 0xf5, KEY_TUNER },	/* Format */

	{ 0x4f, KEY_TV },
	{ 0x3c, KEY_NEW },	/* USB */
	{ 0x38, KEY_COMPOSE },	/* Source */
	{ 0x54, KEY_MENU },

	{ 0x0C, KEY_POWER },
};

static struct rc_map_list samsung_necx_map = {
	.map = {
		.scan    = samsung_necx,
		.size    = ARRAY_SIZE(samsung_necx),
		.rc_type = RC_TYPE_NEC | RC_TYPE_RC6,
		.name    = RC_MAP_SAMSUNG_NECX,
	}
};

static int __init init_rc_map_samsung_necx(void)
{
	return rc_map_register(&samsung_necx_map);
}

static void __exit exit_rc_map_samsung_necx(void)
{
	rc_map_unregister(&samsung_necx_map);
}

module_init(init_rc_map_samsung_necx)
module_exit(exit_rc_map_samsung_necx)

MODULE_DESCRIPTION("SAMSUNG IR Remote Keymap");
MODULE_LICENSE("GPL v2");