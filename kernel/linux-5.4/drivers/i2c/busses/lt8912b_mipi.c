#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/sunxi-gpio.h>
#include <asm/uaccess.h>

#define LT8912B_I2C_NAME "LT8912B"
#define LT8912B_RESET_GPIO      (GPIOE(17))

struct i2c_client *client_lt8912b;

#define ONE_I2C_ADDR 0x48
#define TWO_I2C_ADDR 0x49
#define THR_I2C_ADDR 0x4A
///////////////////////////////////////////////////////////////////////
// Global Setting
///////////////////////////////////////////////////////////////////////
#define _HDMI_Output_  1                                // With video and audio output.
//#define _DVI_Output_ // Only video output, no audio.
#define _LVDS_Output_  0								// LVDS output to drive panel.
enum
{
	H_act = 0,
	V_act,
	H_tol,
	V_tol,
	H_bp,
	H_sync,
	V_sync,
	V_bp
};

#ifdef _HDMI_Output_
enum {
	_32KHz = 0,
	_44d1KHz,
	_48KHz,

	_88d2KHz,
	_96KHz,
	_176Khz,
	_196KHz
};

u16 IIS_N[] =
{
	4096,               // 32K
	6272,               // 44.1K
	6144,               // 48K
	12544,              // 88.2K
	12288,              // 96K
	25088,              // 176K
	24576               // 196K
};

u16 Sample_Freq[] =
{
	0x30,               // 32K
	0x00,               // 44.1K
	0x20,               // 48K
	0x80,               // 88.2K
	0xa0,               // 96K
	0xc0,               // 176K
	0xe0                // 196K
};
#endif
//============================================//
#ifdef _LVDS_Output_
#define _Scaler_Mode_   // Mipi signal resolution and LVDS panel resolution are different.
//#define _Bypass_Mode_	// Mipi signal resolution is the same as LVDS panel resolution.

#define _8_Bit_Color_   // 24 bit
//  #define _6_Bit_Color_ // 18 bit

#define _VESA_
//#define _JEIDA_
//#define _De_mode_
#define _Sync_Mode_
#ifdef _Scaler_Mode_
// 1280x800 panel
   #define Panel_Pixel_CLK 7000  // 71MHZ
   #define Panel_H_Active	1280
   #define Panel_V_Active	800
   #define Panel_H_Total	1324
   #define Panel_V_Total	880
   #define Panel_H_FrontPorch	34
   #define Panel_H_SyncWidth	4
   #define Panel_H_BackPorch	6
   #define Panel_V_FrontPorch	50
   #define Panel_V_SyncWidth	20
   #define Panel_V_BackPorch	10

union Temp
{
	u8	Temp8[4];
	u16 Temp16[2];
	u32 Temp32;
};
#endif

#ifdef _VESA_
#define _VesaJeidaMode 0x00
#else
#define _VesaJeidaMode 0x20
#endif

#ifdef _De_mode_
#define _DE_Sync_mode 0x08
#else
#define _DE_Sync_mode 0x00
#endif

#ifdef _8_Bit_Color_
#define _ColorDeepth 0x13
#else
#define _ColorDeepth 0x17
#endif

#endif

//============================================//
#ifdef _HDMI_Output_

u8	AVI_PB0	   = 0x00;
u8	AVI_PB1	   = 0x00;
u8	AVI_PB2	   = 0x00;
u8	HDMI_VIC = 0x10; // vic ,0x10: 1080P ;  0x04 : 720P ; Refer to the following list
/*************************************
   Resolution			HDMI_VIC
   --------------------------------------
   640x480				1
   720x480P 60Hz		2
   720x480i 60Hz			6
   720x576P 50Hz		17
   720x576i 50Hz			21
   1280x720P 24Hz		60
   1280x720P 25Hz		61
   1280x720P 30Hz		62
   1280x720P 50Hz		19
   1280x720P 60Hz		4
   1920x1080P 24Hz		32
   1920x1080P 25Hz		33
   1920x1080P 30Hz		34
   1920x1080i 50Hz		20
   1920x1080i 60Hz		5
   1920x1080P 50Hz		31
   1920x1080P 60Hz		16
   3840x2160 30Hz		95 // 4K30
   Other resolution	0(default)
 **************************************/
#endif

//============================================//

#define MIPI_Lane 4                         // 4 Lane MIPI input

