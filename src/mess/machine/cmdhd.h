/**********************************************************************

    CMD HD disk drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __CMD_HD__
#define __CMD_HD__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "imagedev/harddriv.h"
#include "machine/6522via.h"
#include "machine/cbmiec.h"
#include "machine/i8255.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define CMD_HD_TAG			"cmdhd"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> cmd_hd_device

class cmd_hd_device :  public device_t,
					   public device_cbm_iec_interface
{

public:
    // construction/destruction
    cmd_hd_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_reset();
    virtual void device_config_complete() { m_shortname = "cmdhd"; }

	// device_cbm_iec_interface overrides
	void cbm_iec_srq(int state);
	void cbm_iec_atn(int state);
	void cbm_iec_data(int state);
	void cbm_iec_reset(int state);

	required_device<cpu_device> m_maincpu;
};


// device type definition
extern const device_type CMD_HD;



#endif
