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

/* mgl2/lib/md_hpcmips.h */
/*
 * MGL -- MobileGear Graphic Library -
 * Copyright (C) 1998, 1999, 2000, 2001
 *      Koji Suzuki (suz@at.sakura.ne.jp)
 *      Yukihiko Sano (yukihiko@yk.rim.or.jp)
 *
 *      Shin Takemura (takemura@netbsd.org)
 *      SATO Kazumi (sato@netbsd.org)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY KOJI SUZUKI AND YUKIHIKO SANO ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE TERRENCE R. LAMBERT BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef	__MD_HPCFBIO_H_
#define	__MD_HPCFBIO_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <termios.h>
#include <fcntl.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplay_usl_io.h>
#include <dev/hpc/hpcfbio.h>

#define DEF_ROWBYTE2 256   /* XX MC-R300 */
#define DEF_ROWBYTE8 1024  /* XX MC-R500 */
#define DEF_ROWBYTE16 1600 /* XX MC-R510 */
#define FBDEV "/dev/ttyE0"
#define MOUSEDEV "/dev/wsmouse0"

#define KEYBUF_SIZE     2048

#define SET_KBMODE(fd, mode)			\
{						\
	if (ioctl(fd, KDSKBMODE, mode) < 0)	\
	{					\
		perror("ioctl");		\
		return -1;			\
	}					\
	return 0;				\
}

/* #define USE_PIXBUF */
#define MGK_ESC 1
#define MGK_ENTER 28
#define MGK_ALT 56
#define MGK_SPACE 57
#define MGK_CAPSLOCK 58
#define MGK_F8 66
#define MGK_NUMLOCK 69
#define MGK_UP 72
#define MGK_LEFT 75
#define MGK_RIGHT 77
#define MGK_DOWN 80
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

typedef char md_video_surface_t;

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

#ifndef MD_BPP
	typedef Uint32 md_pixel_t;
	extern  int md_video_bpp(void);
/* #define md_video_pixbytes(n) ((n) * md_video_bpp / 8) */
	extern int md_video_pixbytes(int n);
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
#endif /* !MD_BPP */


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

#endif /* __MD_HPCFBIO_H_ */
