/**********************************************************************

    Western Digital WD11C00-17 PC/XT Host Interface Logic Device

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "machine/wd11c00_17.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define LOG 1


// status register
#define STATUS_IRQ		0x20
#define STATUS_DRQ		0x10
#define STATUS_BUSY		0x08
#define STATUS_C_D		0x04
#define STATUS_I_O		0x02
#define STATUS_REQ		0x01


// mask register
#define MASK_IRQ		0x02
#define MASK_DMA		0x01



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type WD11C00_17 = &device_creator<wd11c00_17_device>;


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void wd11c00_17_device::device_config_complete()
{
	// inherit a copy of the static data
	const wd11c00_17_interface *intf = reinterpret_cast<const wd11c00_17_interface *>(static_config());
	if (intf != NULL)
		*static_cast<wd11c00_17_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		memset(&m_out_irq5_cb, 0, sizeof(m_out_irq5_cb));
		memset(&m_out_drq3_cb, 0, sizeof(m_out_drq3_cb));
		memset(&m_out_mr_cb, 0, sizeof(m_out_mr_cb));
		memset(&m_out_busy_cb, 0, sizeof(m_out_busy_cb));
		memset(&m_out_req_cb, 0, sizeof(m_out_req_cb));
		memset(&m_out_ra3_cb, 0, sizeof(m_out_ra3_cb));
		memset(&m_in_rd322_cb, 0, sizeof(m_in_rd322_cb));
		memset(&m_in_ramcs_cb, 0, sizeof(m_in_ramcs_cb));
		memset(&m_out_ramwr_cb, 0, sizeof(m_out_ramwr_cb));
		memset(&m_in_cs1010_cb, 0, sizeof(m_in_cs1010_cb));
		memset(&m_out_cs1010_cb, 0, sizeof(m_out_cs1010_cb));
	}
}



//**************************************************************************
//  INLINE HELPERS
//**************************************************************************

//-------------------------------------------------
//  check_interrupt -
//-------------------------------------------------

inline void wd11c00_17_device::check_interrupt()
{
	int irq = ((m_status & STATUS_IRQ) && (m_mask & MASK_IRQ)) ? ASSERT_LINE : CLEAR_LINE;
	int drq = ((m_status & STATUS_DRQ) && (m_mask & MASK_DMA)) ? ASSERT_LINE : CLEAR_LINE;
	int busy = (m_status & STATUS_BUSY) ? ASSERT_LINE : CLEAR_LINE;
	int req = (m_status & STATUS_REQ) ? ASSERT_LINE : CLEAR_LINE;
	
	m_out_irq5_func(irq);
	m_out_drq3_func(drq);
	m_out_busy_func(busy);
	m_out_req_func(req);
}


//-------------------------------------------------
//  increment_address -
//-------------------------------------------------

inline void wd11c00_17_device::increment_address()
{
	int old_ra3 = BIT(m_ra, 1);
	
	m_ra++;
	
	if (BIT(m_ra, 10))
	{
		m_status &= ~STATUS_DRQ;
		
		check_interrupt();
	}
	
	int ra3 = BIT(m_ra, 1);
	
	if (old_ra3 != ra3)
	{
		m_out_ra3_func(ra3 ? ASSERT_LINE : CLEAR_LINE);
	}
}


//-------------------------------------------------
//  read_data -
//-------------------------------------------------

inline UINT8 wd11c00_17_device::read_data()
{
	UINT8 data = 0;
	
	if (m_status & STATUS_BUSY)
	{
		data = m_in_ramcs_func(m_ra & 0x3ff);
		
		increment_address();
	}

	return data;
}


//-------------------------------------------------
//  write_data -
//-------------------------------------------------

inline void wd11c00_17_device::write_data(UINT8 data)
{
	if (m_status & STATUS_BUSY)
	{
		m_out_ramwr_func(m_ra & 0x3ff, data);
		
		increment_address();
	}
}


//-------------------------------------------------
//  software_reset -
//-------------------------------------------------

inline void wd11c00_17_device::software_reset()
{
	m_out_mr_func(ASSERT_LINE);
	m_out_mr_func(CLEAR_LINE);
	
	device_reset();
}


//-------------------------------------------------
//  select -
//-------------------------------------------------

inline void wd11c00_17_device::select()
{
	m_status |= STATUS_BUSY | STATUS_C_D | STATUS_REQ;
	
	check_interrupt();
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  wd11c00_17_device - constructor
//-------------------------------------------------

wd11c00_17_device::wd11c00_17_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, WD11C00_17, "Western Digital WD11C00-17", tag, owner, clock),
	  m_status(0)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void wd11c00_17_device::device_start()
{
	// resolve callbacks
	m_out_irq5_func.resolve(m_out_irq5_cb, *this);
	m_out_drq3_func.resolve(m_out_drq3_cb, *this);
	m_out_mr_func.resolve(m_out_mr_cb, *this);
	m_out_busy_func.resolve(m_out_busy_cb, *this);
	m_out_req_func.resolve(m_out_req_cb, *this);
	m_out_ra3_func.resolve(m_out_ra3_cb, *this);
	m_in_rd322_func.resolve(m_in_rd322_cb, *this);
	m_in_ramcs_func.resolve(m_in_ramcs_cb, *this);
	m_out_ramwr_func.resolve(m_out_ramwr_cb, *this);
	m_in_cs1010_func.resolve(m_in_cs1010_cb, *this);
	m_out_cs1010_func.resolve(m_out_cs1010_cb, *this);
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void wd11c00_17_device::device_reset()
{
	m_status &= ~(STATUS_IRQ | STATUS_DRQ | STATUS_BUSY);
	m_mask = 0;
	m_ra = 0;
	
	check_interrupt();
}


//-------------------------------------------------
//  io_r -
//-------------------------------------------------

READ8_MEMBER( wd11c00_17_device::io_r )
{
	UINT8 data = 0xff;
	
	switch (offset)
	{
	case 0: // Read Data, Board to Host
		data = read_data();
		break;

	case 1: // Read Board Hardware Status
		data = m_status;
		break;

	case 2: // Read Drive Configuration Information
		data = m_in_rd322_func(0);
		break;

	case 3: // Not Used
		break;
	}
	
	return data;
}


//-------------------------------------------------
//  io_w -
//-------------------------------------------------

WRITE8_MEMBER( wd11c00_17_device::io_w )
{
	switch (offset)
	{
	case 0: // Write Data, Host to Board
		if (LOG) logerror("%s WD11C00-17 '%s' Write Data %03x:%02x\n", machine().describe_context(), tag(), m_ra, data);
		write_data(data);
		break;

	case 1: // Board Software Reset
		if (LOG) logerror("%s WD11C00-17 '%s' Software Reset\n", machine().describe_context(), tag());
		software_reset();
		break;

	case 2:	// Board Select
		if (LOG) logerror("%s WD11C00-17 '%s' Select\n", machine().describe_context(), tag());
		select();
		break;

	case 3: // Set/Reset DMA, IRQ Masks
		if (LOG) logerror("%s WD11C00-17 '%s' Mask IRQ %u DMA %u\n", machine().describe_context(), tag(), BIT(data, 1), BIT(data, 0));
		m_mask = data;
		check_interrupt();
		break;
	}
}


//-------------------------------------------------
//  dack_r -
//-------------------------------------------------

UINT8 wd11c00_17_device::dack_r()
{
	return read_data();
}


//-------------------------------------------------
//  dack_w -
//-------------------------------------------------

void wd11c00_17_device::dack_w(UINT8 data)
{
	write_data(data);
}


//-------------------------------------------------
//  read -
//-------------------------------------------------

READ8_MEMBER( wd11c00_17_device::read )
{
	UINT8 data = 0;
	
	switch (offset)
	{
	case 0x00:
		data = read_data();
		break;
		
	case 0x20:
		data = m_in_cs1010_func(m_ra >> 8);
		break;
	}
	
	return data;
}


//-------------------------------------------------
//  write -
//-------------------------------------------------

WRITE8_MEMBER( wd11c00_17_device::write )
{
	switch (offset)
	{
	case 0x00:
		write_data(data);
		break;
		
	case 0x20:
		m_out_cs1010_func(m_ra >> 8, data);
		break;
		
	case 0x60:
		m_ra = ((data & 0x07) << 8) | (m_ra & 0xff);
		break;
	}
}


//-------------------------------------------------
//  ireq_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( wd11c00_17_device::ireq_w )
{
	if (LOG) logerror("%s WD11C00-17 '%s' IREQ %u\n", machine().describe_context(), tag(), state);
	
	if (state) m_status |= STATUS_REQ; else m_status &= ~STATUS_REQ;
}


//-------------------------------------------------
//  io_w -
//-------------------------------------------------
	
WRITE_LINE_MEMBER( wd11c00_17_device::io_w )
{
	if (LOG) logerror("%s WD11C00-17 '%s' I/O %u\n", machine().describe_context(), tag(), state);

	if (state) m_status |= STATUS_I_O; else m_status &= ~STATUS_I_O;
}


//-------------------------------------------------
//  cd_w -
//-------------------------------------------------
	
WRITE_LINE_MEMBER( wd11c00_17_device::cd_w )
{
	if (LOG) logerror("%s WD11C00-17 '%s' C/D %u\n", machine().describe_context(), tag(), state);
	
	if (state) m_status |= STATUS_C_D; else m_status &= ~STATUS_C_D;
}


//-------------------------------------------------
//  clct_w -
//-------------------------------------------------
	
WRITE_LINE_MEMBER( wd11c00_17_device::clct_w )
{
	if (LOG) logerror("%s WD11C00-17 '%s' CLCT %u\n", machine().describe_context(), tag(), state);
	
	if (state)
	{
		m_ra &= 0xff00;
	}
}