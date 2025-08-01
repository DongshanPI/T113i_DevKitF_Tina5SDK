#include <common.h>
#include <i2c.h>
#include <sunxi_i2c.h>
#include <sys_config.h>


#include "lt8912b.h"

static void lcd_power_on(u32 sel);
static void lcd_power_off(u32 sel);

static void lcd_panel_init(u32 sel);
static void lcd_panel_exit(u32 sel);

#define panel_reset(val) sunxi_lcd_gpio_set_value(sel, 0, val)

#define LT8912B_I2C_ID		2

#define I2C_BASS_ADDRESS1	0x48
#define I2C_BASS_ADDRESS2	0x49
#define I2C_BASS_ADDRESS3	0x4a

#define _1080P_60Hz

/* output mode */
#define _hdmi_output

/* lanes */
/* 0: 4lane; 1: 1lane; 2: 2lane; 3: 3lane */
#define lane_cnt   0

struct video_timing{
	unsigned short hfp;
	unsigned short hs;
	unsigned short hbp;
	unsigned short hact;
	unsigned short htotal;
	unsigned short vfp;
	unsigned short vs;
	unsigned short vbp;
	unsigned short vact;
	unsigned short vtotal;
	unsigned int pclk_khz;
};


typedef enum
{
	I2S_2CH,
	I2S_8CH,
	SPDIF
}
_Audio_Input_Mode;

#define     Audio_Input_Mode    I2S_2CH

struct panel_parameter
{
	unsigned short hfp;
	unsigned short hs;
	unsigned short hbp;
	unsigned short hact;
	unsigned short htotal;
	unsigned short vfp;
	unsigned short vs;
	unsigned short vbp;
	unsigned short vact;
	unsigned short vtotal;
	unsigned int pclk_khz;
};


unsigned char I2CADR = I2C_BASS_ADDRESS1;

unsigned char Hsync_H_last = 0x00;
unsigned char Hsync_L_last = 0x00;
unsigned char Vsync_H_last = 0x00;
unsigned char Vsync_L_last = 0x00;

unsigned char Hsync_L, Hsync_H, Vsync_L, Vsync_H;

int suspend_on = 0;

unsigned char Tx_HPD=0;

/*
 * this timing is mipi timing, please set these timing paremeter
 * same with actual mipi timing(processor's timing)
 * hfp, hs, hbp, hact, htotal, vfp, vs, vbp, vact, vtotal, pclk_khz
 */
struct video_timing video_640x480_60Hz     = {8,   96,  40,  640,  784,  33,  2,   10,  480,  525};
struct video_timing video_720x480_60Hz     = {16,  62,  60,  720,  858,  9,   6,   30,  480,  525};
struct video_timing video_1280x720_60Hz    = {110, 40,  220, 1280, 1650, 5,   5,   20,  720,  750};
struct video_timing video_1366x768_60Hz    = {14,  56,  64,  1366, 1500, 1,   3,   28,  768,  800};
struct video_timing video_1920x1080_60Hz   = {88,  44,  148, 1920, 2200, 4,   5,   36,  1080, 1125};
struct video_timing video_3840x1080_60Hz   = {176, 88,  296, 3840, 4400, 4,   5,   36,  1080, 1125};
struct video_timing video_3840x2160_30Hz   = {176, 88,  296, 3840, 4400, 8,   10,  72,  2160, 2250};
struct video_timing video_1024x768_60Hz    = {24,  136, 160, 1024, 1344, 3,   6,   29,  768,  806,  65000};
struct video_timing video_1280x800_60Hz    = {64,  136, 200, 1280, 1680, 1,   3,   24,  800,  828,  74250};
struct video_timing video_800x1280_60Hz    = {80,  20,  20,  800,  920,  15,  6,   8,   1280, 1309, 67200};

/*
 * panel timing
 * this timing is used for scaler output for LVDS,
 * HDMI output and lvds bypass mode will not use this timing.
 * hfp, hs, hbp, hact, htotal, vfp, vs, vbp, vact, vtotal, pclk_khz
 */
struct video_timing video_1024x600_60Hz    = {50,  20,  50,  1024, 1144, 9,  3,   13,   600,  625,  42500};
struct video_timing video_lvds_60Hz        = {24,  136, 160, 1024, 1344, 3,  6,   29,   768,  806,  65000};


static int i2c_dev_write(unsigned char addr, unsigned char data)
{
	int ret;

	ret = i2c_write(I2CADR, addr, 1, &data, 1);
	return ret;
}

static int i2c_dev_read(unsigned char addr, unsigned char* data)
{
	int ret = 0;

	ret = i2c_read(I2CADR, addr, 1, data, 1);
	return ret;
}

static int HDMI_WriteI2C_Byte(unsigned char addr, unsigned char data)
{
	int flag;

	flag = i2c_dev_write(addr, data);
	mdelay(1);
	return flag;
}

static unsigned char HDMI_ReadI2C_Byte(unsigned char addr)
{
	unsigned char p_data = 0;

	if(i2c_dev_read(addr, &p_data) == 0)
		return p_data;
	return 0;
}

void Timer0_Delay1ms(int t)
{
	int tem = t;
	mdelay(tem);
}

void DigitalClockEn(void)
{
	I2CADR = I2C_BASS_ADDRESS1;
	HDMI_WriteI2C_Byte(0x02,0xf7);
	HDMI_WriteI2C_Byte(0x08,0xff);
	HDMI_WriteI2C_Byte(0x09,0xff);
	HDMI_WriteI2C_Byte(0x0a,0xff);
	HDMI_WriteI2C_Byte(0x0b,0x7c);
	HDMI_WriteI2C_Byte(0x0c,0xff);
}

