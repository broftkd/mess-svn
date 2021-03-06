/*************************************************************************


    Taito Z system

*************************************************************************/

#include "machine/eeprom.h"

class taitoz_state : public driver_device
{
public:
	taitoz_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_spriteram(*this, "spriteram")
	{ }

	/* memory pointers */
	required_shared_ptr<UINT16> m_spriteram;

	/* video-related */
	int         m_sci_spriteframe;
	int         m_road_palbank;

	/* misc */
	INT32       m_banknum;
	UINT16      m_cpua_ctrl;
	INT32       m_sci_int6;
	INT32       m_ioc220_port;
	UINT16      m_eep_latch;

	/* devices */
	device_t *m_maincpu;
	device_t *m_audiocpu;
	device_t *m_subcpu;
	eeprom_device *m_eeprom;
	device_t *m_tc0480scp;
	device_t *m_tc0150rod;
	device_t *m_tc0100scn;
	device_t *m_tc0220ioc;
	device_t *m_tc0140syt;

	DECLARE_WRITE16_MEMBER(cpua_ctrl_w);
	DECLARE_WRITE16_MEMBER(chasehq_cpua_ctrl_w);
	DECLARE_WRITE16_MEMBER(dblaxle_cpua_ctrl_w);
	DECLARE_READ16_MEMBER(eep_latch_r);
	DECLARE_WRITE16_MEMBER(spacegun_output_bypass_w);
	DECLARE_READ8_MEMBER(contcirc_input_bypass_r);
	DECLARE_READ8_MEMBER(chasehq_input_bypass_r);
	DECLARE_READ16_MEMBER(bshark_stick_r);
	DECLARE_READ16_MEMBER(nightstr_stick_r);
	DECLARE_WRITE16_MEMBER(bshark_stick_w);
	DECLARE_READ16_MEMBER(sci_steer_input_r);
	DECLARE_READ16_MEMBER(spacegun_input_bypass_r);
	DECLARE_READ16_MEMBER(spacegun_lightgun_r);
	DECLARE_WRITE16_MEMBER(spacegun_lightgun_w);
	DECLARE_WRITE16_MEMBER(spacegun_gun_output_w);
	DECLARE_READ16_MEMBER(dblaxle_steer_input_r);
	DECLARE_READ16_MEMBER(chasehq_motor_r);
	DECLARE_WRITE16_MEMBER(chasehq_motor_w);
	DECLARE_WRITE16_MEMBER(nightstr_motor_w);
	DECLARE_READ16_MEMBER(aquajack_unknown_r);
	DECLARE_WRITE8_MEMBER(sound_bankswitch_w);
	DECLARE_WRITE16_MEMBER(taitoz_sound_w);
	DECLARE_READ16_MEMBER(taitoz_sound_r);
	DECLARE_WRITE16_MEMBER(taitoz_msb_sound_w);
	DECLARE_READ16_MEMBER(taitoz_msb_sound_r);
	DECLARE_WRITE8_MEMBER(taitoz_pancontrol);
	DECLARE_WRITE16_MEMBER(spacegun_pancontrol);
	DECLARE_READ16_MEMBER(sci_spriteframe_r);
	DECLARE_WRITE16_MEMBER(sci_spriteframe_w);
	DECLARE_WRITE16_MEMBER(contcirc_out_w);
	DECLARE_CUSTOM_INPUT_MEMBER(taitoz_pedal_r);
	DECLARE_DRIVER_INIT(taitoz);
	DECLARE_DRIVER_INIT(bshark);
};

/*----------- defined in video/taito_z.c -----------*/


VIDEO_START( taitoz );

SCREEN_UPDATE_IND16( contcirc );
SCREEN_UPDATE_IND16( chasehq );
SCREEN_UPDATE_IND16( bshark );
SCREEN_UPDATE_IND16( sci );
SCREEN_UPDATE_IND16( aquajack );
SCREEN_UPDATE_IND16( spacegun );
SCREEN_UPDATE_IND16( dblaxle );
SCREEN_UPDATE_IND16( racingb );
