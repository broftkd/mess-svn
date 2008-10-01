/*****************************************************************************
 *
 * video/abc806.c
 *
 ****************************************************************************/

/*

	TODO:

	- how do you select between 512x240 and 256x240 HR modes???
	- add proper screen parameters to startup
	- palette configuration from PAL/PROM
	- vertical sync delay
	- horizontal positioning of the text and HR screens
	
*/

#include "driver.h"
#include "includes/abc80x.h"
#include "machine/z80dart.h"
#include "machine/e0516.h"
#include "video/mc6845.h"

/* Palette Initialization */

static PALETTE_INIT( abc806 )
{
	palette_set_color_rgb(machine, 0, 0x00, 0x00, 0x00); // black
	palette_set_color_rgb(machine, 1, 0x00, 0x00, 0xff); // blue
	palette_set_color_rgb(machine, 2, 0xff, 0x00, 0x00); // red
	palette_set_color_rgb(machine, 3, 0xff, 0x00, 0xff); // magenta
	palette_set_color_rgb(machine, 4, 0x00, 0xff, 0x00); // green
	palette_set_color_rgb(machine, 5, 0x00, 0xff, 0xff); // cyan
	palette_set_color_rgb(machine, 6, 0xff, 0xff, 0x00); // yellow
	palette_set_color_rgb(machine, 7, 0xff, 0xff, 0xff); // white
}

/* Timer Callbacks */

static TIMER_CALLBACK( abc806_flash_tick )
{
	abc806_state *state = machine->driver_data;

	state->flshclk = !state->flshclk;
}

/* High Resolution Screen Select */

WRITE8_HANDLER( abc806_hrs_w )
{
	/*

		bit		description

		0		visible screen memory area bit 0
		1		visible screen memory area bit 1
		2		visible screen memory area bit 2 (unused)
		3		visible screen memory area bit 3 (unused)
		4		cpu accessible screen memory area bit 0
		5		cpu accessible screen memory area bit 1
		6		cpu accessible screen memory area bit 2 (unused)
		7		cpu accessible screen memory area bit 3 (unused)

	*/

	abc806_state *state = machine->driver_data;
	
	state->hrs = data;
}

/* High Resolution Palette */

WRITE8_HANDLER( abc806_hrc_w )
{
	abc806_state *state = machine->driver_data;

	int reg = (offset >> 8) & 0x0f;

	state->hrc[reg] = data & 0x0f;
}

/* Character Memory */

READ8_HANDLER( abc806_charram_r )
{
	abc806_state *state = machine->driver_data;

	state->attr_data = state->colorram[offset];

	return state->charram[offset];
}

WRITE8_HANDLER( abc806_charram_w )
{
	abc806_state *state = machine->driver_data;

	state->colorram[offset] = state->attr_data;
	state->charram[offset] = data;
}

/* Attribute Memory */

READ8_HANDLER( abc806_ami_r )
{
	abc806_state *state = machine->driver_data;

	return state->attr_data;
}

WRITE8_HANDLER( abc806_amo_w )
{
	abc806_state *state = machine->driver_data;

	state->attr_data = data;
}

/* High Resolution Palette / Real-Time Clock */

READ8_HANDLER( abc806_cli_r )
{
	abc806_state *state = machine->driver_data;

	const device_config *e0516 = device_list_find_by_tag(machine->config->devicelist, E0516, E0516_TAG);

	/*

		bit		description

		0		HRU II data bit 0
		1		HRU II data bit 1
		2		HRU II data bit 2
		3		HRU II data bit 3
		4		
		5		
		6		
		7		RTC data output

	*/

	UINT16 hru2_addr = (state->hru2_a8 << 8) | (offset >> 8);
	UINT8 data = memory_region(machine, "hru2")[hru2_addr] & 0x0f;

	data |= e0516_dio_r(e0516) << 7;

	return data;
}

/* Special */

