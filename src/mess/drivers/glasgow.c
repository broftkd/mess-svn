/***************************************************************************
Mephisto Glasgow 3 S chess computer
Dirk V.
sp_rinter@gmx.de

68000 CPU
64 KB ROM
16 KB RAM
4 Digit LC Display

3* 74LS138  Decoder/Multiplexer
1*74LS74    Dual positive edge triggered D Flip Flop
1*74LS139 1of4 Demultiplexer
1*74LS05    HexInverter
1*NE555     R=100K C=10uF
2*74LS04  Hex Inverter
1*74LS164   8 Bit Shift register
1*74121 Monostable Multivibrator with Schmitt Trigger Inputs
1*74LS20 Dual 4 Input NAND GAte
1*74LS367 3 State Hex Buffers


***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68k.h"
#include "cpu/m68000/m68000.h"
#include "glasgow.lh"
#include "sound/beep.h"

static UINT8 lcd_shift_counter;
// static UINT8 led_status;
static UINT8 led7;
static UINT8 key_low,
       key_hi,
       key_select;

static WRITE16_HANDLER ( write_lcd )
{
  UINT8 lcd_data;
  lcd_data = data>>8;
  if (led7==0) output_set_digit_value(lcd_shift_counter,lcd_data);
  lcd_shift_counter--;
  lcd_shift_counter&=3;
  logerror("LCD Offset = %d Data low  = %x \n  ",offset,lcd_data);

}

static WRITE32_HANDLER ( write_lcd32 )
{
  UINT8 lcd_data;
  lcd_data = data>>8;
  if (led7==0) output_set_digit_value(lcd_shift_counter,lcd_data);
  lcd_shift_counter--;
  lcd_shift_counter&=3;
  logerror("LCD Offset = %d Data   = %x \n  ",offset,lcd_data);

}

static WRITE16_HANDLER ( write_lcd_flag )
{
  UINT8 lcd_flag;
  lcd_flag=data>>8;
  beep_set_state(0,lcd_flag&1?1:0);
  if (lcd_flag!=0) led7=255;else led7=0;
  logerror("LCD Flag   = %x \n  ",lcd_flag);
}

static WRITE32_HANDLER ( write_lcd_flag32 )
{
  UINT8 lcd_flag;
  lcd_flag=data>>24;
  logerror("LCD Flag   = %x \n  ",lcd_flag);
  //beep_set_state(0,lcd_flag&1?1:0);
  if (lcd_flag!=0) led7=255;else led7=0;

}

static WRITE16_HANDLER ( write_keys )
{
 key_select=data>>8;
 logerror("Write Key   = %x \n  ",data);
}

static WRITE32_HANDLER ( write_keys32 )
{
 key_select=data>>24;
 logerror("Write Key   = %x \n  ",key_select);
}


static WRITE16_HANDLER ( write_beeper )
{
 logerror("Write Beeper   = %x \n  ",data);
}

static READ16_HANDLER(read_keys)
{
  UINT16 data;
  data=0x0300;
  key_low=readinputport(0x00);
  key_hi=readinputport(0x01);
  logerror("Keyboard Offset = %x Data = %x\n  ",offset,data);
  if (key_select==key_low){data=data&0x100;}
  if (key_select==key_hi){data=data&0x200;}
   return data;

}

static READ32_HANDLER(read_keys32)
{
  UINT32 data;
  data=0x00000000;
  key_low=readinputport(0x00);
  key_hi=readinputport(0x01);
  // logerror("read Keyboard Offset = %x Data = %x\n  ",offset,data);
  //if (key_low==0xfe){data=data | 0x11000000;}
  // if (key_select==key_hi){ data=data & 0x22000000;}
  if (key_low==0xfe){ data=data | 0xff000000;}
  if (key_low==0xfd){ data=data | 0x2000000;}
  if (key_low==0xfb){ data=data | 0x4000000;}
  if (key_low==0xf7){ data=data | 0x8000000;}
  logerror("read Keyboard Offset = %x Data = %x\n  ",offset,data);
  return data ;
}

static READ16_HANDLER(read_board)
{
  return 0xff00;	// Mephisto need it for working
}

static READ32_HANDLER(read_board32)
{
  return 0xff000000;	// Mephisto need it for working
}

static WRITE16_HANDLER(write_board)
{
  logerror("Write Board   = %x \n  ",data>>8);
}

static WRITE32_HANDLER(write_board32)
{
  logerror("Write Board   = %x \n  ",data);
}

static TIMER_CALLBACK( update_nmi )
{
	cpunum_set_input_line_and_vector(0,  MC68000_IRQ_7, PULSE_LINE, MC68000_INT_ACK_AUTOVECTOR);
}

static TIMER_CALLBACK( update_nmi32 )
{
	cpunum_set_input_line_and_vector(0,  MC68020_IRQ_7, PULSE_LINE, MC68020_INT_ACK_AUTOVECTOR);
}

static MACHINE_START( glasgow )
{
  lcd_shift_counter=3;
	mame_timer_pulse(MAME_TIME_IN_HZ(60), 0, update_nmi);
  beep_set_frequency(0, 44);
}


static MACHINE_START( dallas32 )
{
  lcd_shift_counter=3;
	mame_timer_pulse(MAME_TIME_IN_HZ(60), 0, update_nmi32);
  beep_set_frequency(0, 44);
}

static MACHINE_RESET( glasgow )
{
	lcd_shift_counter=3;
}



static VIDEO_START( glasgow )
{
}

static VIDEO_UPDATE( glasgow )
{
    return 0;
}


static ADDRESS_MAP_START(glasgow_mem, ADDRESS_SPACE_PROGRAM, 16)
    AM_RANGE( 0x0000, 0xffff ) AM_ROM
    AM_RANGE( 0x00ff0000, 0x00ff0001)  AM_MIRROR(0xfe0000) AM_WRITE( write_lcd)
    AM_RANGE( 0x00ff0002, 0x00ff0003)  AM_MIRROR(0xfe0002) AM_READWRITE( read_keys,write_keys)
    AM_RANGE( 0x00ff0004, 0x00ff0005)  AM_MIRROR(0xfe0004) AM_WRITE( write_lcd_flag)
    AM_RANGE( 0x00ff0006, 0x00ff0007)  AM_MIRROR(0xfe0006) AM_READWRITE( read_board, write_board)
    AM_RANGE( 0x00ff0008, 0x00ff0009)  AM_MIRROR(0xfe0008) AM_WRITE( write_beeper)
    AM_RANGE( 0x00ffC000, 0x00ffFFFF)  AM_MIRROR(0xfeC000) AM_RAM      // 16KB
ADDRESS_MAP_END


static ADDRESS_MAP_START(amsterd_mem, ADDRESS_SPACE_PROGRAM, 16)
   // ADDRESS_MAP_FLAGS( AMEF_ABITS(19) )
    AM_RANGE( 0x0000, 0xffff ) AM_ROM

    AM_RANGE( 0x00800002, 0x00800003)  AM_WRITE( write_lcd)
    AM_RANGE( 0x00800008, 0x00800009)  AM_WRITE( write_lcd_flag)
    AM_RANGE( 0x00800010, 0x00800021)  AM_WRITE( write_keys)
    AM_RANGE( 0x00800020, 0x00800021)  AM_READ ( NULL)  // board
    AM_RANGE( 0x00800040, 0x00800041)  AM_READ( NULL)   // key
    AM_RANGE( 0x00800088, 0x00800089)  AM_WRITE( NULL)

    AM_RANGE( 0x00ffC000, 0x00ffFFFF)  AM_RAM      // 16KB
ADDRESS_MAP_END


static ADDRESS_MAP_START(dallas32_mem, ADDRESS_SPACE_PROGRAM, 32)
     //ADDRESS_MAP_FLAGS( AMEF_ABITS(17) )
     AM_RANGE( 0x0000, 0xffff ) AM_ROM
     AM_RANGE( 0x00800000, 0x00800003)AM_WRITE( write_lcd32)
     AM_RANGE( 0x00800008, 0x0080000B)AM_WRITE(write_lcd_flag32)
     AM_RANGE( 0x00800010, 0x00800013)AM_WRITE(write_keys32)  ///
     AM_RANGE( 0x00800020, 0x00800023)AM_READ(read_keys32 ) ///
     AM_RANGE( 0x00800040, 0x00800043)AM_READ( read_board32)  // board
      AM_RANGE( 0x00800088, 0x0080008b)AM_WRITE( write_board32) /// 
     AM_RANGE( 0x0010000, 0x001FFFF)   AM_RAM      // 64KB
ADDRESS_MAP_END



INPUT_PORTS_START( glasgow )
  PORT_START
    PORT_BIT(0x001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
    PORT_BIT(0x002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CL") PORT_CODE(KEYCODE_F5)
    PORT_BIT(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C3") PORT_CODE(KEYCODE_C)
    PORT_BIT(0x004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C3") PORT_CODE(KEYCODE_3)
    PORT_BIT(0x008, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENT") PORT_CODE(KEYCODE_ENTER)
    PORT_BIT(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D4") PORT_CODE(KEYCODE_D) //
    PORT_BIT(0x010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D4") PORT_CODE(KEYCODE_4) //
    PORT_BIT(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A1") PORT_CODE(KEYCODE_A) //
    PORT_BIT(0x020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A1") PORT_CODE(KEYCODE_1) //
    PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F6") PORT_CODE(KEYCODE_F) //
    PORT_BIT(0x040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F6") PORT_CODE(KEYCODE_6) //
    PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B2") PORT_CODE(KEYCODE_B)//
    PORT_BIT(0x080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B2") PORT_CODE(KEYCODE_2)//
  PORT_START
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E5") PORT_CODE(KEYCODE_E) //
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E5") PORT_CODE(KEYCODE_5) //
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("INFO") PORT_CODE(KEYCODE_F1) //
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("POS") PORT_CODE(KEYCODE_F2) //
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H8") PORT_CODE(KEYCODE_H) //
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H8") PORT_CODE(KEYCODE_8) //
    PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LEV") PORT_CODE(KEYCODE_F3) //
    PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G7") PORT_CODE(KEYCODE_G) //
    PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G7") PORT_CODE(KEYCODE_7) //
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MEM") PORT_CODE(KEYCODE_F4) //
INPUT_PORTS_END



static MACHINE_DRIVER_START(glasgow )
    /* basic machine hardware */
    MDRV_CPU_ADD_TAG("main", M68000, 20000000)
	  MDRV_CPU_PROGRAM_MAP(glasgow_mem, 0)
	  MDRV_MACHINE_START( glasgow )
	  MDRV_MACHINE_RESET( glasgow )
    /* video hardware */
    MDRV_SCREEN_REFRESH_RATE(60)
    MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)
    MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 639, 0, 479)
    MDRV_PALETTE_LENGTH(4)
    MDRV_DEFAULT_LAYOUT(layout_glasgow)
    MDRV_SPEAKER_STANDARD_MONO("mono")
	  MDRV_SOUND_ADD(BEEP, 0)
	  MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
    MDRV_VIDEO_START(glasgow)
    MDRV_VIDEO_UPDATE(glasgow)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START(amsterd )
    /* basic machine hardware */
    MDRV_CPU_ADD_TAG("main", M68000, 20000000)
	  MDRV_CPU_PROGRAM_MAP(amsterd_mem, 0)
	  MDRV_MACHINE_START( glasgow )
	  MDRV_MACHINE_RESET( glasgow )
    /* video hardware */
    MDRV_SCREEN_REFRESH_RATE(60)
    MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)
    MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 639, 0, 479)
    MDRV_PALETTE_LENGTH(4)
    MDRV_DEFAULT_LAYOUT(layout_glasgow)
    MDRV_SPEAKER_STANDARD_MONO("mono")
	  MDRV_SOUND_ADD(BEEP, 0)
	  MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
    MDRV_VIDEO_START(glasgow)
    MDRV_VIDEO_UPDATE(glasgow)
