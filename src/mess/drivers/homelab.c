/***************************************************************************

    Homelab driver by Miodrag Milanovic

    31/08/2008 Preliminary driver.
    15/06/2012 Various updates [Robbbert]

    No info available on the Brailab4, apart from the fact it has
    a speech unit. As the name suggests,  it is meant for use by
    blind people. Looks like speech could be output from port F8.
    It also seems that there is no screen, however some video
    circuitry is retained, because it still reads the vertical
    sync pulse.

    ToDO:
    - homelab2 - sound, cassette
                 Note that rom code 0x40-48 is meaningless garbage,
                 had to patch to stop it crashing. Need a new dump.
    - homelab3 etc - sound, cassout. Need a dump of the TM188.
    - Brailab4 - had to patch, to stop stack getting wiped out.
                 currently reads keys and outputs noise. Needs
                 unknown speech device and maybe other devices.
                 Need a schematic.
    - Z80PIO   - expansion only, doesn't do anything in the
                 machine. /CE connects to A6, A/B = A0, C/D = A1.
                 The bios never talks to it. Not fitted to homelab2.
                 Official port numbers are 3C-3F.

TM188 is (it seems) equivalent to 27S19, TBP18S030N, 6331-1, 74S288, 82S123,
MB7051 - fuse programmed prom.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "sound/speaker.h"
#include "sound/wave.h"
#include "imagedev/cassette.h"


//#define MACHINE_RESET_MEMBER(name) void name::machine_reset()
//#define MACHINE_START_MEMBER(name) void name::machine_start()
#define VIDEO_START_MEMBER(name) void name::video_start()

class homelab_state : public driver_device
{
public:
	homelab_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu"),
	m_speaker(*this, SPEAKER_TAG),
	m_cass(*this, CASSETTE_TAG),
	m_p_videoram(*this, "videoram"){ }

	DECLARE_READ8_MEMBER(key_r);
	DECLARE_WRITE8_MEMBER(cass_w);
	DECLARE_READ8_MEMBER(cass2_r);
	DECLARE_READ8_MEMBER(exxx_r);
	DECLARE_READ8_MEMBER(io_r);
	DECLARE_WRITE8_MEMBER(io_w);
	DECLARE_CUSTOM_INPUT_MEMBER(cass3_r);
	const UINT8 *m_p_chargen;
	//virtual void machine_reset();
	//virtual void machine_start();
	virtual void video_start();
	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_speaker;
	required_device<cassette_image_device> m_cass;
	optional_shared_ptr<const UINT8> m_p_videoram;
private:
};


static INTERRUPT_GEN( homelab_frame )
{
	homelab_state *state = device->machine().driver_data<homelab_state>();
	device_set_input_line(state->m_maincpu, INPUT_LINE_NMI, ASSERT_LINE);
}

READ8_MEMBER( homelab_state::key_r ) // offset 27F-2FE
{
	if (offset == 0x38) // 0x3838
	{
		device_set_input_line(m_maincpu, INPUT_LINE_NMI, CLEAR_LINE);
		return 0;
	}

	UINT8 i,data = 0xff;
	char kbdrow[8];

	for (i=0; i<8; i++)
	{
		if (!BIT(offset, i))
		{
			sprintf(kbdrow,"LINE%d", i);
			data &= ioport(kbdrow)->read();
		}
	}

	return data;
}

READ8_MEMBER( homelab_state::io_r )
{
	return 0xff;
}


WRITE8_MEMBER( homelab_state::io_w )
{
	bool state = BIT(offset, 7);
	speaker_level_w(m_speaker, state );
	m_cass->output(state ? -1.0 : +1.0);
}

READ8_MEMBER( homelab_state::cass2_r )
{
	return (m_cass->input() > 0.03);
}

WRITE8_MEMBER( homelab_state::cass_w )
{
	if (offset == 0x73f) // 0x3f3f
		speaker_level_w(m_speaker, 1);
	else
	if (offset == 0x139) // 0x3939
		speaker_level_w(m_speaker, 0);
	else
	if (offset == 0x400) // 0x3c00
		m_cass->output(BIT(data, 0) ? -1.0 : +1.0); // FIXME
}

CUSTOM_INPUT_MEMBER( homelab_state::cass3_r )
{
	return (m_cass->input() > 0.03);
}


READ8_MEMBER( homelab_state::exxx_r )
{
// keys E800-E813 but E810-E813 are not connected
// cassin E883
// sound routine reads E880 but discards the result

	char kbdrow[8];
	UINT8 data = 0xff;
	if (offset < 0x10)
	{
		sprintf(kbdrow,"X%X", offset);
		data = ioport(kbdrow)->read();
	}
	return data;
}


/* Address maps */
static ADDRESS_MAP_START(homelab2_mem, AS_PROGRAM, 8, homelab_state)
	AM_RANGE( 0x0000, 0x07ff ) AM_ROM  // ROM 1
	AM_RANGE( 0x0800, 0x0fff ) AM_ROM  // ROM 2
	AM_RANGE( 0x1000, 0x17ff ) AM_ROM  // ROM 3
	AM_RANGE( 0x1800, 0x1fff ) AM_ROM  // ROM 4
	AM_RANGE( 0x2000, 0x27ff ) AM_ROM  // ROM 5
	AM_RANGE( 0x2800, 0x2fff ) AM_ROM  // ROM 6
	AM_RANGE( 0x3000, 0x37ff ) AM_ROM  // Empty
	AM_RANGE( 0x3800, 0x3fff ) AM_READWRITE(key_r,cass_w)
	AM_RANGE( 0x4000, 0x7fff ) AM_RAM
	AM_RANGE( 0xc000, 0xc3ff ) AM_RAM AM_SHARE("videoram") // Video RAM 1K
	AM_RANGE( 0xe000, 0xe0ff ) AM_READ(cass2_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START(homelab3_mem, AS_PROGRAM, 8, homelab_state)
	AM_RANGE( 0x0000, 0x3fff ) AM_ROM
	AM_RANGE( 0x4000, 0x7fff ) AM_RAM
	AM_RANGE( 0xe000, 0xe01f ) AM_MIRROR(0x0fe0) AM_READ(exxx_r)
	AM_RANGE( 0xf000, 0xf7ff ) AM_RAM AM_SHARE("videoram") AM_MIRROR(0x800) // Video RAM 2K
ADDRESS_MAP_END

static ADDRESS_MAP_START(homelab3_io, AS_IO, 8, homelab_state)
	AM_RANGE( 0x0000, 0xffff ) AM_READWRITE(io_r,io_w)
ADDRESS_MAP_END

// this is a guess
static ADDRESS_MAP_START(brailab4_mem, AS_PROGRAM, 8, homelab_state)
	AM_RANGE( 0x0000, 0x3fff ) AM_ROM
	AM_RANGE( 0x4000, 0xdfff ) AM_RAM
	AM_RANGE( 0xe000, 0xe01f ) AM_MIRROR(0x0fe0) AM_READ(exxx_r)
	AM_RANGE( 0xf000, 0xffff ) AM_RAM AM_SHARE("videoram")
ADDRESS_MAP_END



/* Input ports */
static INPUT_PORTS_START( homelab )
	PORT_START("LINE0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left Shift")  PORT_CODE(KEYCODE_LSHIFT) //PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right Shift") PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT(0xfc, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("LINE1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Home") PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Run/Brk") PORT_CODE(KEYCODE_RCONTROL)

	PORT_START("LINE2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('<')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')

	PORT_START("LINE3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) PORT_CHAR('<') PORT_CHAR(',')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('=') PORT_CHAR('-')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP) PORT_CHAR('>') PORT_CHAR('.')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('?') PORT_CHAR('/')

	PORT_START("LINE4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('@')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) PORT_CHAR('G')

	PORT_START("LINE5")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) PORT_CHAR('O')

	PORT_START("LINE6")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) PORT_CHAR('W')

	PORT_START("LINE7")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('^')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RALT) PORT_CODE(KEYCODE_LALT) PORT_CHAR('_')
INPUT_PORTS_END

static INPUT_PORTS_START( homelab3 ) // F4 to F8 are foreign characters
	PORT_START("X0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("X1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED) // A
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED) // D
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("X2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_CUSTOM)   PORT_VBLANK("screen")
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left Shift")  PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right Shift") PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ALT") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("X3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_CUSTOM_MEMBER(DEVICE_SELF, homelab_state, cass3_r, " ")
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
	PORT_BIT(0xf8, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("X4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("X5")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("X6")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("X7")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('=') PORT_CHAR('-')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('?') PORT_CHAR('/')
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("X8")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('@')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F4)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("X9")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F5)
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("XA")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("XB")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("XC")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F6)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F7)
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("XD")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("XE")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F8)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("XF")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END


