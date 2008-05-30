/******************************************************************************
 *  Sharp MZ700
 *
 *  system driver
 *
 *  Juergen Buchmueller <pullmoll@t-online.de>, Jul 2000
 *
 *  Reference: http://sharpmz.computingmuseum.com
 *
 *  MZ700 memory map
 *
 *  0000-0FFF   1Z-013A ROM or RAM
 *  1000-CFFF   RAM
 *  D000-D7FF   videoram or RAM
 *  D800-DFFF   colorram or RAM
 *  E000-FFFF   memory mapped IO or RAM
 *
 *      xxx0    PPI8255 port A (output)
 *              bit 7   556RST (reset NE556)
 *              bit 6-4 unused
 *              bit 3-0 keyboard row demux (LS145)
 *
 *      xxx1    PPI8255 port B (input)
 *              bit 7-0 keyboard matrix code
 *
 *      xxx2    PPI8255 port C (input/output)
 *              bit 7 R -VBLANK input
 *              bit 6 R 556OUT (1.5Hz)
 *              bit 5 R RDATA from cassette
 *              bit 4 R MOTOR from cassette
 *              bit 3 W M-ON control
 *              bit 2 W INTMASK 1=enable 0=disabel clock interrupt
 *              bit 1 W WDATA to cassette
 *              bit 0 W unused
 *
 *      xxx3    PPI8255 control
 *
 *      xxx4    PIT8253 timer 0 (clock input 1,108800 MHz)
 *      xxx5    PIT8253 timer 1 (clock input 15,611 kHz)
 *      xxx6    PIT8253 timer 2 (clock input OUT1 1Hz (default))
 *      xxx7    PIT8253 control/status
 *
 *      xxx8    bit 7 R -HBLANK
 *              bit 6 R unused
 *              bit 5 R unused
 *              bit 4 R joystick JB2
 *              bit 3 R joystick JB1
 *              bit 2 R joystick JA2
 *              bit 1 R joystick JA1
 *              bit 0 R NE556 OUT (32Hz IC BJ)
 *                    W gate0 of PIT8253 (sound enable)
 *
 *  MZ800 memory map
 *
 *  0000-0FFF   ROM or RAM
 *  1000-1FFF   PCG ROM or RAM
 *  2000-7FFF   RAM
 *  8000-9FFF   videoram or RAM
 *  A000-BFFF   videoram or RAM
 *  C000-CFFF   PCG RAM or RAM
 *  D000-DFFF   videoram or RAM
 *  E000-FFFF   memory mapped IO or RAM
 *
 *****************************************************************************/

/* Core includes */
#include "driver.h"
#include "machine/8255ppi.h"
#include "includes/mz700.h"
#include "machine/pit8253.h"

/* Devices */
#include "devices/cassette.h"
#include "formats/mz_cas.h"


static ADDRESS_MAP_START( mz700_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x00000, 0x00fff) AM_RAMBANK(1)
	AM_RANGE( 0x01000, 0x0cfff) AM_RAM
	AM_RANGE( 0x0d000, 0x0d7ff) AM_RAMBANK(6)
	AM_RANGE( 0x0d800, 0x0dfff) AM_RAMBANK(7)
	AM_RANGE( 0x0e000, 0x0ffff) AM_RAMBANK(8)
#if 0 //mame37b9 traps
	AM_RANGE( 0x10000, 0x10fff) AM_ROM
	AM_RANGE( 0x12000, 0x127ff) AM_READWRITE(SMH_RAM, videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size )
	AM_RANGE( 0x12800, 0x12fff) AM_READWRITE(SMH_RAM, colorram_w) AM_BASE( &colorram )
	AM_RANGE( 0x16000, 0x16fff) AM_READWRITE(SMH_RAM, pcgram_w)
#endif
ADDRESS_MAP_END

static ADDRESS_MAP_START(mz700_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0xe0, 0xe6) AM_WRITE( mz700_bank_w )
ADDRESS_MAP_END

static ADDRESS_MAP_START(mz800_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x00000, 0x00fff) AM_RAMBANK(1)
	AM_RANGE( 0x01000, 0x01fff) AM_RAMBANK(2)
	AM_RANGE( 0x02000, 0x07fff) AM_RAM
	AM_RANGE( 0x08000, 0x09fff) AM_RAMBANK(3)
	AM_RANGE( 0x0a000, 0x0bfff) AM_RAMBANK(4)
	AM_RANGE( 0x0c000, 0x0cfff) AM_RAMBANK(5)
	AM_RANGE( 0x0d000, 0x0d7ff) AM_RAMBANK(6)
	AM_RANGE( 0x0d800, 0x0dfff) AM_RAMBANK(7)
	AM_RANGE( 0x0e000, 0x0ffff) AM_RAMBANK(8)
