/***************************************************************************

	Acorn Archimedes 7000/7000+

    preliminary driver by Angelo Salese,
    based on work by Tomasz Slanina and Tom Walker

	???
	bp (0382827C) (second trigger)
	do R13 = SR13

****************************************************************************/

#include "emu.h"
#include "cpu/arm7/arm7.h"
#include "cpu/arm7/arm7core.h"

/*
 *
 * VIDC20 chip emulation
 *
 */

static UINT8 vidc20_pal_index;
static UINT16 vidc20_horz_reg[0x10],vidc20_vert_reg[0x10];
static UINT8 vidc20_bpp_mode;

static const char *const vidc20_regnames[] =
{
	"Video Palette", 					// 0
	"Video Palette Address", 			// 1
	"RESERVED", 						// 2
	"LCD offset", 						// 3
	"Border Colour",					// 4
	"Cursor Palette Logical Colour 1", 	// 5
	"Cursor Palette Logical Colour 2", 	// 6
	"Cursor Palette Logical Colour 3", 	// 7
	"Horizontal",						// 8
	"Vertical",							// 9
	"Stereo Image",						// A
	"Sound",							// B
	"External",							// C
	"Frequency Synthesis",				// D
	"Control",							// E
	"Data Control"						// F
};

static const char *const vidc20_horz_regnames[] =
{
	"Horizontal Cycle", 				// 0x80 HCR
	"Horizontal Sync Width", 			// 0x81 HSWR
	"Horizontal Border Start", 			// 0x82 HBSR
	"Horizontal Display Start",		 	// 0x83 HDSR
	"Horizontal Display End", 			// 0x84 HDER
	"Horizontal Border End", 			// 0x85 HBER
	"Horizontal Cursor Start", 			// 0x86 HCSR
	"Horizontal Interlace", 			// 0x87 HIR
	"Horizontal Counter TEST", 			// 0x88
	"Horizontal <UNDEFINED>", 			// 0x89
	"Horizontal <UNDEFINED>", 			// 0x8a
	"Horizontal <UNDEFINED>", 			// 0x8b
	"Horizontal All TEST", 				// 0x8c
	"Horizontal <UNDEFINED>", 			// 0x8d
	"Horizontal <UNDEFINED>", 			// 0x8e
	"Horizontal <UNDEFINED>" 			// 0x8f
};

#define HCR  0
#define HSWR 1
#define HBSR 2
#define HDSR 3
#define HDER 4
#define HBER 5
#define HCSR 6
#define HIR  7

static const char *const vidc20_vert_regnames[] =
{
	"Vertical Cycle", 					// 0x90 VCR
	"Vertical Sync Width", 				// 0x91 VSWR
	"Vertical Border Start", 			// 0x92 VBSR
	"Vertical Display Start", 			// 0x93 VDSR
	"Vertical Display End", 			// 0x94 VDER
	"Vertical Border End", 				// 0x95 VBER
	"Vertical Cursor Start", 			// 0x96 VCSR
	"Vertical Cursor End", 				// 0x97 VCER
	"Vertical Counter TEST", 			// 0x98
	"Horizontal <UNDEFINED>", 			// 0x99
	"Vertical Counter Increment TEST", 	// 0x9a
	"Horizontal <UNDEFINED>", 			// 0x9b
	"Vertical All TEST", 				// 0x9c
	"Horizontal <UNDEFINED>", 			// 0x9d
	"Horizontal <UNDEFINED>", 			// 0x9e
	"Horizontal <UNDEFINED>" 			// 0x9f
};

#define VCR  0
#define VSWR 1
#define VBSR 2
#define VDSR 3
#define VDER 4
#define VBER 5
#define VCSR 6
#define VCER 7

