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

#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include "../md.h"


md_video_t video;


Uint32 md_tbl_pallete15[32 * 32 * 32];

static int width,
	   height;
static int zoom_x,
	   zoom_y;
static int zmode;
static struct virtual_key *vkey;
static int *ps_pbuf;
static int ps_x,
	   ps_y,
	   ps_w,
	   ps_h,
	   ps_pp;
static char *md_title = NULL;

/* pixel 1line buffer */
static int *pixstream_buf;
static int *pixstream_zbuf;	/* zoom */

static struct timeval *start_time;

static int mgl_keys[] =
{
/* --    esc   1     2     3     4     5     6        -- 00-07 */
   00,   27,   49,   50,   51,   52,   53,   54,

/* 7     8     9     0     -     =     BS    TAB      -- 08-15 */
   55,   56,   57,   48,   45,   45,   8,    9, 

/* q     w     e     r     t     y     u     i        -- 16-23 */
   113,  119,  101,  114,  116,  121,  117,  105,

/* o     p     @     [     ENTR  CAPS  a     s        -- 24-31 */
   111,  112,  64,   91,   13,   301,  97,   115,

/* d     f     g     h     j     k     l     ;        -- 32-39 */
   100,  102,  103,  104,  106,  107,  108,  59,

/* :     Z/H   SHFT  ]     z     x     c     v        -- 40-47 */
   58,   -1,   304,  93,   122,  120,  99,   118,

/* b     n     m     ,     .     /     SHFTT MULT     -- 48-55 */
   98,   110,  109,  44,   46,   47,   303,  -1,

/* ALT   SPCE  CTRL  F1    F2    F3    F4    F5       -- 56-63 */
   308,  32,   306,  282,  283,  284,  285,  286,

/* F6    F7    F8    F9    F10   NLCK  SLCK  KP7      -- 64-71 */
   287,  288,  289,  290,  291,  300,  302,  263,

/* KP8   KP9   KP-   KP4   KP5   KP6   KP+   KP1      -- 72-79 */
   264,  265,  269,  260,  261,  262,  270,  257,

/* KP2   KP3   KP0   --    --    --    --    --       -- 80-87 */
   258,  259,  256,  -1,   -1,   -1,   -1,   -1,

/* --    --    --    --    --    --    --    UP       -- 88-95 */
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   273,

/* PGUP  LEFT  RGHT  END   DOWN  PGDN  INS   DEL      -- 96-103 */
   280,  276,  275,  279,  274,  281,  277,  127,

/* PAUS  MENU  RMEN  SELT  --    --    --    --       -- 104-111 */
   19,   319,  319,  -1,   -1,   -1,   -1,   -1,

/* KANA  --    _     --    --    --    --    --       -- 112-119 */
   -1,   -1,   95,   -1,   -1,   -1,   -1,   -1,

/* --    --    --    NFER  --    |     --    --       -- 120-127 */
   -1,   -1,   -1,   -1,   -1,   94,   -1,   -1
};

static int joystate = 0;

static char *icon_zodiac="\
#MGR000200160016
.@@@@@@@@@.@@@@@
@@@@@@@@@@@@@@@@
@@@@@q@@@@@@@q@@
@@.@@@@@@.@@@@@@
@@@@@@@@@@@@@@@@
@@@@.@@@@@@@.@@@
@@@@@@@@.@@@@@@@
Z@Z@Z@Z@Z@Z@Z@Z@
@Z@Z@Z@Z@Z@Z@Z@Z
Z@ZSZ@Z@Z@Z@Z@ZS
ZZZZZZZZZZZZZZZZ
ZZZZZZZZZZUZZZZZ
ZZZZZZZZZZZZZZZZ
ZTZZZZZZZZZZZZZZ
ZZZZZZTZZZZZZZTZ
ZZZZZZZZZZZZZZZZ
";

