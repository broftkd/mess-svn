/*************************************************************************************************************************************

    PC-8801 (c) 1981 NEC

    preliminary driver by Angelo Salese, original MESS PC-88SR by ???

    TODO:
	- used to do more, currently under rewriting stage. We are currently using plain PC-8801 as the base, will add differences for the
	  later models at some point.
	- macros for the various rom / ram areas, it might require various attempts in order to finally get a reasonable arrangement ...
	- dipswitches are WRONG
	- implement proper i8214 routing, also add irq latch mechanism;
	- implement proper upd3301 / i8257 support;
	- below notes states that plain PC-8801 doesn't have a disk CPU, but the BIOS clearly checks the floppy ports. Wrong info?
	- joystick / mouse support;
	- cursor is 8 x 10, not 8 x 8 as current implementation.
	- text DMAC stuff isn't well documented;
	- CRTC really looks like to be a MCU;
	- needs to support V1 / V2 properly;
	- Add limits for extend work RAM;
	- What happens to the palette contents when the analog/digital palette mode changes?

	per-game specific TODO:
	- 100 Yen disk(s): reads kanji ports;
	- 177: gameplay is too fast;
	- Acro Jet: hangs waiting for an irq;
	- Aggress: crashes after that it shows the title screen;
	- American Success: reads the light pen?
	- Alpha (demo): crashes with "illegal function" msg;
	- Alphos: text VRAM garbage during gameplay;
	- Balance of Power: attempt to use the SIO port for mouse polling, worked around for now;
	- Battle Entry: moans with a kanji msg then dies if you try to press either numpad 1 or 2 (what it basically asks by looking at the DASM)
	- Bersekers Front Gaiden 3: checks CPU speed port and polls the PCG stuff too;
	- Bishoujo Baseball Gakuen: checks ym2608 after intro screen;
	- Blue Moon Story: moans with a kanji msg;.
	- Bokosuka Wars: doesn't boot, floppy issue;
	- Bouken Roman: doesn't boot, floppy issue;
	- Bruce Lee: doesn't boot, floppy issue;
	- Bu U Ma: sets gfx compatibility mode (attr bit 4,N-BASIC), dunno how to draw with it ...;
	- Grobda: palette is ugly;
	- Wanderers from Ys: floppy / irq issues?
	- Xevious: game is too fast

	Notes:
	- BIOS disk ROM defines what kind of floppies you could load:
	  * with 0x0800 ROM size you can load 2d floppies only;
	  * with 0x2000 ROM size you can load 2d and 2hd floppies;

	Bankswitch Notes:
	- 0x31 - graphic banking
	- 0x32 - misc banking
	- 0x5c / 0x5f - VRAM banking
	- 0x70 - window offset (banking)
	- 0x71 - extra ROM banking
	- 0x78 - window offset (banking) increment (+ 0x100)
	- 0xe2 / 0xe3 - extra RAM banking
	- 0xf0 / 0xf1 = kanji banking

======================================================================================================================================

    PC-88xx Models (and similar machines like PC-80xx and PC-98DO)

    Model            | release |      CPU     |                      BIOS components                        |       |
                     |         |     clock    | N-BASIC | N88-BASIC | N88-BASIC Enh |  Sound  |  CD |  Dict |  Disk | Notes
    ==================================================================================================================================
    PC-8001          | 1979-03 |   z80A @ 4   |    X    |     -     |       -       |    -    |  -  |   -   |   -   |
    PC-8001A         |   ??    |   z80A @ 4   |    X    |     -     |       -       |    -    |  -  |   -   |   -   | (U)
    PC-8801          | 1981-11 |   z80A @ 4   |    X    |     X     |       -       |    -    |  -  |   -   |   -   | (KO)
    PC-8801A         |   ??    |   z80A @ 4   |    X    |     X     |       -       |    -    |  -  |   -   |   -   | (U)
    PC-8001 mkII     | 1983-03 |   z80A @ 4   |    X    |     -     |       -       |    -    |  -  |   -   |   -   | (GE),(KO)
    PC-8001 mkIIA    |   ??    |   z80A @ 4   |    X    |     -     |       -       |    -    |  -  |   -   |   -   | (U),(GE)
    PC-8801 mkII     | 1983-11 |   z80A @ 4   |    X    |     X     |       -       |    -    |  -  |   -   | (FDM) | (K1)
    PC-8001 mkII SR  | 1985-01 |   z80A @ 4   |    X    |     -     |       -       |    -    |  -  |   -   |   -   | (GE),(NE),(KO)
    PC-8801 mkII SR  | 1985-03 |   z80A @ 4   |    X    |     X     |       X       |    X    |  -  |   -   | (FDM) | (K1)
    PC-8801 mkII TR  | 1985-10 |   z80A @ 4   |    X    |     X     |       X       |    X    |  -  |   -   | (FD2) | (K1)
    PC-8801 mkII FR  | 1985-11 |   z80A @ 4   |    X    |     X     |       X       |    X    |  -  |   -   | (FDM) | (K1)
    PC-8801 mkII MR  | 1985-11 |   z80A @ 4   |    X    |     X     |       X       |    X    |  -  |   -   | (FDH) | (K2)
    PC-8801 FH       | 1986-11 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  -  |   -   | (FDM) | (K2)
    PC-8801 MH       | 1986-11 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  -  |   -   | (FDH) | (K2)
    PC-88 VA         | 1987-03 | z80H+v30 @ 8 |    -    |     X     |       X       |    X    |  -  |   X   | (FDH) | (K2)
    PC-8801 FA       | 1987-11 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  -  |   -   | (FD2) | (K2)
    PC-8801 MA       | 1987-11 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  -  |   X   | (FDH) | (K2)
    PC-88 VA2        | 1988-03 | z80H+v30 @ 8 |    -    |     X     |       X       |    X    |  -  |   X   | (FDH) | (K2)
    PC-88 VA3        | 1988-03 | z80H+v30 @ 8 |    -    |     X     |       X       |    X    |  -  |   X   | (FD3) | (K2)
    PC-8801 FE       | 1988-10 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  -  |   -   | (FD2) | (K2)
    PC-8801 MA2      | 1988-10 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  -  |   X   | (FDH) | (K2)
    PC-98 DO         | 1989-06 |   z80H @ 8   |    X    |     X     |       X       |    X    |  -  |   -   | (FDH) | (KE)
    PC-8801 FE2      | 1989-10 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  -  |   -   | (FD2) | (K2)
    PC-8801 MC       | 1989-11 |  z80H @ 4/8  |    X    |     X     |       X       |    X    |  X  |   X   | (FDH) | (K2)
    PC-98 DO+        | 1990-10 |   z80H @ 8   |    X    |     X     |       X       |    X    |  -  |   -   | (FDH) | (KE)

    info for PC-98 DO & DO+ refers to their 88-mode

    Disk Drive options:
    (FDM): there exist three model of this computer: Model 10 (base model, only optional floppy drive), Model 20
        (1 floppy drive for 5.25" 2D disks) and Model 30 (2 floppy drive for 5.25" 2D disks)
    (FD2): 2 floppy drive for 5.25" 2D disks
    (FDH): 2 floppy drive for both 5.25" 2D disks and 5.25" HD disks
    (FD3): 2 floppy drive for both 5.25" 2D disks and 5.25" HD disks + 1 floppy drive for 3.5" 2TD disks

    Notes:
    (U): US version
    (GE): Graphic Expansion for PC-8001
    (NE): N-BASIC Expansion for PC-8001 (similar to N88-BASIC Expansion for PC-88xx)
    (KO): Optional Kanji ROM
    (K1): Kanji 1st Level ROM
    (K2): Kanji 2nd Level ROM
    (KE): Kanji Enhanced ROM

    Memory mounting locations:
     * N-BASIC 0x0000 - 0x5fff, N-BASIC Expansion & Graph Enhhancement 0x6000 - 0x7fff
     * N-BASIC 0x0000 - 0x5fff, N-BASIC Expansion & Graph Enhhancement 0x6000 - 0x7fff
     * N88-BASIC 0x0000 - 0x7fff, N88-BASIC Expansion & Graph Enhhancement 0x6000 - 0x7fff
     * Sound BIOS: 0x6000 - 0x7fff
     * CD-ROM BIOS: 0x0000 - 0x7fff
     * Dictionary: 0xc000 - 0xffff (32 Banks)

    info from http://www.geocities.jp/retro_zzz/machines/nec/cmn_roms.html
    also, refer to http://www.geocities.jp/retro_zzz/machines/nec/cmn_vers.html for
        info about BASIC revisions in the various models (BASIC V2 is the BASIC
        Expansion, if I unerstood correctly)

*************************************************************************************************************************************/


#include "emu.h"
#include "cpu/z80/z80.h"
#include "imagedev/cassette.h"
#include "imagedev/flopdrv.h"
#include "machine/ctronics.h"
#include "machine/i8255a.h"
#include "machine/upd1990a.h"
#include "machine/upd765.h"
#include "machine/i8214.h"
#include "sound/2203intf.h"
#include "sound/beep.h"
//#include "includes/pc8801.h"

static UINT32 bankr[5],bankw[5];
static UINT8 i8255_0_pc,i8255_1_pc;
static UINT8 fdc_irq_opcode;
static UINT8 ext_rom_bank,gfx_ctrl,vram_sel,misc_ctrl,device_ctrl_data;
static UINT8 vrtc_irq_mask,rtc_irq_mask,sound_irq_mask;
static UINT8 window_offset_bank;
static UINT8 layer_mask;
static UINT8 i8214_irq_level,i8214_irq_state;
static UINT16 dma_counter[4],dma_address[4];
static UINT8 dmac_mode;
static UINT8 alu_ctrl1,alu_ctrl2;
static UINT8 extram_mode,extram_bank;
static UINT8 txt_width, txt_color;
static int vblank_pos;

static void pc8801_update_irq(running_machine *machine)
{
	int level, i;

	level = -1;
	for (i = 0; i < 8; i++)
	{
		if ((i8214_irq_state & (1<<i)) != 0)
			level = i;
	}

	if (level >= 0 && level < i8214_irq_level)
		cputag_set_input_line(machine, "maincpu", 0, HOLD_LINE);
}

static void pc8801_raise_irq(running_machine *machine, int level)
{
	/* TODO ... */
	i8214_irq_state = (1<<level);
	pc8801_update_irq(machine);
}

/*
CRTC command params:
0. CRTC reset

[0] -xxx xxxx screen columns (+2)
[1] xx-- ---- blink speed (in frame unit) (+1, << 3)
[1] --xx xxxx screen lines (+1)
[2] x--- ---- "skip line"
[2] -x-- ---- cursor style (reverse on / underscore off)
[2] --x- ---- cursor blink on/off

[4] x--- ---- attribute not separate flag
[4] -x-- ---- attribute color flag
[4] --x- ---- attribute not special flag (invalidates next register)
[4] ---x xxxx attribute size (+1)
*/

#define blink_speed ((((crtc.param[0][1] & 0xc0) >> 6) + 1) << 3)
#define text_color_flag (crtc.param[0][4] & 0x40)

static struct
{
	UINT8 cmd,param_count,cursor_on,status,irq_mask;
	UINT8 param[8][5];
}crtc;

#define NBASIC_BASE 0x80000
#define N88BASIC_BASE 0x88000
#define WRAM_BASE 0x00000
#define GRAM_BASE 0xc0000
#define ALU_BASE 0xa0000

static VIDEO_START( pc8801 )
{

}