void TxAnalog(void)
{
	I2CADR = I2C_BASS_ADDRESS1;
	HDMI_WriteI2C_Byte(0x31,0xE1);
	HDMI_WriteI2C_Byte(0x32,0xE1);
	/* en/disable hdmid output */
	HDMI_WriteI2C_Byte(0x33,0x0c);
	HDMI_WriteI2C_Byte(0x37,0x00);
	HDMI_WriteI2C_Byte(0x38,0x22);
	HDMI_WriteI2C_Byte(0x60,0x82);
}

void CbusAnalog(void)
{
	I2CADR = I2C_BASS_ADDRESS1;
	HDMI_WriteI2C_Byte(0x39,0x45);
	/* 20180719 */
	HDMI_WriteI2C_Byte(0x3a,0x00);
	HDMI_WriteI2C_Byte(0x3b,0x00);
}

void HDMIPllAnalog(void)
{
	I2CADR = I2C_BASS_ADDRESS1;
	HDMI_WriteI2C_Byte(0x44,0x31);
	HDMI_WriteI2C_Byte(0x55,0x44);
	HDMI_WriteI2C_Byte(0x57,0x01);
	HDMI_WriteI2C_Byte(0x5a,0x02);
}

void AviInfoframe(void)
{
	I2CADR = I2C_BASS_ADDRESS3;
	/* enable null package */
	HDMI_WriteI2C_Byte(0x3c,0x41);

	/* defualt AVI */
	I2CADR = I2C_BASS_ADDRESS1;
	/* sync polarity + */
	HDMI_WriteI2C_Byte(0xab,0x03);

	I2CADR = I2C_BASS_ADDRESS3;
	/* PB0:check sum */
	HDMI_WriteI2C_Byte(0x43,0x27);
	/* PB1 */
	HDMI_WriteI2C_Byte(0x44,0x10);
	/* PB2 */
	HDMI_WriteI2C_Byte(0x45,0x28);
	/* PB3 */
	HDMI_WriteI2C_Byte(0x46,0x00);
	/* PB4;vic */
	HDMI_WriteI2C_Byte(0x47,0x10);

#ifdef _1080P_60Hz
	/* 1080P60Hz 16:9 */
	I2CADR = I2C_BASS_ADDRESS1;
	/* sync polarity + */
	HDMI_WriteI2C_Byte(0xab,0x03);

	I2CADR = I2C_BASS_ADDRESS3;
	/* PB0:check sum */
	HDMI_WriteI2C_Byte(0x43,0x27);
	/* PB1 */
	HDMI_WriteI2C_Byte(0x44,0x10);
	/* PB2 */
	HDMI_WriteI2C_Byte(0x45,0x28);
	/* PB3 */
	HDMI_WriteI2C_Byte(0x46,0x00);
	/* PB4:vic */
	HDMI_WriteI2C_Byte(0x47,0x10);
#endif

#ifdef _720P_60Hz
	/* 720P60Hz 16:9 */
	I2CADR = I2C_BASS_ADDRESS1;
	/* sync polarity + */
	HDMI_WriteI2C_Byte(0xab,0x03);

	I2CADR = I2C_BASS_ADDRESS3;
	/* PB0:check sum */
	HDMI_WriteI2C_Byte(0x43,0x33);
	/* PB1 */
	HDMI_WriteI2C_Byte(0x44,0x10);
	/* PB2 */
	HDMI_WriteI2C_Byte(0x45,0x28);
	/* PB3 */
	HDMI_WriteI2C_Byte(0x46,0x00);
	/* PB4:vic */
	HDMI_WriteI2C_Byte(0x47,0x04);
#endif

#ifdef _480P_60Hz
	/* 720x480 60Hz 4:3 */
	I2CADR = I2C_BASS_ADDRESS1;
	/* sync polarity + */
	HDMI_WriteI2C_Byte(0xab,0x0c);

	I2CADR = I2C_BASS_ADDRESS3;
	/* PB0:check sum */
	HDMI_WriteI2C_Byte(0x43,0x45);
	/* PB1 */
	HDMI_WriteI2C_Byte(0x44,0x10);
	/* PB2 */
	HDMI_WriteI2C_Byte(0x45,0x18);
	/* PB3 */
	HDMI_WriteI2C_Byte(0x46,0x00);
	/* PB4:vic */
	HDMI_WriteI2C_Byte(0x47,0x02);
#endif
}

void MipiAnalog(void)
{
	I2CADR = I2C_BASS_ADDRESS1;
#ifdef _pn_swap_
	/* P/N swap */
	HDMI_WriteI2C_Byte(0x3e,0xf6);
#else
	/*
	 * if mipi pin map follow reference design,
	 * no need swap P/N.
	 */
	HDMI_WriteI2C_Byte(0x3e,0xd6);
#endif
	/* EQ */
	HDMI_WriteI2C_Byte(0x3f,0xd4);
	/* EQ */
	HDMI_WriteI2C_Byte(0x41,0x3c);
}

