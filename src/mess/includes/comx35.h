#ifndef __COMX35__
#define __COMX35__

#include "devices/snapquik.h"

#define SCREEN_TAG			"main"
#define MC6845_SCREEN_TAG	"mc6845"

#define CDP1870_TAG "U1"
#define CDP1869_TAG	"U2"
#define CDP1802_TAG "U3"
#define CDP1871_TAG	"U4"
#define MC6845_TAG	"mc6845"

enum
{
	BANK_NONE = 0,
	BANK_FLOPPY,
	BANK_PRINTER_PARALLEL,
	BANK_PRINTER_PARALLEL_FM,
	BANK_PRINTER_SERIAL,
	BANK_PRINTER_THERMAL,
	BANK_JOYCARD,
	BANK_80_COLUMNS,
	BANK_RAMCARD
};

typedef struct _comx35_state comx35_state;
struct _comx35_state
{
	/* processor state */
	int cdp1802_mode;		/* CPU mode */
	int cdp1802_q;			/* Q flag */
	int cdp1802_ef4;		/* EF4 flag */
	int iden;				/* interrupt/DMA enable */
	int slot;				/* selected slot */
	int bank;				/* selected device bank */
	int rambank;			/* selected RAM bank */
	int dma;				/* memory refresh DMA */

	/* video state */
	int pal_ntsc;			/* PAL/NTSC */
	int cdp1869_prd;		/* CDP1869 predisplay */

	UINT8 *pageram;			/* page memory */
	UINT8 *charram;			/* character memory */

	/* keyboard state */
	int cdp1871_efxa;		/* keyboard data available */
	int cdp1871_efxb;		/* keyboard repeat */

	/* floppy state */
	int fdc_addr;			/* FDC address */
	int fdc_irq;			/* interrupt request */
	int fdc_drq_enable;		/* EF4 enabled */

	/* printer state */
	UINT8 centronics_data;	/* centronics data */

	/* PL-80 plotter state */
	UINT16 font_addr;		/* font ROM pack address latch */
	UINT8 x_motor_phase;	/* X motor phase */
	UINT8 y_motor_phase;	/* Y motor phase */
	UINT8 z_motor_phase;	/* Z motor phase */
	UINT8 plotter_data;		/* plotter data bus */
	int plotter_ack;		/* plotter acknowledge */
	int plotter_online;		/* online LED */

	/* timers */
	emu_timer *reset_timer;	/* power on reset timer */
};

/* ---------- defined in machine/comx35.c ---------- */

WRITE8_HANDLER( comx35_bank_select_w );
READ8_HANDLER( comx35_io_r );
READ8_HANDLER( comx35_io2_r );
WRITE8_HANDLER( comx35_io_w );

WRITE8_HANDLER( pl80_port_a_w );
WRITE8_HANDLER( pl80_port_c_w );
WRITE8_HANDLER( pl80_port_d_w );
READ8_HANDLER( pl80_port_d_r );

MACHINE_START( comx35p );
MACHINE_START( comx35n );
MACHINE_RESET( comx35 );
INPUT_CHANGED( comx35_reset );

DEVICE_IMAGE_LOAD( comx35_floppy );
QUICKLOAD_LOAD( comx35 );

/* ---------- defined in video/comx35.c ---------- */

MACHINE_DRIVER_EXTERN( comx35p_video );
MACHINE_DRIVER_EXTERN( comx35n_video );

#endif
