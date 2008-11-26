/***************************************************************************

        Kramer MC video driver by Miodrag Milanovic

        13/09/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "includes/kramermc.h"

#define KRAMERMC_VIDEO_MEMORY		0xFC00

const gfx_layout kramermc_charlayout =
{
	8, 8,				/* 8x8 characters */
	256,				/* 256 characters */
	1,				  /* 1 bits per pixel */
	{0},				/* no bitplanes; 1 bit per pixel */
	{0, 1, 2, 3, 4, 5, 6, 7},
	{0 * 8, 1 * 8, 2 * 8, 3 * 8, 4 * 8, 5 * 8, 6 * 8, 7 * 8},
	8*8					/* size of one char */
};

VIDEO_START( kramermc )
{
}

VIDEO_UPDATE( kramermc )
{
	int x,y;
	const address_space *space = cpu_get_address_space(screen->machine->cpu[0], ADDRESS_SPACE_PROGRAM);
	
	for(y = 0; y < 16; y++ )
	{
		for(x = 0; x < 64; x++ )
		{
			int code = memory_read_byte(space, KRAMERMC_VIDEO_MEMORY + x + y*64);
			drawgfx(bitmap, screen->machine->gfx[0],  code , 0, 0,0, x*8,y*8,
				NULL, TRANSPARENCY_NONE, 0);
		}
	}
	return 0;
}

