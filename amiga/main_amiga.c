#include "support/gcc8_c_support.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <exec/execbase.h>
#include <graphics/gfxmacros.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>

#include "../plat.h"
#include "../sfx.h"
#include "../misc.h"
#include "gfx_amiga.h"

#include "../player.h"

volatile struct Custom *custom = (struct Custom*)0xdff000;

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct GfxBase *GfxBase;

//backup
static UWORD SystemInts;
static UWORD SystemDMA;
static UWORD SystemADKCON;
static volatile APTR VBR=0;
static APTR SystemIrq;
 
struct View *ActiView;

#ifdef PLAT_HAS_MOUSE
int16_t plat_mouse_x;
int16_t plat_mouse_y;
uint8_t plat_mouse_buttons;   // left:0x01 right:0x02 middle:0x04
static volatile uint8_t prev_mousex = 0;
static volatile uint8_t prev_mousey = 0;
static uint8_t mouse_watchdog = 0; // >0 = active
static void update_mouse();
#endif

static APTR GetVBR(void) {
	APTR vbr = 0;
	UWORD getvbr[] = { 0x4e7a, 0x0801, 0x4e73 }; // MOVEC.L VBR,D0 RTE

	if (SysBase->AttnFlags & AFF_68010) 
		vbr = (APTR)Supervisor((ULONG (*)())getvbr);

	return vbr;
}

void SetInterruptHandler(APTR interrupt) {
	*(volatile APTR*)(((UBYTE*)VBR)+0x6c) = interrupt;
}

APTR GetInterruptHandler() {
	return *(volatile APTR*)(((UBYTE*)VBR)+0x6c);
}

//vblank begins at vpos 312 hpos 1 and ends at vpos 25 hpos 1
//vsync begins at line 2 hpos 132 and ends at vpos 5 hpos 18 
void WaitVbl() {
//	debug_start_idle();
	while (1) {
		volatile ULONG vpos=*(volatile ULONG*)0xDFF004;
		vpos&=0x1ff00;
		if (vpos!=(311<<8))
			break;
	}
	while (1) {
		volatile ULONG vpos=*(volatile ULONG*)0xDFF004;
		vpos&=0x1ff00;
		if (vpos==(311<<8))
			break;
	}
	//debug_stop_idle();
}

void WaitLine(USHORT line) {
	while (1) {
		volatile ULONG vpos=*(volatile ULONG*)0xDFF004;
		if(((vpos >> 8) & 511) == line)
			break;
	}
}

__attribute__((always_inline)) inline void WaitBlt() {
	UWORD tst=*(volatile UWORD*)&custom->dmaconr; //for compatiblity a1000
	(void)tst;
	while (*(volatile UWORD*)&custom->dmaconr&(1<<14)) {} //blitter busy wait
}

void TakeSystem() {
	Forbid();
	//Save current interrupts and DMA settings so we can restore them upon exit. 
	SystemADKCON=custom->adkconr;
	SystemInts=custom->intenar;
	SystemDMA=custom->dmaconr;
	ActiView=GfxBase->ActiView; //store current view

	LoadView(0);
	WaitTOF();
	WaitTOF();

	WaitVbl();
	WaitVbl();

	OwnBlitter();
	WaitBlit();	
	Disable();
	
	custom->intena=0x7fff;//disable all interrupts
	custom->intreq=0x7fff;//Clear any interrupts that were pending
	
	custom->dmacon=0x7fff;//Clear all DMA channels

	//set all colors black
	for(int a=0;a<32;a++)
		custom->color[a]=0;

	WaitVbl();
	WaitVbl();

	VBR=GetVBR();
	SystemIrq=GetInterruptHandler(); //store interrupt register
}

void FreeSystem() { 
	WaitVbl();
	WaitBlit();
	custom->intena=0x7fff;//disable all interrupts
	custom->intreq=0x7fff;//Clear any interrupts that were pending
	custom->dmacon=0x7fff;//Clear all DMA channels

	//restore interrupts
	SetInterruptHandler(SystemIrq);

	/*Restore system copper list(s). */
	custom->cop1lc=(ULONG)GfxBase->copinit;
	custom->cop2lc=(ULONG)GfxBase->LOFlist;
	custom->copjmp1=0x7fff; //start coppper

	/*Restore all interrupts and DMA settings. */
	custom->intena=SystemInts|0x8000;
	custom->dmacon=SystemDMA|0x8000;
	custom->adkcon=SystemADKCON|0x8000;

	WaitBlit();	
	DisownBlitter();
	Enable();

	LoadView(ActiView);
	WaitTOF();
	WaitTOF();

	Permit();
}

__attribute__((always_inline)) inline short Joy1Button(){
    *(volatile UBYTE*)0xBFE201 = 0x03;
    return !((*(volatile UBYTE*)0xbfe001)&128);
}	
__attribute__((always_inline)) inline short MouseLeft(){return !((*(volatile UBYTE*)0xbfe001)&64);}	
__attribute__((always_inline)) inline short MouseRight(){return !((*(volatile UWORD*)0xdff016)&(1<<10));}


