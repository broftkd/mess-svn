#include "driver.h"
#include "cpu/cdp1802/cdp1802.h"
#include "machine/wd17xx.h"
#include "devices/basicdsk.h"
#include "devices/printer.h"
#include "devices/snapquik.h"
#include "video/cdp1869.h"
#include "video/mc6845.h"
#include "includes/comx35.h"

enum
{
	PRINTER_PARALLEL = 0,
	PRINTER_SERIAL,
	PRINTER_THERMAL,
	PRINTER_PLOTTER
};

enum
{
	COMX_TYPE_BINARY = 1,
	COMX_TYPE_BASIC,
	COMX_TYPE_BASIC_FM,
	COMX_TYPE_RESERVED,
	COMX_TYPE_DATA
};

static const device_config *printer_device(running_machine *machine)
{
	return device_list_find_by_tag(machine->config->devicelist, PRINTER, "printer");
}

/* Floppy Disc Controller */

DEVICE_IMAGE_LOAD( comx35_floppy )
{
	if (image_has_been_created(image))
		return INIT_FAIL;

	if (device_load_basicdsk_floppy(image) == INIT_PASS)
	{
		int size = image_length(image);

		switch (size)
		{
		case 35*16*128:
			/* drive, tracks, heads, sectors per track, sector length, first sector id, offset track zero, track skipping */
			basicdsk_set_geometry(image, 35, 1, 16, 128, 0, 0, FALSE);
			break;

		case 70*16*128:
			basicdsk_set_geometry(image, 70, 1, 16, 128, 0, 0, FALSE);
			break;

		case 89*16*128: // ???
			basicdsk_set_geometry(image, 89, 1, 16, 128, 0, 0, FALSE);
			break;
		}

		return INIT_PASS;
	}

	return INIT_FAIL;
}

static void comx35_fdc_callback(running_machine *machine, wd17xx_state_t event, void *param)
{
	comx35_state *state = machine->driver_data;

	switch (event)
	{
	case WD17XX_IRQ_SET:
		state->fdc_irq = 1;
		break;

	case WD17XX_IRQ_CLR:
		state->fdc_irq = 0;
		break;

	case WD17XX_DRQ_SET:
		if (state->fdc_drq_enable)
		{
			state->cdp1802_ef4 = 1;
		}
		break;

	case WD17XX_DRQ_CLR:
		if (state->fdc_drq_enable)
		{
			state->cdp1802_ef4 = 0;
		}
		break;
	}
}

static UINT8 fdc_r(running_machine *machine)
{
	comx35_state *state = machine->driver_data;

	UINT8 data;

	if (state->cdp1802_q)
	{
		data = state->fdc_irq;
	}
	else
	{
		data = wd17xx_r(machine, state->fdc_addr);
	}

	return data;
}

static void fdc_w(running_machine *machine, UINT8 data)
{
	/*

		bit		description

		0		A0
		1		A1
		2		DRIVE0
		3		DRIVE1
		4		F9 DISB
		5		SIDE SELECT

	*/

	comx35_state *state = machine->driver_data;

	if (state->cdp1802_q)
	{
		// latch data to F3

		state->fdc_addr = data & 0x03;

		if (BIT(data, 2))
		{
			wd17xx_set_drive(0);
		}		
		else if (BIT(data, 3))
		{
			wd17xx_set_drive(1);
		}

		state->fdc_drq_enable = BIT(data, 4);

		if (!state->fdc_drq_enable)
		{
			state->cdp1802_ef4 = 1;
		}

		wd17xx_set_side(BIT(data, 5));
	}
	else
	{
		// write data to WD1770

		wd17xx_w(machine, state->fdc_addr, data);
	}
}

/* Printer */

static UINT8 printer_r(running_machine *machine)
{
//	comx35_state *state = machine->driver_data;

	int printer = input_port_read(machine, "PRINTER") & 0x07;
	UINT8 data = 0;

	switch (printer)
	{
	case PRINTER_PARALLEL:
	case PRINTER_PLOTTER:
		/*
			INP 2 for the printer status, where:
			b0=1: Acknowledge Fault
			b1=0: Device Busy
			b2=0: Paper Empty
			b3=1: Device Not Selected 
		*/

		data = 0x06;
		break;

	case PRINTER_SERIAL:
		/*
			INP 2 for the printer status and to start a new range of bits for the next byte. 
		*/
		break;

	case PRINTER_THERMAL:
		/*
			INP 2 is used for the printer status, where:
			b0=1: Printer Not Ready
			b1=1: Energizing Head
			b2=1: Head At Position 0 
		*/
		break;
	}

	return data;
}

