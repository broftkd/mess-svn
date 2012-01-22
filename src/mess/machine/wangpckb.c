/*

Wang Professional Computer

Keyboard PCB Layout
-------------------

Key Tronic A65-02454-002
PCB 201

|-----------------------------------------------------------------------|
|        LS273  PSG  555  LS04  4MHz  LS132  CN1            LS374   XR  |
|                                                    CPU                |
|     LD1           LD2           LD3           LD4           LD5       |
|        SW1              LS259              SW2          LS259         |
|                                                                       |
|                                                                       |
|                                                                       |
|                                                                       |
|                                                                       |
|                                                                       |
|                                                                       |
|-----------------------------------------------------------------------|

Notes:
    Relevant IC's shown.

    CPU         - Intel P8051AH "20-8051-225"
    PSG         - Texas Instruments SN76496N
	XR          - Exar Semiconductor XR22-908-030? (hard to read)
    CN1         - 1x6 pin PCB header
    SW1         - 8-way DIP switch
    SW2         - 8-way DIP switch

*/

#include "wangpckb.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define I8051_TAG		"z5"
#define SN76496_TAG		"z4"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type WANGPC_KEYBOARD = &device_creator<wangpc_keyboard_device>;



//-------------------------------------------------
//  ROM( wangpc_keyboard )
//-------------------------------------------------

ROM_START( wangpc_keyboard )
	ROM_REGION( 0x1000, I8051_TAG, 0 )
	ROM_LOAD( "20-8051-225.z5", 0x0000, 0x1000, CRC(82d2999f) SHA1(2bb34a1de2d94b2885d9e8fcd4964296f6276c5c) )
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *wangpc_keyboard_device::device_rom_region() const
{
	return ROM_NAME( wangpc_keyboard );
}


//-------------------------------------------------
//  ADDRESS_MAP( wangpc_keyboard_io )
//-------------------------------------------------

static ADDRESS_MAP_START( wangpc_keyboard_io, AS_IO, 8, wangpc_keyboard_device )
	AM_RANGE(MCS51_PORT_P1, MCS51_PORT_P1) AM_READWRITE(kb_p1_r, kb_p1_w)
	AM_RANGE(MCS51_PORT_P2, MCS51_PORT_P2) AM_WRITE(kb_p2_w)
	AM_RANGE(MCS51_PORT_P3, MCS51_PORT_P3) AM_WRITE(kb_p3_w)
	AM_RANGE(0x4848, 0x4848) AM_READ_PORT("Y0")
	AM_RANGE(0x4949, 0x4949) AM_READ_PORT("Y1")
	AM_RANGE(0x4a4a, 0x4a4a) AM_READ_PORT("Y2")
	AM_RANGE(0x4b4b, 0x4b4b) AM_READ_PORT("Y3")
	AM_RANGE(0x4c4c, 0x4c4c) AM_READ_PORT("Y4")
	AM_RANGE(0x4d4d, 0x4d4d) AM_READ_PORT("Y5")
	AM_RANGE(0x4e4e, 0x4e4e) AM_READ_PORT("Y6")
	AM_RANGE(0x4f4f, 0x4f4f) AM_READ_PORT("Y7")
	AM_RANGE(0x5050, 0x5050) AM_READ_PORT("Y8")
	AM_RANGE(0x5151, 0x5151) AM_READ_PORT("Y9")
	AM_RANGE(0x5252, 0x5252) AM_READ_PORT("YA")
	AM_RANGE(0x5353, 0x5353) AM_READ_PORT("YB")
	AM_RANGE(0x5454, 0x5454) AM_READ_PORT("YC")
	AM_RANGE(0x5555, 0x5555) AM_READ_PORT("YD")
	AM_RANGE(0x5656, 0x5656) AM_READ_PORT("YE")
	AM_RANGE(0x5757, 0x5757) AM_READ_PORT("YF")
ADDRESS_MAP_END


//-------------------------------------------------
//  MACHINE_DRIVER( wangpc_keyboard )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( wangpc_keyboard )
	MCFG_CPU_ADD(I8051_TAG, I8051, XTAL_4MHz)
	MCFG_CPU_IO_MAP(wangpc_keyboard_io)

	// sound hardware
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(SN76496_TAG, SN76496, 2000000) // ???
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor wangpc_keyboard_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( wangpc_keyboard );
}


//-------------------------------------------------
//  INPUT_PORTS( wangpc_keyboard )
//-------------------------------------------------

INPUT_PORTS_START( wangpc_keyboard )
    PORT_START("Y0")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

    PORT_START("Y1")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

    PORT_START("Y2")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

    PORT_START("Y3")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

    PORT_START("Y4")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

    PORT_START("Y5")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

    PORT_START("Y6")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

    PORT_START("Y7")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

    PORT_START("Y8")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

    PORT_START("Y9")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

    PORT_START("YA")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

    PORT_START("YB")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

    PORT_START("YC")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

    PORT_START("YD")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

    PORT_START("YE")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

    PORT_START("YF")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("SW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("SW2")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW2:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW2:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW2:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW2:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW2:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW2:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

ioport_constructor wangpc_keyboard_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( wangpc_keyboard );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  wangpc_keyboard_device - constructor
//-------------------------------------------------

wangpc_keyboard_device::wangpc_keyboard_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, WANGPC_KEYBOARD, "Wang PC Keyboard", tag, owner, clock),
	  m_maincpu(*this, I8051_TAG)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void wangpc_keyboard_device::device_start()
{
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void wangpc_keyboard_device::device_reset()
{
}


//-------------------------------------------------
//  kb_p1_r -
//-------------------------------------------------

READ8_MEMBER( wangpc_keyboard_device::kb_p1_r )
{
	return 0xff;
}


//-------------------------------------------------
//  kb_p1_w -
//-------------------------------------------------

WRITE8_MEMBER( wangpc_keyboard_device::kb_p1_w )
{
}


//-------------------------------------------------
//  kb_p2_w -
//-------------------------------------------------

WRITE8_MEMBER( wangpc_keyboard_device::kb_p2_w )
{
}


//-------------------------------------------------
//  kb_p3_w -
//-------------------------------------------------

WRITE8_MEMBER( wangpc_keyboard_device::kb_p3_w )
{
}