//* 1920x1080  MIPI , pixel_clock  148.5MHz
#define MIPI_H_Active	1920
#define MIPI_V_Active	1080
#define MIPI_H_Total	2200
#define MIPI_V_Total	1125
#define MIPI_H_FrontPorch	88
#define MIPI_H_SyncWidth	44
#define MIPI_H_BackPorch	148
#define MIPI_V_FrontPorch	4
#define MIPI_V_SyncWidth	5
#define MIPI_V_BackPorch	36

void delay_1ms(__u32 t)
{
    __u32 timeout = t*HZ/1000;
    
    set_current_state(TASK_INTERRUPTIBLE);
    schedule_timeout(timeout);
}

static int lt8912b_i2c_read_data(u8 sla, u8 command, u8 *values,u8 length)
{
	uint8_t retry;
	int err;

	client_lt8912b->addr = sla;
	struct i2c_msg msgs[] = {
	 {
		 .addr = client_lt8912b->addr,
		 .flags = 0,
		 .len = 1,
		 .buf = &command,
	 },
	 {
		 .addr = client_lt8912b->addr,
		 .flags = I2C_M_RD,
		 .len = length,
		 .buf = values,
	 },
	};

	err = i2c_transfer(client_lt8912b->adapter, msgs, 2);

	return 0;
}


static unsigned char lt8912b_i2c_write_byte(u8 sla, u8 write_addr, u8 *reg_data,u8 data_len)
{
 	s32 ret = -1;
	s32 retries = 0;
	u8 *wdbuf;

	wdbuf = kzalloc(data_len + 1, GFP_KERNEL);
    if (!wdbuf)
        return 0;
	wdbuf[0] = write_addr;
	memcpy(wdbuf + 1, reg_data, data_len);
	client_lt8912b->addr = sla;
	
	struct i2c_msg msgs[] = {
		{
			.addr	= client_lt8912b->addr,
			.flags	= 0,
			.len	=  data_len + 1,
			.buf = wdbuf,
		},		
	};

    ret = i2c_transfer(client_lt8912b->adapter, msgs, 1);

	kfree(wdbuf);	
    return ret;
}

unsigned char ReadI2C_Byte(unsigned char I2CADR,unsigned char RegAddr)
{
	u8  p_data=0;
	if( lt8912b_i2c_read_data(I2CADR,RegAddr,&p_data,1) != 0)
	{
		return p_data;
	}
	return p_data;
}

unsigned char WriteI2C_Byte(u8 I2CADR, u8 RegAddr, u8 d)
{
	u8 flag;
	flag=lt8912b_i2c_write_byte(I2CADR, RegAddr,&d,1);
	return flag;
}

