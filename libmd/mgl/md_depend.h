/*
 * Copyright (c) 2000, 2001
 *    SASAKI Shunsuke (eruchan@users.sourceforge.net)
 *    YANAI Hiroyuki  (fk200329@fsinet.or.jp)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#ifndef	__MD_MGL2_H_
#define	__MD_MGL2_H_

#include <stdio.h>
#include <mgl2.h>
#include <mglcol.h>
#include <mglkey.h>

/* #define MGL2_USE_DITHER */
#define MGL2_OPT_UNROLL
#define MGL2_MAX_WIDTH  640
#define MGL2_MAX_HEIGHT 480

#define QUIT_KEY 322	/* xxx */

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define	MD_LITTLE
#else
#define	MD_BIG
#endif

typedef	struct
{
	int width,
	    height;
	int bpp;
	int option;
}	md_video_mode_t;

typedef struct
{
	int x,
	    y;
	int w,
	    h;
} md_video_rect_t;

typedef struct screen md_video_surface_t;

typedef	struct
{
	md_video_surface_t *screen;
	int w,
	    h;				/* 要求したサイズ */
}	md_video_t;

#define md_video_defaultopt() (0);



extern Uint32 md_tbl_pallete15[32 * 32 * 32];
#define	md_maprgb15(r, g, b) md_tbl_pallete15[(r) | (g) << 5 | (b) << 10]

#ifdef MD_BPP
#undef MD_BPP
#endif
#define MD_BPP 32

#ifndef MD_BPP
	typedef Uint32 md_pixel_t;
	extern  int md_video_bpp(void);
#define md_video_pixbytes(n) ((n) * md_video_bpp / 8)
#else
#define md_video_bpp() MD_BPP

#if MD_BPP == 2
	typedef Uint8 md_pixel_t;
#define md_video_pixbytes(n) ((n) >> 2)
#endif
#if	MD_BPP == 8
	typedef Uint8 md_pixel_t;
#define md_video_pixbytes(n) (n)
#endif
#if	MD_BPP == 16
	typedef Uint16 md_pixel_t;
#define md_video_pixbytes(n) ((n) << 1)
#endif
#if	MD_BPP == 32
	typedef Uint32 md_pixel_t;
#define md_video_pixbytes(n) ((n) << 2)
#endif
#endif


extern bool md_init(const char *title);
extern void md_fin(void);
extern void md_settitle(const char *title);

extern void md_video_fixmode(md_video_mode_t *mp);
extern bool md_video_setmode(int width, int height, md_video_mode_t *mp, const char *title);
extern Uint8 *md_video_lockline(int x, int y, int w, int h);
extern void md_video_unlockline(void);
extern int  md_video_pitch(void);

extern void md_video_update(int n, md_video_rect_t *rp);
extern void md_video_fill(int x, int y, int w, int h, Uint32 c);

extern void md_timer_settimer(int n, int (*proc)(int));
extern void md_timer_starttimer(void);
extern void md_timer_stoptimer(void);
extern int md_timer_getticks(void);

/* other */
/* audio */
extern bool md_audio_init(void (*func)(void *, Uint8 *, int),int n,int m);
extern void md_audio_pause(int n);
extern void md_audio_fin(void);
extern void md_audio_lock(void);
extern void md_audio_unlock(void);

typedef int md_mutex_t;
extern int md_mutex_new(void);
extern void md_mutex_lock(int n);
extern void md_mutex_unlock(int n);
extern void md_mutex_del(int n);

extern void md_event_init(void);
extern void md_event_set(int n, int m);		/* XXX */
extern int md_event_gets(int *p, int max);

#endif /* __MD_MGL2_H_ */