static void vidc20_dynamic_screen_change(running_machine *machine)
{
	/* sanity checks - first pass */
	/*
	total cycles + border start/end
	*/
	if(vidc20_horz_reg[HCR] && vidc20_horz_reg[HBSR] && vidc20_horz_reg[HBER] &&
	   vidc20_vert_reg[VCR] && vidc20_vert_reg[VBSR] && vidc20_vert_reg[VBER])
	{
		/* sanity checks - second pass */
		/*
		total cycles > border end > border start
		*/
		if((vidc20_horz_reg[HCR] > vidc20_horz_reg[HBER]) &&
		   (vidc20_horz_reg[HBER] > vidc20_horz_reg[HBSR]) &&
		   (vidc20_vert_reg[VCR] > vidc20_vert_reg[VBER]) &&
		   (vidc20_vert_reg[VBER] > vidc20_vert_reg[VBSR]))
		{
			/* finally ready to change the resolution */
			int hblank_period,vblank_period;
			rectangle visarea = machine->primary_screen->visible_area();
			hblank_period = (vidc20_horz_reg[HCR] & 0x3ffc);
			vblank_period = (vidc20_vert_reg[VCR] & 0x3fff);
			/* note that we use the border registers as the visible area */
			visarea.min_x = (vidc20_horz_reg[HBSR] & 0x3ffe);
			visarea.max_x = (vidc20_horz_reg[HBER] & 0x3ffe)-1;
			visarea.min_y = (vidc20_vert_reg[VBSR] & 0x1fff);
			visarea.max_y = (vidc20_vert_reg[VBER] & 0x1fff)-1;

			machine->primary_screen->configure(hblank_period, vblank_period, visarea, machine->primary_screen->frame_period().attoseconds );
			logerror("VIDC20: successfully changed the screen to:\n Display Size = %d x %d\n Border Size %d x %d\n Cycle Period %d x %d\n",
			           (vidc20_horz_reg[HDER]-vidc20_horz_reg[HDSR]),(vidc20_vert_reg[VDER]-vidc20_vert_reg[VDSR]),
			           (vidc20_horz_reg[HBER]-vidc20_horz_reg[HBSR]),(vidc20_vert_reg[VBER]-vidc20_vert_reg[VBSR]),
			           hblank_period,vblank_period);
		}
	}
}

static WRITE32_HANDLER( a7000_vidc20_w )
{
	int reg = data >> 28;

	switch(reg)
	{
		case 0: // Video Palette
		{
			int r,g,b;

			r = (data & 0x0000ff) >> 0;
			g = (data & 0x00ff00) >> 8;
			b = (data & 0xff0000) >> 16;

			palette_set_color_rgb(space->machine,vidc20_pal_index & 0xff,r,g,b);

			/* auto-increment & wrap-around */
			vidc20_pal_index++;
			vidc20_pal_index &= 0xff;
		}
		break;
		case 1: // Video Palette Address
		{
			// according to RPCEmu, these mustn't be set
			if (data & 0x0fffff00)
				return;

			vidc20_pal_index = data & 0xff;
		}
		break;

		case 4: // Border Color
		{
			int r,g,b;

			r = (data & 0x0000ff) >> 0;
			g = (data & 0x00ff00) >> 8;
			b = (data & 0xff0000) >> 16;

			palette_set_color_rgb(space->machine,0x100,r,g,b);
		}
		break;
		case 5: // Cursor Palette Logical Colour n
		case 6:
		case 7:
		{
			int r,g,b;
			int cursor_index = 0x100 + reg - 4; // 0x101,0x102 and 0x103 (plus 0x100 of above, 2bpp)

			r = (data & 0x0000ff) >> 0;
			g = (data & 0x00ff00) >> 8;
			b = (data & 0xff0000) >> 16;

			palette_set_color_rgb(space->machine,cursor_index,r,g,b);
		}
		break;
		case 8: // Horizontal
		{
			int horz_reg;

			horz_reg = (data >> 24) & 0xf;
			vidc20_horz_reg[horz_reg] = data & 0x3fff;
			if(horz_reg == 0 || horz_reg == 2 || horz_reg == 5)
				vidc20_dynamic_screen_change(space->machine);

			// logerror("VIDC20: %s Register write = %08x (%d)\n",vidc20_horz_regnames[horz_reg],val,val);
		}
		break;
		case 9: // Vertical
		{
			int vert_reg;

			vert_reg = (data >> 24) & 0xf;
			vidc20_vert_reg[vert_reg] = data & 0x1fff;
			if(vert_reg == 0 || vert_reg == 2 || vert_reg == 5)
				vidc20_dynamic_screen_change(space->machine);

			// logerror("VIDC20: %s Register write = %08x (%d)\n",vidc20_vert_regnames[vert_reg],val,val);

		}
		break;
		case 0x0e: // Control
		{
			vidc20_bpp_mode = (data & 0xe0) >> 5;
		}
		break;
		default: logerror("VIDC20: %s Register write = %08x\n",vidc20_regnames[reg],data & 0xfffffff);
	}
}

