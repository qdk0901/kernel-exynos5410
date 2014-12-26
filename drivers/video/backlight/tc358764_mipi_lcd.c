/* linux/drivers/video/backlight/tc358764_mipi_lcd.c
 *
 *
 * Copyright (c) 2011 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
*/

#include <linux/delay.h>

#include <video/mipi_display.h>

#include <plat/dsim.h>
#include <plat/mipi_dsi.h>
#include <plat/cpu.h>
#include <linux/i2c.h>

static struct reg_val_t {
    unsigned short  reg;    // Control register address
    unsigned int    data;   // Register data value
};

struct reg_val_t initcode_65[] = {
	//{ **************************************************
	//{ TC358764/65XBG DSI Basic Parameters.  Following 10 setting should be pefromed in LP mode
	//{ **************************************************
	{ 0x013C, 0x00070009 }, //PPI_TX_RX_TA  BTA parameters
	{ 0x0114, 0x00000006 }, //PPI_LPTXTIMCNT
	{ 0x0164, 0x00000007 }, //PPI_D0S_CLRSIPOCOUNT
	{ 0x0168, 0x00000007 }, //PPI_D1S_CLRSIPOCOUNT
	{ 0x016C, 0x00000007 }, //PPI_D2S_CLRSIPOCOUNT
	{ 0x0170, 0x00000007 }, //PPI_D3S_CLRSIPOCOUNT
	{ 0x0134, 0x0000001F }, //PPI_LANEENABLE
	{ 0x0210, 0x0000001F }, //DSI_LANEENABLE
	{ 0x0104, 0x00000001 }, //PPI_SARTPPI
	{ 0x0204, 0x00000001 }, //DSI_SARTPPI
	//{ **************************************************
	//{ TC358764/65XBG Timing and mode setting
	//{ **************************************************
	{ 0x0450, 0x00000120 }, //VPCTRL
	{ 0x0454, 0x00500014 }, //HTIM1
	//{ 0458
	{ 0x045C, 0x000A000A }, //VTIM1
	//{ 0460
	{ 0x0464, 0x00000001 }, //VFUEN
	{ 0x04A0, 0x00000006 }, //LVPHY0
	//sleep 1
	{ 0x04A0, 0x00000006 }, //LVPHY0
	{ 0x0504, 0x00000004 }, //SYSRST
	//{ **************************************************
	//{ TC358764/65XBG LVDS Color mapping setting
	//{ **************************************************
	{ 0x0480, 0x03020100 }, //LVMX0003
	{ 0x0484, 0x08050704 }, //LVMX0407
	{ 0x0488, 0x0F0E0A09 }, //LVMX0811
	{ 0x048C, 0x100D0C0B }, //LVMX1215
	{ 0x0490, 0x12111716 }, //LVMX1619
	{ 0x0494, 0x1B151413 }, //LVMX2023
	{ 0x0498, 0x061A1918 }, //LVMX2427
	//{ **************************************************
	//{ TC358764/65XBG LVDS enable
	//{ **************************************************
	{ 0x049C, 0x00000403 }, //LVCFG
};

struct reg_val_t initcode_75[] = {
	{	0x013C	,0x00030005},//	PPI_TX_RX_TA  BTA parameters	LP
	{	0x0114	,0x00000003},//		PPI_LPTXTIMCNT	LP
	{	0x0164	,0x00000004},//		PPI_D0S_CLRSIPOCOUNT	LP
	{	0x0168	,0x00000004},//		PPI_D1S_CLRSIPOCOUNT	LP
	{	0x016C	,0x00000004},//		PPI_D2S_CLRSIPOCOUNT	LP
	{	0x0170	,0x00000004},//		PPI_D3S_CLRSIPOCOUNT	LP
	{	0x0134	,0x0000001F},//		PPI_LANEENABLE	LP
	{	0x0210	,0x0000001F},//		DSI_LANEENABLE	LP
	{	0x0104	,0x00000001},//		PPI_SARTPPI	LP
	{	0x0204	,0x00000001},//		DSI_SARTPPI	LP
				