/***********************************************************

***********************************************************/
void MIPI_Digital( void )
{
	WriteI2C_Byte(TWO_I2C_ADDR,0x18, (u8)( MIPI_H_SyncWidth % 256 ) );     // hwidth
	WriteI2C_Byte(TWO_I2C_ADDR,0x19, (u8)( MIPI_V_SyncWidth % 256 ) );     // vwidth
	WriteI2C_Byte(TWO_I2C_ADDR,0x1c, (u8)( MIPI_H_Active % 256 ) );        // H_active[7:0]
	WriteI2C_Byte(TWO_I2C_ADDR,0x1d, (u8)( MIPI_H_Active / 256 ) );        // H_active[15:8]

	WriteI2C_Byte(TWO_I2C_ADDR,0x1e, 0x67);
	WriteI2C_Byte(TWO_I2C_ADDR,0x2f, 0x0c);

	WriteI2C_Byte(TWO_I2C_ADDR,0x34, (u8)( MIPI_H_Total % 256 ) );         // H_total[7:0]
	WriteI2C_Byte(TWO_I2C_ADDR,0x35, (u8)( MIPI_H_Total / 256 ) );         // H_total[15:8]

	WriteI2C_Byte(TWO_I2C_ADDR,0x36, (u8)( MIPI_V_Total % 256 ) );         // V_total[7:0]
	WriteI2C_Byte(TWO_I2C_ADDR,0x37, (u8)( MIPI_V_Total / 256 ) );         // V_total[15:8]

	WriteI2C_Byte(TWO_I2C_ADDR,0x38, (u8)( MIPI_V_BackPorch % 256 ) );     // VBP[7:0]
	WriteI2C_Byte(TWO_I2C_ADDR,0x39, (u8)( MIPI_V_BackPorch / 256 ) );     // VBP[15:8]
	WriteI2C_Byte(TWO_I2C_ADDR,0x3a, (u8)( MIPI_V_FrontPorch % 256 ) );    // VFP[7:0]
	WriteI2C_Byte(TWO_I2C_ADDR,0x3b, (u8)( MIPI_V_FrontPorch / 256 ) );    // VFP[15:8]
	WriteI2C_Byte(TWO_I2C_ADDR,0x3c, (u8)( MIPI_H_BackPorch % 256 ) );     // HBP[7:0]
	WriteI2C_Byte(TWO_I2C_ADDR,0x3d, (u8)( MIPI_H_BackPorch / 256 ) );     // HBP[15:8]
	WriteI2C_Byte(TWO_I2C_ADDR,0x3e, (u8)( MIPI_H_FrontPorch % 256 ) );    // HFP[7:0]
	WriteI2C_Byte(TWO_I2C_ADDR,0x3f, (u8)( MIPI_H_FrontPorch / 256 ) );    // HFP[15:8]
}
/***********************************************************

***********************************************************/
void DDS_Config( void )
{
	WriteI2C_Byte(TWO_I2C_ADDR,0x4e, 0x52 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x4f, 0xde );
	WriteI2C_Byte(TWO_I2C_ADDR,0x50, 0xc0 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x51, 0x80 );
	//WriteI2C_Byte(TWO_I2C_ADDR,0x51, 0x00 );

	WriteI2C_Byte(TWO_I2C_ADDR,0x1e, 0x4f );
	WriteI2C_Byte(TWO_I2C_ADDR,0x1f, 0x5e );
	WriteI2C_Byte(TWO_I2C_ADDR,0x20, 0x01 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x21, 0x2c );
	WriteI2C_Byte(TWO_I2C_ADDR,0x22, 0x01 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x23, 0xfa );
	WriteI2C_Byte(TWO_I2C_ADDR,0x24, 0x00 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x25, 0xc8 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x26, 0x00 );

	WriteI2C_Byte(TWO_I2C_ADDR,0x27, 0x5e );
	WriteI2C_Byte(TWO_I2C_ADDR,0x28, 0x01 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x29, 0x2c );
	WriteI2C_Byte(TWO_I2C_ADDR,0x2a, 0x01 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x2b, 0xfa );
	WriteI2C_Byte(TWO_I2C_ADDR,0x2c, 0x00 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x2d, 0xc8 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x2e, 0x00 );

	WriteI2C_Byte(TWO_I2C_ADDR,0x42, 0x64 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x43, 0x00 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x44, 0x04 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x45, 0x00 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x46, 0x59 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x47, 0x00 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x48, 0xf2 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x49, 0x06 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x4a, 0x00 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x4b, 0x72 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x4c, 0x45 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x4d, 0x00 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x52, 0x08 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x53, 0x00 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x54, 0xb2 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x55, 0x00 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x56, 0xe4 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x57, 0x0d );
	WriteI2C_Byte(TWO_I2C_ADDR,0x58, 0x00 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x59, 0xe4 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x5a, 0x8a );
	WriteI2C_Byte(TWO_I2C_ADDR,0x5b, 0x00 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x5c, 0x34 );

	WriteI2C_Byte(TWO_I2C_ADDR,0x51, 0x00 );
}