static uint8_t vertb_cnt = 0;
static uint8_t blit_cnt = 0;

static __attribute__((interrupt)) void interruptHandler() {

    UWORD req = custom->intreqr;

    if (req & INTF_VERTB) {
        ++vertb_cnt;
    }

    if (req & INTF_BLIT) {
        ++blit_cnt;
        // update the blitq
        gfx_blit_irq_handler();
    }

    custom->intreq = req & 0x7fff;  // reset all
}



void dbug_u16(uint8_t cx, uint8_t cy, uint16_t v)
{
    char buf[4];
    buf[0] = hexdigits[(v>>12) & 0x0F];
    buf[1] = hexdigits[(v>>8) & 0x0F];
    buf[2] = hexdigits[(v>>4) & 0x0F];
    buf[3] = hexdigits[(v>>0) & 0x0F];
    plat_textn(cx,cy,buf,4,1);
}


int main() {
	SysBase = *((struct ExecBase**)4UL);
//	custom = (struct Custom*)0xdff000;

	// We will use the graphics library only to locate and restore the system copper list once we are through.
	GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR)"graphics.library",0);
	if (!GfxBase)
		Exit(0);

	// used for printing
	DOSBase = (struct DosLibrary*)OpenLibrary((CONST_STRPTR)"dos.library", 0);
	if (!DOSBase)
		Exit(0);

	//KPrintF("Hello debugger from Amiga!\n");
	Write(Output(), (APTR)"Hello console!\n", 15);

	//warpmode(1);
	//warpmode(0);

	TakeSystem();

	SetInterruptHandler((APTR)interruptHandler);
	custom->intena = INTF_SETCLR | INTF_INTEN | INTF_BLIT | INTF_VERTB;


	WaitVbl();
    if (!gfx_init()) {
        goto done;
    }

	custom->dmacon = DMAF_SETCLR | DMAF_MASTER | DMAF_RASTER | DMAF_COPPER | DMAF_BLITTER;

    game_init();

    // main loop
	while(1) {
        //custom->color[0] = 0x000;
        WaitVbl();  //Line(240);
        //custom->color[0] = 0x888;
        gfx_present_frame();    // flip buffers
        //custom->color[0] = 0x800;
        gfx_startrender();
        //custom->color[0] = 0x080;
        game_render();
#ifdef PLAT_HAS_MOUSE
        if (mouse_watchdog > 0) {
            sprout16(plat_mouse_x, plat_mouse_y, SPR16_RETICULE);
        }
#endif
        /*
        for( int i = 0; i < 8; ++i) {
            if(tick>>6 & 1) {
            plat_hline_noclip(10-i,i+11,i,0,(i+tick)&0xf);
            } else {
            plat_hline_noclip(11-i,i+12,i,0,(i+tick)&0xf);
            }
        }
        */
        //uint8_t i = (tick>>4);
        //plat_drawbox(0,0,i,i,0, tick & 0x0f);

        //plat_drawbox(21,1,i,i,0, tick & 0x0f);

        /*
        dbug_u16( 10,0, (uint16_t)(plrx[0]>>FX));
        dbug_u16( 15,0, (uint16_t)(plry[0]>>FX));
        */

        //custom->color[0] = 0x008;
        game_tick();
        ++tick;
        //custom->color[0] = 0x444;
#ifdef PLAT_HAS_MOUSE
        update_mouse();
#endif
        gfx_wait_for_blitting();
    }   

	// END
done:
    gfx_shutdown();
	FreeSystem();

	CloseLibrary((struct Library*)DOSBase);
	CloseLibrary((struct Library*)GfxBase);
}





/*******************************************************/

volatile uint8_t tick;

void plat_quit()
{
}

// Noddy profiling (rasterbars)
void plat_gatso(uint8_t t)
{
}



void plat_hud(uint8_t level, uint8_t lives, uint32_t score)
{
    const uint8_t cx = SCREEN_TEXT_W / 2;

    char buf[8];
    // Level
    {
        uint8_t bcd = bin2bcd8(level);
        char* p = buf;
        *p++ = 'L';
        *p++ = 'V';
        *p++ = ' ';
        *p++ = hexdigits[bcd >> 4];
        *p++ = hexdigits[bcd & 0x0f];
        plat_textn(cx - 11, 0, buf, 5, 1);
    }

    // Score
    {
        uint32_t bcd = bin2bcd32(score);
        char* p = buf;
        *p++ = hexdigits[(bcd >> 28)&0x0f];
        *p++ = hexdigits[(bcd >> 24)&0x0f];
        *p++ = hexdigits[(bcd >> 20)&0x0f];
        *p++ = hexdigits[(bcd >> 16)&0x0f];
        *p++ = hexdigits[(bcd >> 12)&0x0f];
        *p++ = hexdigits[(bcd >> 8)&0x0f];
        *p++ = hexdigits[(bcd >> 4)&0x0f];
        *p++ = hexdigits[(bcd >> 0)&0x0f];

        plat_textn(cx-4, 0, buf, 8, 1);
    }

    // Lives
    {
        for (int i = 0; i < 8; ++i) {
            char ch;
            if (i < lives) {
                ch = (i == 7) ? '+' : '*';
            } else {
                ch = ' ';
            }
            buf[i] = ch;
        }
        plat_textn(cx+6, 0, buf, 8, 2);
    }
}