static VIDEO_START( a7000 )
{
}

static VIDEO_UPDATE( a7000 )
{
	int x_size,y_size,x_start,y_start;
	int x,y,xi;
	UINT32 count;
	static UINT8 *vram = memory_region(screen->machine, "vram");

	bitmap_fill(bitmap, cliprect, screen->machine->pens[0x100]);

	x_size = (vidc20_horz_reg[HDER]-vidc20_horz_reg[HDSR]);
	y_size = (vidc20_vert_reg[VDER]-vidc20_vert_reg[VDSR]);
	x_start = vidc20_horz_reg[HDSR];
	y_start = vidc20_vert_reg[VDSR];

	/* check if display is enabled */
	if(x_size <= 0 || y_size <= 0)
		return 0;

//	popmessage("%d",vidc20_bpp_mode);

	count = 0;

	switch(vidc20_bpp_mode)
	{
		case 0: /* 1 bpp */
		{
			for(y=0;y<y_size;y++)
			{
				for(x=0;x<x_size;x+=8)
				{
					for(xi=0;xi<8;xi++)
						*BITMAP_ADDR32(bitmap, y+y_start, x+xi+x_start) = screen->machine->pens[(vram[count]>>(xi))&1];

					count++;
				}
			}
		}
		break;
		case 1: /* 2 bpp */
		{
			for(y=0;y<y_size;y++)
			{
				for(x=0;x<x_size;x+=4)
				{
					for(xi=0;xi<4;xi++)
						*BITMAP_ADDR32(bitmap, y+y_start, x+xi+x_start) = screen->machine->pens[(vram[count]>>(xi*2))&3];

					count++;
				}
			}
		}
		break;
		case 2: /* 4 bpp */
		{
			for(y=0;y<y_size;y++)
			{
				for(x=0;x<x_size;x+=2)
				{
					for(xi=0;xi<2;xi++)
						*BITMAP_ADDR32(bitmap, y+y_start, x+xi+x_start) = screen->machine->pens[(vram[count]>>(xi*4))&0xf];

					count++;
				}
			}
		}
		break;
		case 3: /* 8 bpp */
		{
			for(y=0;y<y_size;y++)
			{
				for(x=0;x<x_size;x++)
				{
					*BITMAP_ADDR32(bitmap, y+y_start, x+x_start) = screen->machine->pens[(vram[count])&0xff];

					count++;
				}
			}
		}
		break;
		case 4: /* 16 bpp */
		{
			for(y=0;y<y_size;y++)
			{
				for(x=0;x<x_size;x++)
				{
					int r,g,b,pen;

					pen = ((vram[count]<<8)|(vram[count+1]))&0xffff;
					r = (pen & 0x000f);
					g = (pen & 0x00f0) >> 4;
					b = (pen & 0x0f00) >> 8;
					r = (r << 4) | (r & 0xf);
					g = (g << 4) | (g & 0xf);
					b = (b << 4) | (b & 0xf);

					*BITMAP_ADDR32(bitmap, y+y_start, x+x_start) =  b | g << 8 | r << 16;

					count+=2;
				}
			}
		}
		break;
		case 6: /* 32 bpp */
		{
			for(y=0;y<y_size;y++)
			{
				for(x=0;x<x_size;x++)
				{
					int r,g,b,pen;

					pen = ((vram[count]<<24)|(vram[count+1]<<16)|(vram[count+2]<<8)|(vram[count+3]<<0));
					r = (pen & 0x0000ff);
					g = (pen & 0x00ff00) >> 8;
					b = (pen & 0xff0000) >> 16;

					*BITMAP_ADDR32(bitmap, y+y_start, x+x_start) =  b | g << 8 | r << 16;

					count+=4;
				}
			}
		}
		break;
		default:
			//fatalerror("VIDC20 %08x BPP mode not supported\n",vidc20_bpp_mode);
			break;
	}

    return 0;
}

