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

volatile struct Custom *custom = (struct Custom*)0xdff000;


//
uint8_t *screen_mem = NULL;
UWORD *copperlist_mem = NULL;

#define COPPERLIST_MEMSIZE 1024
// 4 bitplanes
#define SCREEN_MEMSIZE (SCREEN_W/8 * SCREEN_H * 4)

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


bool gfx_init();
void gfx_shutdown();


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

__attribute__((always_inline)) inline short MouseLeft(){return !((*(volatile UBYTE*)0xbfe001)&64);}	
__attribute__((always_inline)) inline short MouseRight(){return !((*(volatile UWORD*)0xdff016)&(1<<10));}




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

    // Grab some chipmem
    copperlist_mem = (UWORD*)AllocMem(COPPERLIST_MEMSIZE, MEMF_CHIP);
    if (!copperlist_mem) {
        goto done;
    }
    screen_mem = (uint8_t*)AllocMem(SCREEN_MEMSIZE, MEMF_CHIP);
    if (!screen_mem) {
        goto done;
    }

	//KPrintF("Hello debugger from Amiga!\n");
	Write(Output(), (APTR)"Hello console!\n", 15);

	//warpmode(1);
	//warpmode(0);

	TakeSystem();
	WaitVbl();
    if (!gfx_init()) {
        goto done;
    }

    custom->cop1lc = (ULONG)copperlist_mem;
	custom->copjmp1 = 0x7fff; //start coppper
	custom->dmacon = DMAF_SETCLR | DMAF_MASTER | DMAF_RASTER | DMAF_COPPER | DMAF_BLITTER;

    game_init();

    // main loop
    UWORD c = 0xf000;

    int foo = 0;
	while(!MouseLeft()) {
        //custom->color[0] = c++;
        WaitVbl();
        game_tick();
        ++tick;
        game_render();
        ++foo;
    }   

	// END
done:
    gfx_shutdown();
	FreeSystem();
    if (copperlist_mem) {
        FreeMem(copperlist_mem, COPPERLIST_MEMSIZE);
        copperlist_mem = NULL;
    }
    if (screen_mem) {
        FreeMem(screen_mem, SCREEN_MEMSIZE);
        screen_mem = NULL;
    }
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

/*
 * TEXT FUNCTIONS
 */

// Clear text
void plat_clr()
{
}


// TODO: should just pass in level and score as bcd
void plat_hud(uint8_t level, uint8_t lives, uint32_t score)
{
}


// Render bonkers encoding where each byte encodes a 4x2 block of pixels,
// to be rendered via charset.
void plat_mono4x2(uint8_t cx, int8_t cy, const uint8_t* src, uint8_t cw, uint8_t ch, uint8_t basecol)
{
}


// Draw horizontal line of chars, range [cx_begin, cx_end).
void plat_hline_noclip(uint8_t cx_begin, uint8_t cx_end, uint8_t cy, uint8_t chr, uint8_t colour)
{
}


// Draw vertical line of chars, range [cy_begin, cy_end).
void plat_vline_noclip(uint8_t cx, uint8_t cy_begin, uint8_t cy_end, uint8_t chr, uint8_t colour)
{
}


/*
 * SPRITES
 */

/*
 * SOUND
 */

// Set all the params for a PSG channel in one go.
//
// vol: 0=off, 63=max
// freq: for frequency f, this should be f/(48828.125/(2^17))
// waveform: one of PLAT_PSG_*
void plat_psg(uint8_t chan, uint16_t freq, uint8_t vol, uint8_t waveform, uint8_t pulsewidth)
{
}


/*
 * INPUT
 */

// Returns direction + FIRE_ bits.
uint8_t plat_raw_dualstick()
{
    return 0;
}

// Returns direction + PAD_ bits.
uint8_t plat_raw_gamepad()
{
    return 0;
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
int16_t plat_mouse_x;
int16_t plat_mouse_y;
uint8_t plat_mouse_buttons;   // left:0x01 right:0x02 middle:0x04
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