static void printer_w(running_machine *machine, UINT8 data)
{
//	comx35_state *state = machine->driver_data;
	int printer = input_port_read(machine, "PRINTER") & 0x07;

	switch (printer)
	{
	case PRINTER_PARALLEL:
	case PRINTER_PLOTTER:
		/*
			OUT 2 is used to send a byte to the printer
		*/

		printer_output(printer_device(machine), data);
		break;

	case PRINTER_SERIAL:
		/*
			OUT 2 is used to send a bit to the printer
		*/
		break;

	case PRINTER_THERMAL:
		/*
			OUT 2 is used to control the thermal printer where:
			Q = 0, b0-7: Pixel 1 to 8
			Q = 1, b7: Pixel 9 (if b0-6=#21)
			Q = 1, b3=1: Move head right
			Q = 1, b0-7=#12: Move head left 
		*/
		break;
	}
}

/* Read/Write Handlers */

static int get_active_bank(running_machine *machine, UINT8 data)
{
	comx35_state *state = machine->driver_data;
	int bank = 0;

	if (memory_region(REGION_CPU1)[0xe000] != 0xff)
	{
		// expansion box

		char port[6];
		int i;

		for (i = 1; i < 5; i++)
		{
			if (BIT(data, i))
			{
				state->slot = i;
				break;
			}
		}

		if (state->slot > 0)
		{
			sprintf(port, "SLOT%d", state->slot);

			bank = input_port_read(machine, port);
		}
	}
	else
	{
		// no expansion box

		bank = input_port_read(machine, "EXPANSION");
	}

	return bank;
}

WRITE8_HANDLER( comx35_bank_select_w )
{
	comx35_state *state = machine->driver_data;

	int bank = get_active_bank(machine, data);

	state->bank = bank;

	switch (state->bank)
	{
	case BANK_NONE:
		memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xdfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;

	case BANK_FLOPPY:
		memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xdfff, 0, 0, SMH_BANK1, SMH_UNMAP);
		break;

	case BANK_PRINTER_PARALLEL:
		memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xc7ff, 0, 0, SMH_BANK1, SMH_UNMAP);
		memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc800, 0xdfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;

	case BANK_PRINTER_PARALLEL_FM:
		memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xcfff, 0, 0, SMH_BANK1, SMH_UNMAP);
		memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xd000, 0xdfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;

	case BANK_PRINTER_SERIAL:
		memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xc7ff, 0, 0, SMH_BANK1, SMH_UNMAP);
		memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc800, 0xdfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;

	case BANK_PRINTER_THERMAL:
		memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xcfff, 0, 0, SMH_BANK1, SMH_UNMAP);
		memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xd000, 0xdfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;

	case BANK_JOYCARD:
		memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xc002, 0, 0, SMH_BANK1, SMH_UNMAP);
		memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc003, 0xdfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;

	case BANK_80_COLUMNS:
		{
			const device_config *mc6845 = device_list_find_by_tag(machine->config->devicelist, MC6845, MC6845_TAG);

			memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xc7ff, 0, 0, SMH_BANK1, SMH_UNMAP); // ROM
			memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc800, 0xcfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
			memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xd000, 0xd7ff, 0, 0, SMH_BANK1, SMH_BANK1); // RAM
			memory_install_readwrite8_device_handler(mc6845, 0, ADDRESS_SPACE_PROGRAM, 0xd800, 0xd800, 0, 0, SMH_UNMAP, mc6845_address_w);
			memory_install_readwrite8_device_handler(mc6845, 0, ADDRESS_SPACE_PROGRAM, 0xd801, 0xd801, 0, 0, mc6845_register_r, mc6845_register_w);
			memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xd802, 0xdfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		}
		break;

	case BANK_RAMCARD:
		{
			int rambank = (data >> 5) & 0x03;

			bank = BANK_RAMCARD + rambank;

			memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xdfff, 0, 0, SMH_BANK1, SMH_BANK1);
		}
		break;
	}

	memory_set_bank(1, bank);
}

READ8_HANDLER( comx35_io_r )
{
	comx35_state *state = machine->driver_data;

	UINT8 data = 0xff;

	switch (state->bank)
	{
	case BANK_NONE:
		break;

	case BANK_FLOPPY:
		data = fdc_r(machine);
		break;

	case BANK_PRINTER_PARALLEL:
	case BANK_PRINTER_PARALLEL_FM:
	case BANK_PRINTER_SERIAL:
	case BANK_PRINTER_THERMAL:
		data = printer_r(machine);
		break;

	case BANK_JOYCARD:
		data = input_port_read(machine, "JOY1");
		break;

	case BANK_80_COLUMNS:
		break;

	case BANK_RAMCARD:
		break;
	}

	return data;
}