/*
*
* IOMD / ARM7500 / ARM7500FE chip emulation
*
*/

/* TODO: some of these registers are actually ARM7500 specific */
static const char *const iomd_regnames[] =
{
	"I/O Control",						// 0x000 IOCR
	"Keyboard Data",					// 0x004 KBDDAT
	"Keyboard Control",					// 0x008 KBDCR
	"General Purpose I/O Lines",		// 0x00c IOLINES
	"IRQA Status",						// 0x010 IRQSTA
	"IRQA Request/clear",				// 0x014 IRQRQA
	"IRQA Mask",						// 0x018 IRQMSKA
	"Enter SUSPEND Mode",				// 0x01c SUSMODE
	"IRQB Status",						// 0x020 IRQSTB
	"IRQB Request/clear",				// 0x024 IRQRQB
	"IRQB Mask",						// 0x028 IRQMSKB
	"Enter STOP Mode",					// 0x02c STOPMODE
	"FIQ Status",						// 0x030 FIQST
	"FIQ Request/clear",				// 0x034 FIQRQ
	"FIQ Mask",							// 0x038 FIQMSK
	"Clock divider control",			// 0x03c CLKCTL
	"Timer 0 Low Bits",					// 0x040 T0LOW
	"Timer 0 High Bits",				// 0x044 T0HIGH
	"Timer 0 Go Command",				// 0x048 T0GO
	"Timer 0 Latch Command",			// 0x04c T0LATCH
	"Timer 1 Low Bits",					// 0x050 T1LOW
	"Timer 1 High Bits",				// 0x054 T1HIGH
	"Timer 1 Go Command",				// 0x058 T1GO
	"Timer 1 Latch Command",			// 0x05c T1LATCH
	"IRQC Status",						// 0x060 IRQSTC
	"IRQC Request/clear",				// 0x064 IRQRQC
	"IRQC Mask",						// 0x068 IRQMSKC
	"LCD and IIS Control Bits",			// 0x06c VIDIMUX
	"IRQD Status",						// 0x070 IRQSTD
	"IRQD Request/clear",				// 0x074 IRQRQD
	"IRQD Mask",						// 0x078 IRQMSKD
	"<RESERVED>",						// 0x07c
	"ROM Control Bank 0",				// 0x080 ROMCR0
	"ROM Control Bank 1",				// 0x084 ROMCR1
	"DRAM Control (IOMD)",				// 0x088 DRAMCR
	"VRAM and Refresh Control",			// 0x08c VREFCR
	"Flyback Line Size",				// 0x090 FSIZE
	"Chip ID no. Low Byte",				// 0x094 ID0
	"Chip ID no. High Byte",			// 0x098 ID1
	"Chip Version Number",				// 0x09c VERSION
	"Mouse X Position",					// 0x0a0 MOUSEX
	"Mouse Y Position",					// 0x0a4 MOUSEY
	"Mouse Data",						// 0x0a8 MSEDAT
	"Mouse Control",					// 0x0ac MSECR
	"<RESERVED>",						// 0x0b0
	"<RESERVED>",						// 0x0b4
	"<RESERVED>",						// 0x0b8
	"<RESERVED>",						// 0x0bc
	"DACK Timing Control",				// 0x0c0 DMATCR
	"I/O Timing Control",				// 0x0c4 IOTCR
	"Expansion Card Timing",			// 0x0c8 ECTCR
	"DMA External Control",				// 0x0cc DMAEXT (IOMD) / ASTCR (ARM7500)
	"DRAM Width Control",				// 0x0d0 DRAMWID
	"Force CAS/RAS Lines Low",			// 0x0d4 SELFREF
	"<RESERVED>",						// 0x0d8
	"<RESERVED>",						// 0x0dc
	"A to D IRQ Control",				// 0x0e0 ATODICR
	"A to D IRQ Status",				// 0x0e4 ATODCC
	"A to D IRQ Converter Control",		// 0x0e8 ATODICR
	"A to D IRQ Counter 1",				// 0x0ec ATODCNT1
	"A to D IRQ Counter 2",				// 0x0f0 ATODCNT2
	"A to D IRQ Counter 3",				// 0x0f4 ATODCNT3
	"A to D IRQ Counter 4",				// 0x0f8 ATODCNT4
	"<RESERVED>",						// 0x0fc
	"I/O DMA 0 CurA",					// 0x100 IO0CURA
	"I/O DMA 0 EndA",					// 0x104 IO0ENDA
	"I/O DMA 0 CurB",					// 0x108 IO0CURB
	"I/O DMA 0 EndB",					// 0x10c IO0ENDB
	"I/O DMA 0 Control",				// 0x110 IO0CR
	"I/O DMA 0 Status",					// 0x114 IO0ST
	"<RESERVED>",						// 0x118
	"<RESERVED>",						// 0x11c
	"I/O DMA 1 CurA",					// 0x120 IO1CURA
	"I/O DMA 1 EndA",					// 0x124 IO1ENDA
	"I/O DMA 1 CurB",					// 0x128 IO1CURB
	"I/O DMA 1 EndB",					// 0x12c IO1ENDB
	"I/O DMA 1 Control",				// 0x130 IO1CR
	"I/O DMA 1 Status",					// 0x134 IO1ST
	"<RESERVED>",						// 0x138
	"<RESERVED>",						// 0x13c
	"I/O DMA 2 CurA",					// 0x140 IO2CURA
	"I/O DMA 2 EndA",					// 0x144 IO2ENDA
	"I/O DMA 2 CurB",					// 0x148 IO2CURB
	"I/O DMA 2 EndB",					// 0x14c IO2ENDB
	"I/O DMA 2 Control",				// 0x150 IO2CR
	"I/O DMA 2 Status",					// 0x154 IO2ST
	"<RESERVED>",						// 0x158
	"<RESERVED>",						// 0x15c
	"I/O DMA 3 CurA",					// 0x160 IO3CURA
	"I/O DMA 3 EndA",					// 0x164 IO3ENDA
	"I/O DMA 3 CurB",					// 0x168 IO3CURB
	"I/O DMA 3 EndB",					// 0x16c IO3ENDB
	"I/O DMA 3 Control",				// 0x170 IO3CR
	"I/O DMA 3 Status",					// 0x174 IO3ST
	"<RESERVED>",						// 0x178
	"<RESERVED>",						// 0x17c
	"Sound DMA 0 CurA",					// 0x180 SD0CURA
	"Sound DMA 0 EndA",					// 0x184 SD0ENDA
	"Sound DMA 0 CurB",					// 0x188 SD0CURB
	"Sound DMA 0 EndB",					// 0x18c SD0ENDB
	"Sound DMA 0 Control",				// 0x190 SD0CR
	"Sound DMA 0 Status",				// 0x194 SD0ST
	"<RESERVED>",						// 0x198
	"<RESERVED>",						// 0x19c
	"Sound DMA 1 CurA",					// 0x1a0 SD1CURA
	"Sound DMA 1 EndA",					// 0x1a4 SD1ENDA
	"Sound DMA 1 CurB",					// 0x1a8 SD1CURB
	"Sound DMA 1 EndB",					// 0x1ac SD1ENDB
	"Sound DMA 1 Control",				// 0x1b0 SD1CR
	"Sound DMA 1 Status",				// 0x1b4 SD1ST
	"<RESERVED>",						// 0x1b8
	"<RESERVED>",						// 0x1bc
	"Cursor DMA Current",				// 0x1c0 CURSCUR
	"Cursor DMA Init",					// 0x1c4 CURSINIT
	"Duplex LCD Current B",				// 0x1c8 VIDCURB
	"<RESERVED>",						// 0x1cc
	"Video DMA Current",				// 0x1d0 VIDCUR
	"Video DMA End",					// 0x1d4 VIDEND
	"Video DMA Start",					// 0x1d8 VIDSTART
	"Video DMA Init",					// 0x1dc VIDINIT
	"Video DMA Control",				// 0x1e0 VIDCR
	"<RESERVED>",						// 0x1e4
	"Duplex LCD Init B",				// 0x1e8 VIDINITB
	"<RESERVED>",						// 0x1ec
	"DMA IRQ Status",					// 0x1f0 DMAST
	"DMA IRQ Request",					// 0x1f4 DMARQ
	"DMA IRQ Mask",						// 0x1f8 DMAMSK
	"<RESERVED>"						// 0x1fc
};