void MipiBasicSet(void)
{
	I2CADR = I2C_BASS_ADDRESS2;
	/* term en */
	HDMI_WriteI2C_Byte(0x10,0x01);
	/* settle */
	HDMI_WriteI2C_Byte(0x11,0x5);
	/*
	 * 00 4 lane
	 * 01 1 lane
	 * 02 2 lane
	 * 03 3 lane
	 */
	HDMI_WriteI2C_Byte(0x13,lane_cnt);
	/* debug mux */
	HDMI_WriteI2C_Byte(0x14,0x00);

#ifdef _lane_swap_
	/*
	 * for EVB only,
	 * if mipi pin map follow reference design,
	 * no need swap lane.
	 */
	/* lane swap:3210 */
	HDMI_WriteI2C_Byte(0x15,0xa8);
#else
	/* lane swap:0123 */
	HDMI_WriteI2C_Byte(0x15,0x00);
#endif
	/* hshift 3 */
	HDMI_WriteI2C_Byte(0x1a,0x03);
	/* vshift 3 */
	HDMI_WriteI2C_Byte(0x1b,0x03);
}

void MIPI_Video_Setup(struct video_timing *video_format)
{
	I2CADR = I2C_BASS_ADDRESS2;
	/* hwidth */
	HDMI_WriteI2C_Byte(0x18,(unsigned char)(video_format->hs%256));
	/* vwidth 6 */
	HDMI_WriteI2C_Byte(0x19,(unsigned char)(video_format->vs%256));
	/* H_active[7:0] */
	HDMI_WriteI2C_Byte(0x1c,(unsigned char)(video_format->hact%256));
	/* H_active[15:8] */
	HDMI_WriteI2C_Byte(0x1d,(unsigned char)(video_format->hact/256));
	/* fifo_buff_length 12 */
	HDMI_WriteI2C_Byte(0x2f,0x0c);
	/* H_total[7:0] */
	HDMI_WriteI2C_Byte(0x34,(unsigned char)(video_format->htotal%256));
	/* H_total[15:8] */
	HDMI_WriteI2C_Byte(0x35,(unsigned char)(video_format->htotal/256));
	/* V_total[7:0] */
	HDMI_WriteI2C_Byte(0x36,(unsigned char)(video_format->vtotal%256));
	/* V_total[15:8] */
	HDMI_WriteI2C_Byte(0x37,(unsigned char)(video_format->vtotal/256));
	/* VBP[7:0] */
	HDMI_WriteI2C_Byte(0x38,(unsigned char)(video_format->vbp%256));
	/* VBP[15:8] */
	HDMI_WriteI2C_Byte(0x39,(unsigned char)(video_format->vbp/256));
	/* VFP[7:0] */
	HDMI_WriteI2C_Byte(0x3a,(unsigned char)(video_format->vfp%256));
	/* VFP[15:8] */
	HDMI_WriteI2C_Byte(0x3b,(unsigned char)(video_format->vfp/256));
	/* HBP[7:0] */
	HDMI_WriteI2C_Byte(0x3c,(unsigned char)(video_format->hbp%256));
	/* HBP[15:8] */
	HDMI_WriteI2C_Byte(0x3d,(unsigned char)(video_format->hbp/256));
	/* HFP[7:0] */
	HDMI_WriteI2C_Byte(0x3e,(unsigned char)(video_format->hfp%256));
	/* HFP[15:8] */
	HDMI_WriteI2C_Byte(0x3f,(unsigned char)(video_format->hfp/256));
}

void MIPIRxLogicRes(void)
{
	I2CADR = I2C_BASS_ADDRESS1;
	/* mipi rx reset */
	HDMI_WriteI2C_Byte(0x03,0x7f);
	Timer0_Delay1ms(10);
	HDMI_WriteI2C_Byte(0x03,0xff);

	/* dds reset */
	HDMI_WriteI2C_Byte(0x05,0xfb);
	Timer0_Delay1ms(10);
	HDMI_WriteI2C_Byte(0x05,0xff);
}

void DDSConfig(void)
{
	I2CADR = I2C_BASS_ADDRESS2;

	/* strm_sw_freq_word[ 7: 0] */
	HDMI_WriteI2C_Byte(0x4e,0xaa);
	/* strm_sw_freq_word[15: 8] */
	HDMI_WriteI2C_Byte(0x4f,0xaa);
	/* strm_sw_freq_word[23:16] */
	HDMI_WriteI2C_Byte(0x50,0x6a);
	/* [0]=strm_sw_freq_word[24] */
	HDMI_WriteI2C_Byte(0x51,0x80);

	HDMI_WriteI2C_Byte(0x1e,0x4f);
	/* full_value   464 */
	HDMI_WriteI2C_Byte(0x1f,0x5e);
	HDMI_WriteI2C_Byte(0x20,0x01);
	/* full_value1  416 */
	HDMI_WriteI2C_Byte(0x21,0x2c);
	HDMI_WriteI2C_Byte(0x22,0x01);
	/* full_value2  400 */
	HDMI_WriteI2C_Byte(0x23,0xfa);
	HDMI_WriteI2C_Byte(0x24,0x00);
	/* full_value3  384 */
	HDMI_WriteI2C_Byte(0x25,0xc8);
	HDMI_WriteI2C_Byte(0x26,0x00);
	/* empty_value   464 */
	HDMI_WriteI2C_Byte(0x27,0x5e);
	HDMI_WriteI2C_Byte(0x28,0x01);
	/* empty_value1  416 */
	HDMI_WriteI2C_Byte(0x29,0x2c);
	HDMI_WriteI2C_Byte(0x2a,0x01);
	/* empty_value2  400 */
	HDMI_WriteI2C_Byte(0x2b,0xfa);
	HDMI_WriteI2C_Byte(0x2c,0x00);
	/* empty_value3  384 */
	HDMI_WriteI2C_Byte(0x2d,0xc8);
	HDMI_WriteI2C_Byte(0x2e,0x00);
	/* tmr_set[ 7:0]:100us */
	HDMI_WriteI2C_Byte(0x42,0x64);
	/* tmr_set[15:8]:100us */
	HDMI_WriteI2C_Byte(0x43,0x00);
	/* timer step */
	HDMI_WriteI2C_Byte(0x44,0x04);
	HDMI_WriteI2C_Byte(0x45,0x00);
	HDMI_WriteI2C_Byte(0x46,0x59);
	HDMI_WriteI2C_Byte(0x47,0x00);
	HDMI_WriteI2C_Byte(0x48,0xf2);
	HDMI_WriteI2C_Byte(0x49,0x06);
	HDMI_WriteI2C_Byte(0x4a,0x00);
	HDMI_WriteI2C_Byte(0x4b,0x72);
	HDMI_WriteI2C_Byte(0x4c,0x45);
	HDMI_WriteI2C_Byte(0x4d,0x00);
	/* trend step */
	HDMI_WriteI2C_Byte(0x52,0x08);
	HDMI_WriteI2C_Byte(0x53,0x00);
	HDMI_WriteI2C_Byte(0x54,0xb2);
	HDMI_WriteI2C_Byte(0x55,0x00);
	HDMI_WriteI2C_Byte(0x56,0xe4);
	HDMI_WriteI2C_Byte(0x57,0x0d);
	HDMI_WriteI2C_Byte(0x58,0x00);
	HDMI_WriteI2C_Byte(0x59,0xe4);
	HDMI_WriteI2C_Byte(0x5a,0x8a);
	HDMI_WriteI2C_Byte(0x5b,0x00);
	HDMI_WriteI2C_Byte(0x5c,0x34);
	HDMI_WriteI2C_Byte(0x51,0x00);
}

