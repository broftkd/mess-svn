/***************************************************************************

	commodore vic20 home computer
	Peter Trauner
	(peter.trauner@jk.uni-linz.ac.at)

    documentation
     Marko.Makela@HUT.FI (vic6560)
     www.funet.fi

***************************************************************************/

#include <ctype.h>
#include <stdio.h>

#include "driver.h"
#include "deprecat.h"
#include "image.h"
#include "cpu/m6502/m6502.h"

#define VERBOSE_DBG 0
#include "includes/cbm.h"
#include "machine/6522via.h"
#include "includes/vc1541.h"
#include "includes/vc20tape.h"
#include "includes/cbmserb.h"
#include "includes/cbmieeeb.h"
#include "video/vic6560.h"

#include "includes/vc20.h"

static UINT8 keyboard[8] =
{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static int via1_portb, via1_porta, via0_ca2;
static int serial_atn = 1, serial_clock = 1, serial_data = 1;

static int ieee=0; /* ieee cartridge (interface and rom)*/
static UINT8 *vc20_rom_2000;
static UINT8 *vc20_rom_4000;
static UINT8 *vc20_rom_6000;
static UINT8 *vc20_rom_a000;

UINT8 *vc20_memory_9400;

/** via 0 addr 0x9110
 ca1 restore key (low)
 ca2 cassette motor on
 pa0 serial clock in
 pa1 serial data in
 pa2 also joy 0 in
 pa3 also joy 1 in
 pa4 also joy 2 in
 pa5 also joybutton/lightpen in
 pa6 cassette switch in
 pa7 inverted serial atn out
 pa2 till pa6, port b, cb1, cb2 userport
 irq connected to m6502 nmi
*/
static void vc20_via0_irq (running_machine *machine, int level)
{
	cpunum_set_input_line(machine, 0, INPUT_LINE_NMI, level);
}

static READ8_HANDLER( vc20_via0_read_ca1 )
{
	return ! ( input_port_read(machine, "SPECIAL") & 0x02 );
}

static READ8_HANDLER( vc20_via0_read_ca2 )
{
	DBG_LOG (1, "tape", ("motor read %d\n", via0_ca2));
	return via0_ca2;
}

static WRITE8_HANDLER( vc20_via0_write_ca2 )
{
	via0_ca2 = data ? 1 : 0;
	vc20_tape_motor (via0_ca2);
}

static  READ8_HANDLER( vc20_via0_read_porta )
{
	UINT8 value = 0xff;

	if (input_port_read(machine, "CFG") & 0x80)		/* JOYSTICK */
	{
		if (input_port_read(machine, "JOY") & 0x10) /* JOY_BUTTON */
			value&=~0x20;
		if (input_port_read(machine, "JOY") & 4)	/* JOY_LEFT */
			value&=~0x10;
		if (input_port_read(machine, "JOY") & 2)	/* JOY_DOWN */
			value&=~0x8;
		if (input_port_read(machine, "JOY") & 1)	/* JOY_UP */
			value&=~0x4;
	}
	if (input_port_read(machine, "CFG") & 0x40)		 /* PADDLE */
	{
		if (input_port_read(machine, "JOY") & 0x20)  /* PUDDLE1_BUTTON */
			value&=~0x10;
	}
	/* to short to be recognized normally */
	/* should be reduced to about 1 or 2 microseconds */
	/*  if ((input_port_read(machine, "CFG") & 0x20 ) && (input_port_read(machine, "JOY") & 0x80) )  // i.e. LIGHTPEN_BUTTON
		value&=~0x20; */
	if (!serial_clock || !cbm_serial_clock_read ())
		value &= ~1;
	if (!serial_data || !cbm_serial_data_read ())
		value &= ~2;
	if (!vc20_tape_switch ())
		value &= ~0x40;
	return value;
}

static WRITE8_HANDLER( vc20_via0_write_porta )
{
	cbm_serial_atn_write (serial_atn = !(data & 0x80));
	DBG_LOG (1, "serial out", ("atn %s\n", serial_atn ? "high" : "low"));
}

/* via 1 addr 0x9120
 * port a input from keyboard (low key pressed in line of matrix
 * port b select lines of keyboard matrix (low)
 * pb7 also joystick right in! (low)
 * pb3 also cassette write
 * ca1 cassette read
 * ca2 inverted serial clk out
 * cb1 serial srq in
 * cb2 inverted serial data out
 * irq connected to m6502 irq
 */
static void vc20_via1_irq (running_machine *machine, int level)
{
	cpunum_set_input_line(machine, 0, M6502_IRQ_LINE, level);
}

static READ8_HANDLER( vc20_via1_read_porta )
{
	int value = 0xff;

	if (!(via1_portb & 0x01))
		value &= keyboard[0];

	if (!(via1_portb & 0x02))
		value &= keyboard[1];

	if (!(via1_portb & 0x04))
		value &= keyboard[2];

	if (!(via1_portb & 0x08))
		value &= keyboard[3];

	if (!(via1_portb & 0x10))
		value &= keyboard[4];

	if (!(via1_portb & 0x20))
		value &= keyboard[5];

	if (!(via1_portb & 0x40))
		value &= keyboard[6];

	if (!(via1_portb & 0x80))
		value &= keyboard[7];

	return value;
}

static  READ8_HANDLER( vc20_via1_read_ca1 )
{
	return vc20_tape_read ();
}

static WRITE8_HANDLER( vc20_via1_write_ca2 )
{
	cbm_serial_clock_write (serial_clock = !data);
}

static  READ8_HANDLER( vc20_via1_read_portb )
{
	UINT8 value = 0xff;

    if (!(via1_porta&0x80)) {
	UINT8 t=0xff;
	if (!(keyboard[7]&0x80)) t&=~0x80;
	if (!(keyboard[6]&0x80)) t&=~0x40;
	if (!(keyboard[5]&0x80)) t&=~0x20;
	if (!(keyboard[4]&0x80)) t&=~0x10;
	if (!(keyboard[3]&0x80)) t&=~0x08;
	if (!(keyboard[2]&0x80)) t&=~0x04;
	if (!(keyboard[1]&0x80)) t&=~0x02;
	if (!(keyboard[0]&0x80)) t&=~0x01;
	value &=t;
    }
    if (!(via1_porta&0x40)) {
	UINT8 t=0xff;
	if (!(keyboard[7]&0x40)) t&=~0x80;
	if (!(keyboard[6]&0x40)) t&=~0x40;
	if (!(keyboard[5]&0x40)) t&=~0x20;
	if (!(keyboard[4]&0x40)) t&=~0x10;
	if (!(keyboard[3]&0x40)) t&=~0x08;
	if (!(keyboard[2]&0x40)) t&=~0x04;
	if (!(keyboard[1]&0x40)) t&=~0x02;
	if (!(keyboard[0]&0x40)) t&=~0x01;
	value &=t;
    }
    if (!(via1_porta&0x20)) {
	UINT8 t=0xff;
	if (!(keyboard[7]&0x20)) t&=~0x80;
	if (!(keyboard[6]&0x20)) t&=~0x40;
	if (!(keyboard[5]&0x20)) t&=~0x20;
	if (!(keyboard[4]&0x20)) t&=~0x10;
	if (!(keyboard[3]&0x20)) t&=~0x08;
	if (!(keyboard[2]&0x20)) t&=~0x04;
	if (!(keyboard[1]&0x20)) t&=~0x02;
	if (!(keyboard[0]&0x20)) t&=~0x01;
	value &=t;
    }
    if (!(via1_porta&0x10)) {
	UINT8 t=0xff;
	if (!(keyboard[7]&0x10)) t&=~0x80;
	if (!(keyboard[6]&0x10)) t&=~0x40;
	if (!(keyboard[5]&0x10)) t&=~0x20;
	if (!(keyboard[4]&0x10)) t&=~0x10;
	if (!(keyboard[3]&0x10)) t&=~0x08;
	if (!(keyboard[2]&0x10)) t&=~0x04;
	if (!(keyboard[1]&0x10)) t&=~0x02;
	if (!(keyboard[0]&0x10)) t&=~0x01;
	value &=t;
    }
    if (!(via1_porta&0x08)) {
	UINT8 t=0xff;
	if (!(keyboard[7]&0x08)) t&=~0x80;
	if (!(keyboard[6]&0x08)) t&=~0x40;
	if (!(keyboard[5]&0x08)) t&=~0x20;
	if (!(keyboard[4]&0x08)) t&=~0x10;
	if (!(keyboard[3]&0x08)) t&=~0x08;
	if (!(keyboard[2]&0x08)) t&=~0x04;
	if (!(keyboard[1]&0x08)) t&=~0x02;
	if (!(keyboard[0]&0x08)) t&=~0x01;
	value &=t;
    }
    if (!(via1_porta&0x04)) {
	UINT8 t=0xff;
	if (!(keyboard[7]&0x04)) t&=~0x80;
	if (!(keyboard[6]&0x04)) t&=~0x40;
	if (!(keyboard[5]&0x04)) t&=~0x20;
	if (!(keyboard[4]&0x04)) t&=~0x10;
	if (!(keyboard[3]&0x04)) t&=~0x08;
	if (!(keyboard[2]&0x04)) t&=~0x04;
	if (!(keyboard[1]&0x04)) t&=~0x02;
	if (!(keyboard[0]&0x04)) t&=~0x01;
	value &=t;
    }
    if (!(via1_porta&0x02)) {
	UINT8 t=0xff;
	if (!(keyboard[7]&0x02)) t&=~0x80;
	if (!(keyboard[6]&0x02)) t&=~0x40;
	if (!(keyboard[5]&0x02)) t&=~0x20;
	if (!(keyboard[4]&0x02)) t&=~0x10;
	if (!(keyboard[3]&0x02)) t&=~0x08;
	if (!(keyboard[2]&0x02)) t&=~0x04;
	if (!(keyboard[1]&0x02)) t&=~0x02;
	if (!(keyboard[0]&0x02)) t&=~0x01;
	value &=t;
    }
    if (!(via1_porta&0x01)) {
	UINT8 t=0xff;
	if (!(keyboard[7]&0x01)) t&=~0x80;
	if (!(keyboard[6]&0x01)) t&=~0x40;
	if (!(keyboard[5]&0x01)) t&=~0x20;
	if (!(keyboard[4]&0x01)) t&=~0x10;
	if (!(keyboard[3]&0x01)) t&=~0x08;
	if (!(keyboard[2]&0x01)) t&=~0x04;
	if (!(keyboard[1]&0x01)) t&=~0x02;
	if (!(keyboard[0]&0x01)) t&=~0x01;
	value &=t;
    }

	if (input_port_read(machine, "CFG") & 0x80)		/* JOYSTICK */
	{
		if (input_port_read(machine, "JOY") & 8) 	/* JOY_RIGHT */
			value &= ~0x80;
	}
	
	if (input_port_read(machine, "CFG") & 0x40) 	/* PADDLE */
	{
		if (input_port_read(machine, "JOY") & 0x40)  /* PUDDLE2_BUTTON */
			value &= ~0x80;
	}

	return value;
}

static WRITE8_HANDLER( vc20_via1_write_porta )
{
	via1_porta = data;
}


static WRITE8_HANDLER( vc20_via1_write_portb )
{
/*  logerror("via1_write_portb: $%02X\n", data); */
	vc20_tape_write (data & 8 ? 1 : 0);
	via1_portb = data;
}

static  READ8_HANDLER( vc20_via1_read_cb1 )
{
	DBG_LOG (1, "serial in", ("request read\n"));
	return cbm_serial_request_read ();
}

static WRITE8_HANDLER( vc20_via1_write_cb2 )
{
	cbm_serial_data_write (serial_data = !data);
}

/* ieee 6522 number 1 (via4)
 port b
  0 dav out
  1 nrfd out
  2 ndac out (or port a pin 2!?)
  3 eoi in
  4 dav in
  5 nrfd in
  6 ndac in
  7 atn in
 */
static  READ8_HANDLER( vc20_via4_read_portb )
{
	UINT8 data=0;
	if (cbm_ieee_eoi_r()) data|=8;
	if (cbm_ieee_dav_r()) data|=0x10;
	if (cbm_ieee_nrfd_r()) data|=0x20;
	if (cbm_ieee_ndac_r()) data|=0x40;
	if (cbm_ieee_atn_r()) data|=0x80;
	return data;
}

static WRITE8_HANDLER( vc20_via4_write_portb )
{
	cbm_ieee_dav_w(0,data&1);
	cbm_ieee_nrfd_w(0,data&2);
	cbm_ieee_ndac_w(0,data&4);
}

/* ieee 6522 number 2 (via5)
   port a data read
   port b data write
   cb1 srq in ?
   cb2 eoi out
   ca2 atn out
*/
static WRITE8_HANDLER( vc20_via5_write_porta )
{
	cbm_ieee_data_w(0,data);
}

static  READ8_HANDLER( vc20_via5_read_portb )
{
	return cbm_ieee_data_r();
}

static WRITE8_HANDLER( vc20_via5_write_ca2 )
{
	cbm_ieee_atn_w(0,data);
}

static  READ8_HANDLER( vc20_via5_read_cb1 )
{
	return cbm_ieee_srq_r();
}

static WRITE8_HANDLER( vc20_via5_write_cb2 )
{
	cbm_ieee_eoi_w(0,data);
}

static const struct via6522_interface via0 =
{
	vc20_via0_read_porta,
	0,								   /*via0_read_portb, */
	vc20_via0_read_ca1,
	0,								   /*via0_read_cb1, */
	vc20_via0_read_ca2,
	0,								   /*via0_read_cb2, */
	vc20_via0_write_porta,
	0,								   /*via0_write_portb, */
	0,                                 /*via0_write_ca1, */
	0,                                 /*via0_write_cb1, */
	vc20_via0_write_ca2,
	0,								   /*via0_write_cb2, */
	vc20_via0_irq
}, via1 =
{
	vc20_via1_read_porta,
	vc20_via1_read_portb,
	vc20_via1_read_ca1,
	vc20_via1_read_cb1,
	0,								   /*via1_read_ca2, */
	0,								   /*via1_read_cb2, */
	vc20_via1_write_porta,								   /*via1_write_porta, */
	vc20_via1_write_portb,
	0,                                 /*via1_write_ca1, */
	0,                                 /*via1_write_cb1, */
	vc20_via1_write_ca2,
	vc20_via1_write_cb2,
	vc20_via1_irq
},
/* via2,3 used by vc1541 and 2031 disk drives */
via4 =
{
	0, /*vc20_via4_read_porta, */
	vc20_via4_read_portb,
	0, /*vc20_via4_read_ca1, */
	0, /*vc20_via5_read_cb1, */
	0, /* via1_read_ca2 */
	0,								   /*via1_read_cb2, */
	0,								   /*via1_write_porta, */
	vc20_via4_write_portb,
	0,                                 /*via0_write_ca1, */
	0,                                 /*via0_write_cb1, */
	0, /*vc20_via5_write_ca2, */
	0, /*vc20_via5_write_cb2, */
	vc20_via1_irq
}, via5 =
{
	0,/*vc20_via5_read_porta, */
	vc20_via5_read_portb,
	0, /*vc20_via5_read_ca1, */
	vc20_via5_read_cb1,
	0,								   /*via1_read_ca2, */
	0,								   /*via1_read_cb2, */
	vc20_via5_write_porta,
	0,/*vc20_via5_write_portb, */
	0,                                 /*via0_write_ca1, */
	0,                                 /*via0_write_cb1, */
	vc20_via5_write_ca2,
	vc20_via5_write_cb2,
	vc20_via1_irq
};

WRITE8_HANDLER ( vc20_write_9400 )
{
	vc20_memory_9400[offset] = data | 0xf0;
}


int vic6560_dma_read_color (int offset)
{
	return vc20_memory_9400[offset & 0x3ff];
}

int vic6560_dma_read (int offset)
{
	/* should read real system bus between 0x9000 and 0xa000 */
	return program_read_byte(VIC6560ADDR2VC20ADDR (offset));
}

WRITE8_HANDLER( vc20_0400_w ) {
	if ( mess_ram_size >= 8 * 1024 ) {
		mess_ram[ 0x0400 + offset ] = data;
	}
}

WRITE8_HANDLER( vc20_2000_w ) {
	if ( ( mess_ram_size >= 16 * 1024 ) && vc20_rom_2000 == NULL ) {
		mess_ram[ 0x2000 + offset ] = data;
	}
}

WRITE8_HANDLER( vc20_4000_w ) {
	if ( ( mess_ram_size >= 24 * 1024 ) && vc20_rom_4000 == NULL ) {
		mess_ram[ 0x4000 + offset ] = data;
	}
}

WRITE8_HANDLER( vc20_6000_w ) {
	if ( ( mess_ram_size >= 32 * 1024 ) && vc20_rom_6000 == NULL ) {
		mess_ram[ 0x6000 + offset ] = data;
	}
}

static void vc20_memory_init(void)
{
	static int inited=0;
	UINT8 *memory = memory_region (REGION_CPU1);

	if (inited) return;
	/* power up values are random (more likely bit set)
	   measured in cost reduced german vc20
	   6116? 2kbx8 ram in main area
	   2114 1kbx4 ram at non used color ram area 0x9400 */

	memset(memory, 0, 0x400);
	memset(memory+0x1000, 0, 0x1000);

	memory[0x288]=0xff;// makes ae's graphics look correctly
	memory[0xd]=0xff; // for moneywars
	memory[0x1046]=0xff; // for jelly monsters, cosmic cruncher;

#if 0
	/* 2114 poweron ? 64 x 0xff, 64x 0, and so on */
	for (i = 0; i < 0x400; i += 0x40)
	{
		memset (memory + i, i & 0x40 ? 0 : 0xff, 0x40);
		memset (memory+0x9400 + i, 0xf0 | (i & 0x40 ? 0 : 0xf), 0x40);
	}
	// this would be the straight forward memory init for
	// non cost reduced vic20 (2114 rams)
	for (i=0x1000;i<0x2000;i+=0x40) memset(memory+i,i&0x40?0:0xff,0x40);
#endif

	/* clears ieee cartrige rom */
	/* memset (memory + 0xa000, 0xff, 0x2000); */

	inited=1;
}

static void vc20_common_driver_init (void)
{
#ifdef VC1541
	VC1541_CONFIG vc1541= { 1, 8 };
#endif
	vc20_memory_init();

	vc20_tape_open (via_1_ca1_w);

#ifdef VC1541
	vc1541_config (0, 0, &vc1541);
#endif
	via_config (0, &via0);
	via_config (1, &via1);

	/* Set up memory banks */
	memory_set_bankptr( 1, ( ( mess_ram_size >=  8 * 1024 ) ? mess_ram : memory_region(REGION_CPU1) ) + 0x0400 );
	memory_set_bankptr( 2, vc20_rom_2000 ? vc20_rom_2000 : ( ( ( mess_ram_size >= 16 * 1024 ) ? mess_ram : memory_region(REGION_CPU1) ) + 0x2000 ) );
	memory_set_bankptr( 3, vc20_rom_4000 ? vc20_rom_4000 : ( ( ( mess_ram_size >= 24 * 1024 ) ? mess_ram : memory_region(REGION_CPU1) ) + 0x4000 ) );
	memory_set_bankptr( 4, vc20_rom_6000 ? vc20_rom_6000 : ( ( ( mess_ram_size >= 32 * 1024 ) ? mess_ram : memory_region(REGION_CPU1) ) + 0x6000 ) );
	memory_set_bankptr( 5, vc20_rom_a000 ? vc20_rom_a000 : ( memory_region(REGION_CPU1) + 0xa000 ) );
}

/* currently not used, but when time comes */
void vc20_driver_shutdown (void)
{
	vc20_tape_close ();
}

DRIVER_INIT( vc20 )
{
	vc20_common_driver_init ();
	vic6561_init (vic6560_dma_read, vic6560_dma_read_color);
}

DRIVER_INIT( vic20 )
{
	vc20_common_driver_init ();
	vic6560_init (vic6560_dma_read, vic6560_dma_read_color);
}

DRIVER_INIT( vic1001 )
{
	DRIVER_INIT_CALL(vic20);
}

DRIVER_INIT( vic20i )
{
	ieee=1;
	vc20_common_driver_init ();
	vic6560_init (vic6560_dma_read, vic6560_dma_read_color);
	via_config (4, &via4);
	via_config (5, &via5);
	cbm_ieee_open();
}

MACHINE_RESET( vc20 )
{
	cbm_serial_reset_write (0);
#ifdef VC1541
	vc1541_reset ();
#endif
	if (ieee) {
		cbm_drive_0_config (IEEE, 8);
		cbm_drive_1_config (IEEE, 9);
	} else {
		cbm_drive_0_config (SERIAL, 8);
		cbm_drive_1_config (SERIAL, 9);
	}
	via_reset ();
	via_0_ca1_w(machine, 0, vc20_via0_read_ca1(machine, 0) );
}

static int vc20_rom_id(const device_config *image)
{
	static const unsigned char magic[] =
	{0x41, 0x30, 0x20, 0xc3, 0xc2, 0xcd};	/* A0 CBM at 0xa004 (module offset 4) */
	unsigned char buffer[sizeof (magic)];
	const char *cp;
	int retval;

	logerror("vc20_rom_id %s\n", image_filename(image));

	retval = 0;

	image_fseek (image, 4, SEEK_SET);
	image_fread (image, buffer, sizeof (magic));

	if (!memcmp (buffer, magic, sizeof (magic)))
		retval = 1;

	cp = image_filetype(image);
	if (cp)
	{
		if ((mame_stricmp (cp, "a0") == 0)
			|| (mame_stricmp (cp, "20") == 0)
			|| (mame_stricmp (cp, "40") == 0)
			|| (mame_stricmp (cp, "60") == 0)
			|| (mame_stricmp (cp, "bin") == 0)
			|| (mame_stricmp (cp, "rom") == 0)
			|| (mame_stricmp (cp, "prg") == 0))
			retval = 1;
	}

		if (retval)
			logerror("rom %s recognized\n", image_filename(image));
		else
			logerror("rom %s not recognized\n", image_filename(image));

	return retval;
}

DEVICE_START(vc20_rom)
{
	vc20_memory_init();
	vc20_rom_2000 = NULL;
	vc20_rom_4000 = NULL;
	vc20_rom_6000 = NULL;
	vc20_rom_a000 = NULL;
}

DEVICE_IMAGE_LOAD(vc20_rom)
{
	int size, read_;
	const char *cp;
	int addr = 0;

	if (!vc20_rom_id(image))
		return 1;
	image_fseek(image, 0, SEEK_SET);

	size = image_length(image);

	cp = image_filetype(image);
	if (cp)
	{
		if ((cp[0] != 0) && (cp[1] == '0') && (cp[2] == 0))
		{
			switch (toupper (cp[0]))
			{
			case 'A':
				addr = 0xa000;
				break;
			case '2':
				addr = 0x2000;
				break;
			case '4':
				addr = 0x4000;
				break;
			case '6':
				addr = 0x6000;
				break;
			}
		}
		else
		{
			if (mame_stricmp (cp, "prg") == 0)
			{
				unsigned short in;

				image_fread(image, &in, 2);
				in = LITTLE_ENDIANIZE_INT16(in);
				logerror("rom prg %.4x\n", in);
				addr = in;
				size -= 2;
			}
		}
	}
	if (addr == 0)
	{
		if (size == 0x4000)
		{							   /* I think rom at 0x4000 */
			addr = 0x4000;
		}
		else
		{
			addr = 0xa000;
		}
	}

	logerror("loading rom %s at %.4x size:%.4x\n",image_filename(image), addr, size);
	read_ = image_fread(image, new_memory_region( image->machine, REGION_USER1, ( size & 0x1FFF ) ? ( size + 0x2000 ) : size, 0 ), size);
	if (read_ != size)
		return 1;

	/* Perform banking for the cartridge */
	switch( addr ) {
	case 0x2000:
		vc20_rom_2000 = memory_region( REGION_USER1 );
		break;
	case 0x4000:
		vc20_rom_4000 = memory_region( REGION_USER1 );
		if ( size > 0x2000 ) {
			vc20_rom_6000 = memory_region( REGION_USER1 ) + 0x2000;
		}
		break;
	case 0x6000:
		vc20_rom_6000 = memory_region( REGION_USER1 );
		break;
	case 0xa000:
		vc20_rom_a000 = memory_region( REGION_USER1 );
		break;
	}
	return 0;
}

INTERRUPT_GEN( vc20_frame_interrupt )
{
	via_0_ca1_w(machine, 0, vc20_via0_read_ca1 (machine, 0));
	keyboard[0] = input_port_read(machine, "ROW0");
	keyboard[1] = input_port_read(machine, "ROW1");
	keyboard[2] = input_port_read(machine, "ROW2");
	keyboard[3] = input_port_read(machine, "ROW3");
	keyboard[4] = input_port_read(machine, "ROW4");
	keyboard[5] = input_port_read(machine, "ROW5");
	keyboard[6] = input_port_read(machine, "ROW6");
	keyboard[7] = input_port_read(machine, "ROW7");

	vc20_tape_config (input_port_read(machine, "CFG") & 0x08, input_port_read(machine, "CFG") & 0x04);	/* DATASSETTE, DATASSETTE_TONE */
	vc20_tape_buttons (input_port_read(machine, "DEVS") & 4, input_port_read(machine, "DEVS") & 2, input_port_read(machine, "DEVS") & 1);	/* DATASSETTE_PLAY, DATASSETTE_RECORD, DATASSETTE_STOP */
	
	set_led_status (1, input_port_read(machine, "SPECIAL") & 0x01 ? 1 : 0);		/*KB_CAPSLOCK_FLAG */
}