#define IOMD_IOCR 		0x000/4
#define IOMD_KBDDAT		0x004/4
#define IOMD_KBDCR		0x008/4

#define IOMD_IRQSTA		0x010/4
#define IOMD_IRQRQA 	0x014/4
#define IOMD_IRQMSKA	0x018/4

#define IOMD_T0LOW		0x040/4
#define IOMD_T0HIGH 	0x044/4
#define IOMD_T0GO		0x048/4
#define IOMD_T0LATCH	0x04c/4

#define IOMD_ID0 		0x094/4
#define IOMD_ID1 		0x098/4

#define IOMD_VIDCUR		0x1d0/4
#define IOMD_VIDEND		0x1d4/4
#define IOMD_VIDSTART	0x1d8/4
#define IOMD_VIDINIT	0x1dc/4
#define IOMD_VIDCR		0x1e0/4

static UINT16 timer_in[2],timer_out[2];
static int timer_counter[2];
static emu_timer *IOMD_timer[2];
static UINT8 IRQ_status_A,IRQ_mask_A;
static UINT8 IOMD_IO_ctrl,IOMD_keyb_ctrl;

static UINT16 io_id;
static UINT8 viddma_status;
static UINT32 viddma_addr_start,viddma_addr_end;

static void fire_iomd_timer(int timer, int timer_count)
{
	int val = timer_count / 2; // correct?

	if(val==0)
		timer_adjust_oneshot(IOMD_timer[timer], attotime_never, 0);
	else
		timer_adjust_periodic(IOMD_timer[timer], ATTOTIME_IN_USEC(val), 0, ATTOTIME_IN_USEC(val));
}