	//{	**************************************************			
	//{	TC358774/75XBG Timing and mode setting			
	//{	**************************************************			
	{	0x0450	,0x03F00100},//		VPCTRL	LP or HS
	{	0x0454	,0x0032001C},//		HTIM1	LP or HS
	{	0x0458	,0x00320690},//		HTIM2	LP or HS
	{	0x045C	,0x00040002},//		VTIM1	LP or HS
	{	0x0460	,0x000A041A},//		VTIM2	LP or HS
	{	0x0464	,0x00000001},//		VFUEN	LP or HS
	{	0x04A0	,0x0044802D},//		LVPHY0	LP or HS		
	{	0x04A0	,0x0004802D},//		LVPHY0	LP or HS
	{	0x0504	,0x00000004},//		SYSRST	LP or HS
	//{	**************************************************			
	//{	TC358774/75XBG LVDS Color mapping setting			
	//{	**************************************************			
	{	0x0480	,0x03020100},//		LVMX0003	LP or HS
	{	0x0484	,0x08050704},//		LVMX0407	LP or HS
	{	0x0488	,0x0F0E0A09},//		LVMX0811	LP or HS
	{	0x048C	,0x100D0C0B},//		LVMX1215	LP or HS
	{	0x0490	,0x12111716},//		LVMX1619	LP or HS
	{	0x0494	,0x1B151413},//		LVMX2023	LP or HS
	{	0x0498	,0x061A1918},//		LVMX2427	LP or HS
	//{	**************************************************			
	//{	TC358774/75XBG LVDS enable			
	//{	**************************************************			
	{	0x049C	,0x00000433},//		LVCFG	LP or HS
};

#define I2C_ADDRESS 0x0F

struct i2c_adapter *adap = NULL;

static u32 i2c_write(u16 reg, u32 data)
{
	struct i2c_msg msgs[2];
	int ret;
	u8 buf[6] = {reg >> 8, reg & 0xFF, (data >> 0) & 0xFF, (data >> 8) & 0xFF, (data >> 16) & 0xFF, (data >> 24) & 0xFF};

	msgs[0].flags = 0;
	msgs[0].addr = I2C_ADDRESS;
	msgs[0].len = 6;
	msgs[0].buf = buf;


        ret = i2c_transfer(adap, msgs, 1);
	if (ret != 1) {
		printk("################ error! ##################\n");
		return -1;
	}

	return 0;
}

static int i2c_read(u16 reg)
{
	struct i2c_msg msgs[2];
	u32 data = -1;
	int ret;
	u8 buf[2] = {reg >> 8, reg & 0xFF};
	
	msgs[0].flags = 0;
	msgs[0].addr = I2C_ADDRESS;
	msgs[0].len = 2;
	msgs[0].buf = buf;

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr = I2C_ADDRESS;
	msgs[1].len = 4;
	msgs[1].buf = &data;

	ret = i2c_transfer(adap, msgs, 2);
	if (ret != 2)
		return -1;
	
	return data;
}

static int init_lcd_by_i2c()
{
	int i;
	int chip_id;

	adap = i2c_get_adapter(15);
	chip_id = (i2c_read(0x0580) >> 8) & 0xFF;

	if (chip_id == 0x75) {
		for (i = 0; i < ARRAY_SIZE(initcode_75); i++) {
			i2c_write(initcode_75[i].reg, initcode_75[i].data);
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(initcode_65); i++) {
			i2c_write(initcode_65[i].reg, initcode_65[i].data);
		}	
	}

	mdelay(10); 


	printk("0x013C : %08x\n",i2c_read(0x013C));
	printk("0x0114 : %08x\n",i2c_read(0x0114));
	printk("0x0164 : %08x\n",i2c_read(0x0164));
	printk("0x0168 : %08x\n",i2c_read(0x0168));
	printk("0x016C : %08x\n",i2c_read(0x016C));
	printk("0x0134 : %08x\n",i2c_read(0x0134));
	printk("0x0210 : %08x\n",i2c_read(0x0210));
	printk("0x0104 : %08x\n",i2c_read(0x0104));
	printk("0x0204 : %08x\n",i2c_read(0x0204));

	printk("0x0450 : %08x\n",i2c_read(0x0450));
	printk("0x0454 : %08x\n",i2c_read(0x0454));
	printk("0x045C : %08x\n",i2c_read(0x045C));
	printk("0x0464 : %08x\n",i2c_read(0x0464));
	printk("0x04A0 : %08x\n",i2c_read(0x04A0));
	printk("0x0504 : %08x\n",i2c_read(0x0504));

	printk("0x0480 : %08x\n",i2c_read(0x0480));
	printk("0x0484 : %08x\n",i2c_read(0x0484));
	printk("0x0488 : %08x\n",i2c_read(0x0488));
	printk("0x048C : %08x\n",i2c_read(0x048C));
	printk("0x0490 : %08x\n",i2c_read(0x0490));
	printk("0x0494 : %08x\n",i2c_read(0x0494));
	printk("0x0498 : %08x\n",i2c_read(0x0498));
	printk("0x049C : %08x\n",i2c_read(0x049C));

	return 0;
}

