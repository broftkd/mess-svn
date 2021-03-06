/***************************************************************************

    dragon.h

    Dragon Family

Dragon Alpha code added 21-Oct-2004,
            Phill Harvey-Smith (afra@aurigae.demon.co.uk)

            Added AY-8912 and FDC code 30-Oct-2004.

Fixed Dragon Alpha NMI enable/disable, following circuit traces on a real machine.
    P.Harvey-Smith, 11-Aug-2005.

Re-implemented Alpha NMI enable/disable, using direct PIA reads, rather than
keeping track of it in a variable in the driver.
    P.Harvey-Smith, 25-Sep-2006.

Radically re-wrote memory emulation code for CoCo 1/2 & Dragon machines, the
new code emulates the memory mapping of the SAM, dependent on what size of
RAM chips it is programed to use, including proper mirroring of the RAM.

Replaced the kludged emulation of the cart line, with a timer based trigger
this is set to toggle at 1Hz, this seems to be good enough to trigger the
cartline, but is so slow in real terms that it should have very little
impact on the emulation speed.

Re-factored the code common to all machines, and seperated the code different,
into callbacks/functions unique to the machines, in preperation for splitting
the code for individual machine types into seperate files, I have preposed, that
the CoCo 1/2 should stay in coco.c, and that the coco3 and dragon specifc code
should go into coco3.c and dragon.c which should (hopefully) make the code
easier to manage.
    P.Harvey-Smith, Dec 2006-Feb 2007

***************************************************************************/

#include "includes/dragon.h"


/***************************************************************************
  DRAGON32
***************************************************************************/

//-------------------------------------------------
//  ctor
//-------------------------------------------------

dragon_state::dragon_state(const machine_config &mconfig, device_type type, const char *tag)
	: coco12_state(mconfig, type, tag),
	  m_printer(*this, PRINTER_TAG)
{
}



//-------------------------------------------------
//  pia1_pa_changed - called when PIA1 PA changes
//-------------------------------------------------

void dragon_state::pia1_pa_changed(void)
{
	/* call inherited function */
	coco12_state::pia1_pa_changed();

	/* if strobe bit is high send data from pia0 port b to dragon parallel printer */
	if (m_pia_1->a_output() & 0x02)
	{
		UINT8 output = m_pia_1->b_output();
		m_printer->output(output);
	}
}


/***************************************************************************
  DRAGON64
***************************************************************************/

//-------------------------------------------------
//  ctor
//-------------------------------------------------

dragon64_state::dragon64_state(const machine_config &mconfig, device_type type, const char *tag)
	: dragon_state(mconfig, type, tag),
	  m_acia(*this, ACIA_TAG)
{
}



//-------------------------------------------------
//  ff00_read
//-------------------------------------------------

READ8_MEMBER( dragon64_state::ff00_read )
{
	UINT8 result = 0x00;

	switch(offset & 0x07)
	{
		case 0: case 1: case 2: case 3:
			result = dragon_state::ff00_read(space, offset, mem_mask);
			break;

		case 4: case 5: case 6: case 7:
			result = m_acia->read(space, offset, mem_mask);
			break;
	}
	return result;
}



//-------------------------------------------------
//  ff00_write
//-------------------------------------------------

WRITE8_MEMBER( dragon64_state::ff00_write )
{
	switch(offset & 0x07)
	{
		case 0: case 1: case 2: case 3:
			dragon_state::ff00_write(space, offset, data, mem_mask);
			break;

		case 4: case 5: case 6: case 7:
			m_acia->write(space, offset, data, mem_mask);
			break;
	}
}



//-------------------------------------------------
//  pia1_pb_changed
//-------------------------------------------------

void dragon64_state::pia1_pb_changed(void)
{
	dragon_state::pia1_pb_changed();

	UINT8 ddr = ~m_pia_1->port_b_z_mask();

	/* If bit 2 of the pia1 ddrb is 1 then this pin is an output so use it */
	/* to control the paging of the 32k and 64k basic roms */
	/* Otherwise it set as an input, with an EXTERNAL pull-up so it should */
	/* always be high (enabling 32k basic rom) */
	if (ddr & 0x04)
	{
		page_rom(m_pia_1->b_output() & 0x04 ? true : false);
	}
}



//-------------------------------------------------
//  page_rom - Controls rom paging in Dragon 64,
//  and Dragon Alpha.
//
//  On 64, switches between the two versions of the
//  basic rom mapped in at 0x8000
//
//  On the alpha switches between the
//  Boot/Diagnostic rom and the basic rom
//-------------------------------------------------

void dragon64_state::page_rom(bool romswitch)
{
	offs_t offset = romswitch
		? 0x0000	// This is the 32k mode basic(64)/boot rom(alpha)
		: 0x8000;	// This is the 64k mode basic(64)/basic rom(alpha)
	m_sam->set_bank_offset(1, offset);
	m_sam->set_bank_offset(2, offset);
	m_sam->set_bank_offset(3, offset);
}