WRITE8_HANDLER( abc806_sto_w )
{
	abc806_state *state = machine->driver_data;

	const device_config *e0516 = device_list_find_by_tag(machine->config->devicelist, E0516, E0516_TAG);

	int level = BIT(data, 7);

	switch (data & 0x07)
	{
	case 0:
		/* external memory enable */
		state->eme = level;
		break;
	case 1:
		/* 40/80 column display */
		state->_40 = level;
		break;
	case 2:
		/* HRU II address line 8 */
		state->hru2_a8 = level;
		break;
	case 4:
		/* text display enable */
		state->txoff = level;
		break;
	case 5:
		/* RTC chip select */
		e0516_cs_w(e0516, level);
		break;
	case 6:
		/* RTC clock */
		e0516_clk_w(e0516, level);
		break;
	case 7:
		/* RTC data in */
		e0516_dio_w(e0516, level);
		break;
	}
}

/* Sync Delay */

WRITE8_HANDLER( abc806_sso_w )
{
	abc806_state *state = machine->driver_data;

	state->sync = data & 0x3f;
}

/* MC6845 */

static MC6845_UPDATE_ROW( abc806_update_row )
{
	abc806_state *state = device->machine->driver_data;

	int column;
	const UINT8 *charrom = memory_region(device->machine, "chargen");

	UINT8 old_data = 0xff;
	int fg_color = 7;
	int bg_color = 0;
	int underline = 0;
	int flash = 0;
	int e5 = 0;
	int e6 = 0;
	int th = 0;

	if (state->_40)
	{
		e5 = 1;
		e6 = 1;
	}

	for (column = 0; column < x_count; column++)
	{
		UINT8 data = state->charram[(ma + column) & 0x7ff];
		UINT8 attr = state->colorram[(ma + column) & 0x7ff];
		UINT16 rad_addr, chargen_addr;
		UINT8 rad_data, chargen_data;
		int bit, x;

		if ((attr & 0x07) == ((attr >> 3) & 0x07))
		{
			/* special case */

			switch (attr >> 6)
			{
			case 0:
				/* use previously selected attributes */
				break;
			case 1:
				/* reserved for future use */
				break;
			case 2:
				/* blank */
				fg_color = 0;
				bg_color = 0;
				underline = 0;
				flash = 0;
				break;
			case 3:
				/* double width */
				e5 = BIT(attr, 0);
				e6 = BIT(attr, 1);

				/* read attributes from next byte */
				attr = state->colorram[(ma + column + 1) & 0x7ff];

				if (attr != 0x00)
				{
					fg_color = attr & 0x07;
					bg_color = (attr >> 3) & 0x07;
					underline = BIT(attr, 6);
					flash = BIT(attr, 7);
				}
				break;
			}
		}
		else
		{
			/* normal case */

			fg_color = attr & 0x07;
			bg_color = (attr >> 3) & 0x07;
			underline = BIT(attr, 6);
			flash = BIT(attr, 7);
			e5 = state->_40 ? 1 : 0;
			e6 = state->_40 ? 1 : 0;
		}

		if (column == cursor_x)
		{
			rad_data = 0x0f;
		}
		else
		{
			rad_addr = (e6 << 8) | (e5 << 7) | (flash << 6) | (underline << 5) | (state->flshclk << 4) | ra;
			rad_data = memory_region(device->machine, "rad")[rad_addr] & 0x0f;

			rad_data = ra; // HACK because the RAD prom is not dumped yet
		}

		chargen_addr = (th << 12) | (data << 4) | rad_data;
		chargen_data = charrom[chargen_addr & 0xfff] << 2;
		x = column * ABC800_CHAR_WIDTH;

		for (bit = 0; bit < ABC800_CHAR_WIDTH; bit++)
		{
			int color = BIT(chargen_data, 7) ? fg_color : bg_color;

			if (state->txoff)
			{
				color = 0;
			}

			*BITMAP_ADDR16(bitmap, y, x++) = color;

			if (e5 || e6)
			{
				*BITMAP_ADDR16(bitmap, y, x++) = color;
			}

			chargen_data <<= 1;
		}
		
		if (e5 || e6)
		{
			column++;
		}

		old_data = data;
	}
}

static MC6845_ON_HSYNC_CHANGED(abc806_hsync_changed)
{
	abc806_state *state = device->machine->driver_data;

	if (!hsync)
	{
		state->v50_addr++;
	}
}

