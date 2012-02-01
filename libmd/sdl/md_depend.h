/*
 * Copyright (c) 2000-2002 SASAKI Shunsuke (eruchan@users.sourceforge.net).
 * Copyright (c) 2001-2002 The Zodiac project.
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


#ifndef __MD_SDL_H_
#define __MD_SDL_H_

#include <SDL.h>

#if	!defined(WORDS_BIGENDIAN) && SDL_BYTEORDER != SDL_LIL_ENDIAN
#	define WORDS_BIGENDIAN
#endif



typedef struct
{
	int width, height;
	int bpp;
	int option;
}	md_video_mode_t;

typedef	SDL_Rect md_video_rect_t;
typedef	SDL_Surface md_video_surface_t;

typedef struct
{
	md_video_surface_t *screen;
	int w, h;					/* 要求したサイズ */
}	md_video_t;

// XXX
#define	md_video_defaultopt() (SDL_SWSURFACE)


extern Uint32 md_tbl_pallete15[32 * 32 * 32];
#define md_maprgb15(r, g, b) md_tbl_pallete15[(r) | (g) << 5 | (b) << 10]

extern bool md_init(const char *title);

extern void md_video_fixmode(md_video_mode_t *mp);
extern bool md_video_setmode(int width, int height, md_video_mode_t *mp, const char *title);

extern void md_settitle(const char *title);

extern Uint8 *md_video_lockline(int x, int y, int w, int h);
extern void md_video_unlockline(void);

extern int md_video_pitch(void);

#ifndef	MD_BPP
	typedef	Uint32 md_pixel_t;
	extern int md_video_bpp(void);

#else
#	define md_video_bpp() MD_BPP

#	if MD_BPP==8
		typedef Uint8 md_pixel_t;
#	endif
#	if	MD_BPP==16
		typedef Uint16 md_pixel_t;
#	endif
#	if	MD_BPP==32
		typedef Uint32 md_pixel_t;
#	endif
#endif

#define md_video_pixbytes(n) ((n) * md_video_bpp() / 8)




extern void md_video_update(int n, md_video_rect_t *rp);

extern void md_video_fill(int x, int y, int w, int h, Uint32 c);
#define md_video_fillrect SDL_FillRect

#define md_timer_getticks SDL_GetTicks

void md_timer_settimer(Uint32 n, Uint32 (*proc)(Uint32));
void md_timer_stoptimer(void);
void md_timer_starttimer(void);


extern bool md_audio_init(void (*func)(void *, Uint8 *, int),int n,int m);
#define md_audio_pause(n) SDL_PauseAudio(n)
#define md_audio_fin SDL_CloseAudio
#define md_audio_lock SDL_LockAudio
#define md_audio_unlock SDL_UnlockAudio

typedef SDL_mutex* md_mutex_t;
#define md_mutex_new SDL_CreateMutex
#define md_mutex_lock SDL_mutexP
#define md_mutex_unlock SDL_mutexV
#define md_mutex_del SDL_DestroyMutex


extern void md_event_init(void);
extern void md_event_set(int n, int m);	// XXX
extern int md_event_gets(int *p, int max);


#endif /* __MD_SDL_H_ */
