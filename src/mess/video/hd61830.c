/**********************************************************************

    HD61830 LCD Timing Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "hd61830.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

enum
{
	HD61830_INSTRUCTION_MODE_CONTROL = 0,
	HD61830_INSTRUCTION_CHARACTER_PITCH,
	HD61830_INSTRUCTION_NUMBER_OF_CHARACTERS,
	HD61830_INSTRUCTION_NUMBER_OF_TIME_DIVISIONS,
	HD61830_INSTRUCTION_CURSOR_POSITION,
	HD61830_INSTRUCTION_DISPLAY_START_LOW = 8,
	HD61830_INSTRUCTION_DISPLAY_START_HIGH,
	HD61830_INSTRUCTION_CURSOR_ADDRESS_LOW,
	HD61830_INSTRUCTION_CURSOR_ADDRESS_HIGH,
	HD61830_INSTRUCTION_DISPLAY_DATA_WRITE,
	HD61830_INSTRUCTION_DISPLAY_DATA_READ,
	HD61830_INSTRUCTION_CLEAR_BIT,
	HD61830_INSTRUCTION_SET_BIT
};

static const int HD61830_CYCLES[] = {
	4, 4, 4, 4, 4, -1, -1, -1, 4, 4, 4, 4, 6, 6, 36, 36
};

#define HD61830_MODE_EXTERNAL_CG	0x01
#define HD61830_MODE_GRAPHIC		0x02
#define HD61830_MODE_CURSOR			0x04
#define HD61830_MODE_BLINK			0x08
#define HD61830_MODE_MASTER			0x10
#define HD61830_MODE_DISPLAY_ON		0x20

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _hd61830_t hd61830_t;
struct _hd61830_t
{
	address_space *ram;			/* video RAM memory space */
	hd61830_rd_read	rd_r;

	int bf;						/* busy flag */

	UINT8 ir;					/* instruction register */
	UINT8 mcr;					/* mode control register */
	UINT8 dor;					/* data output register */

	UINT16 dsa;					/* display start address */
	UINT16 cac;					/* cursor address counter */

	int vp;						/* vertical character pitch */
	int hp;						/* horizontal character pitch */
	int hn;						/* horizontal number of characters */
	int nx;						/* number of time divisions */
	int cp;						/* cursor position */

    int blink;					/* blinking */
	int cursor;

	/* devices */
	screen_device *screen;

	/* timers */
	emu_timer *busy_timer;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE hd61830_t *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(device->type() == HD61830);
	return (hd61830_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE const hd61830_interface *get_interface(running_device *device)
{
	assert(device != NULL);
	assert(device->type() == HD61830);
	return (const hd61830_interface *) device->baseconfig().static_config();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    TIMER_CALLBACK( busy_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( busy_tick )
{
	running_device *device = (running_device *)ptr;
	hd61830_t *hd61830 = get_safe_token(device);

	/* clear busy flag */
	hd61830->bf = 0;
}

/*-------------------------------------------------
    set_busy_flag - set busy flag and arm timer
                    to clear it later
-------------------------------------------------*/

static void set_busy_flag(hd61830_t *hd61830, int period)
{
	/* set busy flag */
	hd61830->bf = 1;

	/* adjust busy timer */
	timer_adjust_oneshot(hd61830->busy_timer, ATTOTIME_IN_USEC(period), 0);
}

/*-------------------------------------------------
    hd61830_status_r - status read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( hd61830_status_r )
{
	hd61830_t *hd61830 = get_safe_token(device);

	if (LOG) logerror("HD61380 '%s' Status Read: %s\n", device->tag(), hd61830->bf ? "busy" : "ready");

	return hd61830->bf << 7;
}

/*-------------------------------------------------
    hd61830_control_w - control write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( hd61830_control_w )
{
	hd61830_t *hd61830 = get_safe_token(device);

	hd61830->ir = data;
}

/*-------------------------------------------------
    hd61830_data_r - data read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( hd61830_data_r )
{
	hd61830_t *hd61830 = get_safe_token(device);
	UINT8 data = hd61830->dor;

	if (LOG) logerror("HD61380 '%s' Display Data Read %02x\n", device->tag(), hd61830->dor);

	hd61830->dor = hd61830->ram->read_byte(hd61830->cac);

	hd61830->cac++;

	return data;
}

/*-------------------------------------------------
    hd61830_data_w - data write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( hd61830_data_w )
{
	hd61830_t *hd61830 = get_safe_token(device);

	switch (hd61830->ir)
	{
	case HD61830_INSTRUCTION_MODE_CONTROL:
		hd61830->mcr = data;

		if (LOG)
		{
			logerror("HD61380 '%s' %s CG\n", device->tag(), (data & HD61830_MODE_EXTERNAL_CG) ? "External" : "Internal");
			logerror("HD61380 '%s' %s Display Mode\n", device->tag(), (data & HD61830_MODE_GRAPHIC) ? "Graphic" : "Character");
			logerror("HD61380 '%s' %s Mode\n", device->tag(), (data & HD61830_MODE_MASTER) ? "Master" : "Slave");
			logerror("HD61380 '%s' Cursor %s\n", device->tag(), (data & HD61830_MODE_CURSOR) ? "On" : "Off");
			logerror("HD61380 '%s' Blink %s\n", device->tag(), (data & HD61830_MODE_BLINK) ? "On" : "Off");
			logerror("HD61380 '%s' Display %s\n", device->tag(), (data & HD61830_MODE_DISPLAY_ON) ? "On" : "Off");
		}
		break;

	case HD61830_INSTRUCTION_CHARACTER_PITCH:
		hd61830->hp = (data & 0x07) + 1;
		hd61830->vp = (data >> 4) + 1;

		if (LOG) logerror("HD61380 '%s' Horizontal Character Pitch: %u\n", device->tag(), hd61830->hp);
		if (LOG) logerror("HD61380 '%s' Vertical Character Pitch: %u\n", device->tag(), hd61830->vp);
		break;

	case HD61830_INSTRUCTION_NUMBER_OF_CHARACTERS:
		hd61830->hn = (data & 0x7f) + 1;

		if (LOG) logerror("HD61380 '%s' Number of Characters: %u\n", device->tag(), hd61830->hn);
		break;

	case HD61830_INSTRUCTION_NUMBER_OF_TIME_DIVISIONS:
		hd61830->nx = (data & 0x7f) + 1;

		if (LOG) logerror("HD61380 '%s' Number of Time Divisions: %u\n", device->tag(), hd61830->nx);
		break;

	case HD61830_INSTRUCTION_CURSOR_POSITION:
		hd61830->cp = (data & 0x7f) + 1;

		if (LOG) logerror("HD61380 '%s' Cursor Position: %u\n", device->tag(), hd61830->cp);
		break;

	case HD61830_INSTRUCTION_DISPLAY_START_LOW:
		hd61830->dsa = (hd61830->dsa & 0xff00) | data;

		if (LOG) logerror("HD61380 '%s' Display Start Address Low %04x\n", device->tag(), hd61830->dsa);
		break;

	case HD61830_INSTRUCTION_DISPLAY_START_HIGH:
		hd61830->dsa = (data << 8) | (hd61830->dsa & 0xff);

		if (LOG) logerror("HD61380 '%s' Display Start Address High %04x\n", device->tag(), hd61830->dsa);
		break;

	case HD61830_INSTRUCTION_CURSOR_ADDRESS_LOW:
		if (BIT(hd61830->cac, 7) && !BIT(data, 7))
		{
			hd61830->cac = (((hd61830->cac >> 8) + 1) << 8) | data;
		}
		else
		{
			hd61830->cac = (hd61830->cac & 0xff00) | data;
		}

		if (LOG) logerror("HD61380 '%s' Cursor Address Low %02x: %04x\n", device->tag(), data, hd61830->cac);
		break;

	case HD61830_INSTRUCTION_CURSOR_ADDRESS_HIGH:
		hd61830->cac = (data << 8) | (hd61830->cac & 0xff);

		if (LOG) logerror("HD61380 '%s' Cursor Address High %02x: %04x\n", device->tag(), data, hd61830->cac);
		break;

	case HD61830_INSTRUCTION_DISPLAY_DATA_WRITE:
		hd61830->ram->write_byte(hd61830->cac, data);

		if (LOG) logerror("HD61380 '%s' Display Data Write %02x -> %04x row %u col %u\n", device->tag(), data, hd61830->cac, hd61830->cac / 40, hd61830->cac % 40);

		hd61830->cac++;
		break;

	case HD61830_INSTRUCTION_CLEAR_BIT:
		{
		int nb = data & 0x07;
		UINT8 data_ = hd61830->ram->read_byte(hd61830->cac);

		data_ &= ~(2 << nb);

		if (LOG) logerror("HD61380 '%s' Clear Bit %u at %04x\n", device->tag(), nb + 1, hd61830->cac);

		hd61830->ram->write_byte(hd61830->cac, data_);

		hd61830->cac++;
		}
		break;

	case HD61830_INSTRUCTION_SET_BIT:
		{
		int nb = data & 0x07;
		UINT8 data_ = hd61830->ram->read_byte(hd61830->cac);

		data_ |= 2 << nb;

		if (LOG) logerror("HD61380 '%s' Set Bit %u at %04x\n", device->tag(), nb + 1, hd61830->cac);

		hd61830->ram->write_byte(hd61830->cac, data_);

		hd61830->cac++;
		}
		break;

	default:
		logerror("HD61830 '%s' Illegal Instruction %02x!\n", device->tag(), hd61830->ir);
		return;
	}

	/* burn cycles */
	set_busy_flag(hd61830, HD61830_CYCLES[hd61830->ir]);
}

/*-------------------------------------------------
    draw_scanline - draw one scanline
-------------------------------------------------*/

static void draw_scanline(running_device *device, bitmap_t *bitmap, const rectangle *cliprect, int y, UINT16 ra)
{
	hd61830_t *hd61830 = get_safe_token(device);

	for (int sx = 0; sx < hd61830->hn; sx++)
	{
		UINT8 data = hd61830->ram->read_byte(ra++);

		for (int x = 0; x < hd61830->hp; x++)
		{
			*BITMAP_ADDR16(bitmap, y, (sx * hd61830->hp) + x) = BIT(data, x);
		}
	}
}

/*-------------------------------------------------
    update_graphics - draw graphics mode screen
-------------------------------------------------*/

static void update_graphics(running_device *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	hd61830_t *hd61830 = get_safe_token(device);

	for (int y = 0; y < hd61830->nx; y++)
	{
		UINT16 rac1 = hd61830->dsa + (y * hd61830->hn);
		UINT16 rac2 = rac1 + (hd61830->nx * hd61830->hn);

		/* draw upper half scanline */
		draw_scanline(device, bitmap, cliprect, y, rac1);

		/* draw lower half scanline */
		draw_scanline(device, bitmap, cliprect, y + hd61830->nx, rac2);
	}
}

/*-------------------------------------------------
    draw_char - draw a char
-------------------------------------------------*/

static void draw_char(running_device *device, bitmap_t *bitmap, const rectangle *cliprect, UINT16 ma, int x, int y, UINT8 md)
{
	hd61830_t *hd61830 = get_safe_token(device);

	for (int cl = 0; cl < hd61830->vp; cl++)
	{
		for (int cr = 0; cr < hd61830->hp; cr++)
		{
			int sy = y * hd61830->vp + cl;
			int sx = x * hd61830->hp + cr;
			UINT8 data = 0;

			if (hd61830->mcr & HD61830_MODE_EXTERNAL_CG)
			{
				data = hd61830->rd_r(device, (cl << 12) | (ma & 0xfff), md);
			}
			else
			{
				UINT16 addr = 0;

				if (md >= 0x20 && md < 0x80 && cl < 7)
				{
					// 5x7 characters 0x20..0x7f
					addr = (md - 0x20) * 7 + cl;
				}
				else if (md >= 0xa0 && md < 0xe0 && cl < 7)
				{
					// 5x7 characters 0xa0..0xdf
					addr = 96*7 + (md - 0xa0) * 7 + cl;
				}
				else if (md >= 0xe0 && cl < 11)
				{
					// 5x11 characters 0xe0..0xff
					addr = 160*7 + (md - 0xe0) * 11 + cl;
				}

				data = device->subregion("hd61830")->u8(addr);
			}

			int cursor = hd61830->mcr & HD61830_MODE_CURSOR;
			int blink = hd61830->mcr & HD61830_MODE_BLINK;

			// cursor off
			int pixel = BIT(data, cr);

			if (blink && (ma == hd61830->cac))
			{
				// cursor off, character blink
				if (!cursor) 
					pixel = hd61830->cursor ? pixel : 0;

				// cursor blink
				if (cursor && (cl == hd61830->cp))
					pixel = hd61830->cursor;
			}
			else
			{
				// cursor on
				if (cursor && (cl == hd61830->cp))
					pixel = hd61830->cursor;
			}

			*BITMAP_ADDR16(bitmap, sy, sx) = pixel;
		}
	}
}

/*-------------------------------------------------
    update_text - draw text mode screen
-------------------------------------------------*/

static void update_text(running_device *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	hd61830_t *hd61830 = get_safe_token(device);

	for (int y = 0; y < (hd61830->nx / hd61830->vp); y++)
	{
		for (int x = 0; x < hd61830->hn; x++)
		{
			UINT16 ma = y * hd61830->hn + x;
			UINT8 md = hd61830->ram->read_byte(ma);

			draw_char(device, bitmap, cliprect, ma, x, y, md);
		}
	}
}

/*-------------------------------------------------
    hd61830_update - update screen
-------------------------------------------------*/

void hd61830_update(running_device *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	hd61830_t *hd61830 = get_safe_token(device);

	if (hd61830->mcr & HD61830_MODE_DISPLAY_ON)
	{
		if (hd61830->mcr & HD61830_MODE_GRAPHIC)
		{
			update_graphics(device, bitmap, cliprect);
		}
		else
		{
			update_text(device, bitmap, cliprect);
		}
	}
	else
	{
		bitmap_fill(bitmap, cliprect, 0);
	}

	hd61830->blink++;

	if (hd61830->blink == 0x20)
	{
		hd61830->blink = 0;
		hd61830->cursor = !hd61830->cursor;
	}
}

/*-------------------------------------------------
    hd61830_r - register read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( hd61830_r )
{
	return (offset & 0x01) ? hd61830_status_r(device, offset) : hd61830_data_r(device, offset);
}

/*-------------------------------------------------
    hd61830_w - register write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( hd61830_w )
{
	(offset & 0x01) ? hd61830_control_w(device, offset, data) : hd61830_data_w(device, offset, data);
}

/*-------------------------------------------------
    ROM( hd61830 )
-------------------------------------------------*/

ROM_START( hd61830 )
	ROM_REGION( 0x5c0, "hd61830", ROMREGION_LOADBYNAME ) // internal 7360-bit chargen ROM
	ROM_LOAD( "hd61830.bin", 0x000, 0x5c0, BAD_DUMP CRC(06a934da) SHA1(bf3f074db5dc92e6f530cb18d6c013563099a87d) ) // typed in from manual
ROM_END

/*-------------------------------------------------
    ADDRESS_MAP( hd61830 )
-------------------------------------------------*/

static ADDRESS_MAP_START( hd61380_ram, 0, 8 )
	AM_RANGE(0x0000, 0xffff) AM_RAM
ADDRESS_MAP_END

/*-------------------------------------------------
    DEVICE_START( hd61830 )
-------------------------------------------------*/

static DEVICE_START( hd61830 )
{
	hd61830_t *hd61830 = get_safe_token(device);
	const hd61830_interface *intf = get_interface(device);

	hd61830->rd_r = intf->rd_r;

	/* get the screen device */
	hd61830->screen = device->machine->device<screen_device>(intf->screen_tag);
	assert(hd61830->screen != NULL);

	hd61830->ram = device_get_space(device, ADDRESS_SPACE_0);

	/* create the busy timer */
	hd61830->busy_timer = timer_alloc(device->machine, busy_tick, (void *)device);

	/* register for state saving */
	state_save_register_device_item(device, 0, hd61830->bf);
	state_save_register_device_item(device, 0, hd61830->ir);
	state_save_register_device_item(device, 0, hd61830->mcr);
 	state_save_register_device_item(device, 0, hd61830->dor);
	state_save_register_device_item(device, 0, hd61830->cac);
	state_save_register_device_item(device, 0, hd61830->dsa);
	state_save_register_device_item(device, 0, hd61830->vp);
	state_save_register_device_item(device, 0, hd61830->hp);
	state_save_register_device_item(device, 0, hd61830->hn);
	state_save_register_device_item(device, 0, hd61830->nx);
	state_save_register_device_item(device, 0, hd61830->cp);
	state_save_register_device_item(device, 0, hd61830->blink);
	state_save_register_device_item(device, 0, hd61830->cursor);
}

/*-------------------------------------------------
    DEVICE_RESET( hd61830 )
-------------------------------------------------*/

static DEVICE_RESET( hd61830 )
{
	hd61830_t *hd61830 = get_safe_token(device);

	hd61830->mcr = hd61830->mcr & ~(HD61830_MODE_MASTER | HD61830_MODE_DISPLAY_ON);
	hd61830->hp = 6;
}

/*-------------------------------------------------
    DEVICE_GET_INFO( hd61830 )
-------------------------------------------------*/

DEVICE_GET_INFO( hd61830 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(hd61830_t);						break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;										break;
		case DEVINFO_INT_DATABUS_WIDTH_0:				info->i = 8;										break;
		case DEVINFO_INT_ADDRBUS_WIDTH_0:				info->i = 16;										break;
		case DEVINFO_INT_ADDRBUS_SHIFT_0:				info->i = 0;										break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(hd61830);				break;

		/* --- the following bits of info are returned as pointers to data --- */
		case DEVINFO_PTR_DEFAULT_MEMORY_MAP_0:			info->default_map8 = ADDRESS_MAP_NAME(hd61380_ram);	break;

		/* --- the following bits of info are returned as pointers to functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(hd61830);			break;
		case DEVINFO_FCT_STOP:							/* Nothing */										break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(hd61830);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Hitachi HD61830");					break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Hitachi HD61830");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");								break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);							break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");				break;
	}
}

DEFINE_LEGACY_MEMORY_DEVICE(HD61830, hd61830);