static TIMER_CALLBACK( IOMD_timer0_callback )
{
	IRQ_status_A|=0x20;
	if(IRQ_mask_A&0x20)
	{
		generic_pulse_irq_line(machine->device("maincpu"), ARM7_IRQ_LINE);
	}
}

static TIMER_CALLBACK( IOMD_timer1_callback )
{
	/*
	PS7500_IO[IRQSTA]|=0x40;
	if(PS7500_IO[IRQMSKA]&0x40)
	{
		generic_pulse_irq_line(machine->device("maincpu"), ARM7_IRQ_LINE);
	}
	*/
}

static void viddma_transfer_start(const address_space *space)
{
	UINT32 src = viddma_addr_start;
	UINT32 dst = 0;
	UINT32 size = viddma_addr_end;
	UINT32 dma_index;
	UINT8 *vram = memory_region(space->machine, "vram");

	/* TODO: this should actually be a qword transfer */
	for(dma_index = 0;dma_index < size;dma_index++)
	{
		 vram[dst] = memory_read_byte(space, src);

		 src++;
		 dst++;
	}
}

static READ32_HANDLER( a7000_iomd_r )
{
//	if(offset != IOMD_KBDCR)
//		logerror("IOMD: %s Register (%04x) read\n",iomd_regnames[offset & (0x1ff >> 2)],offset*4);


	switch(offset)
	{
		case IOMD_IOCR:
		{
			static UINT8 vblank = input_port_read(space->machine, "VBLANK") & 0x80;

			return IOMD_IO_ctrl | 0x34 | vblank;
		}
		case IOMD_KBDCR: 	return IOMD_keyb_ctrl | 0x80; //IOMD Keyb status

		/*
		1--- ---- always high
		-x-- ---- Timer 1
		--x- ---- Timer 0
		---x ---- Power On Reset
		---- x--- Flyback
		---- -x-- nINT1
		---- --0- always low
		---- ---x INT2
		*/
		case IOMD_IRQSTA:	return (IRQ_status_A & ~2) | 0x80;
		case IOMD_IRQRQA:	return (IRQ_status_A & IRQ_mask_A) | 0x80;
		case IOMD_IRQMSKA:	return (IRQ_mask_A);

		case IOMD_T0LOW:	return timer_out[0] & 0xff;
		case IOMD_T0HIGH:	return (timer_out[0] >> 8) & 0xff;

		case IOMD_ID0: 		return io_id & 0xff; // IOMD ID low
		case IOMD_ID1: 		return (io_id >> 8) & 0xff; // IOMD ID high

		case IOMD_VIDEND: 	return (viddma_addr_end & 0x00fffff8); //bits 31:24 undefined
		case IOMD_VIDSTART: return (viddma_addr_start & 0x1ffffff8); //bits 31, 30, 29 undefined
		case IOMD_VIDCR:	return (viddma_status & 0xa0) | 0x50; //bit 6 = DRAM mode, bit 4 = QWORD transfer

		default: 	logerror("IOMD: %s Register (%04x) read\n",iomd_regnames[offset & (0x1ff >> 2)],offset*4); break;
	}

	return 0;
}