void AudioIIsEn(void)
{
	/* sampling 48K, sclk = 64*fs. */
	I2CADR = I2C_BASS_ADDRESS1;
	HDMI_WriteI2C_Byte(0xB2,0x01);
	I2CADR = I2C_BASS_ADDRESS3;
	HDMI_WriteI2C_Byte(0x06,0x08);
	HDMI_WriteI2C_Byte(0x07,0xF0);
	/* 0xE2:32FS; 0xD2:64FS */
	HDMI_WriteI2C_Byte(0x34,0xD2);
}

void AudioSpdifEn(void)
{
	I2CADR = I2C_BASS_ADDRESS1;
	HDMI_WriteI2C_Byte(0xB2,0x01);
	I2CADR = I2C_BASS_ADDRESS3;
	HDMI_WriteI2C_Byte(0x06,0x0e);
	HDMI_WriteI2C_Byte(0x07,0x00);
	/* 0xE2:32FS; 0xD2:64FS */
	HDMI_WriteI2C_Byte(0x34,0xD2);
}

void Core_Pll_setup(struct video_timing *panel)
{
	unsigned char cpll_m, cpll_k1,cpll_k2;
	unsigned int temp;

	temp=(panel->pclk_khz*7)/25;
	cpll_m=temp/1000;

	temp=(panel->pclk_khz*7)/25;
	temp=temp%1000;
	temp=temp*16.384;

	cpll_k1=temp%256;
	cpll_k2=temp/256;

	I2CADR = I2C_BASS_ADDRESS1;
	/* cp=50uA */
	HDMI_WriteI2C_Byte(0x50,0x24);
	/* xtal_clk as reference,second order passive LPF PLL */
	HDMI_WriteI2C_Byte(0x51,0x05);
	/* use second-order PLL */
	HDMI_WriteI2C_Byte(0x52,0x14);
	/* CP_PRESET_DIV_RATIO */
	HDMI_WriteI2C_Byte(0x69,cpll_m);
	HDMI_WriteI2C_Byte(0x69,(cpll_m|0x80));
	/* RGD_CP_SOFT_K_EN,RGD_CP_SOFT_K[13:8] */
	HDMI_WriteI2C_Byte(0x6c,(cpll_k2|0x80));
	HDMI_WriteI2C_Byte(0x6b,cpll_k1);

	/* core pll reset */
	HDMI_WriteI2C_Byte(0x04,0xfb);
	HDMI_WriteI2C_Byte(0x04,0xff);
}

void Core_Pll_bypass(void)
{
	I2CADR = I2C_BASS_ADDRESS1;
	/* cp=50uA */
	HDMI_WriteI2C_Byte(0x50,0x24);
	/* Pix_clk as reference,second order passive LPF PLL */
	HDMI_WriteI2C_Byte(0x51,0x2d);
	/* loopdiv=0;use second-order PLL */
	HDMI_WriteI2C_Byte(0x52,0x04);
	/* CP_PRESET_DIV_RATIO */
	HDMI_WriteI2C_Byte(0x69,0x0e);
	HDMI_WriteI2C_Byte(0x69,0x8e);
	HDMI_WriteI2C_Byte(0x6a,0x00);
	/* RGD_CP_SOFT_K_EN,RGD_CP_SOFT_K[13:8] */
	HDMI_WriteI2C_Byte(0x6c,0xb8);
	HDMI_WriteI2C_Byte(0x6b,0x51);

	/* core pll reset */
	HDMI_WriteI2C_Byte(0x04,0xfb);
	HDMI_WriteI2C_Byte(0x04,0xff);
}

void Lvds_Pll_Reset(void)
{
	I2CADR = I2C_BASS_ADDRESS1;
	/* lvds pll reset */
	HDMI_WriteI2C_Byte(0x02,0xf7);
	HDMI_WriteI2C_Byte(0x02,0xff);
}

void Scaler_bypass(void)
{
	I2CADR = I2C_BASS_ADDRESS1;
	/* disable scaler */
	HDMI_WriteI2C_Byte(0x7f,0x00);
	HDMI_WriteI2C_Byte(0xa8,0x13);
}