READ8_HANDLER( comx35_io2_r )
{
	comx35_state *state = machine->driver_data;

	UINT8 data = 0xff;

	switch (state->bank)
	{
	case BANK_NONE:
		break;

	case BANK_FLOPPY:
		break;

	case BANK_PRINTER_PARALLEL:
	case BANK_PRINTER_PARALLEL_FM:
	case BANK_PRINTER_SERIAL:
	case BANK_PRINTER_THERMAL:
		break;

	case BANK_JOYCARD:
		data = input_port_read(machine, "JOY2");
		break;

	case BANK_80_COLUMNS:
		break;

	case BANK_RAMCARD:
		break;
	}

	return data;
}

WRITE8_HANDLER( comx35_io_w )
{
	comx35_state *state = machine->driver_data;

	switch (state->bank)
	{
	case BANK_NONE:
		break;

	case BANK_FLOPPY:
		fdc_w(machine, data);
		break;

	case BANK_PRINTER_PARALLEL:
	case BANK_PRINTER_PARALLEL_FM:
	case BANK_PRINTER_SERIAL:
	case BANK_PRINTER_THERMAL:
		printer_w(machine, data);
		break;

	case BANK_JOYCARD:
		data = input_port_read(machine, "JOY2");
		break;

	case BANK_80_COLUMNS:
		break;

	case BANK_RAMCARD:
		break;
	}
}

/* Machine Initialization */

static TIMER_CALLBACK( dma_tick )
{
	comx35_state *state = machine->driver_data;

	if (!state->iden)
	{
	//	cpunum_set_input_line(device->machine, 0, CDP1802_INPUT_LINE_DMAOUT, ASSERT_LINE);
	//	cpunum_set_input_line(device->machine, 0, CDP1802_INPUT_LINE_DMAOUT, CLEAR_LINE);
	}
}

static TIMER_CALLBACK( reset_tick )
{
	comx35_state *state = machine->driver_data;

	state->cdp1802_mode = CDP1802_MODE_RUN;
}

MACHINE_START( comx35p )
{
	comx35_state *state = machine->driver_data;

	// card slot banking

	memory_configure_bank(1, 0, 1, memory_region(REGION_CPU1), 0xc000);
	memory_configure_bank(1, BANK_FLOPPY, 7, memory_region(REGION_USER1), 0x2000);
	memory_configure_bank(1, BANK_RAMCARD, 4, mess_ram, 0x2000);

	memory_set_bank(1, 0);

	// allocate DMA timer

	state->dma_timer = timer_alloc(dma_tick, NULL);
	timer_adjust_periodic(state->dma_timer, attotime_zero, 0, ATTOTIME_IN_HZ(CDP1869_CPU_CLK_PAL / 8));

	// allocate reset timer
	
	state->reset_timer = timer_alloc(reset_tick, NULL);

	// screen format

	state->pal_ntsc = CDP1869_PAL;

	wd17xx_init(machine, WD_TYPE_1770, comx35_fdc_callback, NULL);

	// external flag 4

	state->cdp1802_ef4 = 1;

	// register save states

	state_save_register_global(state->cdp1802_mode);
	state_save_register_global(state->cdp1802_q);
	state_save_register_global(state->cdp1802_ef4);
	state_save_register_global(state->iden);
	state_save_register_global(state->slot);

	state_save_register_global(state->pal_ntsc);
	state_save_register_global(state->cdp1869_prd);

	state_save_register_global(state->cdp1871_efxa);
	state_save_register_global(state->cdp1871_efxb);

	state_save_register_global(state->fdc_addr);
	state_save_register_global(state->fdc_irq);
	state_save_register_global(state->fdc_drq_enable);
}

MACHINE_START( comx35n )
{
	comx35_state *state = machine->driver_data;

	MACHINE_START_CALL(comx35p);

	timer_adjust_periodic(state->dma_timer, attotime_zero, 0, ATTOTIME_IN_HZ(CDP1869_CPU_CLK_NTSC / 8));

	// screen format

	state->pal_ntsc = CDP1869_NTSC;
}

MACHINE_RESET( comx35 )
{
	comx35_state *state = machine->driver_data;

	state->cdp1802_mode = CDP1802_MODE_RESET;
	state->iden = 1;

	timer_adjust_oneshot(state->reset_timer, ATTOTIME_IN_MSEC(200), 0);
}

/* Quickload */

static void image_fread_memory(const device_config *image, UINT16 addr, UINT32 count)
{
	void *ptr = memory_get_write_ptr(0, ADDRESS_SPACE_PROGRAM, addr);

	image_fread(image, ptr, count);
}

static void write_memory(UINT16 addr, UINT8 data)
{
	UINT8 *ptr = (UINT8 *) memory_get_write_ptr(0, ADDRESS_SPACE_PROGRAM, addr);

	ptr[0] = data;
}