VIDEO_START_MEMBER( homelab_state )
{
	m_p_chargen = memregion("chargen")->base();
}

static SCREEN_UPDATE_IND16( homelab )
{
	homelab_state *state = screen.machine().driver_data<homelab_state>();
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;

	for(y = 0; y < 25; y++ )
	{
		for (ra = 0; ra < 8; ra++)
		{
			UINT16 *p = &bitmap.pix16(sy++);

			for (x = ma; x < ma + 40; x++)
			{
				chr = state->m_p_videoram[x]; // get char in videoram
				gfx = state->m_p_chargen[chr | (ra<<8)]; // get dot pattern in chargen

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
		ma+=40;
	}
	return 0;
}

static SCREEN_UPDATE_IND16( homelab3 )
{
	homelab_state *state = screen.machine().driver_data<homelab_state>();
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;

	for(y = 0; y < 32; y++ )
	{
		for (ra = 0; ra < 8; ra++)
		{
			UINT16 *p = &bitmap.pix16(sy++);

			for (x = ma; x < ma + 64; x++)
			{
				chr = state->m_p_videoram[x]; // get char in videoram
				gfx = state->m_p_chargen[chr | (ra<<8)]; // get dot pattern in chargen

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
		ma+=64;
	}
	return 0;
}

/* F4 Character Displayer */
static const gfx_layout homelab_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0, 1*256*8, 2*256*8, 3*256*8, 4*256*8, 5*256*8, 6*256*8, 7*256*8 },
	8					/* every char takes 8 x 1 bytes */
};

static GFXDECODE_START( homelab )
	GFXDECODE_ENTRY( "chargen", 0x0000, homelab_charlayout, 0, 1 )
GFXDECODE_END

static DRIVER_INIT(homelab)
{
}

/* Machine driver */
static MACHINE_CONFIG_START( homelab, homelab_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, XTAL_8MHz / 2)
	MCFG_CPU_PROGRAM_MAP(homelab2_mem)
	MCFG_CPU_VBLANK_INT("screen", homelab_frame)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MCFG_SCREEN_SIZE(40*8, 25*8)
	MCFG_SCREEN_VISIBLE_AREA(0, 40*8-1, 0, 25*8-1)
	MCFG_SCREEN_UPDATE_STATIC( homelab )
	MCFG_GFXDECODE(homelab)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(monochrome_green)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(SPEAKER_TAG, SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MCFG_SOUND_WAVE_ADD(WAVE_TAG, CASSETTE_TAG)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MCFG_CASSETTE_ADD( CASSETTE_TAG, default_cassette_interface )
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( homelab3, homelab_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, XTAL_12MHz / 4)
	MCFG_CPU_PROGRAM_MAP(homelab3_mem)
	MCFG_CPU_IO_MAP(homelab3_io)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MCFG_SCREEN_SIZE(64*8, 32*8)
	MCFG_SCREEN_VISIBLE_AREA(0, 64*8-1, 0, 32*8-1)
	MCFG_SCREEN_UPDATE_STATIC( homelab3 )
	MCFG_GFXDECODE(homelab)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(monochrome_green)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(SPEAKER_TAG, SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MCFG_SOUND_WAVE_ADD(WAVE_TAG, CASSETTE_TAG)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MCFG_CASSETTE_ADD( CASSETTE_TAG, default_cassette_interface )
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( brailab4, homelab3 )
	/* basic machine hardware */
	MCFG_CPU_MODIFY("maincpu")
	MCFG_CPU_PROGRAM_MAP(brailab4_mem)
MACHINE_CONFIG_END


/* ROM definition */

ROM_START( homelab2 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "hl2_1.rom", 0x0000, 0x0800, CRC(205365F7) SHA1(da93b65befd83513dc762663b234227ba804124d))
	ROM_LOAD( "hl2_2.rom", 0x0800, 0x0800, CRC(696AF3C1) SHA1(b53bc6ae2b75975618fc90e7181fa5d21409fce1))
	ROM_LOAD( "hl2_3.rom", 0x1000, 0x0800, CRC(69E57E8C) SHA1(e98510abb715dbf513e1b29fb6b09ab54e9483b7))
	ROM_LOAD( "hl2_4.rom", 0x1800, 0x0800, CRC(97CBBE74) SHA1(34f0bad41302b059322018abc3d1c2336ecfbea8))
	ROM_LOAD( "hl2_m.rom", 0x2000, 0x0800, BAD_DUMP CRC(10040235) SHA1(e121dfb97cc8ea99193a9396a9f7af08585e0ff0) )
	ROM_FILL(0x46, 1, 0x18) // fix bad code
	ROM_FILL(0x47, 1, 0x0E)

	ROM_REGION(0x0800, "chargen",0)
	ROM_LOAD ("hl2.chr", 0x0000, 0x0800, CRC(2E669D40) SHA1(639dd82ed29985dc69830aca3b904b6acc8fe54a))
	// found on net, looks like bad dump
	//ROM_LOAD_OPTIONAL( "hl2_ch.rom", 0x0800, 0x1000, CRC(6a5c915a) SHA1(7e4e966358556c6aabae992f4c2b292b6aab59bd) )
ROM_END

ROM_START( homelab3 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "hl3_1.rom", 0x0000, 0x1000, CRC(6B90A8EA) SHA1(8ac40ca889b8c26cdf74ca309fbafd70dcfdfbec))
	ROM_LOAD( "hl3_2.rom", 0x1000, 0x1000, CRC(BCAC3C24) SHA1(aff371d17f61cb60c464998e092f04d5d85c4d52))
	ROM_LOAD( "hl3_3.rom", 0x2000, 0x1000, CRC(AB1B4AB0) SHA1(ad74c7793f5dc22061a88ef31d3407267ad08719))
	ROM_LOAD( "hl3_4.rom", 0x3000, 0x1000, CRC(BF67EFF9) SHA1(2ef5d46f359616e7d0e5a124df528de44f0e850b))

	ROM_REGION(0x0800, "chargen",0)
	ROM_LOAD ("hl3.chr", 0x0000, 0x0800, CRC(F58EE39B) SHA1(49399c42d60a11b218a225856da86a9f3975a78a))
ROM_END

ROM_START( homelab4 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "hl4_1.rom", 0x0000, 0x1000, CRC(A549B2D4) SHA1(90fc5595da8431616aee56eb5143b9f04281e798))
	ROM_LOAD( "hl4_2.rom", 0x1000, 0x1000, CRC(151D33E8) SHA1(d32004bc1553f802b9d3266709552f7d5315fe44))
	ROM_LOAD( "hl4_3.rom", 0x2000, 0x1000, CRC(39571AB1) SHA1(8470cff2e3442101e6a0bc655358b3a6fc1ef944))
	ROM_LOAD( "hl4_4.rom", 0x3000, 0x1000, CRC(F4B77CA2) SHA1(ffbdb3c1819c7357e2a0fc6317c111a8a7ecfcd5))

	ROM_REGION(0x0800, "chargen",0)
	ROM_LOAD ("hl4.chr", 0x0000, 0x0800, CRC(F58EE39B) SHA1(49399c42d60a11b218a225856da86a9f3975a78a))
ROM_END

ROM_START( brailab4 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "brl1.rom", 0x0000, 0x1000, CRC(02323403) SHA1(3a2e853e0a39e05a04a8db58e1a76de1eda579c9))
	ROM_LOAD( "brl2.rom", 0x1000, 0x1000, CRC(36173fbc) SHA1(1c01398e16a1cbe4103e1be769347ceae873e090))
	ROM_LOAD( "brl3.rom", 0x2000, 0x1000, CRC(d3cdd108) SHA1(1a24e6c5f9c370ff6cb25045cb9d95e664467eb5))
	ROM_LOAD( "brl4.rom", 0x3000, 0x1000, CRC(d4047885) SHA1(00fe40c4c2c64a49bb429fb2b27cc7e0d0025a85))
	ROM_FILL(0x220,1,0xc9) // do not delete the stack

	ROM_REGION(0x0800, "chargen",0)
	ROM_LOAD ("hl4.chr", 0x0000, 0x0800, CRC(F58EE39B) SHA1(49399c42d60a11b218a225856da86a9f3975a78a))

	// these roms were found on the net, to be investigated
	ROM_REGION( 0xa020, "user1", 0 )
	// unknown
	ROM_LOAD_OPTIONAL( "brl5.rom",        0x0000, 0x1000, CRC(8a76be04) SHA1(4b683b9be23b47117901fe874072eb7aa481e4ff) )
	// 256k Homelab 1000  Bios 1.2, 1988
	ROM_LOAD_OPTIONAL( "brailabplus.bin", 0x1000, 0x4000, CRC(521d6952) SHA1(f7405520d86fc7abd2dec51d1d016658472f6fe8) )
	// probably brl1 to 5 merged
	ROM_LOAD_OPTIONAL( "brl.rom",         0x5000, 0x5000, CRC(54af5d30) SHA1(d1e7b7f5866acba0503d47f610456f396526240b) )
	// a small prom
	ROM_LOAD_OPTIONAL( "brlcpm.rom",      0xa000, 0x0020, CRC(b936d568) SHA1(150330eccbc4b664eba4103f051d6e932038e9e8) )
ROM_END

/* Driver */

/*    YEAR  NAME        PARENT     COMPAT  MACHINE      INPUT      INIT      COMPANY                    FULLNAME   FLAGS */
COMP( 1982, homelab2,   0,         0,      homelab,     homelab,   homelab, "Jozsef and Endre Lukacs", "Homelab 2 / Aircomp 16", GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1983, homelab3,   homelab2,  0,      homelab3,    homelab3,  homelab, "Jozsef and Endre Lukacs", "Homelab 3", GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1984, homelab4,   homelab2,  0,      homelab3,    homelab3,  homelab, "Jozsef and Endre Lukacs", "Homelab 4", GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1984, brailab4,   homelab2,  0,      brailab4,    homelab3,  homelab, "Jozsef and Endre Lukacs", "Brailab 4", GAME_NOT_WORKING | GAME_NO_SOUND)