static void draw_bitmap_3bpp(running_machine *machine, bitmap_t *bitmap)
{
	int x,y,xi;
	UINT32 count;
	UINT8 *gvram = machine->region("maincpu")->base() + GRAM_BASE;

	count = 0;

	for(y=0;y<200;y++)
	{
		for(x=0;x<640;x+=8)
		{
			for(xi=0;xi<8;xi++)
			{
				int pen;

				pen = 0;

				if(!(layer_mask & 2)) { pen |= ((gvram[count+0x0000] >> (7-xi)) & 1) << 0; }
				if(!(layer_mask & 4)) { pen |= ((gvram[count+0x4000] >> (7-xi)) & 1) << 1; }
				if(!(layer_mask & 8)) { pen |= ((gvram[count+0x8000] >> (7-xi)) & 1) << 2; }

				*BITMAP_ADDR16(bitmap, y, x+xi) = machine->pens[pen & 7];
			}

			count++;
		}
	}
}

static void draw_bitmap_1bpp(running_machine *machine, bitmap_t *bitmap)
{
	int x,y,xi;
	UINT32 count;
	UINT8 *gvram = machine->region("maincpu")->base() + GRAM_BASE;

	count = 0;

	for(y=0;y<200;y++)
	{
		for(x=0;x<640;x+=8)
		{
			for(xi=0;xi<8;xi++)
			{
				int pen;

				pen = 0;
				/* TODO: dunno if layer_mask is correct here */
				if(!(layer_mask & 2)) { pen = ((gvram[count+0x0000] >> (7-xi)) & 1) << 0; }

				*BITMAP_ADDR16(bitmap, y, x+xi) = machine->pens[pen ? 7 : 0];
			}

			count++;
		}
	}

	count = 0;

	if(!(gfx_ctrl & 1)) // 400 lines
	{
		for(y=200;y<400;y++)
		{
			for(x=0;x<640;x+=8)
			{
				for(xi=0;xi<8;xi++)
				{
					int pen;

					pen = 0;
					/* TODO: dunno if layer_mask is correct here */
					if(!(layer_mask & 4)) { pen = ((gvram[count+0x4000] >> (7-xi)) & 1) << 0; }

					*BITMAP_ADDR16(bitmap, y, x+xi) = machine->pens[pen ? 7 : 0];
				}

				count++;
			}
		}
	}
}

static UINT8 calc_cursor_pos(running_machine *machine,int x,int y,int yi)
{
	if(!(crtc.cursor_on)) // don't bother if cursor is off
		return 0;

	if(x == crtc.param[4][0] && y == crtc.param[4][1]) /* check if position matches */
	{
		/* don't pass through if we are using underscore */
		if((!(crtc.param[0][2] & 0x40)) && yi != 7)
			return 0;

		/* finally check if blinking is currently active high */
		if(!(crtc.param[0][2] & 0x20))
			return 1;

		if(((machine->primary_screen->frame_number() / blink_speed) & 1) == 0)
			return 1;

		return 0;
	}

	return 0;
}



static UINT8 extract_text_attribute(running_machine *machine,UINT32 address,int x)
{
	UINT8 *vram = machine->region("maincpu")->base() + WRAM_BASE;
	int i;
	int fifo_size;

	if(crtc.param[0][4] & 0x80)
	{
		popmessage("Using non-separate mode for text tilemap, contact MESSdev");
		return 0;
	}

	fifo_size = (crtc.param[0][4] & 0x20) ? 0 : ((crtc.param[0][4] & 0x1f) + 1);

	for(i=0;i<fifo_size;i++)
	{
		/* TODO: DMA timing bug? N-BASIC attributes doesn't work here without +2 here ... */
		if(x < vram[address])
		{
			return vram[address+1];
		}
		else
			address+=2;
	}

	return 0;
}

static void draw_text_80(running_machine *machine, bitmap_t *bitmap,int y_size)
{
	int x,y;
	int xi,yi;
	UINT8 *vram = machine->region("maincpu")->base() + WRAM_BASE;
	UINT8 is_cursor;
	UINT8 y_height;
	UINT8 attr;
	UINT8 reverse;
	UINT8 pal;
	UINT8 gfx_mode;

	y_height = y_size == 20 ? 10 : 8;

	for(y=0;y<y_size;y++)
	{
		for(x=0;x<80;x++)
		{
			attr = extract_text_attribute(machine,(((y*120)+80+dma_address[2]) & 0xffff),(x));

			if(text_color_flag)
			{
				pal = (attr & 0xe0) >> 5;
				gfx_mode = (attr & 0x10) >> 4;
			}
			else
			{
				pal = 7;
				reverse = attr & 4;
			}

			pal|=8; //text pal bank

			for(yi=0;yi<8;yi++)
			{
				is_cursor = calc_cursor_pos(machine,x,y,yi);

				for(xi=0;xi<8;xi++)
				{
					int res_x,res_y;
					int tile;
					int color;

					{
						UINT8 *gfx_data = machine->region("gfx1")->base();

						tile = vram[x+(y*120)+dma_address[2]];

						res_x = x*8+xi;
						res_y = y*y_height+yi;

						if(is_cursor || reverse)
							color = ((gfx_data[tile*8+yi] >> (7-xi)) & 1) ? -1 : pal;
						else
							color = ((gfx_data[tile*8+yi] >> (7-xi)) & 1) ? pal : -1;

						if((res_x)<=machine->primary_screen->visible_area().max_x && (res_y)<=machine->primary_screen->visible_area().max_y)
							if(color != -1)
								*BITMAP_ADDR16(bitmap, res_y, res_x) = machine->pens[color];
					}
				}
			}
		}
	}
}

static void draw_text_40(running_machine *machine, bitmap_t *bitmap, int y_size)
{
	int x,y;
	int xi,yi;
	UINT8 *vram = machine->region("maincpu")->base() + WRAM_BASE;
	UINT8 is_cursor;
	UINT8 y_height;
	UINT8 attr;
	UINT8 reverse;
	UINT8 pal;

	y_height = y_size == 20 ? 10 : 8;

	for(y=0;y<y_size;y++)
	{
		for(x=0;x<80;x++)
		{
			if(x & 1)
				continue;

			attr = extract_text_attribute(machine,(((y*120)+80+dma_address[2]) & 0xffff),(x));

			if(text_color_flag)
				pal = (attr & 0xe0) >> 5;
			else
				pal = 7;

			pal|=8; //text pal bank

			reverse = attr & 4;

			for(yi=0;yi<8;yi++)
			{
				is_cursor = calc_cursor_pos(machine,x,y,yi);

				for(xi=0;xi<8;xi++)
				{
					int res_x,res_y;
					int tile;
					int color;
					UINT8 *gfx_data = machine->region("gfx1")->base();

					tile = vram[x+(y*120)+dma_address[2]];

					res_x = (x*8)+xi*2;
					res_y = (y*y_height)+yi;

					if(is_cursor || reverse)
						color = ((gfx_data[tile*8+yi] >> (7-xi)) & 1) ? -1 : pal;
					else
						color = ((gfx_data[tile*8+yi] >> (7-xi)) & 1) ? pal : -1;

					if((res_x)<=machine->primary_screen->visible_area().max_x && (res_y)<=machine->primary_screen->visible_area().max_y)
						if(color != -1)
							*BITMAP_ADDR16(bitmap, res_y, res_x) = machine->pens[color];
					if((res_x+1)<=machine->primary_screen->visible_area().max_x && (res_y)<=machine->primary_screen->visible_area().max_y)
						if(color != -1)
							*BITMAP_ADDR16(bitmap, res_y, res_x+1) = machine->pens[color];
				}
			}
		}
	}
}

static VIDEO_UPDATE( pc8801 )
{
	bitmap_fill(bitmap, cliprect, screen->machine->pens[0]);

	if(gfx_ctrl & 8)
	{
		if(gfx_ctrl & 0x10)
			draw_bitmap_3bpp(screen->machine,bitmap);
		else
			draw_bitmap_1bpp(screen->machine,bitmap);
	}

	if(!(layer_mask & 1) && dmac_mode & 4 && crtc.status & 0x10 && crtc.irq_mask == 3)
	{
		static int y_size;

		//popmessage("%02x %02x",crtc.param[0][0],crtc.param[0][4]);

		y_size = (crtc.param[0][1] & 0x3f) + 1;

		if(y_size < 20) y_size = 20;
		if(y_size > 25) y_size = 25;

		if(txt_width)
			draw_text_80(screen->machine,bitmap,y_size);
		else
			draw_text_40(screen->machine,bitmap,y_size);
	}

	return 0;
}

static READ8_HANDLER( pc8801_mem_0_r )
{
	UINT8 *ram = space->machine->region("maincpu")->base();

	return ram[offset + bankr[0]];
}

static WRITE8_HANDLER( pc8801_mem_0_w )
{
	UINT8 *ram = space->machine->region("maincpu")->base();

	ram[offset + bankw[0]] = data;
}

static READ8_HANDLER( pc8801_mem_1_r )
{
	UINT8 *ram = space->machine->region("maincpu")->base();

	return ram[offset + bankr[1]];
}

static WRITE8_HANDLER( pc8801_mem_1_w )
{
	UINT8 *ram = space->machine->region("maincpu")->base();

	ram[offset + bankw[1]] = data;
}

static READ8_HANDLER( pc8801_mem_2_r )
{
	UINT8 *ram = space->machine->region("maincpu")->base();

	offset &= 0x3ff;

	return ram[offset + bankr[2]];
}

static WRITE8_HANDLER( pc8801_mem_2_w )
{
	UINT8 *ram = space->machine->region("maincpu")->base();

	offset &= 0x3ff;

	ram[offset + bankw[2]] = data;
}

static UINT8 alu_r(running_machine *machine, UINT16 ram_addr)
{
	int i;
	UINT8 *gvram = machine->region("maincpu")->base();
	UINT8 b,r,g;

	/* store data to ALU regs */
	for(i=0;i<3;i++)
	{
		gvram[ALU_BASE + i] = gvram[GRAM_BASE + i*0x4000 + ram_addr];
	}

	b = gvram[GRAM_BASE + ram_addr + 0x0000];
	r = gvram[GRAM_BASE + ram_addr + 0x4000];
	g = gvram[GRAM_BASE + ram_addr + 0x8000];
	if(!(alu_ctrl2 & 1)) { b^=0xff; }
	if(!(alu_ctrl2 & 2)) { r^=0xff; }
	if(!(alu_ctrl2 & 4)) { g^=0xff; }

	return b & r & g;
}

static void alu_w(running_machine *machine, UINT16 ram_addr,UINT8 data)
{
	int i;
	UINT8 *gvram = machine->region("maincpu")->base();

	switch(alu_ctrl2 & 0x30) // alu write mode
	{
		case 0x00: //logic operation
		{
			UINT8 logic_op;

			for(i=0;i<3;i++)
			{
				logic_op = (alu_ctrl1 & (0x11 << i)) >> i;

				switch(logic_op)
				{
					case 0x00: { gvram[GRAM_BASE + i*0x4000 + ram_addr] &= ~data; } break;
					case 0x01: { gvram[GRAM_BASE + i*0x4000 + ram_addr] |= data; } break;
					case 0x10: { gvram[GRAM_BASE + i*0x4000 + ram_addr] ^= data; } break;
					case 0x11: break; // NOP
				}
			}
		}
		break;

		case 0x10: // restore data from ALU regs
		{
			for(i=0;i<3;i++)
				gvram[GRAM_BASE + i*0x4000 + ram_addr] = gvram[ALU_BASE + i];
		}
		break;

		case 0x20: // swap ALU reg 1 into R GVRAM
			gvram[GRAM_BASE + 0x0000 + ram_addr] = gvram[ALU_BASE + 1];
			break;

		case 0x30: // swap ALU reg 0 into B GVRAM
			gvram[GRAM_BASE + 0x4000 + ram_addr] = gvram[ALU_BASE + 0];
			break;
	}
}

