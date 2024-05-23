/* drivers/video/sunxi/disp2/disp/lcd/ili9806e.c
 *
 * Copyright (c) 2022 simon Co., Ltd.
 * Author: xianbin.yin <xianbin.yin@simon.com.cn>
 *
 * ili9806e panel driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include "tft08006.h"

static void lcd_power_on(u32 sel);
static void lcd_power_off(u32 sel);
static void lcd_bl_open(u32 sel);
static void lcd_bl_close(u32 sel);

static void lcd_panel_init(u32 sel);
static void lcd_panel_exit(u32 sel);

#define panel_reset(sel, val) sunxi_lcd_gpio_set_value(sel, 0, val)

static void lcd_cfg_panel_info(panel_extend_para *info)
{
	u32 i = 0, j = 0;
	u32 items;
	pr_err("%s \n",__FUNCTION__);
	u8 lcd_gamma_tbl[][2] = {
		{0, 0},
		{15, 15},
		{30, 30},
		{45, 45},
		{60, 60},
		{75, 75},
		{90, 90},
		{105, 105},
		{120, 120},
		{135, 135},
		{150, 150},
		{165, 165},
		{180, 180},
		{195, 195},
		{210, 210},
		{225, 225},
		{240, 240},
		{255, 255},
	};

	u32 lcd_cmap_tbl[2][3][4] = {
	{
		{LCD_CMAP_G0, LCD_CMAP_B1, LCD_CMAP_G2, LCD_CMAP_B3},
		{LCD_CMAP_B0, LCD_CMAP_R1, LCD_CMAP_B2, LCD_CMAP_R3},
		{LCD_CMAP_R0, LCD_CMAP_G1, LCD_CMAP_R2, LCD_CMAP_G3},
		},
		{
		{LCD_CMAP_B3, LCD_CMAP_G2, LCD_CMAP_B1, LCD_CMAP_G0},
		{LCD_CMAP_R3, LCD_CMAP_B2, LCD_CMAP_R1, LCD_CMAP_B0},
		{LCD_CMAP_G3, LCD_CMAP_R2, LCD_CMAP_G1, LCD_CMAP_R0},
		},
	};

	items = sizeof(lcd_gamma_tbl) / 2;
	for (i = 0; i < items - 1; i++) {
		u32 num = lcd_gamma_tbl[i+1][0] - lcd_gamma_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value = lcd_gamma_tbl[i][1] +
				((lcd_gamma_tbl[i+1][1] - lcd_gamma_tbl[i][1])
				* j) / num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] =
							(value<<16)
							+ (value<<8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items-1][1]<<16) +
					(lcd_gamma_tbl[items-1][1]<<8)
					+ lcd_gamma_tbl[items-1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

}

static s32 lcd_open_flow(u32 sel)
{
	pr_err("%s \n",__FUNCTION__);
	LCD_OPEN_FUNC(sel, lcd_power_on, 120);
	LCD_OPEN_FUNC(sel, lcd_panel_init, 10);
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 50);
	LCD_OPEN_FUNC(sel, lcd_bl_open, 0);

	return 0;
}

static s32 lcd_close_flow(u32 sel)
{
	pr_err("%s \n",__FUNCTION__);
	LCD_CLOSE_FUNC(sel, lcd_bl_close, 0);
	LCD_CLOSE_FUNC(sel, lcd_panel_exit, 1);
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 10);
	LCD_CLOSE_FUNC(sel, lcd_power_off, 0);

	return 0;
}

static void lcd_power_on(u32 sel)
{
	pr_err("%s \n",__FUNCTION__);
/*	sunxi_lcd_pin_cfg(sel, 1);
	panel_reset(sel, 0);
	sunxi_lcd_power_enable(sel, 0); //1.8 IOVCC
	sunxi_lcd_delay_ms(10);  
	sunxi_lcd_power_enable(sel, 1); //3.3 VCI
	sunxi_lcd_delay_ms(10);
	panel_reset(sel, 1);
//	sunxi_lcd_delay_ms(1);
//	panel_reset(sel, 0);
//	sunxi_lcd_delay_ms(1);
//	panel_reset(sel, 1);*/
	sunxi_lcd_pin_cfg(sel, 1);
	panel_reset(sel, 1);
	sunxi_lcd_delay_ms(10);
	sunxi_lcd_power_enable(sel, 0); //1.8 IOVCC
	sunxi_lcd_delay_ms(10);  
	sunxi_lcd_power_enable(sel, 1); //3.3 VCI
	sunxi_lcd_delay_ms(1);
	panel_reset(sel, 0);
	sunxi_lcd_delay_ms(1);
	panel_reset(sel, 1);

}