static WRITE32_HANDLER( a7000_iomd_w )
{
//	logerror("IOMD: %s Register (%04x) write = %08x\n",iomd_regnames[offset & (0x1ff >> 2)],offset*4,data);

	switch(offset)
	{
		case IOMD_IOCR:		IOMD_IO_ctrl = data & ~0xf4; break;

		case IOMD_KBDCR:
			IOMD_keyb_ctrl = data & ~0xf4;
			//keyboard_ctrl_write(data & 0x08);
			break;

		case IOMD_IRQRQA:	IRQ_status_A &= ~data; break;
		case IOMD_IRQMSKA:	IRQ_mask_A = (data & ~2) | 0x80; break;

		case IOMD_T0LOW: 	timer_in[0] = (timer_in[0] & 0xff00) | (data & 0xff); break;
		case IOMD_T0HIGH: 	timer_in[0] = (timer_in[0] & 0x00ff) | ((data & 0xff) << 8); break;
		case IOMD_T0GO:
			timer_counter[0] = timer_in[0];
			fire_iomd_timer(0,timer_counter[0]);
			break;
		case IOMD_T0LATCH:
			{
				static UINT8 readinc;
				readinc^=1;
				timer_out[0] = timer_counter[0];
				if(readinc)
				{
					timer_counter[0]--;
					if(timer_counter[0] < 0)
						timer_counter[0]+= timer_in[0];
				}
			}
			break;

		case IOMD_VIDEND: 	viddma_addr_end = data & 0x00fffff8; //bits 31:24 unused
		case IOMD_VIDSTART: viddma_addr_start = data & 0x1ffffff8; //bits 31, 30, 29 unused
		case IOMD_VIDCR:
			viddma_status = data & 0xa0; if(data & 0x20) { viddma_transfer_start(space); }
			break;


		default: logerror("IOMD: %s Register (%04x) write = %08x\n",iomd_regnames[offset & (0x1ff >> 2)],offset*4,data);
	}
}

static ADDRESS_MAP_START( a7000_mem, ADDRESS_SPACE_PROGRAM, 32)
	AM_RANGE(0x00000000, 0x003fffff) AM_MIRROR(0x00800000) AM_ROM AM_REGION("user1", 0x0)
//	AM_RANGE(0x01000000, 0x01ffffff) AM_NOP //expansion ROM
//	AM_RANGE(0x02000000, 0x02ffffff) AM_RAM //VRAM
//	I/O 03000000 - 033fffff
//	AM_RANGE(0x03010000, 0x03011fff) //Super IO
//	AM_RANGE(0x03012000, 0x03029fff) //FDC
//	AM_RANGE(0x0302b000, 0x0302bfff) //Network podule
//	AM_RANGE(0x03040000, 0x0304ffff) //podule space 0,1,2,3
//	AM_RANGE(0x03070000, 0x0307ffff) //podule space 4,5,6,7
	AM_RANGE(0x03200000, 0x032001ff) AM_READWRITE(a7000_iomd_r,a7000_iomd_w) //IOMD Registers //mirrored at 0x03000000-0x1ff?
//	AM_RANGE(0x03310000, 0x03310003) //Mouse Buttons

	AM_RANGE(0x03400000, 0x037fffff) AM_WRITE(a7000_vidc20_w)