static READ8_HANDLER( pc8801_mem_3_r )
{
	UINT8 *ram = space->machine->region("maincpu")->base();

	if(bankr[3] == 0xa0000)
		return alu_r(space->machine,(offset + bankr[3]) & 0x3fff);

	return ram[offset + bankr[3]];
}

static WRITE8_HANDLER( pc8801_mem_3_w )
{
	UINT8 *ram = space->machine->region("maincpu")->base();

	if(bankw[3] == 0xa0000)
		alu_w(space->machine,(offset + bankw[3]) & 0x3fff,data);
	else
		ram[offset + bankw[3]] = data;
}

static READ8_HANDLER( pc8801_mem_4_r )
{
	UINT8 *ram = space->machine->region("maincpu")->base();

	if(bankr[4] == 0xa3000)
		return alu_r(space->machine,(offset + bankr[4]) & 0x3fff);

	return ram[offset + bankr[4]];
}

static WRITE8_HANDLER( pc8801_mem_4_w )
{
	UINT8 *ram = space->machine->region("maincpu")->base();

	if(bankw[4] == 0xa3000)
		alu_w(space->machine,(offset + bankw[4]) & 0x3fff,data);
	else
		ram[offset + bankw[4]] = data;
}

static UINT32 pc8801_bankswitch_0_r(running_machine *machine)
{
	if(extram_mode & 1) // access R EXT RAM
		return WRAM_BASE + 0x20000 + extram_bank * 0x8000;

	if(gfx_ctrl & 0x2) //wram read select
		return WRAM_BASE;

	if(gfx_ctrl & 0x4) //n basic select
		return NBASIC_BASE;

	return N88BASIC_BASE;
}

static UINT32 pc8801_bankswitch_1_r(running_machine *machine)
{
	if(extram_mode & 1) // access R EXT RAM
		return WRAM_BASE + 0x20000 + 0x6000 + (extram_bank * 0x8000);

	if(gfx_ctrl & 0x2) //wram read select
		return WRAM_BASE + 0x6000;

	if(gfx_ctrl & 0x4) //n basic select
		return NBASIC_BASE + 0x6000;

	if(ext_rom_bank & 1) // NOT ext ROM bank
		return N88BASIC_BASE + 0x6000;

	return N88BASIC_BASE + 0x6000 + (0x2000 * ((misc_ctrl & 3)+1)); // ext ROM bank
}

static UINT32 pc8801_bankswitch_0_w(running_machine *machine)
{
	if(extram_mode & 0x10) // access W EXT RAM
		return WRAM_BASE + 0x20000 + (extram_bank * 0x8000);

	return WRAM_BASE;
}

static UINT32 pc8801_bankswitch_1_w(running_machine *machine)
{
	if(extram_mode & 0x10) // access W EXT RAM
		return WRAM_BASE + 0x20000 + 0x6000 + (extram_bank * 0x8000);

	return WRAM_BASE + 0x6000;
}

static UINT32 pc8801_bankswitch_2_r(running_machine *machine)
{
	if(gfx_ctrl & 6) //wram read select or n basic select banks this as normal wram
		return WRAM_BASE + 0x8000;

	return WRAM_BASE + (window_offset_bank << 8);
}

static UINT32 pc8801_bankswitch_2_w(running_machine *machine)
{
	if(gfx_ctrl & 6) //wram read select or n basic select banks this as normal wram
		return WRAM_BASE + 0x8000;

	return WRAM_BASE + (window_offset_bank << 8);
}

static UINT32 pc8801_bankswitch_3_r(running_machine *machine)
{
	if(misc_ctrl & 0x40)
	{
		vram_sel = 3;
		if(alu_ctrl2 & 0x80)
			return ALU_BASE;
	}

	if(vram_sel == 3)
		return WRAM_BASE + 0xc000;

	return GRAM_BASE + 0x4000 * vram_sel;
}

static UINT32 pc8801_bankswitch_3_w(running_machine *machine)
{
	if(misc_ctrl & 0x40)
	{
		vram_sel = 3;
		if(alu_ctrl2 & 0x80)
			return ALU_BASE;
	}

	if(vram_sel == 3)
		return WRAM_BASE + 0xc000;

	return GRAM_BASE + 0x4000 * vram_sel;
}

static UINT32 pc8801_bankswitch_4_r(running_machine *machine)
{
	if(misc_ctrl & 0x40)
	{
		vram_sel = 3;
		if(alu_ctrl2 & 0x80)
			return ALU_BASE + 0x3000;
	}

	if(vram_sel == 3)
	{
		if((misc_ctrl & 0x10) == 0) // high VRAM
			return WRAM_BASE + 0xf000;

		return WRAM_BASE + 0xf000 + 0x10000;
	}

	return GRAM_BASE + 0x3000 + (0x4000 * vram_sel);
}

static UINT32 pc8801_bankswitch_4_w(running_machine *machine)
{
	if(misc_ctrl & 0x40)
	{
		vram_sel = 3;
		if(alu_ctrl2 & 0x80)
			return ALU_BASE + 0x3000;
	}

	if(vram_sel == 3)
	{
		if((misc_ctrl & 0x10) == 0) // high VRAM
			return WRAM_BASE + 0xf000;

		return WRAM_BASE + 0xf000 + 0x10000;
	}

	return GRAM_BASE + 0x3000 + (0x4000 * vram_sel);
}


