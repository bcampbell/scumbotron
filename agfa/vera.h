// Copyright 2022 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// Scumbotron stand-alone VERA defines, adapted from llvm-mos-sdk cx16.h

// Originally from cc65. Modified from original version.

// clang-format off

/*****************************************************************************/
/*                                                                           */
/*                                  cx16.h                                   */
/*                                                                           */
/*                      CX16 system-specific definitions                     */
/*                             For prerelease 39                             */
/*                                                                           */
/*                                                                           */
/* This software is provided "as-is", without any expressed or implied       */
/* warranty.  In no event will the authors be held liable for any damages    */
/* arising from the use of this software.                                    */
/*                                                                           */
/* Permission is granted to anyone to use this software for any purpose,     */
/* including commercial applications, and to alter it and redistribute it    */
/* freely, subject to the following restrictions:                            */
/*                                                                           */
/* 1. The origin of this software must not be misrepresented; you must not   */
/*    claim that you wrote the original software. If you use this software   */
/*    in a product, an acknowledgment in the product documentation would be  */
/*    appreciated, but is not required.                                      */
/* 2. Altered source versions must be plainly marked as such, and must not   */
/*    be misrepresented as being the original software.                      */
/* 3. This notice may not be removed or altered from any source              */
/*    distribution.                                                          */
/*                                                                           */
/*****************************************************************************/

#ifndef _VERA_STANDALONE_H
#define _VERA_STANDALONE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* get_tv() return codes
** set_tv() argument codes
** NOTE: llvm-mos-sdk added newer 240P modes
*/
enum : uint8_t {
    TV_NONE                     = 0x00,
    TV_VGA,
    TV_NTSC_COLOR,
    TV_RGB,
    TV_NONE2,
    TV_VGA2,
    TV_NTSC_MONO,
    TV_RGB2,
    TV_NONE_240P,
    TV_VGA_240P,
    TV_NTSC_COLOR_240P,
    TV_RGB_240P,
    TV_NONE2_240P,
    TV_VGA2_240P,
    TV_NTSC_MONO_240P,
    TV_RGB2_240P
};

/* Video modes for videomode() */
#define VIDEOMODE_80x60         0x00
#define VIDEOMODE_80x30         0x01
#define VIDEOMODE_40x60         0x02
#define VIDEOMODE_40x30         0x03
#define VIDEOMODE_40x15         0x04
#define VIDEOMODE_20x30         0x05
#define VIDEOMODE_20x15         0x06
#define VIDEOMODE_80COL         VIDEOMODE_80x60
#define VIDEOMODE_40COL         VIDEOMODE_40x30
#define VIDEOMODE_320x240       0x80
#define VIDEOMODE_SWAP          (-1)

/* VERA's address increment/decrement numbers */
enum : uint8_t {
    VERA_DEC_0                  = ((0 << 1) | 1) << 3,
    VERA_DEC_1                  = ((1 << 1) | 1) << 3,
    VERA_DEC_2                  = ((2 << 1) | 1) << 3,
    VERA_DEC_4                  = ((3 << 1) | 1) << 3,
    VERA_DEC_8                  = ((4 << 1) | 1) << 3,
    VERA_DEC_16                 = ((5 << 1) | 1) << 3,
    VERA_DEC_32                 = ((6 << 1) | 1) << 3,
    VERA_DEC_64                 = ((7 << 1) | 1) << 3,
    VERA_DEC_128                = ((8 << 1) | 1) << 3,
    VERA_DEC_256                = ((9 << 1) | 1) << 3,
    VERA_DEC_512                = ((10 << 1) | 1) << 3,
    VERA_DEC_40                 = ((11 << 1) | 1) << 3,
    VERA_DEC_80                 = ((12 << 1) | 1) << 3,
    VERA_DEC_160                = ((13 << 1) | 1) << 3,
    VERA_DEC_320                = ((14 << 1) | 1) << 3,
    VERA_DEC_640                = ((15 << 1) | 1) << 3,
    VERA_INC_0                  = ((0 << 1) | 0) << 3,
    VERA_INC_1                  = ((1 << 1) | 0) << 3,
    VERA_INC_2                  = ((2 << 1) | 0) << 3,
    VERA_INC_4                  = ((3 << 1) | 0) << 3,
    VERA_INC_8                  = ((4 << 1) | 0) << 3,
    VERA_INC_16                 = ((5 << 1) | 0) << 3,
    VERA_INC_32                 = ((6 << 1) | 0) << 3,
    VERA_INC_64                 = ((7 << 1) | 0) << 3,
    VERA_INC_128                = ((8 << 1) | 0) << 3,
    VERA_INC_256                = ((9 << 1) | 0) << 3,
    VERA_INC_512                = ((10 << 1) | 0) << 3,
    VERA_INC_40                 = ((11 << 1) | 0) << 3,
    VERA_INC_80                 = ((12 << 1) | 0) << 3,
    VERA_INC_160                = ((13 << 1) | 0) << 3,
    VERA_INC_320                = ((14 << 1) | 0) << 3,
    VERA_INC_640                = ((15 << 1) | 0) << 3
};