//	AM_RANGE(0x08000000, 0x08ffffff) AM_MIRROR(0x07000000) //EASI space
	AM_RANGE(0x10000000, 0x13ffffff) AM_RAM //SIMM 0 bank 0
	AM_RANGE(0x14000000, 0x17ffffff) AM_RAM //SIMM 0 bank 1
//	AM_RANGE(0x18000000, 0x18ffffff) AM_MIRROR(0x03000000) AM_RAM //SIMM 1 bank 0
//	AM_RANGE(0x1c000000, 0x1cffffff) AM_MIRROR(0x03000000) AM_RAM //SIMM 1 bank 1
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( a7000 )
	PORT_START("VBLANK")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END

static MACHINE_START(a7000)
{
	IOMD_timer[0] = timer_alloc(machine, IOMD_timer0_callback, NULL);
	IOMD_timer[1] = timer_alloc(machine, IOMD_timer1_callback, NULL);

	io_id = 0xd4e7;
}

static MACHINE_RESET(a7000)
{
	IOMD_IO_ctrl = 0x0b | 0x34; //bit 0,1 and 3 set high on reset plus 2,4,5 always high
//	IRQ_status_A = 0x10; // set POR bit ON
	IRQ_mask_A = 0x00;

	IOMD_keyb_ctrl = 0x00;

	timer_adjust_oneshot( IOMD_timer[0], attotime_never, 0);
	timer_adjust_oneshot( IOMD_timer[1], attotime_never, 0);
}

static INTERRUPT_GEN( flyback_irq )
{
	IRQ_status_A|=0x08;
	if(IRQ_mask_A&0x08)
	{
		generic_pulse_irq_line(device, ARM7_IRQ_LINE);
	}
}


static MACHINE_DRIVER_START( a7000 )

	/* Basic machine hardware */
	MDRV_CPU_ADD( "maincpu", ARM7, XTAL_32MHz )
	MDRV_CPU_PROGRAM_MAP( a7000_mem)
	MDRV_CPU_VBLANK_INT( "screen", flyback_irq )

	MDRV_MACHINE_START( a7000 )
	MDRV_MACHINE_RESET( a7000 )

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(60)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(1900, 1080) //max available size
    MDRV_SCREEN_VISIBLE_AREA(0, 1900-1, 0, 1080-1)
    MDRV_PALETTE_LENGTH(0x200)
//  MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(a7000)
    MDRV_VIDEO_UPDATE(a7000)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( a7000p )
	MDRV_IMPORT_FROM( a7000 )

	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_CLOCK(XTAL_48MHz)
MACHINE_DRIVER_END

ROM_START(a7000)
	ROM_REGION( 0x800000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD( "rom1.bin", 0x000000, 0x100000, CRC(ff0e3d12) SHA1(fa489bebede3d13dc43cddec5b5c9b6829a28914) )
	ROM_LOAD( "rom2.bin", 0x100000, 0x100000, CRC(4ae4fd8b) SHA1(1b30d5905d5364dfa48bad69257b0ef8190e9bf6) )
	ROM_LOAD( "rom3.bin", 0x200000, 0x100000, CRC(3108fb2b) SHA1(865b01583f3fb5f4ed5e9201676db327cdeb40b3) )
	ROM_LOAD( "rom4.bin", 0x300000, 0x100000, CRC(55a51980) SHA1(a7191727edd5babf679ebbdea6585833a1fb34e6) )

	ROM_REGION( 0x800000, "vram", ROMREGION_ERASE00 )
ROM_END

ROM_START(a7000p)
	ROM_REGION( 0x800000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD( "riscos-3.71.rom", 0x000000, 0x400000, CRC(211cf888) SHA1(c5fe0645e48894fb4b245abeefdc9a65d659af59))

	ROM_REGION( 0x800000, "vram", ROMREGION_ERASE00 )
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT   INIT    COMPANY FULLNAME        FLAGS */
COMP( 1995, a7000,      0,      0,      a7000,      a7000,	0,      "Acorn",  "Archimedes A7000",   GAME_NOT_WORKING | GAME_NO_SOUND )
COMP( 1997, a7000p,     a7000,  0,      a7000,      a7000,	0,      "Acorn",  "Archimedes A7000+",  GAME_NOT_WORKING | GAME_NO_SOUND )