static ADDRESS_MAP_START( pc8801_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_READWRITE(pc8801_mem_0_r,pc8801_mem_0_w)
	AM_RANGE(0x6000, 0x7fff) AM_READWRITE(pc8801_mem_1_r,pc8801_mem_1_w)
	AM_RANGE(0x8000, 0x83ff) AM_READWRITE(pc8801_mem_2_r,pc8801_mem_2_w)
	AM_RANGE(0x8400, 0xbfff) AM_RAM
	AM_RANGE(0xc000, 0xefff) AM_READWRITE(pc8801_mem_3_r,pc8801_mem_3_w)
	AM_RANGE(0xf000, 0xffff) AM_READWRITE(pc8801_mem_4_r,pc8801_mem_4_w)
ADDRESS_MAP_END

static READ8_HANDLER( pc8801_ctrl_r )
{
	static UINT8 vrtc;

	/*
	--x- ---- vrtc
	---x ---- calendar CDO
	---- x--- (fdc related)
	---- -x-- (RS-232C related)
	---- --x- monitor refresh rate DIP-SW
	---- ---x (pbsy?)
	*/

	vrtc = (space->machine->primary_screen->vpos() < vblank_pos) ? 0 : 1; // vblank

	return (vrtc << 5) | 0xc0;
}

static WRITE8_HANDLER( pc8801_ctrl_w )
{
	/*
	x--- ---- beeper (mirror?)
	-x-- ---- mouse latch (JOP1, routes on OPN sound port A)
	--x- ---- beeper
	---
	*/

	/* TODO: this might actually be two beepers */
	if(((device_ctrl_data & 0x20) == 0x00) && ((data & 0x20) == 0x20))
		beep_set_state(space->machine->device("beeper"),1);

	if(((device_ctrl_data & 0x80) == 0x00) && ((data & 0x80) == 0x80))
		beep_set_state(space->machine->device("beeper"),1);

	if(((device_ctrl_data & 0x20) == 0x20) && ((data & 0x20) == 0x00))
		beep_set_state(space->machine->device("beeper"),0);

	if(((device_ctrl_data & 0x80) == 0x80) && ((data & 0x80) == 0x00))
		beep_set_state(space->machine->device("beeper"),0);

	device_ctrl_data = data;
}

static READ8_HANDLER( pc8801_ext_rom_bank_r )
{
	return ext_rom_bank;
}

static WRITE8_HANDLER( pc8801_ext_rom_bank_w )
{
	ext_rom_bank = data;
	bankr[0] = pc8801_bankswitch_0_r(space->machine);
	bankr[1] = pc8801_bankswitch_1_r(space->machine);
	bankw[0] = pc8801_bankswitch_0_w(space->machine);
	bankw[1] = pc8801_bankswitch_1_w(space->machine);
}

static void pc8801_dynamic_res_change(running_machine *machine)
{
	rectangle visarea;

	visarea.min_x = 0;
	visarea.min_y = 0;
	visarea.max_x = 640 - 1;
	visarea.max_y = ((gfx_ctrl & 1) ? 200 : 400) - 1;

	/* low screen res if graphic screen is disabled */
	if((gfx_ctrl & 0x8) == 0)
		visarea.max_y = 200 - 1;

	/* low screen res if graphic screen is color */
	if(gfx_ctrl & 0x10)
		visarea.max_y = 200 - 1;

	vblank_pos = visarea.max_y + 1;

	machine->primary_screen->configure(640, 480, visarea, machine->primary_screen->frame_period().attoseconds);
}

static WRITE8_HANDLER( pc8801_gfx_ctrl_w )
{
	/*
	--x- ---- VRAM y 25 (1) / 20 (0)
	---x ---- graphic color yes (1) / no (0)
	---- x--- graphic display yes (1) / no (0)
	---- -x-- Basic N (1) / N88 (0)
	---- --x- RAM select yes (1) / no (0)
	---- ---x VRAM 200 lines (1) / 400 lines (0)
	*/

	gfx_ctrl = data;
	bankr[0] = pc8801_bankswitch_0_r(space->machine);
	bankr[1] = pc8801_bankswitch_1_r(space->machine);
	bankw[0] = pc8801_bankswitch_0_w(space->machine);
	bankw[1] = pc8801_bankswitch_1_w(space->machine);

	pc8801_dynamic_res_change(space->machine);
}

static READ8_HANDLER( pc8801_vram_select_r )
{
	return 0xf8 | ((vram_sel == 3) ? 0 : (1 << vram_sel));
}

static WRITE8_HANDLER( pc8801_vram_select_w )
{
	vram_sel = offset & 3;
	bankr[3] = pc8801_bankswitch_3_r(space->machine);
	bankr[4] = pc8801_bankswitch_4_r(space->machine);
	bankw[3] = pc8801_bankswitch_3_w(space->machine);
	bankw[4] = pc8801_bankswitch_4_w(space->machine);
}

static WRITE8_HANDLER( pc8801_irq_level_w )
{
	if(data & 8)
		i8214_irq_level = 7;
	else
		i8214_irq_level = data & 7;
}


static WRITE8_HANDLER( pc8801_irq_mask_w )
{
	rtc_irq_mask = data & 1;
	vrtc_irq_mask = data & 2;

	//if(data & 4)
	//	printf("IRQ mask %02x\n",data);
}

static READ8_HANDLER( pc8801_window_bank_r )
{
	return window_offset_bank;
}

static WRITE8_HANDLER( pc8801_window_bank_w )
{
	window_offset_bank = data;
	bankr[2] = pc8801_bankswitch_2_r(space->machine);
	bankw[2] = pc8801_bankswitch_2_w(space->machine);
}

static WRITE8_HANDLER( pc8801_window_bank_inc_w )
{
	window_offset_bank++;
	window_offset_bank&=0xff;
	bankr[2] = pc8801_bankswitch_2_r(space->machine);
	bankw[2] = pc8801_bankswitch_2_w(space->machine);
}

static READ8_HANDLER( pc8801_misc_ctrl_r )
{
	return misc_ctrl;
}

static WRITE8_HANDLER( pc8801_misc_ctrl_w )
{
	misc_ctrl = data;

	bankr[1] = pc8801_bankswitch_1_r(space->machine);
	bankr[3] = pc8801_bankswitch_3_r(space->machine);
	bankr[4] = pc8801_bankswitch_4_r(space->machine);
	bankw[3] = pc8801_bankswitch_3_w(space->machine);
	bankw[4] = pc8801_bankswitch_4_w(space->machine);

	sound_irq_mask = ((data & 0x80) == 0);
}

static WRITE8_HANDLER( pc8801_palram_w )
{
	static UINT8 b[8],r[8],g[8];

	if(misc_ctrl & 0x20) //analog palette
	{
		if((data & 0x40) == 0)
		{
			b[offset] = data & 0x7;
			r[offset] = (data & 0x38) >> 3;
		}
		else
			g[offset] = data & 0x7;

	}
	else //digital palette
	{
		b[offset] = data & 1 ? 7 : 0;
		r[offset] = data & 2 ? 7 : 0;
		g[offset] = data & 4 ? 7 : 0;
	}

	palette_set_color_rgb(space->machine, offset, pal3bit(r[offset]), pal3bit(g[offset]), pal3bit(b[offset]));
}

static WRITE8_HANDLER( pc8801_layer_masking_w )
{
	/*
	---- x--- green gvram masked flag
	---- -x-- red gvram masked flag
	---- --x- blue gvram masked flag
	---- ---x text vram masked
	*/

	layer_mask = data;
}

static READ8_HANDLER( pc8801_crtc_param_r )
{
	return 0xff;
}

static WRITE8_HANDLER( pc88_crtc_param_w )
{
	if(crtc.param_count < 5)
	{
		crtc.param[crtc.cmd][crtc.param_count] = data;
		crtc.param_count++;
	}
}

static READ8_HANDLER( pc8801_crtc_status_r )
{
	/*
	---x ---- video enable
	---- x--- DMA is running
	---- -x-- special control character IRQ
	---- --x- indication end IRQ
	---- ---x light pen input
	*/

	return crtc.status;
}

static const char *const crtc_command[] =
{
	"Reset / Stop Display",				// 0
	"Start Display", 					// 1
	"Set IRQ MASK",						// 2
	"Read Light Pen",					// 3
	"Load Cursor Position",				// 4
	"Reset IRQ",						// 5
	"Reset Counters",					// 6
	"Read Status"						// 7
};

static WRITE8_HANDLER( pc88_crtc_cmd_w )
{
	crtc.cmd = (data & 0xe0) >> 5;
	crtc.param_count = 0;

	switch(crtc.cmd)
	{
		case 0:  // reset CRTC
			crtc.status &= (~0x10 | ~0x04 | ~0x02);
			break;
		case 1:  // start display
			crtc.status |= 0x10;
			crtc.status &= (~0x08);
			if(data & 1)
				printf("CRTC reverse display ON\n");

			break;
		case 2:  // set irq mask
			crtc.irq_mask = data & 3;
			break;
		case 3:  // read light pen
			crtc.status &= (~0x01);
			break;
		case 4:  // load cursor position ON/OFF
			crtc.cursor_on = data & 1;
			break;
		case 5:  // reset IRQ
		case 6:  // reset counters
			crtc.status &= (~0x04 | ~0x02);
			break;
	}

	//if((data >> 5) != 4)
	//	printf("CRTC cmd %s polled\n",crtc_command[data >> 5]);
}

static WRITE8_HANDLER( pc8801_dmac_w )
{
	static UINT8 dmac_ff;

	if(offset & 1)
		dma_counter[offset >> 1] = (dmac_ff) ? (dma_counter[offset >> 1]&0xff)|(data<<8) : (dma_counter[offset >> 1]&0xff00)|(data&0xff);
	else
		dma_address[offset >> 1] = (dmac_ff) ? (dma_address[offset >> 1]&0xff)|(data<<8) : (dma_address[offset >> 1]&0xff00)|(data&0xff);

	dmac_ff ^= 1;
}

static WRITE8_HANDLER( pc8801_dmac_mode_w )
{
	dmac_mode = data;
}

static READ8_HANDLER( pc8801_extram_mode_r )
{
	return (extram_mode ^ 0x11) | 0xee;
}

static WRITE8_HANDLER( pc8801_extram_mode_w )
{
	/*
	---x ---- Write EXT RAM access at 0x0000 - 0x7fff
	---- ---x Read EXT RAM access at 0x0000 - 0x7fff
	*/

	extram_mode = data & 0x11;
	bankr[0] = pc8801_bankswitch_0_r(space->machine);
	bankr[1] = pc8801_bankswitch_1_r(space->machine);
	bankw[0] = pc8801_bankswitch_0_w(space->machine);
	bankw[1] = pc8801_bankswitch_1_w(space->machine);
}

static READ8_HANDLER( pc8801_extram_bank_r )
{
	return extram_bank;
}

static WRITE8_HANDLER( pc8801_extram_bank_w )
{
	extram_bank = data;
	bankr[0] = pc8801_bankswitch_0_r(space->machine);
	bankr[1] = pc8801_bankswitch_1_r(space->machine);
	bankw[0] = pc8801_bankswitch_0_w(space->machine);
	bankw[1] = pc8801_bankswitch_1_w(space->machine);
}

static WRITE8_HANDLER( pc8801_alu_ctrl1_w )
{
	alu_ctrl1 = data;
	bankr[3] = pc8801_bankswitch_3_r(space->machine);
	bankr[4] = pc8801_bankswitch_4_r(space->machine);
	bankw[3] = pc8801_bankswitch_3_w(space->machine);
	bankw[4] = pc8801_bankswitch_4_w(space->machine);
}

static WRITE8_HANDLER( pc8801_alu_ctrl2_w )
{
	alu_ctrl2 = data;
	bankr[3] = pc8801_bankswitch_3_r(space->machine);
	bankr[4] = pc8801_bankswitch_4_r(space->machine);
	bankw[3] = pc8801_bankswitch_3_w(space->machine);
	bankw[4] = pc8801_bankswitch_4_w(space->machine);
}

static WRITE8_HANDLER( pc8801_pcg8100_w )
{
	if(data)
	printf("Write to PCG-8100 %02x %02x\n",offset,data);
}

/* Balance of Power temp work-around */
static READ8_HANDLER( sio_status_r )
{
	return 0;
}

static WRITE8_HANDLER( pc8801_txt_cmt_ctrl_w )
{
	/* bits 2 to 5 are cmt related */

	txt_width = data & 1;
	txt_color = data & 2;
}

static ADDRESS_MAP_START( pc8801_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x00) AM_READ_PORT("KEY0")
	AM_RANGE(0x01, 0x01) AM_READ_PORT("KEY1")
	AM_RANGE(0x02, 0x02) AM_READ_PORT("KEY2")
	AM_RANGE(0x03, 0x03) AM_READ_PORT("KEY3")
	AM_RANGE(0x04, 0x04) AM_READ_PORT("KEY4")
	AM_RANGE(0x05, 0x05) AM_READ_PORT("KEY5")
	AM_RANGE(0x06, 0x06) AM_READ_PORT("KEY6")
	AM_RANGE(0x07, 0x07) AM_READ_PORT("KEY7")
	AM_RANGE(0x08, 0x08) AM_READ_PORT("KEY8")
	AM_RANGE(0x09, 0x09) AM_READ_PORT("KEY9")
	AM_RANGE(0x0a, 0x0a) AM_READ_PORT("KEY10")
	AM_RANGE(0x0b, 0x0b) AM_READ_PORT("KEY11")
	AM_RANGE(0x0c, 0x0c) AM_READ_PORT("KEY12")
	AM_RANGE(0x0d, 0x0d) AM_READ_PORT("KEY13")
	AM_RANGE(0x0e, 0x0e) AM_READ_PORT("KEY14")
	AM_RANGE(0x0f, 0x0f) AM_READ_PORT("KEY15")
	AM_RANGE(0x00, 0x02) AM_WRITE(pc8801_pcg8100_w)
//	AM_RANGE(0x10, 0x10) AM_WRITE(pc88_rtc_w)
	AM_RANGE(0x21, 0x21) AM_READ(sio_status_r)                                /* RS-232C and cassette */
	AM_RANGE(0x30, 0x30) AM_READ_PORT("DSW1") AM_WRITE(pc8801_txt_cmt_ctrl_w)
	AM_RANGE(0x31, 0x31) AM_READ_PORT("DSW2") AM_WRITE(pc8801_gfx_ctrl_w)
	AM_RANGE(0x32, 0x32) AM_READWRITE(pc8801_misc_ctrl_r, pc8801_misc_ctrl_w)
	AM_RANGE(0x34, 0x34) AM_WRITE(pc8801_alu_ctrl1_w)
	AM_RANGE(0x35, 0x35) AM_WRITE(pc8801_alu_ctrl2_w)
	AM_RANGE(0x40, 0x40) AM_READWRITE(pc8801_ctrl_r, pc8801_ctrl_w)
	AM_RANGE(0x44, 0x45) AM_DEVREADWRITE("ym2203", ym2203_r,ym2203_w)
//  AM_RANGE(0x46, 0x47) AM_NOP                                     /* OPNA extra port */
	AM_RANGE(0x50, 0x50) AM_READWRITE(pc8801_crtc_param_r, pc88_crtc_param_w)
	AM_RANGE(0x51, 0x51) AM_READWRITE(pc8801_crtc_status_r, pc88_crtc_cmd_w)
//	AM_RANGE(0x52, 0x52) //background palette
	AM_RANGE(0x53, 0x53) AM_WRITE(pc8801_layer_masking_w)
	AM_RANGE(0x54, 0x5b) AM_WRITE(pc8801_palram_w)
	AM_RANGE(0x5c, 0x5c) AM_READ(pc8801_vram_select_r)
	AM_RANGE(0x5c, 0x5f) AM_WRITE(pc8801_vram_select_w)
	AM_RANGE(0x60, 0x67) AM_WRITE(pc8801_dmac_w)
	AM_RANGE(0x68, 0x68) AM_WRITE(pc8801_dmac_mode_w)
//  AM_RANGE(0x6e, 0x6e) AM_NOP                                     /* CPU clock info */
//  AM_RANGE(0x6f, 0x6f) AM_NOP                                     /* RS-232C speed ctrl */
	AM_RANGE(0x70, 0x70) AM_READWRITE(pc8801_window_bank_r, pc8801_window_bank_w)
	AM_RANGE(0x71, 0x71) AM_READWRITE(pc8801_ext_rom_bank_r, pc8801_ext_rom_bank_w)
	AM_RANGE(0x78, 0x78) AM_WRITE(pc8801_window_bank_inc_w)
//  AM_RANGE(0x90, 0x9f) AM_NOP                                     /* CD-ROM */
//  AM_RANGE(0xa0, 0xa3) AM_NOP                                     /* music & network */
//  AM_RANGE(0xa8, 0xad) AM_NOP                                     /* second sound board */
//  AM_RANGE(0xb4, 0xb5) AM_NOP                                     /* Video art board */
//  AM_RANGE(0xc1, 0xc1) AM_NOP                                     /* (unknown) */
//  AM_RANGE(0xc2, 0xcf) AM_NOP                                     /* music */
//  AM_RANGE(0xd0, 0xd7) AM_NOP                                     /* music & GP-IB */
//  AM_RANGE(0xd8, 0xd8) AM_NOP                                     /* GP-IB */
//  AM_RANGE(0xdc, 0xdf) AM_NOP                                     /* MODEM */
	AM_RANGE(0xe2, 0xe2) AM_READWRITE(pc8801_extram_mode_r,pc8801_extram_mode_w) 			/* expand RAM mode */
	AM_RANGE(0xe3, 0xe3) AM_READWRITE(pc8801_extram_bank_r,pc8801_extram_bank_w) 			/* expand RAM bank */
	AM_RANGE(0xe4, 0xe4) AM_WRITE(pc8801_irq_level_w)
	AM_RANGE(0xe6, 0xe6) AM_WRITE(pc8801_irq_mask_w)
//  AM_RANGE(0xe7, 0xe7) AM_NOP                                     /* (unknown) */
//	AM_RANGE(0xe8, 0xeb) AM_READWRITE(pc88_kanji_r, pc88_kanji_w)
//	AM_RANGE(0xec, 0xed) AM_READWRITE(pc88_kanji2_r, pc88_kanji2_w) /* JIS level2 Kanji ROM */
//  AM_RANGE(0xf0, 0xf1) AM_NOP                                     /* Kana to Kanji dictionary ROM select (not yet) */
//  AM_RANGE(0xf3, 0xf3) AM_NOP                                     /* DMA floppy (unknown) */
//  AM_RANGE(0xf4, 0xf7) AM_NOP                                     /* DMA 5'floppy (may be not released) */
//  AM_RANGE(0xf8, 0xfb) AM_NOP                                     /* DMA 8'floppy (unknown) */
	AM_RANGE(0xfc, 0xff) AM_DEVREADWRITE("d8255_master", i8255a_r, i8255a_w)
ADDRESS_MAP_END

static READ8_DEVICE_HANDLER( cpu_8255_c_r )
{
	timer_call_after_resynch(device->machine, NULL, 0, 0); // force resync

	return i8255_1_pc >> 4;
}

static WRITE8_DEVICE_HANDLER( cpu_8255_c_w )
{
	timer_call_after_resynch(device->machine, NULL, 0, 0); // force resync

	i8255_0_pc = data;
}

static I8255A_INTERFACE( master_fdd_intf )
{
	DEVCB_DEVICE_HANDLER("d8255_slave", i8255a_pb_r),	// Port A read
	DEVCB_NULL,							// Port B read
	DEVCB_HANDLER(cpu_8255_c_r),		// Port C read
	DEVCB_NULL,							// Port A write
	DEVCB_NULL,							// Port B write
	DEVCB_HANDLER(cpu_8255_c_w)			// Port C write
};

static READ8_DEVICE_HANDLER( fdc_8255_c_r )
{
	timer_call_after_resynch(device->machine, NULL, 0, 0); // force resync

	return i8255_0_pc >> 4;
}

static WRITE8_DEVICE_HANDLER( fdc_8255_c_w )
{
	timer_call_after_resynch(device->machine, NULL, 0, 0); // force resync

	i8255_1_pc = data;
}

static I8255A_INTERFACE( slave_fdd_intf )
{
	DEVCB_DEVICE_HANDLER("d8255_master", i8255a_pb_r),	// Port A read
	DEVCB_NULL,							// Port B read
	DEVCB_HANDLER(fdc_8255_c_r),		// Port C read
	DEVCB_NULL,							// Port A write
	DEVCB_NULL,							// Port B write
	DEVCB_HANDLER(fdc_8255_c_w)			// Port C write
};

static ADDRESS_MAP_START( pc8801fdc_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x4000, 0x7fff) AM_RAM
ADDRESS_MAP_END

static TIMER_CALLBACK( pc8801fd_upd765_tc_to_zero )
{
//	pc88va_state *state = machine->driver_data<pc88va_state>();

	upd765_tc_w(machine->device("upd765"), 0);
}

static WRITE8_HANDLER( upd765_mc_w )
{
	floppy_mon_w(floppy_get_device(space->machine, 0), (data & 1) ? CLEAR_LINE : ASSERT_LINE);
	floppy_mon_w(floppy_get_device(space->machine, 1), (data & 2) ? CLEAR_LINE : ASSERT_LINE);
	floppy_drive_set_ready_state(floppy_get_device(space->machine, 0), (data & 1), 0);
	floppy_drive_set_ready_state(floppy_get_device(space->machine, 1), (data & 2), 0);
}

static READ8_HANDLER( upd765_tc_r )
{
	//pc88va_state *state = space->machine->driver_data<pc88va_state>();

	upd765_tc_w(space->machine->device("upd765"), 1);
	timer_set(space->machine,  ATTOTIME_IN_USEC(500), NULL, 0, pc8801fd_upd765_tc_to_zero ); //TODO: timing of this
	return 0xff; // value is meaningless
}

static WRITE8_HANDLER( fdc_irq_vector_w )
{
	fdc_irq_opcode = data;
}

static ADDRESS_MAP_START( pc8801fdc_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xf0, 0xf0) AM_WRITE(fdc_irq_vector_w) // Interrupt Opcode Port
//	AM_RANGE(0xf4, 0xf4) // Drive mode, 2d, 2dd, 2hd
//	AM_RANGE(0xf7, 0xf7) AM_WRITENOP // printer port output
	AM_RANGE(0xf8, 0xf8) AM_READWRITE(upd765_tc_r,upd765_mc_w) // (R) Terminal Count Port (W) Motor Control Port
	AM_RANGE(0xfa, 0xfa) AM_DEVREAD("upd765", upd765_status_r )
	AM_RANGE(0xfb, 0xfb) AM_DEVREADWRITE("upd765", upd765_data_r, upd765_data_w )
	AM_RANGE(0xfc, 0xff) AM_DEVREADWRITE("d8255_slave", i8255a_r, i8255a_w )
ADDRESS_MAP_END

/* Input Ports */

/* 2008-05 FP:
Small note about the strange default mapping of function keys:
the top line of keys in PC8801 keyboard is as follows
[STOP][COPY]      [F1][F2][F3][F4][F5]      [ROLL UP][ROLL DOWN]
Therefore, in Full Emulation mode, "F1" goes to 'F3' and so on

Also, the Keypad has 16 keys, making impossible to map it in a satisfactory
way to a PC keypad. Therefore, default settings for these keys in Full
Emulation are currently based on the effect of the key rather than on
their real position

About natural keyboards: currently,
- "Keypad =" and "Keypad ," are not mapped
- "Stop" is mapped to 'Pause'
- "Copy" is mapped to 'Print Screen'
- "Kana" is mapped to 'F6'
- "Grph" is mapped to 'F7'
- "Roll Up" and "Roll Down" are mapped to 'Page Up' and 'Page Down'
- "Help" is mapped to 'F8'
 */

static INPUT_PORTS_START( pc8001 )
	PORT_START("KEY0")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0_PAD)		PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1_PAD)		PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2_PAD)		PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3_PAD)		PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4_PAD)		PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5_PAD)		PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6_PAD)		PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7_PAD)		PORT_CHAR(UCHAR_MAMEKEY(7_PAD))

	PORT_START("KEY1")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8_PAD)		PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9_PAD)		PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ASTERISK)	PORT_CHAR(UCHAR_MAMEKEY(ASTERISK))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_PLUS_PAD)	PORT_CHAR(UCHAR_MAMEKEY(PLUS_PAD))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad =") PORT_CODE(KEYCODE_PGUP)
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad ,") PORT_CODE(KEYCODE_PGDN)
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DEL_PAD) 	PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER) PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(13)

	PORT_START("KEY2")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('@')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A)			PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B)			PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C)			PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D)			PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E)			PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F)			PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G)			PORT_CHAR('g') PORT_CHAR('G')

	PORT_START("KEY3")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H)			PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I)			PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J)			PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K)			PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L)			PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)			PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N)			PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O)			PORT_CHAR('o') PORT_CHAR('O')

	PORT_START("KEY4")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P)			PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q)			PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R)			PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S)			PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T)			PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U)			PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V)			PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W)			PORT_CHAR('w') PORT_CHAR('W')

	PORT_START("KEY5")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X)			PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y)			PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z)			PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH2)	PORT_CHAR('\xA5') PORT_CHAR('|')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH)	PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS)		PORT_CHAR('^')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS)		PORT_CHAR('-') PORT_CHAR('=')

	PORT_START("KEY6")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0)			PORT_CHAR('0')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1)			PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2)			PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3)			PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4)			PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5)			PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6)			PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7)			PORT_CHAR('7') PORT_CHAR('\'')

	PORT_START("KEY7")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8)			PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9)			PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE)		PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON)		PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA)		PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)		PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH)		PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("  _") PORT_CODE(KEYCODE_DEL)			PORT_CHAR(0) PORT_CHAR('_')

	PORT_START("KEY8")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Clr Home") PORT_CODE(KEYCODE_HOME)		PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(UTF8_UP) PORT_CODE(KEYCODE_UP)	PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(UTF8_RIGHT) PORT_CODE(KEYCODE_RIGHT)	PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Del Ins") PORT_CODE(KEYCODE_BACKSPACE)	PORT_CHAR(UCHAR_MAMEKEY(DEL)) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Grph") PORT_CODE(KEYCODE_LALT)	PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Kana") PORT_CODE(KEYCODE_LCONTROL) PORT_TOGGLE PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RCONTROL)						PORT_CHAR(UCHAR_SHIFT_2)

	PORT_START("KEY9")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Stop") PORT_CODE(KEYCODE_F1)			PORT_CHAR(UCHAR_MAMEKEY(PAUSE))
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F3)								PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F4)								PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F5)								PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F6)								PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F7)								PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)							PORT_CHAR(' ')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ESC)								PORT_CHAR(UCHAR_MAMEKEY(ESC))

	PORT_START("KEY10")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB)								PORT_CHAR('\t')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(UTF8_DOWN) PORT_CODE(KEYCODE_DOWN)	PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(UTF8_LEFT) PORT_CODE(KEYCODE_LEFT)	PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Help") PORT_CODE(KEYCODE_END)			PORT_CHAR(UCHAR_MAMEKEY(F8))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Copy") PORT_CODE(KEYCODE_F2)			PORT_CHAR(UCHAR_MAMEKEY(PRTSCR))
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS_PAD)						PORT_CHAR(UCHAR_MAMEKEY(MINUS_PAD))
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH_PAD)						PORT_CHAR(UCHAR_MAMEKEY(SLASH_PAD))
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Caps") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))

	PORT_START("KEY11")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Roll Up") PORT_CODE(KEYCODE_F8)			PORT_CHAR(UCHAR_MAMEKEY(PGUP))
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Roll Down") PORT_CODE(KEYCODE_F9)		PORT_CHAR(UCHAR_MAMEKEY(PGDN))
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("KEY12")		/* port 0x0c */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("KEY13")		/* port 0x0d */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("KEY14")		/* port 0x0e */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("KEY15")		/* port 0x0f */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("DSW1")
	PORT_DIPNAME( 0x01, 0x00, "BASIC" )
	PORT_DIPSETTING(    0x01, "N88-BASIC" )
	PORT_DIPSETTING(    0x00, "N-BASIC" )
	PORT_DIPNAME( 0x02, 0x02, "Terminal mode" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Text width" )
	PORT_DIPSETTING(    0x04, "40 chars/line" )
	PORT_DIPSETTING(    0x00, "80 chars/line" )
	PORT_DIPNAME( 0x08, 0x00, "Text height" )
	PORT_DIPSETTING(    0x08, "20 lines/screen" )
	PORT_DIPSETTING(    0x00, "25 lines/screen" )
	PORT_DIPNAME( 0x10, 0x10, "Enable S parameter" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Enable DEL code" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Memory wait" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Disable CMD SING" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("DSW2")
	PORT_DIPNAME( 0x01, 0x01, "Parity generate" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Parity type" )
	PORT_DIPSETTING(    0x00, "Even" )
	PORT_DIPSETTING(    0x02, "Odd" )
	PORT_DIPNAME( 0x04, 0x00, "Serial character length" )
	PORT_DIPSETTING(    0x04, "7 bits/char" )
	PORT_DIPSETTING(    0x00, "8 bits/char" )
	PORT_DIPNAME( 0x08, 0x08, "Stop bit length" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x10, 0x10, "Enable X parameter" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Duplex" )
	PORT_DIPSETTING(    0x20, "Half" )
	PORT_DIPSETTING(    0x00, "Full" )
/*	PORT_DIPNAME( 0x40, 0x00, "Boot from floppy" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Disable floppy" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )*/
	PORT_DIPNAME( 0xc0, 0x40, "Basic mode" )
	PORT_DIPSETTING(    0x80, "N-BASIC" )
	PORT_DIPSETTING(    0xc0, "N88-BASIC (V1)" )
	PORT_DIPSETTING(    0x40, "N88-BASIC (V2)" )

	PORT_START("CFG")		/* EXSWITCH */
	PORT_DIPNAME( 0x04, 0x04, "Speed mode" )
	PORT_DIPSETTING(    0x00, "Slow" )
	PORT_DIPSETTING(    0x04, DEF_STR( High ) )
	PORT_DIPNAME( 0x08, 0x00, "Main CPU clock" )
	PORT_DIPSETTING(    0x00, "4MHz" )
	PORT_DIPSETTING(    0x08, "8MHz" )
	PORT_DIPNAME( 0xf0, 0x80, "Serial speed" )
	PORT_DIPSETTING(    0x10, "75bps" )
	PORT_DIPSETTING(    0x20, "150bps" )
	PORT_DIPSETTING(    0x30, "300bps" )
	PORT_DIPSETTING(    0x40, "600bps" )
	PORT_DIPSETTING(    0x50, "1200bps" )
	PORT_DIPSETTING(    0x60, "2400bps" )
	PORT_DIPSETTING(    0x70, "4800bps" )
	PORT_DIPSETTING(    0x80, "9600bps" )
	PORT_DIPSETTING(    0x90, "19200bps" )

	PORT_START("OPNA")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("OPNB")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("MEM")
	PORT_CONFNAME( 0x1f, 0x00, "Extension memory" )
	PORT_CONFSETTING(    0x00, DEF_STR( None ) )
	PORT_CONFSETTING(    0x01, "32KB (PC-8012-02 x 1)" )
	PORT_CONFSETTING(    0x02, "64KB (PC-8012-02 x 2)" )
	PORT_CONFSETTING(    0x03, "128KB (PC-8012-02 x 4)" )
	PORT_CONFSETTING(    0x04, "128KB (PC-8801-02N x 1)" )
	PORT_CONFSETTING(    0x05, "256KB (PC-8801-02N x 2)" )
	PORT_CONFSETTING(    0x06, "512KB (PC-8801-02N x 4)" )
	PORT_CONFSETTING(    0x07, "1M (PIO-8234H-1M x 1)" )
	PORT_CONFSETTING(    0x08, "2M (PIO-8234H-2M x 1)" )
	PORT_CONFSETTING(    0x09, "4M (PIO-8234H-2M x 2)" )
	PORT_CONFSETTING(    0x0a, "8M (PIO-8234H-2M x 4)" )
	PORT_CONFSETTING(    0x0b, "1.1M (PIO-8234H-1M x 1 + PC-8801-02N x 1)" )
	PORT_CONFSETTING(    0x0c, "2.1M (PIO-8234H-2M x 1 + PC-8801-02N x 1)" )
	PORT_CONFSETTING(    0x0d, "4.1M (PIO-8234H-2M x 2 + PC-8801-02N x 1)" )
INPUT_PORTS_END

static INPUT_PORTS_START( pc88sr )
	PORT_INCLUDE( pc8001 )

	PORT_MODIFY("DSW1")
	PORT_DIPNAME( 0x01, 0x01, "BASIC" )
	PORT_DIPSETTING(    0x01, "N88-BASIC" )
	PORT_DIPSETTING(    0x00, "N-BASIC" )
INPUT_PORTS_END

/* Graphics Layouts */

static const gfx_layout char_layout =
{
	8, 8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8 * 8
};



static GFXDECODE_START( pc8801 )
	GFXDECODE_ENTRY( "gfx1", 0, char_layout, 0, 16 )
GFXDECODE_END

/* uPD1990A Interface */

static UPD1990A_INTERFACE( pc88_upd1990a_intf )
{
	DEVCB_NULL,
	DEVCB_NULL
};

/* YM2203 Interface */
static void pc8801_sound_irq( device_t *device, int irq )
{
	if(sound_irq_mask)
		pc8801_raise_irq(device->machine,4);
}

/* TODO: mouse routing (that's why I don't use DEVCB_INPUT_PORT here) */
static READ8_DEVICE_HANDLER( opn_porta_r ) { return input_port_read(device->machine, "OPNA"); }
static READ8_DEVICE_HANDLER( opn_portb_r ) { return input_port_read(device->machine, "OPNB"); }

static const ym2203_interface pc88_ym2203_intf =
{
	{
		AY8910_LEGACY_OUTPUT,
		AY8910_DEFAULT_LOADS,
		DEVCB_HANDLER(opn_porta_r),
		DEVCB_HANDLER(opn_portb_r),
		DEVCB_NULL,
		DEVCB_NULL
	},
	pc8801_sound_irq
};

/* Floppy Configuration */

static const floppy_config pc88_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(default),
	NULL
};

/* Cassette Configuration */

static const cassette_config pc88_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED),
	NULL
};

static IRQ_CALLBACK( pc8801_irq_callback )
{
	int level, i;

	level = 0;
	for (i = 0; i < 8; i++)
	{
		if ((i8214_irq_state & (1<<i))!=0)
			level = i;
	}

	i8214_irq_state &= ~(1<<level);
	return level*2;
}

static TIMER_CALLBACK( pc8801_rtc_irq )
{
	if(rtc_irq_mask)
		pc8801_raise_irq(machine,2);
}

static MACHINE_START( pc8801 )
{
	cpu_set_irq_callback(machine->device("maincpu"), pc8801_irq_callback);

	timer_pulse(machine, ATTOTIME_IN_HZ(600), NULL, 0, pc8801_rtc_irq);
}

static MACHINE_RESET( pc8801 )
{
	ext_rom_bank = 0xff;
	gfx_ctrl = 0x31;
	window_offset_bank = 0x80;
	misc_ctrl = 0x80;
	layer_mask = 0x00;
	bankr[0] = pc8801_bankswitch_0_r(machine);
	bankr[1] = pc8801_bankswitch_1_r(machine);
	bankw[0] = pc8801_bankswitch_0_w(machine);
	bankw[1] = pc8801_bankswitch_1_w(machine);

	bankr[2] = pc8801_bankswitch_2_r(machine);
	bankw[2] = pc8801_bankswitch_2_w(machine);

	vram_sel = 3;
	bankr[3] = pc8801_bankswitch_3_r(machine);
	bankr[4] = pc8801_bankswitch_4_r(machine);
	bankw[3] = pc8801_bankswitch_3_w(machine);
	bankw[4] = pc8801_bankswitch_4_w(machine);

	pc8801_dynamic_res_change(machine);

	fdc_irq_opcode = 0; //TODO: copied from PC-88VA, could be wrong here ... should be 0x7f ld a,a in the latter case

	cpu_set_input_line_vector(machine->device("fdccpu"), 0, 0);

	{
		UINT8 *gvram = machine->region("maincpu")->base();
		int i;

		for(i=0;i<0x10000;i++)
			gvram[GRAM_BASE + i] = 0x00;

		for(i=0;i<3;i++)
			gvram[ALU_BASE + i] = 0x00;
	}

	{
		crtc.param_count = 0;
		crtc.cmd = 0;
		crtc.status = 0;
	}

	beep_set_frequency(machine->device("beeper"),2400);
	beep_set_state(machine->device("beeper"),0);

	{
		vrtc_irq_mask = 0;
		rtc_irq_mask = 0;
		sound_irq_mask = 0;

		i8214_irq_level = 0;
		i8214_irq_state = 0;
	}

	{
		dma_address[2] = 0xf300;
	}

	{
		extram_bank = 0;
		extram_mode = 0;
	}

	{
		int i;

		for(i=0;i<0x10;i++) //text + bitmap
			palette_set_color_rgb(machine, i, pal1bit(i >> 1), pal1bit(i >> 2), pal1bit(i >> 0));
	}
}

static PALETTE_INIT( pc8801 )
{
	int i;

	for(i=0;i<0x10;i++) //text + bitmap
		palette_set_color_rgb(machine, i, pal1bit(i >> 1), pal1bit(i >> 2), pal1bit(i >> 0));
}

static const struct upd765_interface pc8801_upd765_interface =
{
	DEVCB_CPU_INPUT_LINE("fdccpu", INPUT_LINE_IRQ0),
	DEVCB_NULL, //DRQ, TODO
	NULL,
	UPD765_RDY_PIN_CONNECTED,
	{FLOPPY_0, FLOPPY_1, NULL, NULL}
};

static INTERRUPT_GEN( pc8801_vrtc_irq )
{
	if(vrtc_irq_mask)
		pc8801_raise_irq(device->machine,1);
}

static MACHINE_CONFIG_START( pc8801, driver_device )
	/* main CPU */
	MCFG_CPU_ADD("maincpu", Z80, 4000000)        /* 4 MHz */
	MCFG_CPU_PROGRAM_MAP(pc8801_mem)
	MCFG_CPU_IO_MAP(pc8801_io)
	MCFG_CPU_VBLANK_INT("screen", pc8801_vrtc_irq)

	/* sub CPU(5 inch floppy drive) */
	MCFG_CPU_ADD("fdccpu", Z80, 4000000)		/* 4 MHz */
	MCFG_CPU_PROGRAM_MAP(pc8801fdc_mem)
	MCFG_CPU_IO_MAP(pc8801fdc_io)

	//MCFG_QUANTUM_TIME(HZ(300000))

	MCFG_MACHINE_START( pc8801 )
	MCFG_MACHINE_RESET( pc8801 )

	MCFG_I8255A_ADD( "d8255_master", master_fdd_intf )
	MCFG_I8255A_ADD( "d8255_slave", slave_fdd_intf )

	MCFG_UPD765A_ADD("upd765", pc8801_upd765_interface)
	//MCFG_UPD1990A_ADD("upd1990a", XTAL_32_768kHz, pc88_upd1990a_intf)
	//MCFG_CENTRONICS_ADD("centronics", standard_centronics)
	//MCFG_CASSETTE_ADD("cassette", pc88_cassette_config)

	MCFG_FLOPPY_2_DRIVES_ADD(pc88_floppy_config)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1)
	MCFG_GFXDECODE( pc8801 )
	MCFG_PALETTE_LENGTH(0x10)
	MCFG_PALETTE_INIT( pc8801 )

	MCFG_VIDEO_START(pc8801)
	MCFG_VIDEO_UPDATE(pc8801)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("ym2203", YM2203, 4000000) //unknown clock
	MCFG_SOUND_CONFIG(pc88_ym2203_intf)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MCFG_SOUND_ADD("beeper", BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)