void Scaler_setup(struct video_timing *input_video,struct video_timing *panel)
{
	/*
	 * for example: 720P to 1280x800
	 * These register base on MIPI resolution and LVDS panel resolution.
	 */
	unsigned int h_ratio,v_ratio;
	unsigned char i;
	unsigned int htotal;
	h_ratio = input_video->hact*4096.00/panel->hact;
	v_ratio = input_video->vact*4096.00/panel->vact;

	I2CADR = I2C_BASS_ADDRESS1;
	HDMI_WriteI2C_Byte(0x80,0x00);
	HDMI_WriteI2C_Byte(0x81,0xff);
	HDMI_WriteI2C_Byte(0x82,0x03);
	HDMI_WriteI2C_Byte(0x83,(unsigned char)(input_video->hact%256));
	HDMI_WriteI2C_Byte(0x84,(unsigned char)(input_video->hact/256));
	HDMI_WriteI2C_Byte(0x85,0x80);
	HDMI_WriteI2C_Byte(0x86,0x10);
	HDMI_WriteI2C_Byte(0x87,(unsigned char)(panel->htotal%256));
	HDMI_WriteI2C_Byte(0x88,(unsigned char)(panel->htotal/256));
	HDMI_WriteI2C_Byte(0x89,(unsigned char)(panel->hs%256));
	HDMI_WriteI2C_Byte(0x8a,(unsigned char)(panel->hbp%256));
	HDMI_WriteI2C_Byte(0x8b,(unsigned char)(panel->vs%256));
	HDMI_WriteI2C_Byte(0x8c,(unsigned char)(panel->hact%256));
	HDMI_WriteI2C_Byte(0x8d,(unsigned char)(panel->vact%256));
	HDMI_WriteI2C_Byte(0x8e,(unsigned char)(panel->vact/256)*16+(panel->hact/256));
	HDMI_WriteI2C_Byte(0x8f,(unsigned char)(h_ratio%256));
	HDMI_WriteI2C_Byte(0x90,(unsigned char)(h_ratio/256));
	HDMI_WriteI2C_Byte(0x91,(unsigned char)(v_ratio%256));
	HDMI_WriteI2C_Byte(0x92,(unsigned char)(v_ratio/256));
	HDMI_WriteI2C_Byte(0x7f,0x96);
	HDMI_WriteI2C_Byte(0xa8,0x13);

	/* lvds pll reset */
	HDMI_WriteI2C_Byte(0x02,0xf7);
	HDMI_WriteI2C_Byte(0x02,0xff);

	/* scaler reset */
	HDMI_WriteI2C_Byte(0x03,0xcf);
	HDMI_WriteI2C_Byte(0x03,0xff);

	HDMI_WriteI2C_Byte(0x7f,0xb0);

	for(i = 0; i < 5; i++)
	{
		if(HDMI_ReadI2C_Byte(0xa7)&0x20)
		{
			htotal = (HDMI_ReadI2C_Byte(0xa7)&0x0f)*0x100 + HDMI_ReadI2C_Byte(0xa6);
			printf("\r\n scaler setup htotal = %d", htotal);
			break;
		}
		Timer0_Delay1ms(100);
	}
}

void LvdsPowerUp(void)
{
	I2CADR = I2C_BASS_ADDRESS1;
	HDMI_WriteI2C_Byte(0x44,0x30);
	HDMI_WriteI2C_Byte(0x51,0x05);
}
void LvdsPowerDown(void)
{
	I2CADR = I2C_BASS_ADDRESS1;
	HDMI_WriteI2C_Byte(0x51,0x15);
}

void LvdsBypass(void)
{
	I2CADR = I2C_BASS_ADDRESS1;
	/* cp=50uA */
	HDMI_WriteI2C_Byte(0x50,0x24);
	/* Pix_clk as reference,second order passive LPF PLL */
	HDMI_WriteI2C_Byte(0x51,0x2d);
	/* loopdiv=0;use second-order PLL */
	HDMI_WriteI2C_Byte(0x52,0x04);
	/* CP_PRESET_DIV_RATIO */
	HDMI_WriteI2C_Byte(0x69,0x0e);
	HDMI_WriteI2C_Byte(0x69,0x8e);
	HDMI_WriteI2C_Byte(0x6a,0x00);
	/* RGD_CP_SOFT_K_EN,RGD_CP_SOFT_K[13:8] */
	HDMI_WriteI2C_Byte(0x6c,0xb8);
	HDMI_WriteI2C_Byte(0x6b,0x51);

	/* core pll reset */
	HDMI_WriteI2C_Byte(0x04,0xfb);
	HDMI_WriteI2C_Byte(0x04,0xff);

	/* disable scaler */
	HDMI_WriteI2C_Byte(0x7f,0x00);
	/* 0x13:VSEA ; 0x33:JEIDA */
	HDMI_WriteI2C_Byte(0xa8,0x13);
}

void LvdsOutput(int on)
{
	if(on)
	{
		I2CADR = I2C_BASS_ADDRESS1;
		/* lvds pll reset */
		HDMI_WriteI2C_Byte(0x02,0xf7);
		/* scaler module reset */
		HDMI_WriteI2C_Byte(0x02,0xff);
		/* lvds tx module reset */
		HDMI_WriteI2C_Byte(0x03,0xcb);
		HDMI_WriteI2C_Byte(0x03,0xfb);
		HDMI_WriteI2C_Byte(0x03,0xff);

		/* enbale lvds output */
		HDMI_WriteI2C_Byte(0x44,0x30);
	}
	else
	{
		I2CADR = I2C_BASS_ADDRESS1;
		HDMI_WriteI2C_Byte(0x44,0x31);
	}
}