static enum zmode_val
{
	MGL2_ZMODE_NORMAL = 0,
	MGL2_ZMODE_HPC,
	MGL2_ZMODE_VGA
};


static void put_refresh_line(void);
static inline void md_video_fillrect(md_video_surface_t *scr, md_video_rect_t *r, int color);




void md_video_init(void)
{
	video.screen = NULL;
}


void md_video_fixmode(md_video_mode_t *mp)
{
	width = mp->width;
	height = mp->height;
	zoom_x = zoom_y = 1;
	zmode = MGL2_ZMODE_NORMAL;
}


bool md_video_setmode(int width, int height, md_video_mode_t *mp, const char *title)
{
	int i;

#if 0
fprintf(stderr, "video: set video mode %d x %d (%d x %d).\n", width, height, mp->width, mp->height);
#endif

	SCREEN_WIDTH = MGL2_MAX_WIDTH;
	SCREEN_HEIGHT = MGL2_MAX_HEIGHT;

	if (!open_graph())
	{
		fprintf(stderr, "fail: md_video_setmode:open_graph");
		return FALSE;
	}
	set_color(COLOR_BLACK);
	clear_screen();

	/*
		set raw_key mode
	*/
	mgl_set_keymode(MGL_SK_RAW);

	/*
	  set virtual key to MGL2's screen (touch screen or mouse)
	*/
#if 0
	vkey = create_virtual_key3(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, MK_V1, MK_V2, MK_V3);
	if (vkey == NULL)
	{
		fprintf(stderr, "fail: md_video_setmode:create_virtual_key3");
		return FALSE;
	}
	vk_attach(NULL, vkey);
#endif

	if (md_title != NULL)
		set_icon(icon_zodiac, md_title);

	/*
	  XXX

	  これはあんまりだなぁ...
	*/
	video.w = (width - 256) / 2;
	video.h = (height - 212) / 2;


	for (i = 0; i < 32 * 32 * 32; i++)
	{
		md_tbl_pallete15[i] = mc_from_rgb(packRGB((i & 0x1e) >> 1, (i & 0x3c0) >> 6, (i & 0x7800) >> 11));

#ifdef MGL2_USE_DITHER
		md_tbl_pallete15[i] |= COLOR_DITHER;
#endif /* MGL2_USE_DITHER */

	}

/*
== SDL's RGB bit mapping.
	0000 0000 1111 1000 0x00f7		RGB level values(8bit width)

	0000 0000 0001 1111 0x001f <<3		R
	0000 0011 1110 0000 0x03e0 >>2		G
	0111 1100 0000 0000 0x7b00 >>7		B


== MGL2's RGB bit mapping.
	0000 0000 0000 1111 0x000f		RGB level values(4bit width)
	0000 1111 0000 0000 0x0f00 >>8		R
	0000 0000 1111 0000 0x00f0 >>4		G
	0000 0000 0000 1111 0x000f		B

== SDL -> MGL2
	0000 0000 0001 1111 0x001e >>1		R
	0000 0011 1110 0000 0x03c0 >>6		G
	0111 1100 0000 0000 0x7800 >>11		B
*/

	pixstream_buf = NULL;
	pixstream_buf = (int *) malloc(width * height * sizeof(int));
	if (pixstream_buf == NULL)
	{
		md_fin();
		return FALSE;
	}

	pixstream_zbuf = NULL;
	pixstream_zbuf = (int *) malloc(width * 2 * sizeof(int));
	if (pixstream_zbuf == NULL)
	{
		md_fin();
		return FALSE;
	}
	return TRUE;
}


void md_settitle(const char *title)
{
	if (!md_title)
		free(md_title);

	if (title)
	{
		md_title = (char *) malloc(strlen(title) + 1);
		if (md_title)
			strcpy(md_title, title);

		if (video.screen)
			set_icon(icon_zodiac, (char *)title);
		return;
	}
	md_title = NULL;
}