MACHINE_CONFIG_END

/* ROMs */

ROM_START( pc8801 ) /* Basic version 1.0 */
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "n80.rom",   0x80000, 0x8000, CRC(5cb8b584) SHA1(063609dd518c124a4fc9ba35d1bae35771666a34) )
	ROM_LOAD( "n88.rom",   0x88000, 0x8000, CRC(ffd68be0) SHA1(3518193b8207bdebf22c1380c2db8c554baff329) )
	ROM_LOAD( "n88_0.rom", 0x90000, 0x2000, CRC(61984bab) SHA1(d1ae642aed4f0584eeb81ff50180db694e5101d4) )

	ROM_REGION( 0x10000, "fdccpu", 0)
	ROM_LOAD( "disk.rom", 0x0000, 0x0800, CRC(2158d307) SHA1(bb7103a0818850a039c67ff666a31ce49a8d516f) )

	ROM_REGION( 0x40000, "gfx1", 0)
	ROM_LOAD( "font.rom", 0x0000, 0x0800, CRC(56653188) SHA1(84b90f69671d4b72e8f219e1fe7cd667e976cf7f) )
ROM_END

/* The dump only included "maincpu". Other roms arbitrariely taken from PC-8801 & PC-8801 MkIISR (there should be
at least 1 Kanji ROM). */
ROM_START( pc8801mk2 ) /* Basic version 1.3 */
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "m2_n80.rom",   0x80000, 0x8000, CRC(91d84b1a) SHA1(d8a1abb0df75936b3fc9d226ccdb664a9070ffb1) )
	ROM_LOAD( "m2_n88.rom",   0x88000, 0x8000, CRC(f35169eb) SHA1(ef1f067f819781d9fb2713836d195866f0f81501) )
	ROM_LOAD( "m2_n88_0.rom", 0x90000, 0x2000, CRC(5eb7a8d0) SHA1(95a70af83b0637a5a0f05e31fb0452bb2cb68055) )

	ROM_REGION( 0x10000, "fdccpu", 0)
	ROM_LOAD( "disk.rom", 0x0000, 0x0800, CRC(2158d307) SHA1(bb7103a0818850a039c67ff666a31ce49a8d516f) )

	/* should this be here? */
	ROM_REGION( 0x40000, "gfx1", 0)
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )
ROM_END