#ifdef _HDMI_Output_
/***********************************************************

***********************************************************/
void Audio_Config( void )
{
	// Audio config
	WriteI2C_Byte(ONE_I2C_ADDR,0xB2, 0x01 );   // 0x01:HDMI; 0x00: DVI
#if 1   // IIS input
//	AudioIIsEn(); // IIS Input
	WriteI2C_Byte(THR_I2C_ADDR,0x06, 0x08 );   // 0x09
	WriteI2C_Byte(THR_I2C_ADDR,0x07, 0xF0 );   // enable Audio: 0xF0;	Audio Mute: 0x00
	WriteI2C_Byte(THR_I2C_ADDR,0x09, 0x00 );   // 0x00:Left justified; //default 0x02:Right justified;
#else // SPDIF input
	//  AudioSPDIFEn(); // SPDIF Input
	WriteI2C_Byte(THR_I2C_ADDR,0x06, 0x0e );
	WriteI2C_Byte(THR_I2C_ADDR,0x07, 0x00 );
#endif
	WriteI2C_Byte(THR_I2C_ADDR,0x0f, 0x0b + Sample_Freq[_48KHz] );
	WriteI2C_Byte(THR_I2C_ADDR,0x37, (u8)( IIS_N[_48KHz] / 0x10000 ) );
	WriteI2C_Byte(THR_I2C_ADDR,0x36, (u8)( ( IIS_N[_48KHz] & 0x00FFFF ) / 0x100 ) );
	WriteI2C_Byte(THR_I2C_ADDR,0x35, (u8)( IIS_N[_48KHz] & 0x0000FF ) );
	WriteI2C_Byte(THR_I2C_ADDR,0x34, 0xD2 );   
//	WriteI2C_Byte(THR_I2C_ADDR,0x34, 0xE2 ); 
	WriteI2C_Byte(THR_I2C_ADDR,0x3c, 0x41 );   
}
/***********************************************************

***********************************************************/
void AVI_Config( void )
{
	WriteI2C_Byte(THR_I2C_ADDR,0x3e, 0x0A );
	//	HDMI_VIC = 0x04; // 720P 60; Corresponding to the resolution to be output
	HDMI_VIC = 0x10;                        // 1080P 60
	//	HDMI_VIC = 0x1F; // 1080P 50
	//	HDMI_VIC = 0x00; // If the resolution is non-standard, set to 0x00
	AVI_PB1 = 0x10;                         // PB1,color space: YUV444 0x70;YUV422 0x30; RGB 0x10
	AVI_PB2 = 0x2A;                         // PB2; picture aspect rate: 0x19:4:3 ;     0x2A : 16:9
	/********************************************************************************
	   The 0x43 register is checksums,
	   changing the value of the 0x45 or 0x47 register,
	   and the value of the 0x43 register is also changed.
	   0x43, 0x44, 0x45, and 0x47 are the sum of the four register values is 0x6F.
	 *********************************************************************************/
	AVI_PB0 = ( ( AVI_PB1 + AVI_PB2 + HDMI_VIC ) <= 0x6f ) ? ( 0x6f - AVI_PB1 - AVI_PB2 - HDMI_VIC ) : ( 0x16f - AVI_PB1 - AVI_PB2 - HDMI_VIC );
	WriteI2C_Byte(THR_I2C_ADDR,0x43, AVI_PB0 );    //avi packet checksum ,avi_pb0
	WriteI2C_Byte(THR_I2C_ADDR,0x44, AVI_PB1 );    //avi packet output RGB 0x10
	WriteI2C_Byte(THR_I2C_ADDR,0x45, AVI_PB2 );    //0x19:4:3 ; 0x2A : 16:9
	WriteI2C_Byte(THR_I2C_ADDR,0x47, HDMI_VIC );   //VIC(as below);1080P60 : 0x10
}

#endif

#ifdef _LVDS_Output_