void HdmiOutput(int on)
{
	if(on)
	{
		I2CADR = I2C_BASS_ADDRESS1;
		/* enable hdmi output */
		HDMI_WriteI2C_Byte(0x33,0x0e);
	}
	else
	{
		I2CADR = I2C_BASS_ADDRESS1;
		/* disable hdmi output */
		HDMI_WriteI2C_Byte(0x33,0x0c);
	}
}

void LvdsScalerResult(void)
{
	I2CADR = I2C_BASS_ADDRESS1;
	HDMI_WriteI2C_Byte(0x7f,0xb0);
}

void ScalerReset(void)
{
	I2CADR = I2C_BASS_ADDRESS1;
	HDMI_WriteI2C_Byte(0x03,0xcf);
	HDMI_WriteI2C_Byte(0x03,0xff);
}

void lt8912_check_dds(void)
{
	unsigned char reg_920d, reg_920e;
	unsigned char i;
	for(i = 0; i < 10; i++)
	{
		I2CADR = I2C_BASS_ADDRESS2;
		reg_920d = HDMI_ReadI2C_Byte(0x0d);
		reg_920e = HDMI_ReadI2C_Byte(0x0e);
		/* shall update threshold here base on actual dds result. */
		if((reg_920e == 0xd2)&&(reg_920d < 0xff)&&(reg_920d > 0xd0))
		{
			printf("\r\nlvds_check_dds: stable!");
			break;
		}

		Timer0_Delay1ms(1000);
	}
}

void lvds_output_cfg(void)
{
	LvdsPowerUp();
#ifdef _lvds_bypass
	LvdsBypass();
#else
	Core_Pll_setup(&video_lvds_60Hz);
	Scaler_setup(&video_lvds_60Hz,&video_lvds_60Hz);
#endif
}

void MIPI_Input_det(void)
{
	I2CADR = I2C_BASS_ADDRESS1;
	Hsync_L = HDMI_ReadI2C_Byte(0x9c);
	Hsync_H = HDMI_ReadI2C_Byte(0x9d);
	Vsync_L = HDMI_ReadI2C_Byte(0x9e);
	Vsync_H = HDMI_ReadI2C_Byte(0x9f);

	/* hiht byte changed */
	if((Hsync_H!=Hsync_H_last)||(Vsync_H!=Vsync_H_last))
	{
		if(Vsync_H == 0x02 && Vsync_L == 0x71)
		{
			MIPI_Video_Setup(&video_1024x600_60Hz);
		}
		else if(Vsync_H==0x02 && Vsync_L <= 0xef&&Vsync_L >= 0xec)
		{

			MIPI_Video_Setup(&video_1280x720_60Hz);
		}
		else if(Vsync_H == 0x03 && Vsync_L <= 0x3a &&Vsync_L >= 0x34)
		{
			MIPI_Video_Setup(&video_1280x800_60Hz);
		}
		else if(Vsync_H == 0x04 && Vsync_L <= 0x67&&Vsync_L >= 0x63)
		{
			MIPI_Video_Setup(&video_1920x1080_60Hz);
		}
		else if(Vsync_H == 0x03 && Vsync_L <= 0x23&&Vsync_L >= 0x1d)
		{

			MIPI_Video_Setup(&video_lvds_60Hz);
		}
		else if(Vsync_H == 0x1d && Vsync_L <= 0x05 &&Vsync_L >= 0x1d)
		{
			MIPI_Video_Setup(&video_800x1280_60Hz);
		}
		else
		{
			MIPI_Video_Setup(&video_1920x1080_60Hz);
		}
		Hsync_L_last = Hsync_L;
		Hsync_H_last = Hsync_H;
		Vsync_L_last = Vsync_L;
		Vsync_H_last = Vsync_H;

		MIPIRxLogicRes();
	}
}

void dds_clock_debug(void)
{
#ifdef dds_debug
	unsigned char reg_920c, reg_920d, reg_920e, reg_920f;

	while(1)
	{
		I2CADR = I2C_BASS_ADDRESS2;
		reg_920c = HDMI_ReadI2C_Byte(0x0c);
		reg_920d = HDMI_ReadI2C_Byte(0x0d);
		reg_920e = HDMI_ReadI2C_Byte(0x0e);
		reg_920f = HDMI_ReadI2C_Byte(0x0f);

		printf("\r\n0x0c~0e = %02x, %02x, %02x",reg_920c, reg_920d, reg_920e);
		printf("\r\nEnter the dds_clock_debug cycle");

		Timer0_Delay1ms(1000);
	}
#endif
}