#if 0
	AM_RANGE( 0x10000, 0x10fff) AM_ROM
	AM_RANGE( 0x11000, 0x11fff) AM_ROM
	AM_RANGE( 0x12000, 0x16fff) AM_READWRITE(SMH_RAM, videoram_w) AM_BASE( &videoram) AM_SIZE( &videoram_size )
	AM_RANGE( 0x12800, 0x12fff) AM_WRITE( colorram_w) AM_BASE( &colorram )
#endif
	ADDRESS_MAP_END

static ADDRESS_MAP_START(mz800_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE( 0xcc, 0xcc) AM_WRITE( mz800_write_format_w )
	AM_RANGE( 0xcd, 0xcd) AM_WRITE( mz800_read_format_w )
	AM_RANGE( 0xce, 0xce) AM_READWRITE( mz800_crtc_r, mz800_display_mode_w )
	AM_RANGE( 0xcf, 0xcf) AM_WRITE( mz800_scroll_border_w )
	AM_RANGE( 0xd0, 0xd7) AM_READWRITE( mz800_mmio_r, mz800_mmio_w )
	AM_RANGE( 0xe0, 0xe9) AM_READWRITE( mz800_bank_r, mz800_bank_w )
	AM_RANGE( 0xea, 0xea) AM_READWRITE( mz800_ramdisk_r, mz800_ramdisk_w )
	AM_RANGE( 0xeb, 0xeb) AM_WRITE( mz800_ramaddr_w )
	AM_RANGE( 0xf0, 0xf0) AM_WRITE( mz800_palette_w )
ADDRESS_MAP_END

/* 2008-05 FP:
Notice that there is no Backspace key, only a 'Del' one.

Small note about natural keyboard support: currently,
- "Alpha" is mapped to 'F6'
- "Graph" is mapped to 'F7'                      
- "Break" is mapped to 'F8'                      */

static INPUT_PORTS_START( mz700 )
	PORT_START_TAG("STATUS")
	PORT_BIT(0x80, 0x80, IPT_VBLANK)
	PORT_BIT(0x7f, 0x00, IPT_UNUSED)

    PORT_START_TAG("ROW0")
	PORT_BIT(0x01, 0x01, IPT_KEYBOARD) PORT_NAME("CR") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x02, 0x02, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE)			PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x04, 0x04, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON)			PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0x08, 0x08, IPT_UNUSED )
	PORT_BIT(0x10, 0x10, IPT_KEYBOARD) PORT_NAME("Alpha") PORT_CODE(KEYCODE_TAB) PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT(0x20, 0x20, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x93  \xC2\xA3") PORT_CODE(KEYCODE_TILDE) PORT_CHAR('\xA3') // this one would be 2nd row, 3rd key after 'P'
	PORT_BIT(0x40, 0x40, IPT_KEYBOARD) PORT_NAME("Graph") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT(0x80, 0x80, IPT_KEYBOARD) PORT_CODE(KEYCODE_PGUP)			PORT_CHAR('_') // this one would be 2nd row, 4th key after 'P'

	PORT_START_TAG("ROW1")
	PORT_BIT(0x07, 0x07, IPT_UNUSED )
	PORT_BIT(0x08, 0x08, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH)		PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT(0x10, 0x10, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT(0x20, 0x20, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)		PORT_CHAR('@') PORT_CHAR('`')
	PORT_BIT(0x40, 0x40, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z)				PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT(0x80, 0x80, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y)				PORT_CHAR('y') PORT_CHAR('Y')

	PORT_START_TAG("ROW2")
	PORT_BIT(0x01, 0x01, IPT_KEYBOARD) PORT_CODE(KEYCODE_X)				PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT(0x02, 0x02, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) 			PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT(0x04, 0x04, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) 			PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x08, 0x08, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) 			PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x10, 0x10, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) 			PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x20, 0x20, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) 			PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x40, 0x40, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) 			PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT(0x80, 0x80, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) 			PORT_CHAR('q') PORT_CHAR('Q')

	PORT_START_TAG("ROW3")
	PORT_BIT(0x01, 0x01, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) 			PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x02, 0x02, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) 			PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT(0x04, 0x04, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) 			PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x08, 0x08, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) 			PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x10, 0x10, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) 			PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x20, 0x20, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) 			PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT(0x40, 0x40, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) 			PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x80, 0x80, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) 			PORT_CHAR('i') PORT_CHAR('I')

	PORT_START_TAG("ROW4")
	PORT_BIT(0x01, 0x01, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) 			PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x02, 0x02, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) 			PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT(0x04, 0x04, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) 			PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x08, 0x08, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) 			PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x10, 0x10, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) 			PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x20, 0x20, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) 			PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x40, 0x40, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) 			PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x80, 0x80, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) 			PORT_CHAR('a') PORT_CHAR('A')

	PORT_START_TAG("ROW5")
	PORT_BIT(0x01, 0x01, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) 			PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x02, 0x02, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) 			PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x04, 0x04, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) 			PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x08, 0x08, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) 			PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x10, 0x10, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) 			PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x20, 0x20, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) 			PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x40, 0x40, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) 			PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT(0x80, 0x80, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) 			PORT_CHAR('1') PORT_CHAR('!')

	PORT_START_TAG("ROW6")
	PORT_BIT(0x01, 0x01, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP) 			PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x02, 0x02, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) 		PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x04, 0x04, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) 			PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x08, 0x08, IPT_KEYBOARD) PORT_NAME("0  Pi")				PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT(0x10, 0x10, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)			PORT_CHAR(' ')
	PORT_BIT(0x20, 0x20, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS)			PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT(0x40, 0x40, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x91  ~") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('~')
	PORT_BIT(0x80, 0x80, IPT_KEYBOARD) PORT_CODE(KEYCODE_PGDN)			PORT_CHAR('\\') PORT_CHAR('|')	// this one would be 1st row, 3rd key after '0'

	PORT_START_TAG("ROW7")
	PORT_BIT(0x01, 0x01, IPT_KEYBOARD) PORT_NAME("/  \xE2\x86\x90") PORT_CODE(KEYCODE_SLASH)	PORT_CHAR('/')
	PORT_BIT(0x02, 0x02, IPT_KEYBOARD) PORT_NAME("?  \xE2\x86\x92") PORT_CODE(KEYCODE_END)		PORT_CHAR('?')	// this one would be 4th row, 4th key after 'M'
	PORT_BIT(0x04, 0x04, IPT_KEYBOARD) PORT_CODE(KEYCODE_LEFT)									PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x08, 0x08, IPT_KEYBOARD) PORT_CODE(KEYCODE_RIGHT)									PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x10, 0x10, IPT_KEYBOARD) PORT_CODE(KEYCODE_DOWN)									PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x20, 0x20, IPT_KEYBOARD) PORT_CODE(KEYCODE_UP)									PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x40, 0x40, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)					PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT(0x80, 0x80, IPT_KEYBOARD) PORT_NAME("INST") PORT_CODE(KEYCODE_INSERT)				PORT_CHAR(UCHAR_MAMEKEY(INSERT))

	PORT_START_TAG("ROW8")
	PORT_BIT(0x01, 0x01, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x3e, 0x3e, IPT_UNUSED)
    PORT_BIT(0x40, 0x40, IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x80, 0x80, IPT_KEYBOARD) PORT_NAME("Break") PORT_CODE(KEYCODE_HOME)				PORT_CHAR(UCHAR_MAMEKEY(F8))	// this one would be at Backspace position

	PORT_START_TAG("ROW9")
	PORT_BIT(0x07, 0x07, IPT_UNUSED)
    PORT_BIT(0x08, 0x08, IPT_KEYBOARD) PORT_CODE(KEYCODE_F5) 			PORT_CHAR(UCHAR_MAMEKEY(F5))
    PORT_BIT(0x10, 0x10, IPT_KEYBOARD) PORT_CODE(KEYCODE_F4) 			PORT_CHAR(UCHAR_MAMEKEY(F4))
    PORT_BIT(0x20, 0x20, IPT_KEYBOARD) PORT_CODE(KEYCODE_F3) 			PORT_CHAR(UCHAR_MAMEKEY(F3))
    PORT_BIT(0x40, 0x40, IPT_KEYBOARD) PORT_CODE(KEYCODE_F2) 			PORT_CHAR(UCHAR_MAMEKEY(F2))
    PORT_BIT(0x80, 0x80, IPT_KEYBOARD) PORT_CODE(KEYCODE_F1) 			PORT_CHAR(UCHAR_MAMEKEY(F1))

	/* 2005-08 FP: why is this here? According to the Service Manual ROW9 is the last one */
	PORT_START_TAG("ROW10") /* KEY ROW 10 */ 

	PORT_START_TAG("JOY")
	PORT_BIT(0x01, 0x00, IPT_UNUSED)
	PORT_BIT(0x02, 0x00, IPT_JOYSTICK_UP)		PORT_8WAY
	PORT_BIT(0x04, 0x00, IPT_JOYSTICK_DOWN)		PORT_8WAY
	PORT_BIT(0x08, 0x00, IPT_JOYSTICK_LEFT)		PORT_8WAY
	PORT_BIT(0x10, 0x00, IPT_JOYSTICK_RIGHT)	PORT_8WAY