static UINT8 read_memory(UINT16 addr)
{
	UINT8 *ptr = (UINT8 *) memory_get_read_ptr(0, ADDRESS_SPACE_PROGRAM, addr);

	return ptr[0];
}

QUICKLOAD_LOAD( comx35 )
{
	UINT8 header[16] = {0};
	int size = image_length(image);

	if (size > mess_ram_size)
	{
		return INIT_FAIL;
	}

	image_fread(image, header, 5);

	if (header[1] != 'C' || header[2] != 'O' || header[3] != 'M' || header[4] != 'X' )
	{
		return INIT_FAIL;
	}

	switch (header[0])
	{
	case COMX_TYPE_BINARY:
		/*

			Type 1: pure machine code (i.e. no basic)

			Byte 0 to 4: 1 - 'COMX'
			Byte 5 and 6: Start address (1802 way; see above)
			Byte 6 and 7: End address
			Byte 9 and 10: Execution address

			Byte 11 to Eof, should be stored in ram from start to end; execution address
			'xxxx' for the CALL (@xxxx) basic statement to actually run the code.

		*/
		{
			UINT16 start_address, end_address, run_address;
			
			image_fread(image, header, 6);

			start_address = header[0] << 8 | header[1];
			end_address = header[2] << 8 | header[3];
			run_address = header[4] << 8 | header[5];

			image_fread_memory(image, start_address, end_address - start_address);

			popmessage("Type CALL (@%04x) to start program", run_address);
		}
		break;

	case COMX_TYPE_BASIC:
		/*

			Type 2: Regular basic code or machine code followed by basic

			Byte 0 to 4: 2 - 'COMX'
			Byte 5 and 6: DEFUS value, to be stored on 0x4281 and 0x4282
			Byte 7 and 8: EOP value, to be stored on 0x4283 and 0x4284
			Byte 9 and 10: End array, start string to be stored on 0x4292 and 0x4293
			Byte 11 and 12: start array to be stored on 0x4294 and 0x4295
			Byte 13 and 14: EOD and end string to be stored on 0x4299 and 0x429A

			Byte 15 to Eof to be stored on 0x4400 and onwards

			Byte 0x4281-0x429A (or at least the ones above) should be set otherwise
			BASIC won't 'see' the code.

		*/

		image_fread_memory(image, 0x4281, 4);
		image_fread_memory(image, 0x4292, 4);
		image_fread_memory(image, 0x4299, 2);
		image_fread_memory(image, 0x4400, size);
		break;

	case COMX_TYPE_BASIC_FM:
		/*

			Type 3: F&M basic load

			Not the most important! But we designed our own basic extension, you can
			find it in the F&M basic folder as F&M Basic.comx. When you run this all
			basic code should start at address 0x6700 instead of 0x4400 as from
			0x4400-0x6700 the F&M basic stuff is loaded. So format is identical to Type
			2 except Byte 15 to Eof should be stored on 0x6700 instead. .comx files of
			this format can also be found in the same folder as the F&M basic.comx file.

		*/

		image_fread_memory(image, 0x4281, 4);
		image_fread_memory(image, 0x4292, 4);
		image_fread_memory(image, 0x4299, 2);
		image_fread_memory(image, 0x6700, size);
		break;

	case COMX_TYPE_RESERVED:
		/*

			Type 4: Incorrect DATA format, I suggest to forget this one as it won't work
			in most cases. Instead I left this one reserved and designed Type 5 instead.

		*/
		break;

	case COMX_TYPE_DATA:
		/*

			Type 5: Data load

			Byte 0 to 4: 5 - 'COMX'
			Byte 5 and 6: Array length
			Byte 7 to Eof: Basic 'data'

			To load this first get the 'start array' from the running COMX, i.e. address
			0x4295/0x4296. Calculate the EOD as 'start array' + length of the data (i.e.
			file length - 7). Store the EOD back on 0x4299 and ox429A. Calculate the
			'Start String' as 'start array' + 'Array length' (Byte 5 and 6). Store the
			'Start String' on 0x4292/0x4293. Load byte 7 and onwards starting from the
			'start array' value fetched from 0x4295/0x4296.

		*/
		{
			UINT16 start_array, end_array, start_string, array_length;

			image_fread(image, header, 2);

			array_length = (header[0] << 8) | header[1];
			start_array = (read_memory(0x4295) << 8) | read_memory(0x4296);
			end_array = start_array + (size - 7);

			write_memory(0x4299, end_array >> 8);
			write_memory(0x429a, end_array & 0xff);
			
			start_string = start_array + array_length;

			write_memory(0x4292, start_string >> 8);
			write_memory(0x4293, start_string & 0xff);

			image_fread_memory(image, start_array, size);
		}
		break;
	}

	return INIT_PASS;
}