ROM_START( pc8801mk2sr )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "mk2sr_n80.rom",   0x80000, 0x8000, CRC(27e1857d) SHA1(5b922ed9de07d2a729bdf1da7b57c50ddf08809a) )
	ROM_LOAD( "mk2sr_n88.rom",   0x88000, 0x8000, CRC(a0fc0473) SHA1(3b31fc68fa7f47b21c1a1cb027b86b9e87afbfff) )
	ROM_LOAD( "mk2sr_n88_0.rom", 0x90000, 0x2000, CRC(710a63ec) SHA1(d239c26ad7ac5efac6e947b0e9549b1534aa970d) )
	ROM_LOAD( "n88_1.rom", 0x92000, 0x2000, CRC(c0bd2aa6) SHA1(8528eef7946edf6501a6ccb1f416b60c64efac7c) )
	ROM_LOAD( "n88_2.rom", 0x94000, 0x2000, CRC(af2b6efa) SHA1(b7c8bcea219b77d9cc3ee0efafe343cc307425d1) )
	ROM_LOAD( "n88_3.rom", 0x96000, 0x2000, CRC(7713c519) SHA1(efce0b51cab9f0da6cf68507757f1245a2867a72) )

	ROM_REGION( 0x10000, "fdccpu", 0)
	ROM_LOAD( "disk.rom", 0x0000, 0x0800, CRC(2158d307) SHA1(bb7103a0818850a039c67ff666a31ce49a8d516f) )

	/* No idea of the proper size: it has never been dumped */
	ROM_REGION( 0x2000, "audiocpu", 0)
	ROM_LOAD( "soundbios.rom", 0x0000, 0x2000, NO_DUMP )

	ROM_REGION( 0x40000, "kanji", 0)
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )
	ROM_LOAD( "kanji2.rom", 0x20000, 0x20000, CRC(154803cc) SHA1(7e6591cd465cbb35d6d3446c5a83b46d30fafe95) )	// it should not be here

	ROM_REGION( 0x40000, "gfx1", 0)
	ROM_COPY( "kanji", 0x1000, 0x0000, 0x1000 )