MACHINE_DRIVER_END



static MACHINE_DRIVER_START(dallas32 )
    /* basic machine hardware */
    MDRV_CPU_ADD_TAG("main", M68020, 14000000)
	  MDRV_CPU_PROGRAM_MAP(dallas32_mem, 0)
	  MDRV_MACHINE_START( dallas32 )
	  MDRV_MACHINE_RESET( glasgow )
    /* video hardware */
    MDRV_SCREEN_REFRESH_RATE(60)
    MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)
    MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 639, 0, 479)
    MDRV_PALETTE_LENGTH(4)
    MDRV_DEFAULT_LAYOUT(layout_glasgow)
    MDRV_SPEAKER_STANDARD_MONO("mono")
	  MDRV_SOUND_ADD(BEEP, 0)
	  MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
    MDRV_VIDEO_START(glasgow)
    MDRV_VIDEO_UPDATE(glasgow)
MACHINE_DRIVER_END


/***************************************************************************
  ROM definitions
***************************************************************************/

ROM_START( glasgow )
    ROM_REGION16_BE( 0x1000000, REGION_CPU1, 0 )
    //ROM_LOAD("glasgow.rom", 0x000000, 0x10000, CRC(3e73eff3) )
    ROM_LOAD16_BYTE("me3_3_1u.410",0x00000, 0x04000,CRC(bc8053ba) SHA1(57ea2d5652bfdd77b17d52ab1914de974bd6be12))
    ROM_LOAD16_BYTE("me3_1_1l.410",0x00001, 0x04000,CRC(d5263c39) SHA1(1bef1cf3fd96221eb19faecb6ec921e26ac10ac4))
    ROM_LOAD16_BYTE("me3_4_2u.410",0x08000, 0x04000,CRC(8dba504a) SHA1(6bfab03af835cdb6c98773164d32c76520937efe))
    ROM_LOAD16_BYTE("me3_2_2l.410",0x08001, 0x04000,CRC(b3f27827) SHA1(864ba897d24024592d08c4ae090aa70a2cc5f213))

