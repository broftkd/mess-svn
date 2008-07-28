#ifndef _osdsdl_h_
#define _osdsdl_h_

/* Notes

	- working ui mouse inputs for SDL1.2 and SDL1.3 incl. yuv modes
	- added blitmode "hwblit" (SDL1.3) for rgb hardware scaling
	- rename "-yuvmode" option to "-scalemode"
	- rename yuv_mode and derivatives to scale_mode
	- moved extra_flags into sdl_info
	- moved callbacks indow sdl_window_info
	- made a number of flags (e.g. yuvmode) window relative
         - changing of yuvmodes and opengl scale effects is working with SDL 1.3
	- improved fullscreen handling
	- removed HAS_WINDOW_MENU - this has no effect nowhere
	- removed underscores in header defines, e.g. __SDL_SYNC__ ==> __SDLSYNC__
	- added SDLMAME_HAS_DEBUGGER define
	- removed #if 0 inw window.c
	- added option "-audiodriver" to specify the SDL audio driver
	- added option "-videodriver" to specify the SDL video driver
	- added option "-renderdriver" to specify the SDL renderer driver
	- changed environment SDLMAME_GL_LIB into option -gl_lib
	- added some more SDL_ENV defines
	- move keymap reading into separate function
	- SDL_EnableUNICODE for all builds (not only MESS)
	- SDL1.3 : Mouse & text input for ui

	- removed  osd_event_wait_multiple from sdlsync.h
	- removed some includes not needed
	- reorganized texcopy/scale2x
	- removed effect.h
	- removed effect_func.h
	- indenting
	- put osd_copyfile, osd_stat into #ifdef MESS
	- added code which implements events without the need
 	  for pthreads. This is commented out, since 
 	  owever, it horribly fails, if
 	  threads > num processors as is the case if you 
 	  enable "-mt"
 	- rename mame_bitmap to bitmap_t
 	- introduced define OSDWORK_CALLBACK to prototype and
 	  define functions to be passed to sdlwork.c

	- fixes from judge for warnings / may reappear (glade)
	- more warnings fixed / may reappear (glade)
	- moved osdefs.h into sdlprefix.h
	- removed osdefs.h
	- finally removed sdlmisc.h

	- create drawogl.c and moved ogl relevant stuff there
	- draw.window_init() now called after window creation
	- removed window.opengl flag
	- added sdl_window_info as parameter to all functions in window.h
	- rename SDL_VERSIONNUM to SDL_VERSION_ATLEAST
	- removed all uclock stuff in sdlmisc.[ch]
	- minor cleanups

	- fixed compile issues against SDL13
	- fixed input issues with SDL13
	- stricter checks for USE_OPENGL, e.g. for options
	- move sdlvideo_loadgl to window.c, rename it to sdlwindow_loadgl and make it static
	- moved yuv_blit.c into drawsdl.c
	- renamed compute_blit_surface_size to sdlwindow_blit_surface_size
	- renamed drawsdl_destroy_all_textures to drawogl_destroy_all_textures and
	  moved it to _sdl_draw_callbacks
	- removed print_colums
	- rename misc.h to sdlmisc.h
	- moved some includes from .h to .c
	- rename led_init to sdlled_init for consistency
	- rename sdl_init_audio to sdlaudio_init for consistency
	- fixed some indentation issues
	- removed ticker.h & dirty.h
	
	- changed all [f]printf to mame_printf_verbose|error|warning
	- removed obsolete frameskipping code
	- removed obsolete throttle code
	- removed fastforward
	- removed framestorun
	- introduced SDLOPTION constants for a number of options
	- add more verbose info for YUV 

	- removed dirty.h
	- removed ticker.h

	- remove win_trying_to_quit
	- changed win_use_mouse to static use_mouse
	- removed win_key_trans_table
	- removed keyboard typematic definitions from input.h
	- made sdl_monitor_list static
	- removed hwstretch (sdl_video_config)
	- removed syncrefresh (sdl_video_config)
	- removed triplebuf (sdl_video_config)
	- removed sdl_has_menu
	- fixed memory_leak (window.c)
	
	- moved prototypes from drawsdl.c to window.h 
	- removed joystick calibration code 
	- "#if 0" code which is unreachable  
	- "#if 0" code which is never used 
	- moved pick_best_mode to window.c	
	- removed pause_brightness option
	- added more SDLOPTION_ defines

	- renamed void yuv_lookup_init to drawsdl_yuv_init (global namespace)
	- rmoved some obsolete code
	- add SDL1.3 compatibility 
	
	- fixed some compile issues
	- moved clear_surface into window thread
	- got SDL1.3 -mt working - still crashing on exit
	
	- removed "digital" option
	- removed device selection options
	- added more SDLOPTION defines
*/

//============================================================
//	System dependent defines
//============================================================

// Process events in worker thread
#if defined(SDLMAME_WIN32) || (SDL_VERSION_ATLEAST(1,3,0))
#define SDLMAME_EVENTS_IN_WORKER_THREAD	(1)
#else
#define SDLMAME_EVENTS_IN_WORKER_THREAD	(0)
#endif

#if defined(SDLMAME_WIN32)
#define SDLMAME_INIT_IN_WORKER_THREAD	(1)
#else
#define SDLMAME_INIT_IN_WORKER_THREAD	(0)
#endif

