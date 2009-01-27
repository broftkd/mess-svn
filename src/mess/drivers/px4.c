/***************************************************************************

    Epson PX-4

	Note: We are missing a dump of the slave 7508 CPU that controls
	the keyboard and some other things.

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "devices/cartslot.h"


/***************************************************************************
    GAPNIT
***************************************************************************/

/* input capture register low command trigger */
static READ8_HANDLER( px4_icrlc_r )
{
	logerror("%s: px4_icrlc_r\n", cpuexec_describe_context(space->machine));
	return 0xff;
}

/* control register 1 */
static WRITE8_HANDLER( px4_ctrl1_w )
{
	logerror("%s: px4_ctrl1_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* input capture register high command trigger */
static READ8_HANDLER( px4_icrhc_r )
{
	logerror("%s: px4_icrhc_r\n", cpuexec_describe_context(space->machine));
	return 0xff;
}

/* command register */
static WRITE8_HANDLER( px4_cmdr_w )
{
	logerror("%s: px4_cmdr_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* barcode trigger */
static READ8_HANDLER( px4_icrlb_r )
{
	logerror("%s: px4_icrlb_r\n", cpuexec_describe_context(space->machine));
	return 0xff;
}

/* control register 2 */
static WRITE8_HANDLER( px4_ctrl2_w )
{
	logerror("%s: px4_ctrl2_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* barcode trigger */
static READ8_HANDLER( px4_icrhb_r )
{
	logerror("%s: px4_icrhb_r\n", cpuexec_describe_context(space->machine));
	return 0xff;
}

/* interrupt status register */
static READ8_HANDLER( px4_isr_r )
{
	logerror("%s: px4_isr_r\n", cpuexec_describe_context(space->machine));
	return 0xff;
}

/* interrupt enable register */
static WRITE8_HANDLER( px4_ier_w )
{
	logerror("%s: px4_ier_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* status register */
static READ8_HANDLER( px4_str_r )
{
	logerror("%s: px4_str_r\n", cpuexec_describe_context(space->machine));
	return 0xff;
}

/* bank register */
static WRITE8_HANDLER( px4_bankr_w )
{
	logerror("%s: px4_bankr_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);

	/* clock switch */
	switch (data & 0x02)
	{
	case 0x00:
	case 0x01:
		cpu_set_clock(space->machine->cpu[0], 3457600);
		break;

	case 0x02:
		cpu_set_clock(space->machine->cpu[0], XTAL_3_6864MHz);
		break;

	case 0x03:
		cpu_set_clock(space->machine->cpu[0], 3072000);
		break;
	}
}

/* serial io register */
static READ8_HANDLER( px4_sior_r )
{
	logerror("%s: px4_sior_r\n", cpuexec_describe_context(space->machine));
	return 0xff;
}

/* serial io register */
static WRITE8_HANDLER( px4_sior_w )
{
	logerror("%s: px4_sior_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}


/***************************************************************************
    GAPNDI
***************************************************************************/

/* vram start address register */
static WRITE8_HANDLER( px4_vadr_w )
{
	logerror("%s: px4_vadr_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* y offset register */
static WRITE8_HANDLER( px4_yoff_w )
{
	logerror("%s: px4_yoff_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* frame register */
static WRITE8_HANDLER( px4_fr_w )
{
	logerror("%s: px4_fr_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* speed-up register */
static WRITE8_HANDLER( px4_spur_w )
{
	logerror("%s: px4_spur_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}


/***************************************************************************
    GAPNDL
***************************************************************************/

/* cartridge interface */
static READ8_HANDLER( px4_ctgif_r )
{
	logerror("%s: px4_ctgif_r @ 0x%02x\n", cpuexec_describe_context(space->machine), offset);
	return 0xff;
}

/* cartridge interface */
static WRITE8_HANDLER( px4_ctgif_w )
{
	logerror("%s: px4_ctgif_w (0x%02x @ 0x%02x)\n", cpuexec_describe_context(space->machine), data, offset);
}

/* art data input register */
static READ8_HANDLER( px4_artdir_r )
{
	logerror("%s: px4_artdir_r\n", cpuexec_describe_context(space->machine));
	return 0xff;
}

/* art data output register */
static WRITE8_HANDLER( px4_artdor_w )
{
	logerror("%s: px4_artdor_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* art status register */
static READ8_HANDLER( px4_artsr_r )
{
	logerror("%s: px4_artsr_r\n", cpuexec_describe_context(space->machine));
	return 0xff;
}

/* art mode register */
static WRITE8_HANDLER( px4_artmr_w )
{
	logerror("%s: px4_artmr_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* io status register */
static READ8_HANDLER( px4_iostr_r )
{
	logerror("%s: px4_iostr_r\n", cpuexec_describe_context(space->machine));
	return 0xff;
}

/* art command register */
static WRITE8_HANDLER( px4_artcr_w )
{
	logerror("%s: px4_artcr_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* printer data register */
static WRITE8_HANDLER( px4_pdr_w )
{
	logerror("%s: px4_pdr_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* switch register */
static WRITE8_HANDLER( px4_swr_w )
{
	logerror("%s: px4_swr_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}

/* io control register */
static WRITE8_HANDLER( px4_ioctlr_w )
{
	logerror("%s: px4_ioctlr_w (0x%02x)\n", cpuexec_describe_context(space->machine), data);
}


/***************************************************************************
    DRIVER INIT
***************************************************************************/

static DRIVER_INIT( px4 )
{
	const address_space *space = cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM);

	/* map os rom */
	memory_install_readwrite8_handler(space, 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_NOP);
	memory_set_bankptr(machine, 1, memory_region(machine, "os"));

	/* memory */
	memory_install_readwrite8_handler(space, 0x8000, 0xffff, 0, 0, SMH_BANK(2), SMH_BANK(2));
	memory_set_bankptr(machine, 2, mess_ram);
}


/***************************************************************************
    ADDRESS MAPS
***************************************************************************/

static ADDRESS_MAP_START( px4_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROMBANK(1)
	AM_RANGE(0x8000, 0xffff) AM_RAMBANK(2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( px4_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_READWRITE(px4_icrlc_r, px4_ctrl1_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(px4_icrhc_r, px4_cmdr_w)
	AM_RANGE(0x02, 0x02) AM_READWRITE(px4_icrlb_r, px4_ctrl2_w)
	AM_RANGE(0x03, 0x03) AM_READ(px4_icrhb_r)
	AM_RANGE(0x04, 0x04) AM_READWRITE(px4_isr_r, px4_ier_w)
	AM_RANGE(0x05, 0x05) AM_READWRITE(px4_str_r, px4_bankr_w)
	AM_RANGE(0x06, 0x06) AM_READWRITE(px4_sior_r, px4_sior_w)
	AM_RANGE(0x07, 0x07) AM_NOP
	AM_RANGE(0x08, 0x08) AM_WRITE(px4_vadr_w)
	AM_RANGE(0x09, 0x09) AM_WRITE(px4_yoff_w)
	AM_RANGE(0x0a, 0x0a) AM_WRITE(px4_fr_w)
	AM_RANGE(0x0b, 0x0b) AM_WRITE(px4_spur_w)
	AM_RANGE(0x0c, 0x0f) AM_NOP
	AM_RANGE(0x10, 0x13) AM_READWRITE(px4_ctgif_r, px4_ctgif_w)
	AM_RANGE(0x14, 0x14) AM_READWRITE(px4_artdir_r, px4_artdor_w)
	AM_RANGE(0x15, 0x15) AM_READWRITE(px4_artsr_r, px4_artmr_w)
	AM_RANGE(0x16, 0x16) AM_READWRITE(px4_iostr_r, px4_artcr_w)
	AM_RANGE(0x17, 0x17) AM_WRITE(px4_pdr_w)
	AM_RANGE(0x18, 0x18) AM_WRITE(px4_swr_w)
	AM_RANGE(0x19, 0x19) AM_WRITE(px4_ioctlr_w)
	AM_RANGE(0x1a, 0x1f) AM_NOP
	AM_RANGE(0x20, 0xff) AM_NOP /* external i/o */
ADDRESS_MAP_END


/***************************************************************************
    INPUT PORTS
***************************************************************************/

/* The PX-4 has an exchangable keyboard. Available is a standard ASCII
 * keyboard and an "item" keyboard, as well as regional variants for
 * UK, France, Germany, Denmark, Sweden, Norway, Italy and Spain.
 */
static INPUT_PORTS_START( px4_ascii )
INPUT_PORTS_END


/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

static MACHINE_DRIVER_START( px4 )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", Z80, XTAL_3_6864MHz)	/* uPD70008 */
	MDRV_CPU_PROGRAM_MAP(px4_mem, 0)
	MDRV_CPU_IO_MAP(px4_io, 0)

	/* video hardware */
	MDRV_SCREEN_ADD("main", LCD)
	MDRV_SCREEN_REFRESH_RATE(44)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(240, 64)
	MDRV_SCREEN_VISIBLE_AREA(0, 239, 0, 63)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	/* rom capsules */
	MDRV_CARTSLOT_ADD("capsule1")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_ADD("capsule2")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_NOT_MANDATORY
MACHINE_DRIVER_END


/***************************************************************************
    ROM DEFINITIONS
***************************************************************************/

ROM_START( px4 )
    ROM_REGION(0x8000, "os", 0)
    ROM_LOAD("po_px4.bin", 0x0000, 0x8000, CRC(62d60dc6) SHA1(3d32ec79a317de7c84c378302e95f48d56505502))

	ROM_REGION(0x8000, "capsule1", 0)
	ROM_CART_LOAD("capsule1", 0x0000, 0x8000, ROM_OPTIONAL)

	ROM_REGION(0x8000, "capsule2", 0)
	ROM_CART_LOAD("capsule2", 0x0000, 0x8000, ROM_OPTIONAL)
ROM_END

ROM_START( px4p )
    ROM_REGION(0x8000, "os", 0)
    ROM_LOAD("b0_pxa.bin", 0x0000, 0x8000, CRC(d74b9ef5) SHA1(baceee076c12f5a16f7a26000e9bc395d021c455))

    ROM_REGION(0x8000, "capsule1", 0)
    ROM_CART_LOAD("capsule1", 0x0000, 0x8000, ROM_OPTIONAL)

    ROM_REGION(0x8000, "capsule2", 0)
    ROM_CART_LOAD("capsule2", 0x0000, 0x8000, ROM_OPTIONAL)
ROM_END


/***************************************************************************
    SYSTEM CONFIG
***************************************************************************/

static SYSTEM_CONFIG_START( px4 )
	CONFIG_RAM_DEFAULT(64 * 1024) /* 64KB RAM */
SYSTEM_CONFIG_END


/***************************************************************************
    GAME DRIVERS
***************************************************************************/

/*    YEAR  NAME  PARENT  COMPAT  MACHINE  INPUT      INIT  CONFIG  COMPANY  FULLNAME  FLAGS */
COMP( 1985, px4,  0,      0,      px4,     px4_ascii, px4,  px4,    "Epson", "PX-4",   GAME_NOT_WORKING )
COMP( 1985, px4p, px4,    0,      px4,     px4_ascii, px4,  px4,    "Epson", "PX-4+",  GAME_NOT_WORKING )