void md_video_fill(int x, int y, int w, int h, Uint32 c)
{
	md_video_rect_t	rect;

	rect.x = 0;
	rect.w = width * zoom_x;
	rect.y = 0;
	rect.h = height * zoom_y;

	md_video_fillrect(video.screen, &rect, c);
}


Uint8 *md_video_lockline(int x, int y, int w, int h)
{
	ps_x = x * zoom_x;
	ps_y = y * zoom_y;
	ps_w = w;
	ps_h = h;
	ps_pp = (width - w) * zoom_x;

	ps_pbuf = pixstream_buf + (y + video.h) * w + (x + video.w);
	return (Uint8 *) ps_pbuf;
}


void md_video_unlockline(void)
{
	put_refresh_line();
	return;
}


void md_video_update(int n, md_video_rect_t *rp)
{
	return;
}


int md_video_pitch(void)
{
	return md_video_pixbytes(width);
}


/* md_joy */
void md_joystick_init(void)
{
	joystate = 0;
}

int md_joystick_getall(int N)
{
	return joystate;
}


int md_mouse(Uint8 N)
{
	return 0;
}


bool md_init(const char *title)
{
	atexit(md_fin);

	md_video_init();
	md_joystick_init();
	md_settitle(title);

	return TRUE;
}

void md_fin(void)
{
	if (pixstream_buf)
		free(pixstream_buf);

	if (pixstream_zbuf)
		free(pixstream_zbuf);

#if 0
	if (vkey)
		vk_detach(vkey, 1);
#endif

	close_graph();
	return;
}


static void put_refresh_line(void)
{
	int j, xz;
	int *src, *dst;

	if (zmode == MGL2_ZMODE_NORMAL)
	{
		put_pixstream(ps_x, ps_y, (int *)ps_pbuf, ps_w, DIR_NORTH);
		return;
	}

	xz = zoom_x * ps_w;
	src = ps_pbuf;
	dst = pixstream_zbuf;
#ifndef MGL2_OPT_UNROLL
	for (j = 0; j < ps_w; j++)
	{
		*dst++ = *src;
		*dst++ = *src;
		src++;
	}
#else
	/*
	  XXX

	  w が 8pix 単位でくることを前提。そうでないとマズイ。
	*/
	for (j = 0; j < ps_w; j += 8)
	{
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[1];
		dst[3] = src[1];
		dst[4] = src[2];
		dst[5] = src[2];
		dst[6] = src[3];
		dst[7] = src[3];
		dst[8] = src[4];
		dst[9] = src[4];
		dst[10] = src[5];
		dst[11] = src[5];
		dst[12] = src[6];
		dst[13] = src[6];
		dst[14] = src[7];
		dst[15] = src[7];
		src += 8;
		dst += 16;
	}
#endif  /* MGL2_OPT_UNROLL */
	put_pixstream(ps_x, ps_y, (int *)pixstream_zbuf, xz, DIR_NORTH);
}


static inline void md_video_fillrect(md_video_surface_t *scr, md_video_rect_t *r, int col)
{
	set_color(col);
	fill_rect(r->x, r->y, r->w, r->h);
}



static int (*md_timer_proc)(int) = NULL;
static struct itimerval md_timer;

static RETSIGTYPE md_timer_itimerproc(int n)
{
	if (md_timer_proc == NULL)
		return;

/* 	md_timer_proc();  XXX */
}

void md_timer_settimer(int n, int (*proc)(int))
{
	signal(SIGALRM, md_timer_itimerproc);
	md_timer_proc = proc;

	md_timer.it_interval.tv_sec = md_timer.it_value.tv_sec = n / 1000;
	md_timer.it_interval.tv_usec = md_timer.it_value.tv_usec = (n % 1000) * 1000;
}

void md_timer_starttimer(void)
{
	setitimer(ITIMER_REAL, &md_timer, NULL);
}