#ifdef _Scaler_Mode_
/***********************************************************

***********************************************************/
void LVDS_Scaler_Config( void )
{
	union Temp	Core_PLL_Ratio;
	float		f_DIV;

	Core_PLL_Ratio.Temp32 = ( Panel_Pixel_CLK / 25 ) * 7;

	WriteI2C_Byte(ONE_I2C_ADDR,0x50, 0x24 );
	WriteI2C_Byte(ONE_I2C_ADDR,0x51, 0x05 );
	WriteI2C_Byte(ONE_I2C_ADDR,0x52, 0x14 );
	WriteI2C_Byte(ONE_I2C_ADDR,0x69, (u8)( ( Core_PLL_Ratio.Temp32 / 100 ) & 0x000000FF ) );
	WriteI2C_Byte(ONE_I2C_ADDR,0x69, 0x80 + (u8)( ( Core_PLL_Ratio.Temp32 / 100 ) & 0x000000FF ) );

	Core_PLL_Ratio.Temp32  = ( Core_PLL_Ratio.Temp32 % 100 ) & 0x000000FF;
	Core_PLL_Ratio.Temp32  = Core_PLL_Ratio.Temp32 * 16384;
	Core_PLL_Ratio.Temp32  = Core_PLL_Ratio.Temp32 / 100;
	//***********************************************************//

	//WriteI2C_Byte(ONE_I2C_ADDR,0x6c, 0x80 + Core_PLL_Ratio.Temp8[2] );	//leo.debug
	//WriteI2C_Byte(ONE_I2C_ADDR,0x6b, Core_PLL_Ratio.Temp8[3] );
	WriteI2C_Byte(ONE_I2C_ADDR,0x6c, 0xa4 );
    WriteI2C_Byte(ONE_I2C_ADDR,0x6b, 0x9b );


	//  LT8912B_I2C_Write_Byte(0x6c,0x80 + Core_PLL_Ratio.Temp8[1]);
	//  LT8912B_I2C_Write_Byte(0x6b,Core_PLL_Ratio.Temp8[0]);
	//***********************************************************//
	WriteI2C_Byte(ONE_I2C_ADDR,0x04, 0xfb ); //core pll reset
	WriteI2C_Byte(ONE_I2C_ADDR,0x04, 0xff );
	//------------------------------------------//

	//  void LVDS_Scale_Ratio(void)
	WriteI2C_Byte(ONE_I2C_ADDR,0x80, 0x00 );
	WriteI2C_Byte(ONE_I2C_ADDR,0x81, 0xff );
	WriteI2C_Byte(ONE_I2C_ADDR,0x82, 0x03 );
	WriteI2C_Byte(ONE_I2C_ADDR,0x83, (u8)( MIPI_H_Active % 256 ) );
	WriteI2C_Byte(ONE_I2C_ADDR,0x84, (u8)( MIPI_H_Active / 256 ) );
	WriteI2C_Byte(ONE_I2C_ADDR,0x85, 0x80 );
	WriteI2C_Byte(ONE_I2C_ADDR,0x86, 0x10 );
	WriteI2C_Byte(ONE_I2C_ADDR,0x87, (u8)( Panel_H_Total % 256 ) );
	WriteI2C_Byte(ONE_I2C_ADDR,0x88, (u8)( Panel_H_Total / 256 ) );
	WriteI2C_Byte(ONE_I2C_ADDR,0x89, (u8)( Panel_H_SyncWidth % 256 ) );
	WriteI2C_Byte(ONE_I2C_ADDR,0x8a, (u8)( Panel_H_BackPorch % 256 ) );
	WriteI2C_Byte(ONE_I2C_ADDR,0x8b, ( ( (u8)( Panel_H_BackPorch/ 256 ) ) & 0x01 ) * 0x80 + ( (u8)( Panel_V_SyncWidth % 256 ) ) & 0x0f );
	WriteI2C_Byte(ONE_I2C_ADDR,0x8c, (u8)( Panel_H_Active % 256 ) );
	WriteI2C_Byte(ONE_I2C_ADDR,0x8d, (u8)( Panel_V_Active % 256 ) );
	WriteI2C_Byte(ONE_I2C_ADDR,0x8e, ( (u8)( Panel_V_Active / 256 ) ) * 0x10 + (u8)( Panel_H_Active / 256 ) );

	f_DIV				   = ( ( (float)( MIPI_H_Active - 1 ) ) / (float)( Panel_H_Active - 1 ) ) * 4096;
	Core_PLL_Ratio.Temp32  = (u32)f_DIV;
	//***********************************************************//

	//WriteI2C_Byte(ONE_I2C_ADDR,0x8f, Core_PLL_Ratio.Temp8[3] );
	//WriteI2C_Byte(ONE_I2C_ADDR,0x90, Core_PLL_Ratio.Temp8[2] );


	//WriteI2C_Byte(ONE_I2C_ADDR,0x8f,Core_PLL_Ratio.Temp8[1]);	//leo.debug
	//WriteI2C_Byte(ONE_I2C_ADDR,0x90,Core_PLL_Ratio.Temp8[0]);
	WriteI2C_Byte(ONE_I2C_ADDR,0x8f, 0x02 );   //leo.debug
    WriteI2C_Byte(ONE_I2C_ADDR,0x90, 0x18 );
	//***********************************************************//

	f_DIV				   = ( ( (float)( MIPI_V_Active - 1 ) ) / (float)( Panel_V_Active - 1 ) ) * 4096;
	Core_PLL_Ratio.Temp32  = (u32)f_DIV;
	//***********************************************************//

	//WriteI2C_Byte(ONE_I2C_ADDR,0x91, Core_PLL_Ratio.Temp8[3] );
	//WriteI2C_Byte(ONE_I2C_ADDR,0x92, Core_PLL_Ratio.Temp8[2] );


	//WriteI2C_Byte(ONE_I2C_ADDR,0x91,Core_PLL_Ratio.Temp8[1]);   //leo.debug
	//WriteI2C_Byte(ONE_I2C_ADDR,0x92,Core_PLL_Ratio.Temp8[0]);
	WriteI2C_Byte(ONE_I2C_ADDR,0x91, 0x9b );	//leo.debug
    WriteI2C_Byte(ONE_I2C_ADDR,0x92, 0x15 );
	//***********************************************************//
	WriteI2C_Byte(ONE_I2C_ADDR,0x7f, 0x9c );
	WriteI2C_Byte(ONE_I2C_ADDR,0xa8, _VesaJeidaMode + _DE_Sync_mode + _ColorDeepth );
	delay_1ms(300);
}

