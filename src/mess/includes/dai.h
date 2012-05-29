/*****************************************************************************
 *
 * includes/dai.h
 *
 ****************************************************************************/

#ifndef DAI_H_
#define DAI_H_

#include "machine/i8255.h"
#include "machine/tms5501.h"

#define DAI_DEBUG	1


class dai_state : public driver_device
{
public:
	dai_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_pit(*this, "pit8253")
		{ }

	required_device<device_t> m_pit;

	UINT8 m_paddle_select;
	UINT8 m_paddle_enable;
	UINT8 m_cassette_motor[2];
	device_t *m_sound;
	device_t *m_tms5501;
	UINT8 m_keyboard_scan_mask;
	unsigned short m_4_colours_palette[4];
	DECLARE_WRITE8_MEMBER(dai_stack_interrupt_circuit_w);
	DECLARE_READ8_MEMBER(dai_io_discrete_devices_r);
	DECLARE_WRITE8_MEMBER(dai_io_discrete_devices_w);
	DECLARE_READ8_MEMBER(dai_amd9511_r);
	DECLARE_WRITE8_MEMBER(dai_amd9511_w);
	DECLARE_READ8_MEMBER(dai_pit_r);
	DECLARE_WRITE8_MEMBER(dai_pit_w);
};


/*----------- defined in machine/dai.c -----------*/

extern const struct pit8253_config dai_pit8253_intf;
extern const i8255_interface dai_ppi82555_intf;
extern const tms5501_interface dai_tms5501_interface;

MACHINE_START( dai );
MACHINE_RESET( dai );


/*----------- defined in video/dai.c -----------*/

extern const unsigned char dai_palette[16*3];

VIDEO_START( dai );
SCREEN_UPDATE_IND16( dai );
PALETTE_INIT( dai );


/*----------- defined in audio/dai.c -----------*/

DECLARE_LEGACY_SOUND_DEVICE(DAI, dai_sound);

void dai_set_input(device_t *device, int index, int state);
void dai_set_volume(device_t *device, int offset, UINT8 data);


#endif /* DAI_H_ */