void md_timer_stoptimer(void)
{
	struct itimerval timer;

	memset(&timer, 0, sizeof(timer));
	setitimer(ITIMER_REAL, &timer, NULL);
}


static void md_timer_startticks(void)
{
	start_time = malloc(sizeof(struct timeval));

	gettimeofday(start_time, NULL);
}

int md_timer_getticks(void)
{
	struct timeval now_time;

	if (!start_time)
		md_timer_startticks();

	gettimeofday(&now_time, NULL);
	return (now_time.tv_sec - start_time->tv_sec) * 1000 +
	       (now_time.tv_usec - start_time->tv_usec) / 1000;
}


bool md_audio_init(void (*func)(void *, Uint8 *, int),int n,int m)
{
	return 0;
}

void md_audio_pause(int n)
{
	return;
}

void md_audio_fin(void)
{
	return;
}
 
void md_audio_lock(void)
{
	return;
}

void md_audio_unlock(void)
{
	return;
}

static int conv[500];
static int events[500];		/* XXX key はこの方法をやめる */

void md_event_init(void)
{
	memset(conv, 0xff, sizeof(conv));
	memset(events, 0xff, sizeof(events));
}

void md_event_set(int n, int m)
{
	if (n < sizeof(conv) / sizeof(int))
		conv[n] = m;
}

int md_event_gets(int *p, int max)
{
	int key, i;

	for (;;)
	{
		key = get_key(0);
		if (key == -1)
			break;

		if (key & 0x80)
		{
			/* KEY UP */
			key &= 0x7f;

			/* joy stick */
			switch (key)
			{
			case MGL_KEY_SPACE:
				joystate &= 0xdf;
				break;
			case MGL_KEY_ALT:
				joystate &= 0xef;
				break;
			case MGL_KEY_UP:
				joystate &= 0xce;
				break;
			case MGL_KEY_DOWN:
				joystate &= 0xcd;
				break;
			case MGL_KEY_LEFT:
				joystate &= 0xcb;
				break;
			case MGL_KEY_RIGHT:
				joystate &= 0xc7;
				break;
			}

			events[mgl_keys[key]] = -1;
		} else 
		{
			/*  KEY DOWN */
			switch(key)
			{
			case MGL_KEY_F8:
			case MGL_KEY_ESC:
				events[QUIT_KEY] = conv[QUIT_KEY];
				break;

			case MGL_KEY_F6:
				/* 
				   zoom (on the fly)
				*/
				zmode++;
				if (zmode > MGL2_ZMODE_VGA)
					zmode = MGL2_ZMODE_NORMAL;

				zoom_x = (zmode != MGL2_ZMODE_NORMAL && SCREEN_WIDTH > 240) ? (2) : (1);
				zoom_y = (zmode == MGL2_ZMODE_VGA  && SCREEN_HEIGHT > 270) ? (2) : (1);
				set_color(COLOR_BLACK);
				clear_screen();
				break;

			/* joy stick */
			case MGL_KEY_SPACE:
				joystate |= 0x10;
				break;
			case MGL_KEY_ALT:
				joystate |= 0x20;
				break;
			case MGL_KEY_UP:
				joystate |= 0x01;
				break;
			case MGL_KEY_DOWN:
				joystate |= 0x02;
				break;
			case MGL_KEY_LEFT:
				joystate |= 0x04;
				break;
			case MGL_KEY_RIGHT:
				joystate |= 0x08;
				break;

			}
			key = mgl_keys[key];
			events[key] = conv[key];
		}
	}

	key = 0;
	for (i = 0; i < sizeof(events) / sizeof(int); ++i)
	{
		if (events[i] == -1)
			continue;
		if (key >= max)
			break;

		p[key++] = events[i];
	}
	return key;
}


int md_mutex_new(void)
{
	return 0;
}

void md_mutex_lock(int n)
{
	return;
}

void md_mutex_unlock(int n)
{
	return;
}

void md_mutex_del(int n)
{
	return;
}