void pattern_test(void)
{
	/* 1080P Pattern output */
	I2CADR = I2C_BASS_ADDRESS1;
	HDMI_WriteI2C_Byte(0x08,0xff);
	HDMI_WriteI2C_Byte(0x09,0xff);
	HDMI_WriteI2C_Byte(0x0a,0xff);
	HDMI_WriteI2C_Byte(0x0b,0xff);
	HDMI_WriteI2C_Byte(0x0c,0xff);
	HDMI_WriteI2C_Byte(0x31,0xa1);
	HDMI_WriteI2C_Byte(0x32,0xa1);
	HDMI_WriteI2C_Byte(0x33,0x03);
	HDMI_WriteI2C_Byte(0x37,0x00);
	HDMI_WriteI2C_Byte(0x38,0x22);
	HDMI_WriteI2C_Byte(0x60,0x82);
	HDMI_WriteI2C_Byte(0x39,0x45);
	HDMI_WriteI2C_Byte(0x3b,0x00);
	HDMI_WriteI2C_Byte(0x44,0x31);
	HDMI_WriteI2C_Byte(0x55,0x44);
	HDMI_WriteI2C_Byte(0x57,0x01);
	HDMI_WriteI2C_Byte(0x5a,0x02);

	DigitalClockEn();
	TxAnalog();
	CbusAnalog();
	HDMIPllAnalog();
	AudioIIsEn();
	AviInfoframe();

	I2CADR = I2C_BASS_ADDRESS2;
	/* term en  To analog phy for trans lp mode to hs mode */
	HDMI_WriteI2C_Byte(0x10,0x00);
	/* settle Set timing for dphy trans state from PRPR to SOT state */
	HDMI_WriteI2C_Byte(0x11,0x04);
	/* trail */
	HDMI_WriteI2C_Byte(0x12,0x04);
	/* 4 lane, 01 lane, 02 2lane, 03 3lane */
	HDMI_WriteI2C_Byte(0x13,0x00);
	/* debug mux */
	HDMI_WriteI2C_Byte(0x14,0x00);
	HDMI_WriteI2C_Byte(0x15,0x00);
	/* hshift 3 */
	HDMI_WriteI2C_Byte(0x1a,0x03);
	/* vshift 3 */
	HDMI_WriteI2C_Byte(0x1b,0x03);
	/* hwidth 62 */
	HDMI_WriteI2C_Byte(0x18,0x28);
	/* vwidth 6 */
	HDMI_WriteI2C_Byte(0x19,0x05);
	/* pix num hactive */
	HDMI_WriteI2C_Byte(0x1c,0x00);
	HDMI_WriteI2C_Byte(0x1d,0x05);
	/* h v d pol hdmi sel pll sel */
	HDMI_WriteI2C_Byte(0x1e,0x67);
	/* fifo_buff_length 12 */
	HDMI_WriteI2C_Byte(0x2f,0x0c);
	/* htotal */
	HDMI_WriteI2C_Byte(0x34,0x72);
	/* htotal */
	HDMI_WriteI2C_Byte(0x35,0x06);
	/* vtotal */
	HDMI_WriteI2C_Byte(0x36,0xee);
	/* vtotal */
	HDMI_WriteI2C_Byte(0x37,0x02);
	/* vbp */
	HDMI_WriteI2C_Byte(0x38,0x14);
	/* vbp */
	HDMI_WriteI2C_Byte(0x39,0x00);
	/* vfp */
	HDMI_WriteI2C_Byte(0x3a,0x05);
	/* vfp */
	HDMI_WriteI2C_Byte(0x3b,0x00);
	/* hbp */
	HDMI_WriteI2C_Byte(0x3c,0xdc);
	/* hbp */
	HDMI_WriteI2C_Byte(0x3d,0x00);
	/* hfp */
	HDMI_WriteI2C_Byte(0x3e,0x6e);
	/* hfp */
	HDMI_WriteI2C_Byte(0x3f,0x00);
	HDMI_WriteI2C_Byte(0x72,0x12);
	/* RGD_PTN_DE_DLY[7:0] */
	HDMI_WriteI2C_Byte(0x73,0x04);
	/* RGD_PTN_DE_DLY[11:8]  260 */
	HDMI_WriteI2C_Byte(0x74,0x01);
	/* RGD_PTN_DE_TOP[6:0]   150 */
	HDMI_WriteI2C_Byte(0x75,0x19);
	/* RGD_PTN_DE_CNT[7:0] */
	HDMI_WriteI2C_Byte(0x76,0x00);
	/* RGD_PTN_DE_LIN[7:0] */
	HDMI_WriteI2C_Byte(0x77,0xd0);
	/* RGD_PTN_DE_LIN[10:8],RGD_PTN_DE_CNT[11:8] */
	HDMI_WriteI2C_Byte(0x78,0x25);
	/* RGD_PTN_H_TOTAL[7:0] */
	HDMI_WriteI2C_Byte(0x79,0x72);
	/* RGD_PTN_V_TOTAL[7:0] */
	HDMI_WriteI2C_Byte(0x7a,0xee);
	/* RGD_PTN_V_TOTAL[10:8],RGD_PTN_H_TOTAL[11:8] */
	HDMI_WriteI2C_Byte(0x7b,0x26);
	/* RGD_PTN_HWIDTH[7:0] */
	HDMI_WriteI2C_Byte(0x7c,0x28);
	/* RGD_PTN_HWIDTH[9:8],RGD_PTN_VWIDTH[5:0] */
	HDMI_WriteI2C_Byte(0x7d,0x05);
	/* pattern enable */
	HDMI_WriteI2C_Byte(0x70,0x80);
	HDMI_WriteI2C_Byte(0x71,0x51);
	HDMI_WriteI2C_Byte(0x42,0x12);
	/* strm_sw_freq_word[ 7: 0] */
	HDMI_WriteI2C_Byte(0x4e,0xAA);
	/* strm_sw_freq_word[15: 8] */
	HDMI_WriteI2C_Byte(0x4f,0xAA);
	/* strm_sw_freq_word[23:16] */
	HDMI_WriteI2C_Byte(0x50,0x6A);
	/* pattern en */
	HDMI_WriteI2C_Byte(0x51,0x80);

	HdmiOutput(1);
}