#if defined(SDLMAME_NO_X11) || defined(SDLMAME_WIN32)
#define SDLMAME_HAS_DEBUGGER 			(0)
#else
#define SDLMAME_HAS_DEBUGGER 			(1)
#endif

//============================================================
//	Defines
//============================================================

#define SDLOPTION_INIPATH				"inipath"
#define SDLOPTION_AUDIO_LATENCY			"audio_latency"
#define SDLOPTION_SCREEN(x)				"screen" x
#define SDLOPTION_ASPECT(x)				"aspect" x
#define SDLOPTION_RESOLUTION(x)			"resolution" x
#define SDLOPTION_VIEW(x)				"view" x 
#define SDLOPTION_SDLVIDEOFPS			"sdlvideofps"
#define SDLOPTION_KEEPASPECT			"keepaspect"
#define SDLOPTION_WINDOW				"window"
#define SDLOPTION_NUMSCREENS			"numscreens"
#define SDLOPTION_UNEVENSTRETCH			"unevenstretch"
#define SDLOPTION_USEALLHEADS			"useallheads"
#define SDLOPTION_MAXIMIZE				"maximize"
#define SDLOPTION_EFFECT				"effect"
#define SDLOPTION_VIDEO					"video"
#define SDLOPTION_SWITCHRES				"switchres"
#define SDLOPTION_FILTER				"filter"
#define SDLOPTION_CENTERH				"centerh"
#define SDLOPTION_CENTERV				"centerv"
#define SDLOPTION_PRESCALE				"prescale"
#define SDLOPTION_PRESCALE_EFFECT		"prescale_effect"
#define SDLOPTION_SCALEMODE				"scalemode"
#define SDLOPTION_MULTITHREADING		"multithreading"
#define SDLOPTION_WAITVSYNC				"waitvsync"
#define SDLOPTION_KEYMAP				"keymap"
#define SDLOPTION_KEYMAP_FILE			"keymap_file"
#define SDLOPTION_JOYMAP				"remapjoys"
#define SDLOPTION_JOYMAP_FILE			"remapjoyfile"
#define SDLOPTION_UIMODEKEY				"uimodekey"
#define SDLOPTION_SIXAXIS				"sixaxis"
#define SDLOPTION_OSLOG					"oslog"

#define SDLOPTION_SHADER_MAME(x)		"glsl_shader_mame" x
#define SDLOPTION_SHADER_SCREEN(x)		"glsl_shader_screen" x
#define SDLOPTION_GLSL_FILTER			"gl_glsl_filter"
#define SDLOPTION_GL_GLSL				"gl_glsl"
#define SDLOPTION_GL_PBO				"gl_pbo"
#define SDLOPTION_GL_VBO				"gl_vbo"
#define SDLOPTION_GL_NOTEXTURERECT		"gl_notexturerect"
#define SDLOPTION_GL_FORCEPOW2TEXTURE	"gl_forcepow2texture"
#define SDLOPTION_GL_GLSL_VID_ATTR		"gl_glsl_vid_attr"

#define SDLOPTION_AUDIODRIVER			"audiodriver"
#define SDLOPTION_VIDEODRIVER			"videodriver"
#define SDLOPTION_RENDERDRIVER			"renderdriver"
#define SDLOPTION_GL_LIB				"gl_lib"

#define SDLOPTVAL_NONE					"none"
#define SDLOPTVAL_AUTO					"auto"

#define SDLOPTVAL_OPENGL				"opengl"
#define SDLOPTVAL_OPENGL16				"opengl16"
#define SDLOPTVAL_SOFT					"soft"

#define SDLOPTVAL_HWBLIT				"hwblit"
#define SDLOPTVAL_YV12					"yv12"
#define SDLOPTVAL_YV12x2				"yv12x2"
#define SDLOPTVAL_YUY2					"yuy2"
#define SDLOPTVAL_YUY2x2				"yuy2x2"

#define SDL_LED(x)						"led" #x

// read by sdlmame

#define SDLENV_DESKTOPDIM				"SDLMAME_DESKTOPDIM"
#define SDLENV_VMWARE					"SDLMAME_VMWARE"

#define sdl_use_unsupported()			(getenv("SDLMAME_UNSUPPORTED") != NULL)

// set by sdlmame

#define SDLENV_VISUALID					"SDL_VIDEO_X11_VISUALID"
#define SDLENV_VIDEODRIVER				"SDL_VIDEODRIVER"
#define SDLENV_AUDIODRIVER				"SDL_AUDIODRIVER"
#define SDLENV_RENDERDRIVER				"SDL_VIDEO_RENDERER"

#define SDL_SOUND_LOG					"sound.log"

#ifdef SDLMAME_MACOSX
/* Vas Crabb: Default GL-lib for MACOSX */
#define SDLOPTVAL_GLLIB					"/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib"
#else
#define SDLOPTVAL_GLLIB					SDLOPTVAL_AUTO
#endif


//============================================================
//	sound.c
//============================================================

void sdlaudio_init(running_machine *machine);

//============================================================
// keybled.c
//============================================================

void sdlled_init(void);


#endif
