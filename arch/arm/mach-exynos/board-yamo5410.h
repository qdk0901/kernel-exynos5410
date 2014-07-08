/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MACH_EXYNOS_BOARD_YAMO5410_H
#define __MACH_EXYNOS_BOARD_YAMO5410_H


enum board_version_type {
	BOARD_EVT = 0,
	BOARD_DVT,
	BOARD_PVT,
};
void exynos5_yamo5410_clock_init(void);
void exynos5_yamo5410_mmc_init(void);
void exynos5_yamo5410_power_init(void);
void exynos5_yamo5410_audio_init(void);
void exynos5_yamo5410_usb_init(void);
void exynos5_yamo5410_input_init(void);
void exynos5_yamo5410_media_init(void);
void exynos5_yamo5410_display_init(void);
void exynos5_yamo5410_bt_init(void);
void yamo5410_nfc_init(void);
enum board_version_type get_board_version(void);

#endif