static MC6845_ON_VSYNC_CHANGED(abc806_vsync_changed)
{
	abc806_state *state = device->machine->driver_data;
	
	const device_config *z80dart = device_list_find_by_tag(device->machine->config->devicelist, Z80DART, "z80dart");

	if (vsync)
	{
		state->v50_addr = 0;
	}

	z80dart_set_ri(z80dart, 1, vsync);
}

/* MC6845 Interfaces */

static const mc6845_interface abc806_mc6845_interface = {
	SCREEN_TAG,
	ABC800_CCLK,
	ABC800_CHAR_WIDTH,
	NULL,
	abc806_update_row,
	NULL,
	NULL,
	abc806_hsync_changed,
	abc806_vsync_changed
};

/* HR */

static void abc806_hr_update(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect)
{
	abc806_state *state = machine->driver_data;

	UINT16 addr = (state->hrs & 0x03) << 14;
	int sx, y, dot;

	for (y = 0; y < 240; y++)
	{
		/* 256x240 */
		for (sx = 0; sx < 64; sx++)
		{
			UINT16 data = (state->videoram[addr++] << 8) | state->videoram[addr++];

			for (dot = 0; dot < 4; dot++)
			{
				int color = state->hrc[(data >> 12) & 0x0f];
				int x = (sx << 3) | (dot << 1);

				*BITMAP_ADDR16(bitmap, y, x) = color;
				*BITMAP_ADDR16(bitmap, y, x + 1) = color;

				data <<= 4;
			}
		}

		/* 512x240 */
		for (sx = 0; sx < 64; sx++)
		{
			UINT16 data = (state->videoram[addr++] << 8) | state->videoram[addr++];

			for (dot = 0; dot < 8; dot++)
			{
				int color = state->hrc[(data >> 14) & 0x03];
				int x = (sx << 3) | dot;

				*BITMAP_ADDR16(bitmap, y, x) = color;

				data <<= 4;
			}
		}
	}
}

/* Video Start */

static VIDEO_START(abc806)
{
	abc806_state *state = machine->driver_data;
	
	int i;

	/* initialize variables */

	for (i = 0; i < 16; i++)
	{
		state->hrc[i] = 0;
	}

	state->sync = 10;

	/* allocate memory */

	state->charram = auto_malloc(ABC806_CHAR_RAM_SIZE);
	state->colorram = auto_malloc(ABC806_ATTR_RAM_SIZE);

	/* allocate timer */

	state->flash_timer = timer_alloc(abc806_flash_tick, NULL);
	timer_adjust_periodic(state->flash_timer, attotime_zero, 0, ATTOTIME_IN_HZ(2));

	/* register for state saving */

	state_save_register_global_pointer(state->charram, ABC806_CHAR_RAM_SIZE);
	state_save_register_global_pointer(state->colorram, ABC806_ATTR_RAM_SIZE);
	state_save_register_global_pointer(state->videoram, ABC806_VIDEO_RAM_SIZE);

	state_save_register_global(state->v50_addr);
	state_save_register_global(state->attr_data);
	state_save_register_global(state->hrs);
	state_save_register_global(state->sync);
	state_save_register_global_array(state->hrc);
	state_save_register_global(state->eme);
	state_save_register_global(state->txoff);
	state_save_register_global(state->_40);
	state_save_register_global(state->flshclk);
	state_save_register_global(state->hru2_a8);
}

/* Video Update */

static VIDEO_UPDATE( abc806 )
{
	const device_config *mc6845 = device_list_find_by_tag(screen->machine->config->devicelist, MC6845, MC6845_TAG);

	abc806_hr_update(screen->machine, bitmap, cliprect);
	mc6845_update(mc6845, bitmap, cliprect);
	
	return 0;
}

/* Machine Drivers */

MACHINE_DRIVER_START( abc806_video )
	/* device interface */
	MDRV_DEVICE_ADD(MC6845_TAG, MC6845)
	MDRV_DEVICE_CONFIG(abc806_mc6845_interface)

	/* video hardware */
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0,640-1, 0, 400-1)

	MDRV_PALETTE_LENGTH(8)

	MDRV_PALETTE_INIT(abc806)
	MDRV_VIDEO_START(abc806)
	MDRV_VIDEO_UPDATE(abc806)
MACHINE_DRIVER_END
