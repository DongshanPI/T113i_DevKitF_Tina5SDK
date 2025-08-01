#include "lt8912b.h"

static void lcd_panel_init(u32 sel);
static void lcd_panel_exit(u32 sel);

#define panel_reset(val) sunxi_lcd_gpio_set_value(sel, 0, val)

static void lcd_cfg_panel_info(struct panel_extend_para * info)
{
	u32 i = 0, j=0;
	u32 items;
	u8 lcd_gamma_tbl[][2] =
	{
		/* {input value, corrected value} */
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
		{LCD_CMAP_G0,LCD_CMAP_B1,LCD_CMAP_G2,LCD_CMAP_B3},
		{LCD_CMAP_B0,LCD_CMAP_R1,LCD_CMAP_B2,LCD_CMAP_R3},
		{LCD_CMAP_R0,LCD_CMAP_G1,LCD_CMAP_R2,LCD_CMAP_G3},
		},
		{
		{LCD_CMAP_B3,LCD_CMAP_G2,LCD_CMAP_B1,LCD_CMAP_G0},
		{LCD_CMAP_R3,LCD_CMAP_B2,LCD_CMAP_R1,LCD_CMAP_B0},
		{LCD_CMAP_G3,LCD_CMAP_R2,LCD_CMAP_G1,LCD_CMAP_R0},
		},
	};

	items = sizeof(lcd_gamma_tbl)/2;
	for (i=0; i<items-1; i++) {
		u32 num = lcd_gamma_tbl[i+1][0] - lcd_gamma_tbl[i][0];

		for (j=0; j<num; j++) {
			u32 value = 0;

			value = lcd_gamma_tbl[i][1] + ((lcd_gamma_tbl[i+1][1] - lcd_gamma_tbl[i][1]) * j)/num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] = (value<<16) + (value<<8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items-1][1]<<16) + (lcd_gamma_tbl[items-1][1]<<8) + lcd_gamma_tbl[items-1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

}

static s32 lcd_open_flow(u32 sel)
{
	/* open lcd power, than delay 50ms */
	LCD_OPEN_FUNC(sel, lcd_panel_init, 50);
	/* open lcd controller, and delay 100ms */
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 100);

	return 0;
}

static s32 lcd_close_flow(u32 sel)
{
	/* close lcd controller, and delay 0ms */
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 0);
	/* open lcd power, than delay 200ms */
	LCD_CLOSE_FUNC(sel, lcd_panel_exit, 200);

	return 0;
}

static void lcd_panel_init(u32 sel)
{
	sunxi_lcd_dsi_clk_enable(sel);
	sunxi_lcd_delay_ms(50);

	/* reset lcd panel */
	sunxi_lcd_dsi_dcs_write_0para(sel, DSI_DCS_SOFT_RESET);
	sunxi_lcd_delay_ms(50);

	/* exit sleep mode and set display on */
	sunxi_lcd_dsi_dcs_write_0para(sel, DSI_DCS_EXIT_SLEEP_MODE);
	sunxi_lcd_delay_ms(120);
	sunxi_lcd_dsi_dcs_write_0para(sel, DSI_DCS_SET_DISPLAY_ON);
	sunxi_lcd_delay_ms(10);
}

static void lcd_panel_exit(u32 sel)
{
	sunxi_lcd_dsi_dcs_write_0para(sel, DSI_DCS_SET_DISPLAY_OFF);
	sunxi_lcd_delay_ms(80);
	sunxi_lcd_dsi_dcs_write_0para(sel, DSI_DCS_ENTER_SLEEP_MODE);
	sunxi_lcd_delay_ms(50);
}

/* sel: 0:lcd0; 1:lcd1 */
static s32 lcd_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

static s32 lcd_set_bright(u32 sel, u32 bright)
{
	return 0;
}

struct __lcd_panel lt8912b_panel = {
	/* panel driver name, must mach the name of lcd_drv_name in sys_config.fex */
	.name = "panel-lt8912b-hdmi",
	.func = {
		.cfg_panel_info = lcd_cfg_panel_info,
		.cfg_open_flow = lcd_open_flow,
		.cfg_close_flow = lcd_close_flow,
		.lcd_user_defined_func = lcd_user_defined_func,
		.set_bright = lcd_set_bright,
	},
};
