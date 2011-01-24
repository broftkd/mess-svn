/* GP9001 Video Controller */

class gp9001vdp_device_config : public device_config,
								 public device_config_memory_interface
{
	friend class gp9001vdp_device;
	gp9001vdp_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
public:
	static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
	virtual device_t *alloc_device(running_machine &machine) const;
	static void static_set_gfx_region(device_config *device, int gfxregion);
protected:
	virtual bool device_validity_check(const game_driver &driver) const;
	virtual const address_space_config *memory_space_config(int spacenum = 0) const;
	address_space_config		m_space_config;
	UINT8						m_gfxregion;
};

struct gp9001layeroffsets
{
	int normal;
	int flipped;
};

struct gp9001layer
{
	UINT16 flip;
	UINT16 scrollx;
	UINT16 scrolly;

	gp9001layeroffsets extra_xoffset;
	gp9001layeroffsets extra_yoffset;

	UINT16* vram16; // vram for this layer
};

struct gp9001tilemaplayer : gp9001layer
{
	tilemap_t *tmap;
};

struct gp9001spritelayer : gp9001layer
{
	bool use_sprite_buffer;
	UINT16 *vram16_buffer; // vram buffer for this layer
};


class gp9001vdp_device : public device_t,
						  public device_memory_interface
{
	friend class gp9001vdp_device_config;
	gp9001vdp_device(running_machine &_machine, const gp9001vdp_device_config &config);
public:
	UINT16 gp9001_voffs;
	UINT16 gp9001_scroll_reg;

	gp9001tilemaplayer bg, top, fg;
	gp9001spritelayer sp;

	int	   tile_region; // we also use this to figure out which vdp we're using in some debug logging features

	// technically this is just rom banking, allowing the chip to see more graphic ROM, however it's easier to handle it
	// in the chip implementation than externally for now (which would require dynamic decoding of the entire charsets every
	// time the bank was changed)
	int gp9001_gfxrom_is_banked;
	int gp9001_gfxrom_bank_dirty;		/* dirty flag of object bank (for Batrider) */
	UINT16 gp9001_gfxrom_bank[8];		/* Batrider object bank */


	void draw_sprites( running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect, const UINT8* primap );
	void gp9001_draw_custom_tilemap(running_machine* machine, bitmap_t* bitmap, tilemap_t* tilemap, const UINT8* priremap, const UINT8* pri_enable );
	void gp9001_render_vdp(running_machine* machine, bitmap_t* bitmap, const rectangle* cliprect);
	void gp9001_video_eof(void);
	void create_tilemaps(int region);
	void init_scroll_regs(void);

	bitmap_t *custom_priority_bitmap;

protected:
	virtual void device_start();
	virtual void device_reset();
	const gp9001vdp_device_config &m_config;
	UINT8						m_gfxregion;

};

const device_type gp9001vdp_ = gp9001vdp_device_config::static_alloc_device_config;

#define GP9001_BG_VRAM_SIZE   0x1000	/* Background RAM size */
#define GP9001_FG_VRAM_SIZE   0x1000	/* Foreground RAM size */
#define GP9001_TOP_VRAM_SIZE  0x1000	/* Top Layer  RAM size */
#define GP9001_SPRITERAM_SIZE 0x800	/* Sprite     RAM size */
#define GP9001_SPRITE_FLIPX 0x1000	/* Sprite flip flags */
#define GP9001_SPRITE_FLIPY 0x2000



/* vdp map 0, gfx region 0 */
#define MCFG_DEVICE_ADD_VDP0 \
	MCFG_DEVICE_ADD("gp9001vdp0", gp9001vdp_, 0) \
	gp9001vdp_device_config::static_set_gfx_region(device, 0); \

/* vdp map 1, gfx region 2 */
#define MCFG_DEVICE_ADD_VDP1 \
	MCFG_DEVICE_ADD("gp9001vdp1", gp9001vdp_, 0) \
	gp9001vdp_device_config::static_set_gfx_region(device, 2); \


// access to VDP
READ16_DEVICE_HANDLER( gp9001_vdp_r );
WRITE16_DEVICE_HANDLER( gp9001_vdp_w );
READ16_DEVICE_HANDLER( gp9001_vdp_alt_r );
WRITE16_DEVICE_HANDLER( gp9001_vdp_alt_w );
// this bootleg has strange access
READ16_DEVICE_HANDLER ( pipibibi_bootleg_videoram16_r );
WRITE16_DEVICE_HANDLER( pipibibi_bootleg_videoram16_w  );
READ16_DEVICE_HANDLER ( pipibibi_bootleg_spriteram16_r );
WRITE16_DEVICE_HANDLER( pipibibi_bootleg_spriteram16_w );
WRITE16_DEVICE_HANDLER( pipibibi_bootleg_scroll_w );