#endif

#ifdef _Bypass_Mode_
/***********************************************************

***********************************************************/
void LVDS_Bypass_Config( void )
{
	//core pll bypass
	WriteI2C_Byte(ONE_I2C_ADDR,0x50, 0x24 );   //cp=50uA
	WriteI2C_Byte(ONE_I2C_ADDR,0x51, 0x2d );   //Pix_clk as reference,second order passive LPF PLL
	WriteI2C_Byte(ONE_I2C_ADDR,0x52, 0x04 );   //loopdiv=0;use second-order PLL

	//PLL CLK
	WriteI2C_Byte(ONE_I2C_ADDR,0x69, 0x0e );
	WriteI2C_Byte(ONE_I2C_ADDR,0x69, 0x8e );
	WriteI2C_Byte(ONE_I2C_ADDR,0x6a, 0x00 );
	WriteI2C_Byte(ONE_I2C_ADDR,0x6c, 0xb8 );
	WriteI2C_Byte(ONE_I2C_ADDR,0x6b, 0x51 );

	WriteI2C_Byte(ONE_I2C_ADDR,0x04, 0xfb ); //core pll reset
	WriteI2C_Byte(ONE_I2C_ADDR,0x04, 0xff );

	//scaler bypass
	WriteI2C_Byte(ONE_I2C_ADDR,0x7f, 0x00 );
	WriteI2C_Byte(ONE_I2C_ADDR,0xa8, _VesaJeidaMode + _DE_Sync_mode + _ColorDeepth );
	delay_1ms( 100 );
	WriteI2C_Byte(ONE_I2C_ADDR,0x02, 0xf7 );   //lvds pll reset
	WriteI2C_Byte(ONE_I2C_ADDR,0x02, 0xff );
	WriteI2C_Byte(ONE_I2C_ADDR,0x03, 0xcb );   //scaler module reset
	WriteI2C_Byte(ONE_I2C_ADDR,0x03, 0xfb );   //lvds tx module reset
	WriteI2C_Byte(ONE_I2C_ADDR,0x03, 0xff );
}

#endif
#endif

void lt8912b_RSTN(void)
{
    gpio_request(LT8912B_RESET_GPIO,"reset_pin");
    gpio_export(LT8912B_RESET_GPIO,true);
	gpio_direction_output(LT8912B_RESET_GPIO, 0);
	delay_1ms(100);
	gpio_direction_output(LT8912B_RESET_GPIO, 1);
	delay_1ms(100);
}

void Read_LT8912B_ID( void )
{
	u8 Temp_ID0, Temp_ID1;
	
	Temp_ID0   = ReadI2C_Byte( ONE_I2C_ADDR, 0x00 ); 
	Temp_ID1   = ReadI2C_Byte( ONE_I2C_ADDR, 0x01 ); 
	printk( "\r\nLT8912B ID: Temp_ID0=0x%x,Temp_ID1=0x%x.\n",Temp_ID0,Temp_ID1);
}

void GCMWindow1080P( void )
{
	WriteI2C_Byte(TWO_I2C_ADDR,0x72, 0x12 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x73, 0xc0 );   //RGD_PTN_DE_DLY[7:0]
	WriteI2C_Byte(TWO_I2C_ADDR,0x74, 0x00 );   //RGD_PTN_DE_DLY[11:8]  192
	WriteI2C_Byte(TWO_I2C_ADDR,0x75, 0x29 );   //RGD_PTN_DE_TOP[6:0]  41
	WriteI2C_Byte(TWO_I2C_ADDR,0x76, 0x80 );   //RGD_PTN_DE_CNT[7:0]
	WriteI2C_Byte(TWO_I2C_ADDR,0x77, 0x38 );   //RGD_PTN_DE_LIN[7:0]
	WriteI2C_Byte(TWO_I2C_ADDR,0x78, 0x47 );   //RGD_PTN_DE_LIN[10:8],RGD_PTN_DE_CNT[11:8]
	WriteI2C_Byte(TWO_I2C_ADDR,0x79, 0x98 );   //RGD_PTN_H_TOTAL[7:0]
	WriteI2C_Byte(TWO_I2C_ADDR,0x7a, 0x65 );   //RGD_PTN_V_TOTAL[7:0]
	WriteI2C_Byte(TWO_I2C_ADDR,0x7b, 0x48 );   //RGD_PTN_V_TOTAL[10:8],RGD_PTN_H_TOTAL[11:8]
	WriteI2C_Byte(TWO_I2C_ADDR,0x7c, 0x2c );   //RGD_PTN_HWIDTH[7:0]
	WriteI2C_Byte(TWO_I2C_ADDR,0x7d, 0x05 );   //RGD_PTN_HWIDTH[9:8],RGD_PTN_VWIDTH[5:0]

	WriteI2C_Byte(TWO_I2C_ADDR,0x70, 0x80 );   // pattern en
	WriteI2C_Byte(TWO_I2C_ADDR,0x71, 0x76 );

	WriteI2C_Byte(TWO_I2C_ADDR,0x4e, 0x33 );   ////strm_sw_freq_word[ 7: 0]
	WriteI2C_Byte(TWO_I2C_ADDR,0x4f, 0x33 );   ////strm_sw_freq_word[15: 8]
	WriteI2C_Byte(TWO_I2C_ADDR,0x50, 0xD3 );   ////strm_sw_freq_word[23:16]
	WriteI2C_Byte(TWO_I2C_ADDR,0x51, 0x80 );   //
}