ROM_END

ROM_START( pc8801mk2fr )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "m2fr_n80.rom",   0x80000, 0x8000, CRC(27e1857d) SHA1(5b922ed9de07d2a729bdf1da7b57c50ddf08809a) )
	ROM_LOAD( "m2fr_n88.rom",   0x88000, 0x8000, CRC(b9daf1aa) SHA1(696a480232bcf8c827c7aeea8329db5c44420d2a) )
	ROM_LOAD( "m2fr_n88_0.rom", 0x90000, 0x2000, CRC(710a63ec) SHA1(d239c26ad7ac5efac6e947b0e9549b1534aa970d) )
	ROM_LOAD( "m2fr_n88_1.rom", 0x92000, 0x2000, CRC(e3e78a37) SHA1(85ecd287fe72b56e54c8b01ea7492ca4a69a7470) )
	ROM_LOAD( "m2fr_n88_2.rom", 0x94000, 0x2000, CRC(98c3a7b2) SHA1(fc4980762d3caa56964d0ae583424756f511d186) )
	ROM_LOAD( "m2fr_n88_3.rom", 0x96000, 0x2000, CRC(0ca08abd) SHA1(a5a42d0b7caa84c3bc6e337c9f37874d82f9c14b) )

	ROM_REGION( 0x10000, "fdccpu", 0)
	ROM_LOAD( "m2fr_disk.rom", 0x0000, 0x0800, CRC(2163b304) SHA1(80da2dee49d4307f00895a129a5cfeff00cf5321) )

	/* No idea of the proper size: it has never been dumped */
	ROM_REGION( 0x2000, "audiocpu", 0)
	ROM_LOAD( "soundbios.rom", 0x0000, 0x2000, NO_DUMP )

	ROM_REGION( 0x40000, "gfx1", 0)
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )
ROM_END

ROM_START( pc8801mk2mr )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "m2mr_n80.rom",   0x80000, 0x8000, CRC(f074b515) SHA1(ebe9cf4cf57f1602c887f609a728267f8d953dce) )
	ROM_LOAD( "m2mr_n88.rom",   0x88000, 0x8000, CRC(69caa38e) SHA1(3c64090237152ee77c76e04d6f36bad7297bea93) )
	ROM_LOAD( "m2mr_n88_0.rom", 0x90000, 0x2000, CRC(710a63ec) SHA1(d239c26ad7ac5efac6e947b0e9549b1534aa970d) )
	ROM_LOAD( "m2mr_n88_1.rom", 0x92000, 0x2000, CRC(e3e78a37) SHA1(85ecd287fe72b56e54c8b01ea7492ca4a69a7470) )
	ROM_LOAD( "m2mr_n88_2.rom", 0x94000, 0x2000, CRC(11176e0b) SHA1(f13f14f3d62df61498a23f7eb624e1a646caea45) )
	ROM_LOAD( "m2mr_n88_3.rom", 0x96000, 0x2000, CRC(0ca08abd) SHA1(a5a42d0b7caa84c3bc6e337c9f37874d82f9c14b) )

	ROM_REGION( 0x10000, "fdccpu", 0)
	ROM_LOAD( "m2mr_disk.rom", 0x0000, 0x2000, CRC(2447516b) SHA1(1492116f15c426f9796dc2bb6fcccf2656c0ca75) )

	/* No idea of the proper size: it has never been dumped */
	ROM_REGION( 0x2000, "audiocpu", 0)
	ROM_LOAD( "soundbios.rom", 0x0000, 0x2000, NO_DUMP )

	ROM_REGION( 0x40000, "gfx1", 0)
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )

	ROM_REGION( 0x20000, "kanji2", 0)
	ROM_LOAD( "m2mr_kanji2.rom", 0x00000, 0x20000, CRC(376eb677) SHA1(bcf96584e2ba362218b813be51ea21573d1a2a78) )
ROM_END

ROM_START( pc8801mh )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "mh_n80.rom",   0x80000, 0x8000, CRC(8a2a1e17) SHA1(06dae1db384aa29d81c5b6ed587877e7128fcb35) )
	ROM_LOAD( "mh_n88.rom",   0x88000, 0x8000, CRC(64c5d162) SHA1(3e0aac76fb5d7edc99df26fa9f365fd991742a5d) )
	ROM_LOAD( "mh_n88_0.rom", 0x90000, 0x2000, CRC(deb384fb) SHA1(5f38cafa8aab16338038c82267800446fd082e79) )
	ROM_LOAD( "mh_n88_1.rom", 0x92000, 0x2000, CRC(7ad5d943) SHA1(4ae4d37409ff99411a623da9f6a44192170a854e) )
	ROM_LOAD( "mh_n88_2.rom", 0x94000, 0x2000, CRC(6aa6b6d8) SHA1(2a077ab444a4fd1470cafb06fd3a0f45420c39cc) )
	ROM_LOAD( "mh_n88_3.rom", 0x96000, 0x2000, CRC(692cbcd8) SHA1(af452aed79b072c4d17985830b7c5dca64d4b412) )

	ROM_REGION( 0x10000, "fdccpu", 0)
	ROM_LOAD( "mh_disk.rom", 0x0000, 0x2000, CRC(a222ecf0) SHA1(79e9c0786a14142f7a83690bf41fb4f60c5c1004) )

	/* No idea of the proper size: it has never been dumped */
	ROM_REGION( 0x2000, "audiocpu", 0)
	ROM_LOAD( "soundbios.rom", 0x0000, 0x2000, NO_DUMP )

	ROM_REGION( 0x40000, "gfx1", 0)
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )

	ROM_REGION( 0x20000, "kanji2", 0)
	ROM_LOAD( "mh_kanji2.rom", 0x00000, 0x20000, CRC(376eb677) SHA1(bcf96584e2ba362218b813be51ea21573d1a2a78) )
ROM_END

ROM_START( pc8801fa )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "fa_n80.rom",   0x80000, 0x8000, CRC(8a2a1e17) SHA1(06dae1db384aa29d81c5b6ed587877e7128fcb35) )
	ROM_LOAD( "fa_n88.rom",   0x88000, 0x8000, CRC(73573432) SHA1(9b1346d44044eeea921c4cce69b5dc49dbc0b7e9) )
	ROM_LOAD( "fa_n88_0.rom", 0x90000, 0x2000, CRC(a72697d7) SHA1(5aedbc5916d67ef28767a2b942864765eea81bb8) )
	ROM_LOAD( "fa_n88_1.rom", 0x92000, 0x2000, CRC(7ad5d943) SHA1(4ae4d37409ff99411a623da9f6a44192170a854e) )
	ROM_LOAD( "fa_n88_2.rom", 0x94000, 0x2000, CRC(6aee9a4e) SHA1(e94278682ef9e9bbb82201f72c50382748dcea2a) )
	ROM_LOAD( "fa_n88_3.rom", 0x96000, 0x2000, CRC(692cbcd8) SHA1(af452aed79b072c4d17985830b7c5dca64d4b412) )

	ROM_REGION( 0x10000, "fdccpu", 0)
	ROM_LOAD( "fa_disk.rom", 0x0000, 0x0800, CRC(2163b304) SHA1(80da2dee49d4307f00895a129a5cfeff00cf5321) )

	/* No idea of the proper size: it has never been dumped */
	ROM_REGION( 0x2000, "audiocpu", 0)
	ROM_LOAD( "soundbios.rom", 0x0000, 0x2000, NO_DUMP )

	ROM_REGION( 0x40000, "gfx1", 0 )
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )

	ROM_REGION( 0x20000, "kanji2", 0)
	ROM_LOAD( "fa_kanji2.rom", 0x00000, 0x20000, CRC(376eb677) SHA1(bcf96584e2ba362218b813be51ea21573d1a2a78) )