/*
 * SPRITES
 */

/*
 * SOUND
 */


void sfx_init()
{
}

void sfx_tick(uint8_t frametick)
{
}

void sfx_play(uint8_t effect, uint8_t pri)
{
}

uint8_t sfx_continuous = SFX_NONE;

/*
 * INPUT
 */

// Returns direction + FIRE_ bits.
// dualstick faked using plat_raw_gamepad()
static uint8_t firelock = 0;    // fire bits if locked (else 0)
static uint8_t facing = 0;  // last non-zero direction

uint8_t plat_raw_dualstick()
{
    uint8_t pad = plat_raw_gamepad();

    uint8_t out = pad & (INP_UP|INP_DOWN|INP_LEFT|INP_RIGHT);
    if (out != 0) {
        facing = out;
    }

    if (pad & INP_PAD_A) {
        if (!firelock) {
            firelock = (facing<<4);
        }
        out |= firelock;
    } else {
        firelock = 0;
    }

    return out;
}

// Returns direction + PAD_ bits.
uint8_t plat_raw_gamepad()
{

    UWORD dat = custom->joy1dat;
    UWORD pra0 = (*(volatile UBYTE*)0xbfe001);

    uint8_t out = 0;

    // Left: bit9
    if ((dat >> 9) & 1) {
        out |= INP_LEFT;
    }
    // Right: bit1
    if ((dat >> 1) & 1) {
        out |= INP_RIGHT;
    }
    //Forward: bit9 xor bit#8
    if (((dat >> 9) & 1) != ((dat>>8) & 1)) {
        out |= INP_UP;
    }
    //Back: bit1 xor bit0
    if (((dat >> 1) & 1) != ((dat>>0) & 1)) {
        out |= INP_DOWN;
    }

    if (Joy1Button()) {
        out |= INP_PAD_A;
    }

    if (MouseLeft() && MouseRight()) {
        out |= INP_PAD_START;
    }
    return out;
}


// Returns direction + KEY_ bits.
uint8_t plat_raw_keys()
{
    return 0;
}

uint8_t plat_raw_cheatkeys()
{
    return 0;
}

#ifdef PLAT_HAS_MOUSE

static inline int16_t clip(int16_t v, int16_t vmin, int16_t vmax) {
    if (v<vmin) {
        return vmin;
    }
    if (v>vmax) {
        return vmax;
    }
    return v;
}

static void update_mouse()
{
    uint16_t dat = custom->joy0dat;
    uint8_t cntx = dat & 0xff;
    uint8_t cnty = (dat>>8) & 0xff;

    int16_t dx = cntx - prev_mousex;
    int16_t dy = cnty - prev_mousey;
    prev_mousex = cntx;
    prev_mousey = cnty;

    uint8_t mb = 0;
    if (MouseLeft()) {
        mb |= 0x01;
    }
    if (MouseRight()) {
        mb |= 0x02;
    }

    if (mb != 0 || dx != 0 || dy != 0) {
        plat_mouse_buttons = mb;
        plat_mouse_x += dx<<FX;
        plat_mouse_x = clip( plat_mouse_x, 0, (SCREEN_W-1)<<FX);
        plat_mouse_y += dy<<FX;
        plat_mouse_y = clip( plat_mouse_y, 0, (SCREEN_H-1)<<FX);
        mouse_watchdog = 60;
    } else {
       if (mouse_watchdog > 0) {
           --mouse_watchdog;
       }
    }
}


#endif // PLAT_HAS_MOUSE


#ifdef PLAT_HAS_TEXTENTRY
// start PLAT_HAS_TEXTENTRY (fns should be no-ops if not supported)
void plat_textentry_start()
{
}

void plat_textentry_stop()
{
}

// Returns printable ascii or DEL (0x7f) or LF (0x0A), or 0 for no input.
char plat_textentry_getchar()
{
    return '\0';
}

#endif // PLAT_HAS_TEXTENTRY


/*
 * HIGHSCORE PERSISTENCE
 */

#ifdef PLAT_HAS_SCORESAVE

bool plat_savescores(const void* begin, int nbytes)
{
    return false;
}

bool plat_loadscores(void* begin, int nbytes)
{
    return false;
}
#endif // PLAT_HAS_SCORESAVE