INPUT_PORTS_END

static const gfx_layout char_layout =
{
	8, 8,		/* 8 x 8 graphics */
	512,		/* 512 codes */
	1,			/* 1 bit per pixel */
	{ 0 },		/* no bitplanes */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8 * 8		/* code takes 8 times 8 bits */
};

static GFXDECODE_START( mz700 )
	GFXDECODE_ENTRY( REGION_GFX1, 0, char_layout, 0, 256 )
GFXDECODE_END



static MACHINE_DRIVER_START(mz700)
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3500000)
	MDRV_CPU_PROGRAM_MAP(mz700_mem, 0)
	MDRV_CPU_IO_MAP(mz700_io, 0)

	MDRV_MACHINE_RESET( mz700 )

	MDRV_DEVICE_ADD( "pit8253", PIT8253 )
	MDRV_DEVICE_CONFIG( mz700_pit8253_config )

	MDRV_DEVICE_ADD( "ppi8255", PPI8255 )
	MDRV_DEVICE_CONFIG( mz700_ppi8255_interface )

	/* video hardware - include overscan */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(40*8, 25*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 40*8 - 1, 0*8, 25*8 - 1)

	MDRV_GFXDECODE(mz700)
	MDRV_PALETTE_LENGTH(256*2)

	MDRV_PALETTE_INIT(mz700)
	MDRV_VIDEO_UPDATE(mz700)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD(BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(mz800)
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3500000)
	MDRV_CPU_PROGRAM_MAP(mz800_mem, 0)
	MDRV_CPU_IO_MAP(mz800_io, 0)

	MDRV_MACHINE_RESET( mz700 )

	MDRV_DEVICE_ADD( "pit8253", PIT8253 )
	MDRV_DEVICE_CONFIG( mz700_pit8253_config )

	MDRV_DEVICE_ADD( "ppi8255", PPI8255 )
	MDRV_DEVICE_CONFIG( mz700_ppi8255_interface )

	/* video hardware - include overscan */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(40*8, 25*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 40*8 - 1, 0*8, 25*8 - 1)

	MDRV_GFXDECODE(mz700)
	MDRV_PALETTE_LENGTH(256*2)

	MDRV_PALETTE_INIT(mz700)
	MDRV_VIDEO_UPDATE(mz700)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD(BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END



ROM_START(mz700)
	ROM_REGION(0x18000,REGION_CPU1,0)
		ROM_LOAD("1z-013a.rom", 0x10000, 0x1000, CRC(4c6c6b7b) SHA1(ef8f7399e86c1dc638a5cb83efdb73369c2b5735))
	ROM_REGION(0x01000,REGION_GFX1,0)
		ROM_LOAD("mz700fon.int",0x00000, 0x1000, CRC(42b9e8fb) SHA1(5128ad179a702f8e0bd9910a58bad8fbe4c20167))
ROM_END

ROM_START(mz700j)
	ROM_REGION(0x18000,REGION_CPU1,0)
		ROM_LOAD("1z-013a.rom", 0x10000, 0x1000, CRC(4c6c6b7b) SHA1(ef8f7399e86c1dc638a5cb83efdb73369c2b5735))
	ROM_REGION(0x01000,REGION_GFX1,0)
		ROM_LOAD("mz700fon.jap",0x00000, 0x1000, CRC(425eedf5) SHA1(bd2cc750f2d2f63e50a59786668509e81a276e32))
ROM_END

ROM_START(mz800)
	ROM_REGION(0x18000,REGION_CPU1,0)
		ROM_LOAD("mz800h.rom",  0x10000, 0x2000, BAD_DUMP CRC(0c281675) SHA1(0adb6201f114f96f06a50de07d1c1ca2bcb4cf43))
	ROM_REGION(0x10000,REGION_USER1,0)
		/* RAMDISK */
    ROM_REGION(0x01000,REGION_GFX1,0)
		ROM_LOAD("mz700fon.int",0x00000, 0x1000, CRC(42b9e8fb) SHA1(5128ad179a702f8e0bd9910a58bad8fbe4c20167))
ROM_END

static void mz700_cassette_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_CASSETTE_FORMATS:				info->p = (void *) mz700_cassette_formats; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(mz700)
	CONFIG_DEVICE(mz700_cassette_getinfo)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT     INIT    CONFIG  COMPANY      FULLNAME */
COMP( 1982, mz700,	  0,		0,		mz700,	  mz700,	mz700,	mz700,	"Sharp",     "MZ-700" , 0)
COMP( 1982, mz700j,   mz700,	0,		mz700,	  mz700,	mz700,	mz700,	"Sharp",     "MZ-700 (Japan)" , 0)
COMP( 1982, mz800,	  mz700,	0,		mz800,	  mz700,	mz800,	mz700,	"Sharp",     "MZ-800" , GAME_NOT_WORKING )