static void lcd_power_off(u32 sel)
{
	pr_err("%s \n",__FUNCTION__);
	panel_reset(sel, 0);
	sunxi_lcd_delay_ms(130);
	sunxi_lcd_power_disable(sel, 1);  //3.3 VCI
	sunxi_lcd_delay_ms(1);
	sunxi_lcd_power_disable(sel, 0);   //1.8 IOVCC
	sunxi_lcd_pin_cfg(sel, 0);
}

static void lcd_bl_open(u32 sel)
{
	pr_err("%s \n",__FUNCTION__);
	sunxi_lcd_pwm_enable(sel);
	sunxi_lcd_backlight_enable(sel);
}

static void lcd_bl_close(u32 sel)
{
	pr_err("%s \n",__FUNCTION__);
	sunxi_lcd_backlight_disable(sel);
	sunxi_lcd_pwm_disable(sel);
}

static void lcd_panel_init(u32 sel)
{
	printk(" tft08006 %s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	pr_err("%s \n",__FUNCTION__);
	sunxi_lcd_dsi_clk_enable(sel);
	sunxi_lcd_delay_ms(150);	sunxi_lcd_dsi_dcs_write_5para(sel,0xFF,0xff,0x98,0x06,0x04,0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x08,0x10);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x20,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x21,0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x30,0x02);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x31,0x02);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x60,0x07);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x61,0x06);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x62,0x06);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x63,0x04);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x40,0x18);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x41,0x33);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x42,0x11);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x43,0x09);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x44,0x0c);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x46,0x55);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x47,0x55);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x45,0x14);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x50,0x50);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x51,0x50);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x52,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x53,0x38);

	sunxi_lcd_dsi_dcs_write_1para(sel,0xA0,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xa1,0x09);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xa2,0x0c);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xa3,0x0f);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xa4,0x06);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xa5,0x09);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xa6,0x07);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xa7,0x16);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xa8,0x06);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xa9,0x09);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xaa,0x11);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xab,0x06);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xac,0x0e);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xad,0x19);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xae,0x0e);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xaf,0x00);


	sunxi_lcd_dsi_dcs_write_1para(sel,0xc0,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xc1,0x09);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xc2,0x0c);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xc3,0x0f);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xc4,0x06);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xc5,0x09);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xc6,0x07);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xc7,0x16);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xc8,0x06);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xc9,0x09);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xca,0x11);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xcb,0x06);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xcc,0x0e);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xcd,0x19);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xce,0x0e);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xcf,0x00);


	sunxi_lcd_dsi_dcs_write_5para(sel,0xff,0xff,0x98,0x06,0x04,0x06);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x00,0xa0);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x01,0x05);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x02,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x03,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x04,0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x05,0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x06,0x88);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x07,0x04);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x08,0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x09,0x90);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x0a,0x04);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x0b,0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x0c,0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x0d,0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x0e,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x0f,0x00);


	sunxi_lcd_dsi_dcs_write_1para(sel,0x10,0x55);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x11,0x50);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x12,0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x13,0x85);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x14,0x85);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x15,0xc0);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x16,0x0b);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x17,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x18,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x19,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x1a,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x1b,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x1c,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x1d,0x00);



	sunxi_lcd_dsi_dcs_write_1para(sel,0x20,0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x21,0x23);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x22,0x45);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x23,0x67);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x24,0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x25,0x23);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x26,0x45);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x27,0x67);


	sunxi_lcd_dsi_dcs_write_1para(sel,0x30,0x02);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x31,0x22);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x32,0x11);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x33,0xaa);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x34,0xbb);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x35,0x66);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x36,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x37,0x22);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x38,0x22);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x39,0x22);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x3a,0x22);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x3b,0x22);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x3c,0x22);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x3d,0x20);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x3e,0x22);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x3f,0x22);


	sunxi_lcd_dsi_dcs_write_1para(sel,0x40,0x22);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x52,0x12);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x53,0x12);


	sunxi_lcd_dsi_dcs_write_5para(sel,0xff,0xff,0x98,0x06,0x04,0x07);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x17,0x32);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x02,0x17);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x18,0x1d);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xe1,0x79);

	
	sunxi_lcd_dsi_dcs_write_5para(sel,0xff,0xff,0x98,0x06,0x04,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x3a,0x70);
	sunxi_lcd_dsi_dcs_write_0para(sel,0x11);
	sunxi_lcd_delay_ms(150);
	sunxi_lcd_dsi_dcs_write_0para(sel,0x29);
	sunxi_lcd_delay_ms(150); 