ROM_END

ROM_START( pc8801ma )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "ma_n80.rom",   0x80000, 0x8000, CRC(8a2a1e17) SHA1(06dae1db384aa29d81c5b6ed587877e7128fcb35) )
	ROM_LOAD( "ma_n88.rom",   0x88000, 0x8000, CRC(73573432) SHA1(9b1346d44044eeea921c4cce69b5dc49dbc0b7e9) )
	ROM_LOAD( "ma_n88_0.rom", 0x90000, 0x2000, CRC(a72697d7) SHA1(5aedbc5916d67ef28767a2b942864765eea81bb8) )
	ROM_LOAD( "ma_n88_1.rom", 0x92000, 0x2000, CRC(7ad5d943) SHA1(4ae4d37409ff99411a623da9f6a44192170a854e) )
	ROM_LOAD( "ma_n88_2.rom", 0x94000, 0x2000, CRC(6aee9a4e) SHA1(e94278682ef9e9bbb82201f72c50382748dcea2a) )
	ROM_LOAD( "ma_n88_3.rom", 0x96000, 0x2000, CRC(692cbcd8) SHA1(af452aed79b072c4d17985830b7c5dca64d4b412) )

	ROM_REGION( 0x10000, "fdccpu", 0)
	ROM_LOAD( "ma_disk.rom", 0x0000, 0x2000, CRC(a222ecf0) SHA1(79e9c0786a14142f7a83690bf41fb4f60c5c1004) )

	/* No idea of the proper size: it has never been dumped */
	ROM_REGION( 0x2000, "audiocpu", 0)
	ROM_LOAD( "soundbios.rom", 0x0000, 0x2000, NO_DUMP )

	ROM_REGION( 0x40000, "gfx1", 0 )
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )

	ROM_REGION( 0x20000, "kanji2", 0)
	ROM_LOAD( "ma_kanji2.rom", 0x00000, 0x20000, CRC(376eb677) SHA1(bcf96584e2ba362218b813be51ea21573d1a2a78) )

	/* 32 banks, to be loaded at 0xc000 - 0xffff */
	ROM_REGION( 0x80000, "dictionary", 0 )
	ROM_LOAD( "ma_jisyo.rom", 0x00000, 0x80000, CRC(a6108f4d) SHA1(3665db538598abb45d9dfe636423e6728a812b12) )
ROM_END

ROM_START( pc8801ma2 )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "ma2_n80.rom",   0x80000, 0x8000, CRC(8a2a1e17) SHA1(06dae1db384aa29d81c5b6ed587877e7128fcb35) )
	ROM_LOAD( "ma2_n88.rom",   0x88000, 0x8000, CRC(ae1a6ebc) SHA1(e53d628638f663099234e07837ffb1b0f86d480d) )
	ROM_LOAD( "ma2_n88_0.rom", 0x90000, 0x2000, CRC(a72697d7) SHA1(5aedbc5916d67ef28767a2b942864765eea81bb8) )
	ROM_LOAD( "ma2_n88_1.rom", 0x92000, 0x2000, CRC(7ad5d943) SHA1(4ae4d37409ff99411a623da9f6a44192170a854e) )
	ROM_LOAD( "ma2_n88_2.rom", 0x94000, 0x2000, CRC(1d6277b6) SHA1(dd9c3e50169b75bb707ef648f20d352e6a8bcfe4) )
	ROM_LOAD( "ma2_n88_3.rom", 0x96000, 0x2000, CRC(692cbcd8) SHA1(af452aed79b072c4d17985830b7c5dca64d4b412) )

	ROM_REGION( 0x10000, "fdccpu", 0)
	ROM_LOAD( "ma2_disk.rom", 0x0000, 0x2000, CRC(a222ecf0) SHA1(79e9c0786a14142f7a83690bf41fb4f60c5c1004) )

	/* No idea of the proper size: it has never been dumped */
	ROM_REGION( 0x2000, "audiocpu", 0)
	ROM_LOAD( "soundbios.rom", 0x0000, 0x2000, NO_DUMP )

	ROM_REGION( 0x40000, "gfx1", 0 )
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )

	ROM_REGION( 0x20000, "kanji2", 0)
	ROM_LOAD( "ma2_kanji2.rom", 0x00000, 0x20000, CRC(376eb677) SHA1(bcf96584e2ba362218b813be51ea21573d1a2a78) )

	/* 32 banks, to be loaded at 0xc000 - 0xffff */
	ROM_REGION( 0x80000, "dictionary", 0 )
	ROM_LOAD( "ma2_jisyo.rom", 0x00000, 0x80000, CRC(856459af) SHA1(06241085fc1d62d4b2968ad9cdbdadc1e7d7990a) )
ROM_END

ROM_START( pc8801mc )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "mc_n80.rom",   0x80000, 0x8000, CRC(8a2a1e17) SHA1(06dae1db384aa29d81c5b6ed587877e7128fcb35) )
	ROM_LOAD( "mc_n88.rom",   0x88000, 0x8000, CRC(356d5719) SHA1(5d9ba80d593a5119f52aae1ccd61a1457b4a89a1) )
	ROM_LOAD( "mc_n88_0.rom", 0x90000, 0x2000, CRC(a72697d7) SHA1(5aedbc5916d67ef28767a2b942864765eea81bb8) )
	ROM_LOAD( "mc_n88_1.rom", 0x92000, 0x2000, CRC(7ad5d943) SHA1(4ae4d37409ff99411a623da9f6a44192170a854e) )
	ROM_LOAD( "mc_n88_2.rom", 0x94000, 0x2000, CRC(1d6277b6) SHA1(dd9c3e50169b75bb707ef648f20d352e6a8bcfe4) )
	ROM_LOAD( "mc_n88_3.rom", 0x96000, 0x2000, CRC(692cbcd8) SHA1(af452aed79b072c4d17985830b7c5dca64d4b412) )

	ROM_REGION( 0x10000, "fdccpu", 0)
	ROM_LOAD( "mc_disk.rom", 0x0000, 0x2000, CRC(a222ecf0) SHA1(79e9c0786a14142f7a83690bf41fb4f60c5c1004) )

	ROM_REGION( 0x10000, "cdrom", 0 )
	ROM_LOAD( "cdbios.rom", 0x0000, 0x10000, CRC(5c230221) SHA1(6394a8a23f44ea35fcfc3e974cf940bc8f84d62a) )

	/* No idea of the proper size: it has never been dumped */
	ROM_REGION( 0x2000, "audiocpu", 0)
	ROM_LOAD( "soundbios.rom", 0x0000, 0x2000, NO_DUMP )

	ROM_REGION( 0x40000, "gfx1", 0 )
	ROM_LOAD( "kanji1.rom", 0x00000, 0x20000, CRC(6178bd43) SHA1(82e11a177af6a5091dd67f50a2f4bafda84d6556) )

	ROM_REGION( 0x20000, "kanji2", 0)
	ROM_LOAD( "mc_kanji2.rom", 0x00000, 0x20000, CRC(376eb677) SHA1(bcf96584e2ba362218b813be51ea21573d1a2a78) )

	/* 32 banks, to be loaded at 0xc000 - 0xffff */
	ROM_REGION( 0x80000, "dictionary", 0 )
	ROM_LOAD( "mc_jisyo.rom", 0x00000, 0x80000, CRC(bd6eb062) SHA1(deef0cc2a9734ba891a6d6c022aa70ffc66f783e) )
ROM_END

/* System Drivers */

/*    YEAR  NAME            PARENT  COMPAT  MACHINE   INPUT   INIT  COMPANY FULLNAME */

COMP( 1981, pc8801,         0,		0,     pc8801,  pc88sr,  0,    "Nippon Electronic Company",  "PC-8801", GAME_NOT_WORKING )
COMP( 1983, pc8801mk2,      pc8801, 0,     pc8801,  pc88sr,  0,    "Nippon Electronic Company",  "PC-8801mkII", GAME_NOT_WORKING )
COMP( 1985, pc8801mk2sr,    pc8801,	0,     pc8801,  pc88sr,  0,    "Nippon Electronic Company",  "PC-8801mkIISR", GAME_NOT_WORKING )
//COMP( 1985, pc8801mk2tr,  pc8801, 0,     pc8801,  pc88sr,  0,    "Nippon Electronic Company",  "PC-8801mkIITR", GAME_NOT_WORKING )
COMP( 1985, pc8801mk2fr,    pc8801,	0,     pc8801,  pc88sr,  0,    "Nippon Electronic Company",  "PC-8801mkIIFR", GAME_NOT_WORKING )
COMP( 1985, pc8801mk2mr,    pc8801,	0,     pc8801,  pc88sr,  0,    "Nippon Electronic Company",  "PC-8801mkIIMR", GAME_NOT_WORKING )

//COMP( 1986, pc8801fh,     0,      0,     pc8801,  pc88sr,  0,    "Nippon Electronic Company",  "PC-8801FH", GAME_NOT_WORKING )
COMP( 1986, pc8801mh,       pc8801,	0,     pc8801,  pc88sr,  0,    "Nippon Electronic Company",  "PC-8801MH", GAME_NOT_WORKING )
COMP( 1987, pc8801fa,       pc8801,	0,     pc8801,  pc88sr,  0,    "Nippon Electronic Company",  "PC-8801FA", GAME_NOT_WORKING )
COMP( 1987, pc8801ma,       pc8801,	0,     pc8801,  pc88sr,  0,    "Nippon Electronic Company",  "PC-8801MA", GAME_NOT_WORKING )
//COMP( 1988, pc8801fe,     pc8801, 0,     pc8801,  pc88sr,  0,    "Nippon Electronic Company",  "PC-8801FE", GAME_NOT_WORKING )
COMP( 1988, pc8801ma2,      pc8801,	0,     pc8801,  pc88sr,  0,    "Nippon Electronic Company",  "PC-8801MA2", GAME_NOT_WORKING )
//COMP( 1989, pc8801fe2,    pc8801, 0,     pc8801,  pc88sr,  0,    "Nippon Electronic Company",  "PC-8801FE2", GAME_NOT_WORKING )
COMP( 1989, pc8801mc,       pc8801,	0,     pc8801,  pc88sr,  0,    "Nippon Electronic Company",  "PC-8801MC", GAME_NOT_WORKING )

//COMP( 1989, pc98do,       0,      0,     pc88va,   pc88sr,  0,    "Nippon Electronic Company",  "PC-98DO", GAME_NOT_WORKING )
//COMP( 1990, pc98dop,      0,      0,     pc88va,   pc88sr,  0,    "Nippon Electronic Company",  "PC-98DO+", GAME_NOT_WORKING )