static int init_lcd(struct mipi_dsim_device *dsim)
{
#if 1
    	unsigned char   cmd[6], i, chip_id;

	adap = i2c_get_adapter(15);
	chip_id = (i2c_read(0x0580) >> 8) & 0xFF;

	printk("MIPI DSI CHIP ID = %02x\n", chip_id);

	if (chip_id == 0x75) {
		for (i = 0; i < ARRAY_SIZE(initcode_75); i++) {
		    cmd[0] = initcode_75[i].reg;	       cmd[1] = initcode_75[i].reg  >> 8;
		    cmd[2] = initcode_75[i].data;         cmd[3] = initcode_75[i].data >> 8;
		    cmd[4] = initcode_75[i].data >> 16;   cmd[5] = initcode_75[i].data >> 24;

		    if(s5p_mipi_dsi_wr_data(dsim, MIPI_DSI_GENERIC_LONG_WRITE, (unsigned int)cmd, sizeof(cmd)) == -1)   
			return  0;       
		}
	} else {

		for (i = 0; i < ARRAY_SIZE(initcode_65); i++) {
		    cmd[0] = initcode_65[i].reg;	       cmd[1] = initcode_65[i].reg  >> 8;
		    cmd[2] = initcode_65[i].data;         cmd[3] = initcode_65[i].data >> 8;
		    cmd[4] = initcode_65[i].data >> 16;   cmd[5] = initcode_65[i].data >> 24;

		    if(s5p_mipi_dsi_wr_data(dsim, MIPI_DSI_GENERIC_LONG_WRITE, (unsigned int)cmd, sizeof(cmd)) == -1)   
			return  0;       
		}
	}

	mdelay(10); 

	

	printk("0x013C : %08x\n",i2c_read(0x013C));
	printk("0x0114 : %08x\n",i2c_read(0x0114));
	printk("0x0164 : %08x\n",i2c_read(0x0164));
	printk("0x0168 : %08x\n",i2c_read(0x0168));
	printk("0x016C : %08x\n",i2c_read(0x016C));
	printk("0x0134 : %08x\n",i2c_read(0x0134));
	printk("0x0210 : %08x\n",i2c_read(0x0210));
	printk("0x0104 : %08x\n",i2c_read(0x0104));
	printk("0x0204 : %08x\n",i2c_read(0x0204));

	printk("0x0450 : %08x\n",i2c_read(0x0450));
	printk("0x0454 : %08x\n",i2c_read(0x0454));
	printk("0x0458 : %08x\n",i2c_read(0x0458));
	printk("0x045C : %08x\n",i2c_read(0x045C));
	printk("0x0460 : %08x\n",i2c_read(0x0460));
	printk("0x0464 : %08x\n",i2c_read(0x0464));
	printk("0x04A0 : %08x\n",i2c_read(0x04A0));
	printk("0x0504 : %08x\n",i2c_read(0x0504));

	printk("0x0480 : %08x\n",i2c_read(0x0480));
	printk("0x0484 : %08x\n",i2c_read(0x0484));
	printk("0x0488 : %08x\n",i2c_read(0x0488));
	printk("0x048C : %08x\n",i2c_read(0x048C));
	printk("0x0490 : %08x\n",i2c_read(0x0490));
	printk("0x0494 : %08x\n",i2c_read(0x0494));
	printk("0x0498 : %08x\n",i2c_read(0x0498));
	printk("0x049C : %08x\n",i2c_read(0x049C));
#else
	init_lcd_by_i2c();
#endif
	return 1;
}

static int tc358764_mipi_lcd_probe(struct mipi_dsim_device *dsim)
{
	return 0;
}

static int tc358764_mipi_lcd_suspend(struct mipi_dsim_device *dsim)
{
	return 0;
}

static int tc358764_mipi_lcd_displayon(struct mipi_dsim_device *dsim)
{
	return init_lcd(dsim);
}

static int tc358764_mipi_lcd_resume(struct mipi_dsim_device *dsim)
{
	return init_lcd(dsim);
}

struct mipi_dsim_lcd_driver tc358764_mipi_lcd_driver = {
	.probe 		= tc358764_mipi_lcd_probe,
	.suspend	= tc358764_mipi_lcd_suspend,
	.displayon	= tc358764_mipi_lcd_displayon,
	.resume		= tc358764_mipi_lcd_resume,
};