/********ILI9881CBOE initial code **********
sunxi_lcd_dsi_dcs_write_3para(sel,0xFF,0x98,0x81,0x03);
//GIP_1
sunxi_lcd_dsi_dcs_write_1para(sel,0x01,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x02,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x03,0x73);
sunxi_lcd_dsi_dcs_write_1para(sel,0x04,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x05,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x06,0x08);
sunxi_lcd_dsi_dcs_write_1para(sel,0x07,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x08,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x09,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x0A,0x01);
sunxi_lcd_dsi_dcs_write_1para(sel,0x0B,0x01);
sunxi_lcd_dsi_dcs_write_1para(sel,0x0C,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x0D,0x01);
sunxi_lcd_dsi_dcs_write_1para(sel,0x0E,0x01);
sunxi_lcd_dsi_dcs_write_1para(sel,0x0F,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x10,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x11,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x12,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x13,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x14,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x15,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x16,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x17,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x18,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x19,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x1A,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x1B,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x1C,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x1D,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x1E,0x40);
sunxi_lcd_dsi_dcs_write_1para(sel,0x1F,0xC0);
sunxi_lcd_dsi_dcs_write_1para(sel,0x20,0x06);
sunxi_lcd_dsi_dcs_write_1para(sel,0x21,0x01);
sunxi_lcd_dsi_dcs_write_1para(sel,0x22,0x06);
sunxi_lcd_dsi_dcs_write_1para(sel,0x23,0x01);
sunxi_lcd_dsi_dcs_write_1para(sel,0x24,0x88);
sunxi_lcd_dsi_dcs_write_1para(sel,0x25,0x88);
sunxi_lcd_dsi_dcs_write_1para(sel,0x26,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x27,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x28,0x3B);
sunxi_lcd_dsi_dcs_write_1para(sel,0x29,0x03);
sunxi_lcd_dsi_dcs_write_1para(sel,0x2A,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x2B,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x2C,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x2D,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x2E,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x2F,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x30,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x31,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x32,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x33,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x34,0x00);// GPWR1/2 non overlap time 2.62us
sunxi_lcd_dsi_dcs_write_1para(sel,0x35,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x36,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x37,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x38,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x39,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x3A,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x3B,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x3C,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x3D,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x3E,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x3F,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x40,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x41,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x42,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x43,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x44,0x00);

//GIP_2
sunxi_lcd_dsi_dcs_write_1para(sel,0x50,0x01);
sunxi_lcd_dsi_dcs_write_1para(sel,0x51,0x23);
sunxi_lcd_dsi_dcs_write_1para(sel,0x52,0x45);
sunxi_lcd_dsi_dcs_write_1para(sel,0x53,0x67);
sunxi_lcd_dsi_dcs_write_1para(sel,0x54,0x89);
sunxi_lcd_dsi_dcs_write_1para(sel,0x55,0xAB);
sunxi_lcd_dsi_dcs_write_1para(sel,0x56,0x01);
sunxi_lcd_dsi_dcs_write_1para(sel,0x57,0x23);
sunxi_lcd_dsi_dcs_write_1para(sel,0x58,0x45);
sunxi_lcd_dsi_dcs_write_1para(sel,0x59,0x67);
sunxi_lcd_dsi_dcs_write_1para(sel,0x5A,0x89);
sunxi_lcd_dsi_dcs_write_1para(sel,0x5B,0xAB);
sunxi_lcd_dsi_dcs_write_1para(sel,0x5C,0xCD);
sunxi_lcd_dsi_dcs_write_1para(sel,0x5D,0xEF);

//GIP_3
sunxi_lcd_dsi_dcs_write_1para(sel,0x5E,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x5F,0x01);
sunxi_lcd_dsi_dcs_write_1para(sel,0x60,0x01);
sunxi_lcd_dsi_dcs_write_1para(sel,0x61,0x06);
sunxi_lcd_dsi_dcs_write_1para(sel,0x62,0x06);
sunxi_lcd_dsi_dcs_write_1para(sel,0x63,0x07);
sunxi_lcd_dsi_dcs_write_1para(sel,0x64,0x07);
sunxi_lcd_dsi_dcs_write_1para(sel,0x65,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x66,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x67,0x02);
sunxi_lcd_dsi_dcs_write_1para(sel,0x68,0x02);
sunxi_lcd_dsi_dcs_write_1para(sel,0x69,0x05);
sunxi_lcd_dsi_dcs_write_1para(sel,0x6A,0x05);
sunxi_lcd_dsi_dcs_write_1para(sel,0x6B,0x02);
sunxi_lcd_dsi_dcs_write_1para(sel,0x6C,0x0D);
sunxi_lcd_dsi_dcs_write_1para(sel,0x6D,0x0D);
sunxi_lcd_dsi_dcs_write_1para(sel,0x6E,0x0C);
sunxi_lcd_dsi_dcs_write_1para(sel,0x6F,0x0C);
sunxi_lcd_dsi_dcs_write_1para(sel,0x70,0x0F);
sunxi_lcd_dsi_dcs_write_1para(sel,0x71,0x0F);
sunxi_lcd_dsi_dcs_write_1para(sel,0x72,0x0E);
sunxi_lcd_dsi_dcs_write_1para(sel,0x73,0x0E);
sunxi_lcd_dsi_dcs_write_1para(sel,0x74,0x02);
sunxi_lcd_dsi_dcs_write_1para(sel,0x75,0x01);
sunxi_lcd_dsi_dcs_write_1para(sel,0x76,0x01);
sunxi_lcd_dsi_dcs_write_1para(sel,0x77,0x06);
sunxi_lcd_dsi_dcs_write_1para(sel,0x78,0x06);
sunxi_lcd_dsi_dcs_write_1para(sel,0x79,0x07);
sunxi_lcd_dsi_dcs_write_1para(sel,0x7A,0x07);
sunxi_lcd_dsi_dcs_write_1para(sel,0x7B,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x7C,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0x7D,0x02);
sunxi_lcd_dsi_dcs_write_1para(sel,0x7E,0x02);
sunxi_lcd_dsi_dcs_write_1para(sel,0x7F,0x05);
sunxi_lcd_dsi_dcs_write_1para(sel,0x80,0x05);
sunxi_lcd_dsi_dcs_write_1para(sel,0x81,0x02);
sunxi_lcd_dsi_dcs_write_1para(sel,0x82,0x0D);
sunxi_lcd_dsi_dcs_write_1para(sel,0x83,0x0D);
sunxi_lcd_dsi_dcs_write_1para(sel,0x84,0x0C);
sunxi_lcd_dsi_dcs_write_1para(sel,0x85,0x0C);
sunxi_lcd_dsi_dcs_write_1para(sel,0x86,0x0F);
sunxi_lcd_dsi_dcs_write_1para(sel,0x87,0x0F);
sunxi_lcd_dsi_dcs_write_1para(sel,0x88,0x0E);
sunxi_lcd_dsi_dcs_write_1para(sel,0x89,0x0E);
sunxi_lcd_dsi_dcs_write_1para(sel,0x8A,0x02);

//Page 4 command
sunxi_lcd_dsi_dcs_write_3para(sel,0xFF,0x98,0x81,0x04);

sunxi_lcd_dsi_dcs_write_1para(sel,0x3B,0xC0);// ILI4003D sel
sunxi_lcd_dsi_dcs_write_1para(sel,0x6C,0x15);//Set VCORE voltage =1.5V
sunxi_lcd_dsi_dcs_write_1para(sel,0x6E,0x2A);//di_pwr_reg=0 for power mode 2A //VGH clamp 18V
sunxi_lcd_dsi_dcs_write_1para(sel,0x6F,0x33);//45 //pumping ratio VGH=5x VGL=-3x
sunxi_lcd_dsi_dcs_write_1para(sel,0x8D,0x1B);//VGL clamp -10V
sunxi_lcd_dsi_dcs_write_1para(sel,0x87,0xBA);//ESD
sunxi_lcd_dsi_dcs_write_1para(sel,0x3A,0x24);//POWER SAVING
sunxi_lcd_dsi_dcs_write_1para(sel,0x26,0x76);
sunxi_lcd_dsi_dcs_write_1para(sel,0xB2,0xD1);

 // Page 1 command
sunxi_lcd_dsi_dcs_write_3para(sel,0xFF,0x98,0x81,0x01);
sunxi_lcd_dsi_dcs_write_1para(sel,0x22,0x0A);//BGR, SS
sunxi_lcd_dsi_dcs_write_1para(sel,0x31,0x00);//Zigzag type3 inversion
sunxi_lcd_dsi_dcs_write_1para(sel,0x40,0x53);// ILI4003D sel
sunxi_lcd_dsi_dcs_write_1para(sel,0x43,0x66);
sunxi_lcd_dsi_dcs_write_1para(sel,0x53,0x4C);
sunxi_lcd_dsi_dcs_write_1para(sel,0x50,0x87);
sunxi_lcd_dsi_dcs_write_1para(sel,0x51,0x82);
sunxi_lcd_dsi_dcs_write_1para(sel,0x60,0x15);
sunxi_lcd_dsi_dcs_write_1para(sel,0x61,0x01);
sunxi_lcd_dsi_dcs_write_1para(sel,0x62,0x0C);
sunxi_lcd_dsi_dcs_write_1para(sel,0x63,0x00);

// Gamma P
sunxi_lcd_dsi_dcs_write_1para(sel,0xA0,0x00);
sunxi_lcd_dsi_dcs_write_1para(sel,0xA1,0x13);//VP251
sunxi_lcd_dsi_dcs_write_1para(sel,0xA2,0x23);//VP247
sunxi_lcd_dsi_dcs_write_1para(sel,0xA3,0x14);//VP243
sunxi_lcd_dsi_dcs_write_1para(sel,0xA4,0x16);//VP239
sunxi_lcd_dsi_dcs_write_1para(sel,0xA5,0x29);//VP231
sunxi_lcd_dsi_dcs_write_1para(sel,0xA6,0x1E);//VP219
sunxi_lcd_dsi_dcs_write_1para(sel,0xA7,0x1D);//VP203
sunxi_lcd_dsi_dcs_write_1para(sel,0xA8,0x86);//VP175
sunxi_lcd_dsi_dcs_write_1para(sel,0xA9,0x1E);//VP144
sunxi_lcd_dsi_dcs_write_1para(sel,0xAA,0x29);//VP111
sunxi_lcd_dsi_dcs_write_1para(sel,0xAB,0x74);//VP80
sunxi_lcd_dsi_dcs_write_1para(sel,0xAC,0x19);//VP52
sunxi_lcd_dsi_dcs_write_1para(sel,0xAD,0x17);//VP36
sunxi_lcd_dsi_dcs_write_1para(sel,0xAE,0x4B);//VP24
sunxi_lcd_dsi_dcs_write_1para(sel,0xAF,0x20);//VP16
sunxi_lcd_dsi_dcs_write_1para(sel,0xB0,0x26);//VP12
sunxi_lcd_dsi_dcs_write_1para(sel,0xB1,0x4C);//VP8
sunxi_lcd_dsi_dcs_write_1para(sel,0xB2,0x5D);//VP4
sunxi_lcd_dsi_dcs_write_1para(sel,0xB3,0x3F);//VP0

// Gamma N
sunxi_lcd_dsi_dcs_write_1para(sel,0xC0,0x00);//VN255 GAMMA N
sunxi_lcd_dsi_dcs_write_1para(sel,0xC1,0x13);//VN251
sunxi_lcd_dsi_dcs_write_1para(sel,0xC2,0x23);//VN247
sunxi_lcd_dsi_dcs_write_1para(sel,0xC3,0x14);//VN243
sunxi_lcd_dsi_dcs_write_1para(sel,0xC4,0x16);//VN239
sunxi_lcd_dsi_dcs_write_1para(sel,0xC5,0x29);//VN231
sunxi_lcd_dsi_dcs_write_1para(sel,0xC6,0x1E);//VN219
sunxi_lcd_dsi_dcs_write_1para(sel,0xC7,0x1D);//VN203
sunxi_lcd_dsi_dcs_write_1para(sel,0xC8,0x86);//VN175
sunxi_lcd_dsi_dcs_write_1para(sel,0xC9,0x1E);//VN144
sunxi_lcd_dsi_dcs_write_1para(sel,0xCA,0x29);//VN111
sunxi_lcd_dsi_dcs_write_1para(sel,0xCB,0x74);//VN80
sunxi_lcd_dsi_dcs_write_1para(sel,0xCC,0x19);//VN52
sunxi_lcd_dsi_dcs_write_1para(sel,0xCD,0x17);//VN36
sunxi_lcd_dsi_dcs_write_1para(sel,0xCE,0x4B);//VN24
sunxi_lcd_dsi_dcs_write_1para(sel,0xCF,0x20);//VN16
sunxi_lcd_dsi_dcs_write_1para(sel,0xD0,0x26);//VN12
sunxi_lcd_dsi_dcs_write_1para(sel,0xD1,0x4C);//VN8
sunxi_lcd_dsi_dcs_write_1para(sel,0xD2,0x5D);//VN4
sunxi_lcd_dsi_dcs_write_1para(sel,0xD3,0x3F);//VN0

// Page 0 command
sunxi_lcd_dsi_dcs_write_3para(sel,0xFF,0x98,0x81,0x00);

sunxi_lcd_dsi_dcs_write_0para(sel,0x11);
sunxi_lcd_delay_ms(120);
sunxi_lcd_dsi_dcs_write_0para(sel,0x29);
sunxi_lcd_delay_ms(5);
//--- TE----//
sunxi_lcd_dsi_dcs_write_0para(sel,0x35);
//end -----------*/
/*	sunxi_lcd_dsi_dcs_write_3para(sel,0xFF,0xFF,0x98,0x06);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xBA,0x60);

	tmp[0]= 0x01;
	tmp[1]= 0x12;
	tmp[2]= 0x61;
	tmp[3]= 0xFF;
	tmp[4]= 0x10;
	tmp[5]= 0x10;
	tmp[6]= 0x0B;
	tmp[7]= 0x13;
	tmp[8]= 0x32;
	tmp[9]= 0x73;
	tmp[10]= 0xFF;
	tmp[11]= 0xFF;
	tmp[12]= 0x0E;
	tmp[13]= 0x0E;
	tmp[14]= 0x00;
	tmp[15]= 0x03;
	tmp[16]= 0x66;
	tmp[17]= 0x63;
	tmp[18]= 0x01;
	tmp[19]= 0x00;
	tmp[20]= 0x00;
	sunxi_lcd_dsi_dcs_write_npara(sel,0xBC,tmp,21);
	tmp[0]= 0x01;
	tmp[1]= 0x23;
	tmp[2]= 0x45;
	tmp[3]= 0x67;
	tmp[4]= 0x01;
	tmp[5]= 0x23;
	tmp[6]= 0x45;
	tmp[7]= 0x67;
	sunxi_lcd_dsi_dcs_write_npara(sel,0xBD,tmp,8);
	tmp[0]= 0x00;
	tmp[1]= 0x21;
	tmp[2]= 0xAB;
	tmp[3]= 0x60;
	tmp[4]= 0x22;
	tmp[5]= 0x22;
	tmp[6]= 0x22;
	tmp[7]= 0x22;
	tmp[8]= 0x22;
	sunxi_lcd_dsi_dcs_write_npara(sel,0xBE,tmp,9);
	sunxi_lcd_dsi_dcs_write_3para(sel,0xC7,0x5E,0x80,0x6F);
	sunxi_lcd_dsi_dcs_write_3para(sel,0xED,0x7F,0x0F,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xB6,0x02);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x3A,0x55);
	sunxi_lcd_dsi_dcs_write_2para(sel,0xB5,0x3E,0x18);
	sunxi_lcd_dsi_dcs_write_3para(sel,0xC0,0xAB,0x0B,0x0A);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xFC,0x09);
	tmp[0]= 0x00;
	tmp[1]= 0x00;
	tmp[2]= 0x00;
	tmp[3]= 0x00;
	tmp[4]= 0x00;
	tmp[5]= 0x20;
	sunxi_lcd_dsi_dcs_write_npara(sel,0xDF,tmp,6);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xF3,0x74);
	sunxi_lcd_dsi_dcs_write_3para(sel,0xB4,0x0,0x0,0x0);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xF7,0x82);
	sunxi_lcd_dsi_dcs_write_3para(sel,0xB1,0x0,0x12,0x13);
	sunxi_lcd_dsi_dcs_write_4para(sel,0xF2,0x00,0x59,0x40,0x28);
	sunxi_lcd_dsi_dcs_write_4para(sel,0xC1,0x07,0x80,0x80,0x20);
	tmp[0]= 0x00;
	tmp[1]= 0x17;
	tmp[2]= 0x1A;
	tmp[3]= 0x0D;
	tmp[4]= 0x0E;
	tmp[5]= 0x0B;
	tmp[6]= 0x07;
	tmp[7]= 0x05;
	tmp[8]= 0x05;
	tmp[9]= 0x09;
	tmp[10]= 0x0E;
	tmp[11]= 0x0F;
	tmp[12]= 0x0D;
	tmp[13]= 0x1D;
	tmp[15]= 0x1A;
	tmp[15]= 0x00;
	sunxi_lcd_dsi_dcs_write_npara(sel,0xE0,tmp,16);
	tmp[0]= 0x00;
	tmp[1]= 0x06;
	tmp[2]= 0x0E;
	tmp[3]= 0x0D;
	tmp[4]= 0x0E;
	tmp[5]= 0x0D;
	tmp[6]= 0x06;
	tmp[7]= 0x06;
	tmp[8]= 0x05;
	tmp[9]= 0x09;
	tmp[10]= 0x0D;
	tmp[11]= 0x0E;
	tmp[12]= 0x0D;
	tmp[13]= 0x1F;
	tmp[14]= 0x1D;
	tmp[15]= 0x00;
	sunxi_lcd_dsi_dcs_write_npara(sel,0xE1,tmp,16);

	sunxi_lcd_dsi_dcs_write_1para(sel,0x36,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x35,0x00);
	sunxi_lcd_dsi_dcs_write_0para(sel,0x11);
	sunxi_lcd_delay_ms(120);
	sunxi_lcd_dsi_dcs_write_0para(sel,0x29);
	sunxi_lcd_delay_ms(20);  */





 /*	sunxi_lcd_dsi_dcs_write_5para(sel,0xFF,0xff,0x98,0x06,0x04,0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x08,0x10);
//	sunxi_lcd_dsi_dcs_write_1para(sel,0x20,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x21,0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x30,0x02);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x31,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x60,0x07);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x61,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x62,0x08);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x63,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x40,0x10);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x41,0x55);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x42,0x02);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x43,0x09);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x44,0x07);
//	sunxi_lcd_dsi_dcs_write_1para(sel,0x46,0x55);
//	sunxi_lcd_dsi_dcs_write_1para(sel,0x47,0x55);
//	sunxi_lcd_dsi_dcs_write_1para(sel,0x45,0x14);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x50,0x78);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x51,0x78);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x52,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x53,0x77);

	sunxi_lcd_dsi_dcs_write_1para(sel,0xA0,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xa1,0x03);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xa2,0x08);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xa3,0x0f);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xa4,0x07);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xa5,0x0b);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xa6,0x07);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xa7,0x06);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xa8,0x07);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xa9,0x0c);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xaa,0x0e);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xab,0x05);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xac,0x0c);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xad,0x21);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xae,0x1c);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xaf,0x00);


	sunxi_lcd_dsi_dcs_write_1para(sel,0xc0,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xc1,0x04);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xc2,0x09);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xc3,0x0e);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xc4,0x06);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xc5,0x0b);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xc6,0x07);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xc7,0x05);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xc8,0x08);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xc9,0x0c);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xca,0x0f);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xcb,0x06);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xcc,0x0c);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xcd,0x20);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xce,0x1c);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xcf,0x00);


	sunxi_lcd_dsi_dcs_write_5para(sel,0xff,0xff,0x98,0x06,0x04,0x06);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x00,0x20);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x01,0x0a);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x02,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x03,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x04,0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x05,0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x06,0x98);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x07,0x06);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x08,0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x09,0x80);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x0a,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x0b,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x0c,0x0a);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x0d,0x0a);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x0e,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x0f,0x00);


	sunxi_lcd_dsi_dcs_write_1para(sel,0x10,0xf0);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x11,0xf4);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x12,0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x13,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x14,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x15,0xc0);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x16,0x08);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x17,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x18,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x19,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x1a,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x1b,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x1c,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x1d,0x00);



	sunxi_lcd_dsi_dcs_write_1para(sel,0x20,0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x21,0x23);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x22,0x45);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x23,0x67);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x24,0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x25,0x23);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x26,0x45);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x27,0x67);


	sunxi_lcd_dsi_dcs_write_1para(sel,0x30,0x11);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x31,0x11);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x32,0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x33,0xee);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x34,0xff);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x35,0xbb);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x36,0xaa);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x37,0xdd);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x38,0xcc);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x39,0x66);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x3a,0x77);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x3b,0x22);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x3c,0x22);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x3d,0x20);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x3e,0x22);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x3f,0x22);


	sunxi_lcd_dsi_dcs_write_1para(sel,0x40,0x22);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x52,0x10);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x53,0x10);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x54,0x13);


	sunxi_lcd_dsi_dcs_write_5para(sel,0xff,0xff,0x98,0x06,0x04,0x07);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xb3,0x10);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x17,0x22);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x02,0x77);
//	sunxi_lcd_dsi_dcs_write_1para(sel,0x18,0x1d);
	sunxi_lcd_dsi_dcs_write_1para(sel,0xe1,0x79);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x26,0xb2);

	
	sunxi_lcd_dsi_dcs_write_5para(sel,0xff,0xff,0x98,0x06,0x04,0x00);
	sunxi_lcd_delay_ms(10);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x35,0x00);
	sunxi_lcd_delay_ms(10);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x36,0x08);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x3a,0x77);
	sunxi_lcd_delay_ms(10);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x11,0x00);
	sunxi_lcd_delay_ms(120);
	sunxi_lcd_dsi_dcs_write_1para(sel,0x29,0);
	sunxi_lcd_delay_ms(10); */
}

static void lcd_panel_exit(u32 sel)
{
	pr_err("%s \n",__FUNCTION__);
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x10);
	sunxi_lcd_delay_ms(1);
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x28);
	sunxi_lcd_delay_ms(1);
}

/*sel: 0:lcd0; 1:lcd1*/
static s32 lcd_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

__lcd_panel_t tft08006_panel = {
	/* panel driver name, must mach the name of
	 * lcd_drv_name in sys_config.fex
	 */
	.name = "ili9806e",
	.func = {
		.cfg_panel_info = lcd_cfg_panel_info,
			.cfg_open_flow = lcd_open_flow,
			.cfg_close_flow = lcd_close_flow,
			.lcd_user_defined_func = lcd_user_defined_func,
	},
};