void InitLT8912B(void)
{
	Read_LT8912B_ID( );
 #ifdef _LVDS_Output_
	WriteI2C_Byte(ONE_I2C_ADDR,0x08, 0xff );   // Register address : 0x08;	 Value : 0xff
	WriteI2C_Byte(ONE_I2C_ADDR,0x09, 0xff );
	WriteI2C_Byte(ONE_I2C_ADDR,0x0a, 0xff );
	WriteI2C_Byte(ONE_I2C_ADDR,0x0b, 0x7c );   //
	WriteI2C_Byte(ONE_I2C_ADDR,0x0c, 0xff );
	WriteI2C_Byte(ONE_I2C_ADDR,0x51, 0x15 );
#else
	WriteI2C_Byte(ONE_I2C_ADDR,0x08, 0xff );   // Register address : 0x08;	 Value : 0xff
	WriteI2C_Byte(ONE_I2C_ADDR,0x09, 0x81 );
	WriteI2C_Byte(ONE_I2C_ADDR,0x0a, 0xff );
	WriteI2C_Byte(ONE_I2C_ADDR,0x0b, 0x64 );   //
	WriteI2C_Byte(ONE_I2C_ADDR,0x0c, 0xff );
	WriteI2C_Byte(ONE_I2C_ADDR,0x44, 0x31 );   // Close LVDS ouput
	WriteI2C_Byte(ONE_I2C_ADDR,0x51, 0x1f );
#endif
	//  TxAnalog();
	WriteI2C_Byte(ONE_I2C_ADDR,0x31, 0xa1 );
	WriteI2C_Byte(ONE_I2C_ADDR,0x32, 0xbf );
	WriteI2C_Byte(ONE_I2C_ADDR,0x33, 0x17 ); // bit0/bit1 =1 Turn On HDMI Tx��  bit0/bit1 = 0 Turn Off HDMI Tx
	WriteI2C_Byte(ONE_I2C_ADDR,0x37, 0x00 );
	WriteI2C_Byte(ONE_I2C_ADDR,0x38, 0x22 );
	WriteI2C_Byte(ONE_I2C_ADDR,0x60, 0x82 );
	//  CbusAnalog();
	WriteI2C_Byte(ONE_I2C_ADDR,0x39, 0x45 );
	WriteI2C_Byte(ONE_I2C_ADDR,0x3a, 0x00 );
	WriteI2C_Byte(ONE_I2C_ADDR,0x3b, 0x00 );
	// MIPIAnalog()
	WriteI2C_Byte(ONE_I2C_ADDR,0x3e, 0xc6 );   //
	WriteI2C_Byte(ONE_I2C_ADDR,0x41, 0x7c );   //HS_eq current
	/* EQ Seeting
	   WriteI2C_Byte(ONE_I2C_ADDR,0x3f, 0x14 );// 54
	   WriteI2C_Byte(ONE_I2C_ADDR,0x41, 0x3c );// bc
	   WriteI2C_Byte(ONE_I2C_ADDR,0x40, 0x7c );// 7d
	*/
	//  HDMIPllAnalog();
	WriteI2C_Byte(ONE_I2C_ADDR,0x44, 0x31 );
	WriteI2C_Byte(ONE_I2C_ADDR,0x55, 0x44 );
	WriteI2C_Byte(ONE_I2C_ADDR,0x57, 0x01 );
	WriteI2C_Byte(ONE_I2C_ADDR,0x5a, 0x02 );
	//  MipiBasicSet();
	WriteI2C_Byte(TWO_I2C_ADDR,0x10, 0x01 );               // 0x05
	WriteI2C_Byte(TWO_I2C_ADDR,0x11, 0x08 );               // 0x12
	WriteI2C_Byte(TWO_I2C_ADDR,0x12, 0x04 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x13, MIPI_Lane % 0x04 );   // 00 4 lane  // 01 lane // 02 2lane                                            //03 3 lane
	WriteI2C_Byte(TWO_I2C_ADDR,0x14, 0x00 );
	//WriteI2C_Byte(TWO_I2C_ADDR,0x15, 0xa8 );	//3210
	WriteI2C_Byte(TWO_I2C_ADDR,0x15, 0x00 );	//0123
	WriteI2C_Byte(TWO_I2C_ADDR,0x1a, 0x03 );
	WriteI2C_Byte(TWO_I2C_ADDR,0x1b, 0x03 );


	MIPI_Digital( );
	//  DDS Config();
	DDS_Config( );
#ifdef _DVI_Output_
	WriteI2C_Byte(ONE_I2C_ADDR,0xB2, 0x00 ); // 0x01:HDMI; 0x00: DVI
#endif

#ifdef _HDMI_Output_
	Audio_Config( );
	AVI_Config( ); // AVI: Auxiliary Video Information
#endif

	//  MIPIRxLogicRes();
	WriteI2C_Byte(TWO_I2C_ADDR,0x03, 0x7f );       // mipi rx reset
	delay_1ms(10);
	WriteI2C_Byte(TWO_I2C_ADDR,0x03, 0xff );

	WriteI2C_Byte(TWO_I2C_ADDR,0x05, 0xfb );       // DDS reset
	delay_1ms(10);
	WriteI2C_Byte(TWO_I2C_ADDR,0x05, 0xff );

//	WriteI2C_Byte(TWO_I2C_ADDR,0x51, 0x80 );
//	delay_1ms( 10 );
//	WriteI2C_Byte(TWO_I2C_ADDR,0x51, 0x00 );
	//------------------------------------------//
	//GCMWindow1080P( );

#ifdef _LVDS_Output_
#ifdef _Scaler_Mode_
	LVDS_Scaler_Config( );
#endif
#ifdef _Bypass_Mode_
	LVDS_Bypass_Config( );
#endif
	//  void LvdsPowerUp(void)
	WriteI2C_Byte(ONE_I2C_ADDR,0x44, 0x30 ); // Turn on LVDS output
#endif
	delay_1ms(1000);
}

