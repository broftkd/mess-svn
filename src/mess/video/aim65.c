/******************************************************************************

    PeT mess@utanet.at Nov 2000
    Updated by Dan Boris, 3/4/2007

******************************************************************************/

#include "includes/aim65.h"

//UINT16 *printerRAM;




/******************************************************************************
 Printer
******************************************************************************/

/*
  aim65 thermal printer (20 characters)
  10 heat elements (place on 1 line, space between 2 characters(about 14dots))
  (pa0..pa7,pb0,pb1 1 heat element on)

  cb2 0 motor, heat elements on
  cb1 output start!?
  ca1 input

  normally printer 5x7 characters
  (horizontal movement limits not known, normally 2 dots between characters)

  3 dots space between lines?
 */


static TIMER_CALLBACK(aim65_printer_timer)
{
	aim65_state *state = machine.driver_data<aim65_state>();
	via6522_device *via_0 = machine.device<via6522_device>("via6522_0");

	via_0->write_cb1(state->m_printer_level);
	via_0->write_cb1(!state->m_printer_level);
	state->m_printer_level ^= 1;

	if (state->m_printer_dir)
	{
		if (state->m_printer_x > 0)
			state->m_printer_x--;
		else
		{
			state->m_printer_dir = 0;
			state->m_printer_x++;
			state->m_printer_y++;
		}
	}
	else
	{
		if (state->m_printer_x < 9)
			state->m_printer_x++;
		else
		{
			state->m_printer_dir = 1;
			state->m_printer_x--;
			state->m_printer_y++;
		}
	}

	if (state->m_printer_y > 500) state->m_printer_y = 0;

	state->m_flag_a=0;
	state->m_flag_b=0;
}


WRITE8_MEMBER( aim65_state::aim65_printer_on )
{
	via6522_device *via_0 = machine().device<via6522_device>("via6522_0");
	if (!data)
	{
		m_printer_x=0;
		m_printer_y++;
		if (m_printer_y > 500) m_printer_y = 0;
		m_flag_a = m_flag_b=0;
		via_0->write_cb1(0);
		//m_print_timer->adjust(attotime::zero, 0, attotime::from_usec(10));
		m_printer_level = 1;
	}
	else
	{
		m_print_timer->reset();
	}
}


WRITE8_MEMBER( aim65_state::aim65_pa_w )
{
// All bits are for printer data (not emulated)
#if 0
	if (m_flag_a == 0)
	{
		printerRAM[(m_printer_y * 20) + m_printer_x] |= data;
		m_flag_a = 1;
	}
#endif
}

WRITE8_MEMBER( aim65_state::aim65_pb_w )
{
/*
	d7 = cass out (both decks)
	d5 = cass2 motor
	d4 = cass1 motor
	d2 = tty out (not emulated)
	d0/1 = printer data (not emulated)
*/

	UINT8 bits = data ^ m_pb_save;
	m_pb_save = data;

	if (BIT(bits, 7))
	{
		m_cass1->output(BIT(data, 7) ? -1.0 : +1.0);
		m_cass2->output(BIT(data, 7) ? -1.0 : +1.0);
	}

	if (BIT(bits, 5))
	{
		if (BIT(data, 5))
			m_cass2->change_state(CASSETTE_MOTOR_DISABLED,CASSETTE_MASK_MOTOR);
		else
			m_cass2->change_state(CASSETTE_MOTOR_ENABLED,CASSETTE_MASK_MOTOR);
	}

	if (BIT(bits, 4))
	{
		if (BIT(data, 4))
			m_cass1->change_state(CASSETTE_MOTOR_DISABLED,CASSETTE_MASK_MOTOR);
		else
			m_cass1->change_state(CASSETTE_MOTOR_ENABLED,CASSETTE_MASK_MOTOR);
	}


#if 0
	data &= 0x03;

	if (m_flag_b == 0)
	{
		printerRAM[(m_printer_y * 20) + m_printer_x ] |= (data << 8);
		m_flag_b = 1;
	}
#endif
}

VIDEO_START( aim65 )
{
	aim65_state *state = machine.driver_data<aim65_state>();
	state->m_print_timer = machine.scheduler().timer_alloc(FUNC(aim65_printer_timer));

#if 0
	videoram_size = 600 * 10 * 2;
	printerRAM = auto_alloc_array(machine, UINT16, videoram_size / 2);
	memset(printerRAM, 0, videoram_size);
	VIDEO_START_CALL(generic);
#endif
	state->m_printer_x = 0;
	state->m_printer_y = 0;
	state->m_printer_dir = 0;
	state->m_flag_a = 0;
	state->m_flag_b = 0;
	state->m_printer_level = 0;
	state->m_pb_save = 0;
}


#ifdef UNUSED_FUNCTION
SCREEN_UPDATE( aim65 )
{
	/* Display printer output */
	dir = 1;
	for(y = 0; y<500;y++)
	{
		for(x = 0; x< 10; x++)
		{
			if (dir == 1) {
				data = printerRAM[y * 10  + x];
			}
			else {
				data = printerRAM[(y * 10) + (9 - x)];
			}


			for (b = 0; b<10; b++)
			{
				color=machine.pens[(data & 0x1)?2:0];
				plot_pixel(bitmap,700 - ((b * 10) + x), y,color);
				data = data >> 1;
			}
		}

		if (dir == 0)
			dir = 1;
		else
			dir = 0;
	}
	return 0;
}
#endif


READ8_MEMBER( aim65_state::aim65_pb_r )
{
/*
	d7 = cassette in (deck 1)
	d6 = tty in (not emulated)
	d3 = kb/tty switch
*/

	UINT8 data = input_port_read(machine(), "switches");
	data |= (m_cass1->input() > +0.03) ? 0x80 : 0;
	data |= 0x40; // TTY must be H if not used.
	data |= m_pb_save & 0x37;
	return data;
}

