/**********************************************************************

    RCA Cosmac VIP Expansion Interface emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "machine/vip_exp.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************

#define LOG 0



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type VIP_EXPANSION_SLOT = &device_creator<vip_expansion_slot_device>;



//**************************************************************************
//  DEVICE VIP_EXPANSION CARD INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_vip_expansion_card_interface - constructor
//-------------------------------------------------

device_vip_expansion_card_interface::device_vip_expansion_card_interface(const machine_config &mconfig, device_t &device)
	: device_slot_card_interface(mconfig, device)
{
	m_slot = dynamic_cast<vip_expansion_slot_device *>(device.owner());
}


//-------------------------------------------------
//  ~device_vip_expansion_card_interface - destructor
//-------------------------------------------------

device_vip_expansion_card_interface::~device_vip_expansion_card_interface()
{
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  vip_expansion_slot_device - constructor
//-------------------------------------------------

vip_expansion_slot_device::vip_expansion_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, VIP_EXPANSION_SLOT, "VIP expansion port", tag, owner, clock),
		device_slot_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  vip_expansion_slot_device - destructor
//-------------------------------------------------

vip_expansion_slot_device::~vip_expansion_slot_device()
{
}


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void vip_expansion_slot_device::device_config_complete()
{
	// inherit a copy of the static data
	const vip_expansion_slot_interface *intf = reinterpret_cast<const vip_expansion_slot_interface *>(static_config());
	if (intf != NULL)
	{
		*static_cast<vip_expansion_slot_interface *>(this) = *intf;
	}

	// or initialize to defaults if none provided
	else
	{
    	memset(&m_out_interrupt_cb, 0, sizeof(m_out_interrupt_cb));
    	memset(&m_out_dma_out_cb, 0, sizeof(m_out_dma_out_cb));
    	memset(&m_out_dma_in_cb, 0, sizeof(m_out_dma_in_cb));
	}
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void vip_expansion_slot_device::device_start()
{
	m_cart = dynamic_cast<device_vip_expansion_card_interface *>(get_card_device());

	// resolve callbacks
	m_out_interrupt_func.resolve(m_out_interrupt_cb, *this);
	m_out_dma_out_func.resolve(m_out_dma_out_cb, *this);
	m_out_dma_in_func.resolve(m_out_dma_in_cb, *this);
}


//-------------------------------------------------
//  program_r - program read
//-------------------------------------------------

UINT8 vip_expansion_slot_device::program_r(address_space &space, offs_t offset, int cdef, int *indis)
{
	UINT8 data = 0;

	if (m_cart != NULL)
	{
		data = m_cart->vip_program_r(space, offset, cdef, indis);
	}

	return data;
}


//-------------------------------------------------
//  program_w - program write
//-------------------------------------------------

void vip_expansion_slot_device::program_w(address_space &space, offs_t offset, UINT8 data, int cdef)
{
	if (m_cart != NULL)
	{
		m_cart->vip_program_w(space, offset, data, cdef);
	}
}


//-------------------------------------------------
//  screen_update -
//-------------------------------------------------

UINT32 vip_expansion_slot_device::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bool value = false;

	if (m_cart != NULL)
	{
		value = m_cart->vip_screen_update(screen, bitmap, cliprect);
	}

	return value;
}

READ_LINE_MEMBER( vip_expansion_slot_device::ef1_r ) { int state = 1; if (m_cart != NULL) state = m_cart->vip_ef1_r(); return state; }
READ_LINE_MEMBER( vip_expansion_slot_device::ef3_r ) { int state = 1; if (m_cart != NULL) state = m_cart->vip_ef3_r(); return state; }
READ_LINE_MEMBER( vip_expansion_slot_device::ef4_r ) { int state = 1; if (m_cart != NULL) state = m_cart->vip_ef4_r(); return state; }
WRITE_LINE_MEMBER( vip_expansion_slot_device::sc0_w ) { if (m_cart != NULL) m_cart->vip_sc0_w(state); }
WRITE_LINE_MEMBER( vip_expansion_slot_device::sc1_w ) { if (m_cart != NULL) m_cart->vip_sc1_w(state); }
WRITE_LINE_MEMBER( vip_expansion_slot_device::q_w ) { if (m_cart != NULL) m_cart->vip_q_w(state); }

WRITE_LINE_MEMBER( vip_expansion_slot_device::interrupt_w ) { m_out_interrupt_func(state); }
WRITE_LINE_MEMBER( vip_expansion_slot_device::dma_out_w ) { m_out_dma_out_func(state); }
WRITE_LINE_MEMBER( vip_expansion_slot_device::dma_in_w ) { m_out_dma_in_func(state); }