ROM_END

ROM_START( amsterd )
    ROM_REGION16_BE( 0x1000000, REGION_CPU1, 0 )
    //ROM_LOAD16_BYTE("output.bin", 0x000000, 0x10000, CRC(3e73eff3) )
    ROM_LOAD16_BYTE("amsterda-u.bin",0x00000, 0x05a00,CRC(16cefe29) SHA1(9f8c2896e92fbfd47159a59cb5e87706092c86f4))
    ROM_LOAD16_BYTE("amsterda-l.bin",0x00001, 0x05a00,CRC(c859dfde) SHA1(b0bca6a8e698c322a8c597608db6735129d6cdf0))
ROM_END



ROM_START( dallas )
    ROM_REGION16_BE( 0x1000000, REGION_CPU1, 0 )
    //ROM_LOAD("glasgow.rom", 0x000000, 0x10000, CRC(3e73eff3) )
    ROM_LOAD16_BYTE("dal_g_pr.dat",0x00000, 0x04000,CRC(66deade9) SHA1(07ec6b923f2f053172737f1fc94aec84f3ea8da1))
    ROM_LOAD16_BYTE("dal_g_pl.dat",0x00001, 0x04000,CRC(c5b6171c) SHA1(663167a3839ed7508ecb44fd5a1b2d3d8e466763))
    ROM_LOAD16_BYTE("dal_g_br.dat",0x08000, 0x04000,CRC(e24d7ec7) SHA1(a936f6fcbe9bfa49bf455f2d8a8243d1395768c1))
    ROM_LOAD16_BYTE("dal_g_bl.dat",0x08001, 0x04000,CRC(144a15e2) SHA1(c4fcc23d55fa5262f5e01dbd000644a7feb78f32))
