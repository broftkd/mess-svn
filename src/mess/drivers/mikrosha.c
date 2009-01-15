/***************************************************************************

        Mikrosha driver by Miodrag Milanovic

        05/06/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "sound/wave.h"
#include "machine/8255ppi.h"
#include "machine/8257dma.h"
#include "machine/pit8253.h"
#include "video/i8275.h"
#include "devices/cassette.h"
#include "formats/rk_cas.h"
#include "includes/radio86.h"

/* Address maps */
static ADDRESS_MAP_START(mikrosha_mem, ADDRESS_SPACE_PROGRAM, 8)
    AM_RANGE( 0x0000, 0x0fff ) AM_RAMBANK(1) // First bank
    AM_RANGE( 0x1000, 0x7fff ) AM_RAM // RAM
    AM_RANGE( 0x8000, 0xbfff ) AM_READ(radio_cpu_state_r) // Not connected
    AM_RANGE( 0xc000, 0xc003 ) AM_DEVREADWRITE(PPI8255, "ppi8255_1", ppi8255_r, ppi8255_w) AM_MIRROR(0x07fc)
    AM_RANGE( 0xc800, 0xc803 ) AM_DEVREADWRITE(PPI8255, "ppi8255_2", ppi8255_r, ppi8255_w) AM_MIRROR(0x07fc)
    AM_RANGE( 0xd000, 0xd001 ) AM_DEVREADWRITE(I8275, "i8275", i8275_r, i8275_w) AM_MIRROR(0x07fe) // video
    AM_RANGE( 0xd803, 0xd803 ) AM_READ(radio_cpu_state_r) AM_MIRROR(0x07fc) // Not connected
//    AM_RANGE( 0xd800, 0xd803 ) AM_DEVREADWRITE(PIT8253, "pit8253", pit8253_r,pit8253_w) AM_MIRROR(0x07fc) // Timer
    AM_RANGE( 0xe000, 0xf7ff ) AM_READ(radio_cpu_state_r) // Not connected
  	AM_RANGE( 0xf800, 0xffff ) AM_DEVWRITE(DMA8257, "dma8257", dma8257_w)	 // DMA
    AM_RANGE( 0xf800, 0xffff ) AM_ROM  // System ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( mikrosha_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x00, 0xff ) AM_READWRITE(radio_io_r,radio_io_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( mikrosha )
	PORT_START("LINE0")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(":") PORT_CODE(KEYCODE_EQUALS)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
	PORT_START("LINE1")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Esc") PORT_CODE(KEYCODE_ESC)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
	PORT_START("LINE2")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Home") PORT_CODE(KEYCODE_HOME)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(":") PORT_CODE(KEYCODE_EQUALS)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
	PORT_START("LINE3")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Del") PORT_CODE(KEYCODE_DEL)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("`") PORT_CODE(KEYCODE_TILDE)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_OPENBRACE)
	PORT_START("LINE4")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PgUp") PORT_CODE(KEYCODE_PGUP)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("<") PORT_CODE(KEYCODE_COMMA)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("'") PORT_CODE(KEYCODE_QUOTE)
	PORT_START("LINE5")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ins") PORT_CODE(KEYCODE_INSERT)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("End") PORT_CODE(KEYCODE_END)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_CLOSEBRACE)
	PORT_START("LINE6")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Center") PORT_CODE(KEYCODE_F3)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(">") PORT_CODE(KEYCODE_STOP)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\") PORT_CODE(KEYCODE_BACKSLASH)
	PORT_START("LINE7")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PgDn") PORT_CODE(KEYCODE_PGDN)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("?") PORT_CODE(KEYCODE_SLASH)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Back") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_START("LINE8")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Rus/Lat") PORT_CODE(KEYCODE_LALT) PORT_CODE(KEYCODE_RALT)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)
INPUT_PORTS_END

/* Machine driver */
static const cassette_config mikrosha_cassette_config =
{
	rkm_cassette_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_SPEAKER_ENABLED | CASSETTE_MOTOR_ENABLED
};


static UINT8 *mikrosha_io_mirror = NULL;

static DIRECT_UPDATE_HANDLER( mikrosha_direct )
{		
	if (address >= 0x8000 && address <=0xFFFF) {
			direct->bytemask = 0xffff;
			direct->raw = mikrosha_io_mirror;
			direct->decrypted = mikrosha_io_mirror;
			direct->bytestart = 0x8000;
			direct->byteend = 0xffff;
			mikrosha_io_mirror[address] = cpu_get_reg(space->machine->cpu[0], I8085_STATUS);
	} 
	return address;
}

static MACHINE_START( mikrosha )
{
	mikrosha_io_mirror = auto_malloc( 0x10000 );
	memory_set_direct_update_handler( cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), mikrosha_direct );
}

static PIT8253_OUTPUT_CHANGED(mikrosha_pit_out2)
{

}

static const struct pit8253_config mikrosha_pit8253_intf =
{
	{
		{
			0,
			NULL
		},
		{
			0,
			NULL
		},
		{
			2000000,
			mikrosha_pit_out2
		}
	}
};

static MACHINE_DRIVER_START( mikrosha )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", 8080, XTAL_16MHz / 9)
	MDRV_CPU_PROGRAM_MAP(mikrosha_mem, 0)
	MDRV_CPU_IO_MAP(mikrosha_io, 0)
	MDRV_MACHINE_START( mikrosha )
	MDRV_MACHINE_RESET( radio86 )

	MDRV_PPI8255_ADD( "ppi8255_1", mikrosha_ppi8255_interface_1 )

	MDRV_PPI8255_ADD( "ppi8255_2", mikrosha_ppi8255_interface_2 )

	MDRV_I8275_ADD  ( "i8275", mikrosha_i8275_interface)

	MDRV_PIT8253_ADD( "pit8253", mikrosha_pit8253_intf )

  /* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(78*6, 30*10)
	MDRV_SCREEN_VISIBLE_AREA(0, 78*6-1, 0, 30*10-1)
	MDRV_PALETTE_LENGTH(3)
	MDRV_PALETTE_INIT(radio86)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(radio86)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("cassette", WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_DMA8257_ADD("dma8257", XTAL_16MHz / 9, radio86_dma)

	MDRV_CASSETTE_ADD( "cassette", mikrosha_cassette_config )
MACHINE_DRIVER_END


/* ROM definition */
ROM_START( mikrosha )
	ROM_REGION( 0x10000, "main", ROMREGION_ERASEFF )
	ROM_LOAD( "mikrosha.rom", 0xf800, 0x0800, CRC(86A83556) SHA1(94b1baad0a419145939a891ff51f4324e8e4ddd2))
	ROM_REGION(0x0800, "gfx1",0)
	ROM_LOAD ("mikrosha.fnt", 0x0000, 0x0800, CRC(B315DA1C) SHA1(b5bf9abc0fff75b1aba709a7f08b23d4a89bb04b))
ROM_END


/* Driver */

/*    YEAR  NAME      PARENT  COMPAT    MACHINE     INPUT       INIT        CONFIG      COMPANY     FULLNAME        FLAGS */
COMP( 1987, mikrosha, radio86,0, 		mikrosha, 	mikrosha,	radio86,	0,  		"Lianozovo Electromechanical Factory", 		"Mikrosha",		0)
