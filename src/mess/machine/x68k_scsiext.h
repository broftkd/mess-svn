/*
 * x68k_scsiext.h
 *
 *  Created on: 5/06/2012
 */

#ifndef X68K_SCSIEXT_H_
#define X68K_SCSIEXT_H_

#include "emu.h"
#include "machine/mb89352.h"
#include "machine/x68kexp.h"

class x68k_scsiext_device : public device_t,
							public device_x68k_expansion_card_interface
{
public:
	// construction/destruction
	x68k_scsiext_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual machine_config_constructor device_mconfig_additions() const;
    virtual const rom_entry *device_rom_region() const;

	void irq_w(int state);
	void drq_w(int state);
	DECLARE_READ8_MEMBER(register_r);
	DECLARE_WRITE8_MEMBER(register_w);

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
    virtual void device_config_complete() { m_shortname = "x68k_cz6bs1"; }
private:
	x68k_expansion_slot_device *m_slot;

	required_device<mb89352_device> m_spc;
};

// device type definition
extern const device_type X68K_SCSIEXT;


#endif /* X68K_SCSIEXT_H_ */