ROM_END

ROM_START( roma )
    ROM_REGION16_BE( 0x1000000, REGION_CPU1, 0 )
    ROM_LOAD("roma32.bin", 0x000000, 0x10000, CRC(587d03bf) SHA1(504e9ff958084700076d633f9c306fc7baf64ffd))
ROM_END

ROM_START( dallas32 )
    ROM_REGION16_BE( 0x1000000, REGION_CPU1, 0 )
    ROM_LOAD("dallas32.epr", 0x000000, 0x10000, CRC(83b9ff3f) SHA1(97bf4cb3c61f8ec328735b3c98281bba44b30a28) )
ROM_END
/***************************************************************************
  System config
***************************************************************************/



/***************************************************************************
  Game drivers
***************************************************************************/

/*     YEAR,NAME,PARENT,BIOS,COMPAT,MACHINE,INPUT,INIT,CONFIG,COMPANY,FULLNAME,FLAGS */
CONS(  1984, glasgow,   0,                0,       glasgow,  glasgow,   NULL,     NULL,   "Hegener & Glaser Muenchen",  "Mephisto III S Glasgow", 0)
CONS(  1984, amsterd,  0,               0,       amsterd,  glasgow,   NULL,     NULL,   "Hegener & Glaser Muenchen",  "Mephisto Amsterdam", 0)
CONS(  1984, dallas,   0,                0,       glasgow,  glasgow,   NULL,     NULL,   "Hegener & Glaser Muenchen",  "Mephisto Dallas", 0)
CONS(  1984, roma,  0,               0,       glasgow,  glasgow,   NULL,     NULL,   "Hegener & Glaser Muenchen",  "Mephisto Roma", 0)
CONS(  1984, dallas32,  0,               0,       dallas32,  glasgow,   NULL,     NULL,   "Hegener & Glaser Muenchen",  "Mephisto Dallas 32 Bit", 0)