unsigned char LT8912_Get_HPD(void)
{
	I2CADR = I2C_BASS_ADDRESS1;
	if((HDMI_ReadI2C_Byte(0xc1)&0x80)==0x80)
	{
		printf("\r\nLT8912_Get_HPD: high");
		return 1;
	}
	else
	{
		printf("\r\nLT8912_Get_HPD: low");
		return 0;
	}
}

void read_LT8912_chip_ID(void)
{
	I2CADR = I2C_BASS_ADDRESS1;
	printk("LT8912b chip ID: 0x%x, 0x%x\r\n",HDMI_ReadI2C_Byte(0x00), HDMI_ReadI2C_Byte(0x01));
}

void LT8912B_Suspend(int on)
{
	/* 9mA,HPD detect is normal. */
	if(on)
	{
		if(!suspend_on)
		{
			/* enter suspend mode */
			I2CADR = I2C_BASS_ADDRESS1;
			HDMI_WriteI2C_Byte( 0x54, 0x1d );
			HDMI_WriteI2C_Byte( 0x51, 0x15 );
			HDMI_WriteI2C_Byte( 0x44, 0x31 );
			HDMI_WriteI2C_Byte( 0x41, 0xbd );
			HDMI_WriteI2C_Byte( 0x5c, 0x11 );
			suspend_on = 1;
			printf("\r\nsuspend on");
		}
	}
	else
	{
		if(suspend_on)
		{
			/* exist suspend mode */
			I2CADR = I2C_BASS_ADDRESS1;
			HDMI_WriteI2C_Byte( 0x5c, 0x10 );
			HDMI_WriteI2C_Byte( 0x54, 0x1c );
			HDMI_WriteI2C_Byte( 0x51, 0x2d );
			HDMI_WriteI2C_Byte( 0x44, 0x30 );
			HDMI_WriteI2C_Byte( 0x41, 0xbc );

			Timer0_Delay1ms(10);
			HDMI_WriteI2C_Byte(0x03,0x7f);
			Timer0_Delay1ms(10);
			HDMI_WriteI2C_Byte(0x03,0xff);

			HDMI_WriteI2C_Byte(0x05,0xfb);
			Timer0_Delay1ms(10);
			HDMI_WriteI2C_Byte(0x05,0xff);
			suspend_on = 0;
			printf("\r\nsuspend off");
		}
	}
}

void lt8912_enable(void)
{
	read_LT8912_chip_ID();

#ifdef _pattern_test_
	pattern_test();
	while(1);
#else

	DigitalClockEn();
	TxAnalog();
	CbusAnalog();
	HDMIPllAnalog();
	MipiAnalog();
	MipiBasicSet();
	DDSConfig();
	MIPI_Video_Setup(&video_1920x1080_60Hz);
	MIPI_Input_det();
	AudioIIsEn();
	AviInfoframe();
	MIPIRxLogicRes();
#ifdef dds_debug
	lt8912_check_dds();
#endif
#endif
	MIPI_Input_det();
	dds_clock_debug();
	Timer0_Delay1ms(1000);

	LT8912B_Suspend(0);
	HdmiOutput(1);
}

static void lcd_cfg_panel_info(panel_extend_para * info)
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
	/* open lcd power, and delay 50ms */
	LCD_OPEN_FUNC(sel, lcd_power_on, 50);
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
	/* close lcd power, and delay 500ms */
	LCD_CLOSE_FUNC(sel, lcd_power_off, 500);

	return 0;
}

static void lcd_power_on(u32 sel)
{
	/* config lcd_power pin to open lcd power */
	sunxi_lcd_power_enable(sel, 0);
	sunxi_lcd_pin_cfg(sel, 1);
	sunxi_lcd_delay_ms(30);
	panel_reset(1);
	sunxi_lcd_delay_ms(30);
	panel_reset(0);
	sunxi_lcd_delay_ms(30);
	panel_reset(1);
	sunxi_lcd_delay_ms(30);
	/* LT8912b en */
	sunxi_lcd_gpio_set_value(sel, 1, 0);
	sunxi_lcd_delay_ms(30);

}

static void lcd_power_off(u32 sel)
{
	sunxi_lcd_gpio_set_value(sel, 1, 1);
	sunxi_lcd_delay_ms(10);
	panel_reset(0);
	sunxi_lcd_delay_ms(10);
	sunxi_lcd_pin_cfg(sel, 0);
	/* config lcd_power pin to open lcd power */
	sunxi_lcd_power_disable(sel, 0);
	sunxi_lcd_dsi_clk_disable(sel);
}

static void lcd_panel_init(u32 sel)
{
	sunxi_lcd_dsi_clk_enable(sel);
	sunxi_lcd_delay_ms(50);

	/* reset lcd panel */
	sunxi_lcd_dsi_dcs_write_0para(sel, DSI_DCS_SOFT_RESET);
	sunxi_lcd_delay_ms(50);
	/*exit sleep mode and set display on*/
	sunxi_lcd_dsi_dcs_write_0para(sel, DSI_DCS_EXIT_SLEEP_MODE);
	sunxi_lcd_delay_ms(120);
	sunxi_lcd_dsi_dcs_write_0para(sel, DSI_DCS_SET_DISPLAY_ON);
	sunxi_lcd_delay_ms(10);

	lt8912_enable();
}

static void lcd_panel_exit(u32 sel)
{
	sunxi_lcd_dsi_dcs_write_0para(sel, DSI_DCS_SET_DISPLAY_OFF);
	sunxi_lcd_delay_ms(20);
	sunxi_lcd_dsi_dcs_write_0para(sel, DSI_DCS_ENTER_SLEEP_MODE);
	sunxi_lcd_delay_ms(20);
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

__lcd_panel_t lt8912b_panel = {
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
