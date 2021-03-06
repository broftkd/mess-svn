/***************************************************************************

        Robotron K8915

        30/08/2010 Skeleton driver

        When it says DIAGNOSTIC RAZ P, press enter.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/keyboard.h"

#define MACHINE_RESET_MEMBER(name) void name::machine_reset()
#define VIDEO_START_MEMBER(name) void name::video_start()
#define SCREEN_UPDATE16_MEMBER(name) UINT32 name::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)

class k8915_state : public driver_device
{
public:
	k8915_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu")
	,
		m_p_videoram(*this, "p_videoram"){ }

	required_device<cpu_device> m_maincpu;
	DECLARE_READ8_MEMBER( k8915_52_r );
	DECLARE_READ8_MEMBER( k8915_53_r );
	DECLARE_WRITE8_MEMBER( k8915_a8_w );
	DECLARE_WRITE8_MEMBER( kbd_put );
	required_shared_ptr<UINT8> m_p_videoram;
	UINT8 *m_p_chargen;
	UINT8 m_framecnt;
	UINT8 m_term_data;
	virtual void machine_reset();
	virtual void video_start();
	UINT32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	DECLARE_DRIVER_INIT(k8915);
};

READ8_MEMBER( k8915_state::k8915_52_r )
{
// get data from ascii keyboard
	UINT8 ret = m_term_data;
	m_term_data = 0;
	return ret;
}

READ8_MEMBER( k8915_state::k8915_53_r )
{
// keyboard status
	return m_term_data ? 1 : 0;
}

WRITE8_MEMBER( k8915_state::k8915_a8_w )
{
// seems to switch ram and rom around.
	if (data == 0x87)
		membank("boot")->set_entry(0); // ram at 0000
	else
		membank("boot")->set_entry(1); // rom at 0000
}

static ADDRESS_MAP_START(k8915_mem, AS_PROGRAM, 8, k8915_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_RAMBANK("boot")
	AM_RANGE(0x1000, 0x17ff) AM_RAM AM_SHARE("p_videoram")
	AM_RANGE(0x1800, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START(k8915_io, AS_IO, 8, k8915_state)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x52, 0x52) AM_READ(k8915_52_r)
	AM_RANGE(0x53, 0x53) AM_READ(k8915_53_r)
	AM_RANGE(0xa8, 0xa8) AM_WRITE(k8915_a8_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( k8915 )
INPUT_PORTS_END

MACHINE_RESET_MEMBER(k8915_state)
{
	membank("boot")->set_entry(1);
}

DRIVER_INIT_MEMBER(k8915_state,k8915)
{
	UINT8 *RAM = memregion("maincpu")->base();
	membank("boot")->configure_entries(0, 2, &RAM[0x0000], 0x10000);
}

VIDEO_START_MEMBER( k8915_state )
{
	m_p_chargen = memregion("chargen")->base();
}

SCREEN_UPDATE16_MEMBER( k8915_state )
{
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;

	m_framecnt++;

	for (y = 0; y < 25; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16 *p = &bitmap.pix16(sy++);

			for (x = ma; x < ma + 80; x++)
			{
				gfx = 0;

				if (ra < 9)
				{
					chr = m_p_videoram[x];

					/* Take care of flashing characters */
					if ((chr & 0x80) && (m_framecnt & 0x08))
						chr = 0x20;

					chr &= 0x7f;

					gfx = m_p_chargen[(chr<<4) | ra ];
				}

				/* Display a scanline of a character */
				*p++ = BIT(gfx, 7);
				*p++ = BIT(gfx, 6);
				*p++ = BIT(gfx, 5);
				*p++ = BIT(gfx, 4);
				*p++ = BIT(gfx, 3);
				*p++ = BIT(gfx, 2);
				*p++ = BIT(gfx, 1);
				*p++ = BIT(gfx, 0);
			}
		}
		ma+=80;
	}
	return 0;
}

WRITE8_MEMBER( k8915_state::kbd_put )
{
	m_term_data = data;
}

static ASCII_KEYBOARD_INTERFACE( keyboard_intf )
{
	DEVCB_DRIVER_MEMBER(k8915_state, kbd_put)
};

static MACHINE_CONFIG_START( k8915, k8915_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, XTAL_16MHz / 4)
	MCFG_CPU_PROGRAM_MAP(k8915_mem)
	MCFG_CPU_IO_MAP(k8915_io)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_UPDATE_DRIVER(k8915_state, screen_update)
	MCFG_SCREEN_SIZE(640, 250)
	MCFG_SCREEN_VISIBLE_AREA(0, 639, 0, 249)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(monochrome_green)

	MCFG_ASCII_KEYBOARD_ADD(KEYBOARD_TAG, keyboard_intf)
MACHINE_CONFIG_END


/* ROM definition */
ROM_START( k8915 )
	ROM_REGION( 0x11000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "k8915.bin", 0x10000, 0x1000, CRC(ca70385f) SHA1(a34c14adae9be821678aed7f9e33932ee1f3e61c))

	/* character generator not dumped, using the one from 'c10' for now */
	ROM_REGION( 0x2000, "chargen", 0 )
	ROM_LOAD( "c10_char.bin", 0x0000, 0x2000, BAD_DUMP CRC(cb530b6f) SHA1(95590bbb433db9c4317f535723b29516b9b9fcbf))
ROM_END

/* Driver */

/*   YEAR  NAME    PARENT  COMPAT   MACHINE  INPUT  INIT        COMPANY   FULLNAME       FLAGS */
COMP( 1982, k8915,  0,       0, 	k8915,	k8915, k8915_state,	 k8915, "Robotron",   "K8915", GAME_NOT_WORKING | GAME_NO_SOUND)