static int lt8912b_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	int ret=0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk("Need I2C_FUNC_I2C\n");
		return -1;
	}	
	client_lt8912b = client;
            
	lt8912b_RSTN();			
	InitLT8912B();
	return ret;
}

static int lt8912b_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id lt8912b_i2c_id[] = {
	{ LT8912B_I2C_NAME, 0 },
	{ }
};

static const struct of_device_id lt8912b_dt_match[] = {
	{ .compatible = "lt,lt8912b", },
	{ },
};

MODULE_DEVICE_TABLE(i2c, lt8912b_i2c_id);

static struct i2c_driver lt8912b_i2c_driver = {
	.driver = {
		.name	= LT8912B_I2C_NAME,	
		.of_match_table = lt8912b_dt_match,
	},
	.probe		= lt8912b_probe,
	.remove		= lt8912b_remove,
	.id_table	= lt8912b_i2c_id,
};



static int __init lt8912b_driver_init(void)
{
	int ret = 0;
	pr_info("%s entered.\n", __func__);

	ret = i2c_add_driver(&lt8912b_i2c_driver);
	if(ret)
		pr_info("lt8912b_driver init failed");
	
	return ret;
}


static void __exit lt8912b_driver_exit(void)
{
	pr_info("%s exited.\n", __func__);
	i2c_del_driver(&lt8912b_i2c_driver);
}

module_init(lt8912b_driver_init);
module_exit(lt8912b_driver_exit);

MODULE_AUTHOR("leo.feng <leo.feng@citybrandhk.com>");
MODULE_DESCRIPTION("lt8912b I2C driver, set mipi transform to edp");
MODULE_LICENSE("GPL v2");