/* VERA's interrupt flags */
#define VERA_IRQ_VSYNC          0b00000001
#define VERA_IRQ_RASTER         0b00000010
#define VERA_IRQ_SPR_COLL       0b00000100
#define VERA_IRQ_AUDIO_LOW      0b00001000

/* A structure with the Video Enhanced Retro Adapter's external registers */
struct __attribute__((packed)) __vera {
    uint16_t      address;        /* Address for data ports */
    uint8_t       address_hi;
    uint8_t       data0;          /* Data port 0 */
    uint8_t       data1;          /* Data port 1 */
    uint8_t       control;        /* Control register */
    uint8_t       irq_enable;     /* Interrupt enable bits */
    uint8_t       irq_flags;      /* Interrupt flags */
    uint8_t       irq_raster;     /* Line where IRQ will occur */
    union {
        struct __attribute__((packed)) {                        /* Visible when DCSEL flag = 0 */
            uint8_t video;        /* Flags to enable video layers */
            uint8_t hscale;       /* Horizontal scale factor */
            uint8_t vscale;       /* Vertical scale factor */
            uint8_t border;       /* Border color (NTSC mode) */
        };
        struct __attribute__((packed)) {                        /* Visible when DCSEL flag = 1 */
            uint8_t hstart;       /* Horizontal start position */
            uint8_t hstop;        /* Horizontal stop position */
            uint8_t vstart;       /* Vertical start position */
            uint8_t vstop;        /* Vertical stop position */
        };
        struct __attribute__((packed)) {                        /* Visible when DCSEL flag = 2 */
            uint8_t fxctrl;
            uint8_t fxtilebase;
            uint8_t fxmapbase;
            uint8_t fxmult;
        };
        struct __attribute__((packed)) {                        /* Visible when DCSEL flag = 3 */
            uint8_t fxxincrl;
            uint8_t fxxincrh;
            uint8_t fxyincrl;
            uint8_t fxyincrh;
        };
        struct __attribute__((packed)) {                        /* Visible when DCSEL flag = 4 */
            uint8_t fxxposl;
            uint8_t fxxposh;
            uint8_t fxyposl;
            uint8_t fxyposh;
        };
        struct __attribute__((packed)) {                        /* Visible when DCSEL flag = 5 */
            uint8_t fxxposs;
            uint8_t fxyposs;
            uint8_t fxpolyfilll;
            uint8_t fxpolyfillh;
        };
        struct __attribute__((packed)) {                        /* Visible when DCSEL flag = 6 */
            uint8_t fxcachel;
            uint8_t fxcachem;
            uint8_t fxcacheh;
            uint8_t fxcacheu;
        };
        struct __attribute__((packed)) {                        /* Visible when DCSEL flag = 63 */
            uint8_t dcver0;
            uint8_t dcver1;
            uint8_t dcver2;
            uint8_t dcver3;
        };
    } display;
    struct __attribute__((packed)) {
        uint8_t   config;         /* Layer map geometry */
        uint8_t   mapbase;        /* Map data address */
        uint8_t   tilebase;       /* Tile address and geometry */
        uint16_t    hscroll;        /* Smooth scroll horizontal offset */
        uint16_t    vscroll;        /* Smooth scroll vertical offset */
    } layer0;
    struct __attribute__((packed)) {
        uint8_t   config;
        uint8_t   mapbase;
        uint8_t   tilebase;
        uint16_t    hscroll;
        uint16_t    vscroll;
    } layer1;
    struct __attribute__((packed)) {
        uint8_t   control;        /* PCM format */
        uint8_t   rate;           /* Sample rate */
        uint8_t   data;           /* PCM output queue */
    } audio;                            /* Pulse-Code Modulation registers */
    struct __attribute__((packed)) {
        uint8_t   data;
        uint8_t   control;
    } spi;                              /* SD card interface */
};
#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L) || (defined(__cplusplus) && __cplusplus >= 201103L)
static_assert(sizeof(struct __vera) == 32, "struct __vera must be 32 bytes");
#endif

/** VERA Programmable Sound Generator (PSG) layout */
struct __attribute__((packed)) __vera_psg {
    union {
        uint16_t freq;
        struct {
            uint8_t freq_lo; //!< Frequency word (7:0)
            uint8_t freq_hi; //!< Frequency word (15:8)
        };
    };
    uint8_t volume;   //!< Left (bit 7); right (bit 6); volume (bit 5:0)
    uint8_t waveform; //!< Waveform (bit 7:6) and pulse width (bit 5:0)
};

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L) || (defined(__cplusplus) && __cplusplus >= 201103L)
static_assert(sizeof(struct __vera_psg) == 4, "struct __vera_psg must be 4 bytes");
#endif

#ifdef __cplusplus
}
#endif
#endif  // _VERA_STANDALONE_H
