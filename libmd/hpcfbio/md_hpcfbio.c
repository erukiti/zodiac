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

#include "../md.h"


md_video_t video;


Uint32 md_tbl_pallete15[32 * 32 * 32];

static int width,
	   height,
	   fb_width,
	   fb_height;
static char *fb;
static int *ps_pbuf;
static int ps_x,
	   ps_y,
	   ps_w,
	   ps_h,
	   ps_pp;
static int real_depth;
static int fake_depth;
static char *platform = NULL;
static int rowbytes;
static int fb_fd = -1;
static int fb_off = 0;
static int mouse_fd = -1;
static int mouse_x = 0;
static int mouse_y = 0;
static unsigned char red[256], green[256], blue[256];
static struct termios save_tty, cur_tty;
static int *pixbuf;

/* handler */
static int (*video_pixbytes_hpcfb)(int n);
static Uint8 *(*video_lockline_hpcfb)(int x, int y, int w, int h);
static void (*clear_hpcfb)(int col);
static void (*put_refresh_line_hpcfb)(void);


struct _platinfo {
	int depth;
	int rowbyte;
	int off;
	char name[20];
} platinfo[] = {
	{ 2, 80, 0,  "FreeStyle" },
	{ 2, 160, 0,  "MC-R32" },
	{ 2, 256, 0,  "MC-R3" },
	{ 2, 256, 0,  "MC-CS" },
	{ 2, 256, 0,  "MobileGearII for Do" },
	{ 2, 256, 0,  "Cassiopeia E55" },
	{ 2, 256, 0,  "Cassiopeia for DoCo" },
	{ 2, 256, 0,  "E-55" },
	{ 8, 1024, 0,  "MC-R500" },
	{ 8, 1024, 0,  "MC-R510" },
	{ 8, 640, 0,  "INTERTOP" },
	{ 16, 1280, 0, "sigmarion" }, 
	{ 16, 1280, 256, "MC-R530" }, 
	{ 16, 1280, 256, "MC-R430" }, 
	{ 16, 1600, 0, "MC-R5" }, 
	{ 16, 1600, 1536, "MC-R730" }, 
	{ 16, 1600, 0, "MC-R700" }, 
	{ 16, 512, 0,  "Cassiopeia E100" },
	{ 16, 512, 0,  "E-100" },
	{ 16, 512, 0,  "Cassiopeia E500" },
	{ 16, 512, 0,  "E-500" },
	{ 16, 1280, 0,  "WorkPad" },
	{ 16, 1280, 0,  "z50" },
	{ 0, 0, 0, "" }
};

static struct timeval *start_time;

static int pckbd_keys[] =
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

/* :     Z/H   SHFTL ]     z     x     c     v        -- 40-47 */
   58,   -1,   304,  93,   122,  120,  99,   118,

/* b     n     m     ,     .     /     SHFTR MULT     -- 48-55 */
   98,   110,  109,  44,   46,   47,   303,  -1,

/* ALT   SPCE  CTRL  F1    F2    F3    F4    F5       -- 56-63 */
   308,  32,   306,  282,  283,  284,  285,  286,

/* F6    F7    F8    F9    F10   NLCK  SLCK  KP7      -- 64-71 */
   287,  288,  289,  290,  291,  300,  302,  263,

/* UP    PGUP  KP-   LEFT  KP5   RIGT  KP+   KP1      -- 72-79 */
   273,  280,  269,  276,  261,  275,  270,  257,

/* DOWN  PGDN  KP0   --    --    --    --    F11      -- 80-87 */
   274,  281,  256,  -1,   -1,   -1,   -1,   -1,

/* F12   --    --    --    --    --    --    --       -- 88-95 */
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,

/* --    --    --    --    --    --    --    --       -- 96-103 */
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,

/* --    --    --    --    --    --    --    --       -- 104-111 */
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,

/* KANA  --    --    _     --    --    --    --       -- 112-119 */
   -1,   -1,   -1,   95,   -1,   -1,   -1,   -1,

/* --    XFER  --    NFER  --    |     --    PAUS     -- 120-127 */
   -1,   -1,   -1,   -1,   -1,   94,   -1,   19,

/* --    --    --    --    --    --    --    --       -- 128-135 */
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,

/* --    --    --    --    --    --    --    --       -- 136-143 */
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,

/* --    --    --    --    --    --    --    --       -- 144-151 */
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,

/* --    --    --    --    --    --    --    --       -- 152-159 */
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,

/* --    --    --    --    --    --    --    --       -- 160-167 */
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,

/* --    --    --    --    --    --    --    --       -- 168-175 */
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,

/* --    --    --    --    --    --    --    --       -- 176-183 */
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,

/* --    --    --    --    --    --    --    --       -- 184-191 */
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,

/* --    --    --    --    --    --    --    HOME     -- 192-199 */
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,

/* UP    PGUP  --    LEFT  --    RGHT  --    END      -- 200-207 */
   273,  280,  -1,   276,  -1,   275,  -1,   279,

/* DOWN  PGDN  INS   DEL   --    --    --    --       -- 208-215 */
   274,  281,  277,  127,  -1,   -1,   -1,   -1,

/* --    --    --    METAL METAR MENU  --    --       -- 216-223 */
   -1,   -1,   -1,   -1,   -1,   319,  -1,   -1,
};

static int joystate = 0;


static int alloc_pixbuf(int w, int h);
static void free_pixbuf(void);

static Uint8 *video_lockline_hpcfb2(int x, int y, int w, int h);
static Uint8 *video_lockline_hpcfb8(int x, int y, int w, int h);
static Uint8 *video_lockline_hpcfb16(int x, int y, int w, int h);

static void clear_hpcfb2(int col);
static void clear_hpcfb8(int col);
static void clear_hpcfb16(int col);

static void put_refresh_line_hpcfb2(void);
static void put_refresh_line_hpcfb8(void);
static void put_refresh_line_hpcfb16(void);

static int video_pixbytes_hpcfb2(int n);
static int video_pixbytes_hpcfb8(int n);
static int video_pixbytes_hpcfb16(int n);
static int video_pixbytes_hpcfb32(int n);

static inline void md_video_fillrect(md_video_surface_t *scr, md_video_rect_t *r, int color);

static int open_mouse(void);
static void close_mouse(void);
static void mouse_event(void);
static void get_mouse_values(int *x, int *y, int *btn);
static void text_mode(void);
static void graph_mode(void);
static struct _platinfo *platmatch(char *name, int depth);
static void getplatform(void);
static int fbsetinfo_hpcfb(struct hpcfb_fbconf *fbconf);
static int fbsetinfo(struct wsdisplay_fbinfo *fbinfo);
static void init_tty(void);
static void reset_tty(void);
static int kbmode_raw(int fd);
static int kbmode_normal(int fd);
static int get_key(void);
static inline int power2(int v);
static int get_color(unsigned char r, unsigned char g, unsigned char b);



void md_video_init(void)
{
	video.screen = NULL;
}


void md_video_fixmode(md_video_mode_t *mp)
{
	width = mp->width;
	height = mp->height;
}


bool md_video_setmode(int width, int height, md_video_mode_t *mp, const char *title)
{
	int i;
	int fbsize, dispmode;
	struct wsdisplay_fbinfo fbinfo;
	struct hpcfb_fbconf hpcfbconf;
	struct wsdisplay_cmap cmap;

#if 0
fprintf(stderr, "video: set video mode %d x %d (%d x %d).\n", width, height, mp->width, mp->height);
#endif

	getplatform();

	if ((fb_fd = open(FBDEV, O_RDWR)) < 0)
	{
		perror(FBDEV);
		return FALSE;
	}

	hpcfbconf.hf_conf_index = HPCFB_CURRENT_CONFIG;
	if (ioctl(fb_fd, HPCFBIO_GCONF, &hpcfbconf) == 0)
	{
		printf("md_hpcfbio init HPCFBIO_GCONF\n");
		fbsize = fbsetinfo_hpcfb(&hpcfbconf);
	} 
	else
	{
		printf("md_hpcfbio init HPCFBIO_GCONF HPCFB_CURRENT_CONFIG fail\n");
		if (ioctl(fb_fd, WSDISPLAYIO_GINFO, &fbinfo) < 0)
		{
			perror("ioctl(WSDISPLAYIO_GINFO)");
			return FALSE;
		}
		printf("md_hpcfbio init GINFO\n");
		fbsize = fbsetinfo(&fbinfo);
	}

	dispmode = WSDISPLAYIO_MODE_MAPPED;
	if (ioctl(fb_fd, WSDISPLAYIO_SMODE, &dispmode) < 0)
	{
		perror("ioctl(WSDISPLAYIO_SMODE) WSDISPLAYIO_MODE_MAPPED");
		return FALSE;
	}

	fb = mmap(NULL, fbsize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
	if (fb == MAP_FAILED)
	{
		text_mode();
		perror("mmap()");
		return FALSE;
	}

	if (fb_off)
		fb += fb_off;

	fake_depth = real_depth;
	switch (real_depth)
	{
	case 2:
		video_lockline_hpcfb = video_lockline_hpcfb2;
		clear_hpcfb = clear_hpcfb2;
		put_refresh_line_hpcfb = put_refresh_line_hpcfb2;
#ifdef USE_PIXBUF
		fake_depth = 32;
		video_pixbytes_hpcfb = video_pixbytes_hpcfb32;
#else
		video_pixbytes_hpcfb = video_pixbytes_hpcfb2;
#endif /* USE_PIXBUF */

		break;
	case 8:
		video_lockline_hpcfb = video_lockline_hpcfb8;
		clear_hpcfb = clear_hpcfb8;
		put_refresh_line_hpcfb = put_refresh_line_hpcfb8;
		video_pixbytes_hpcfb = video_pixbytes_hpcfb8;

		cmap.index = 0;
		cmap.count = 256;
		cmap.red = red;
		cmap.green = green;
		cmap.blue = blue;
		if (ioctl(fb_fd, WSDISPLAYIO_GETCMAP, &cmap) < 0)
		{
			text_mode();
			perror("GETCMAP");
			return FALSE;
		}
		break;
	case 16:
		video_lockline_hpcfb = video_lockline_hpcfb16;
		clear_hpcfb = clear_hpcfb16;
		put_refresh_line_hpcfb = put_refresh_line_hpcfb16;
		video_pixbytes_hpcfb = video_pixbytes_hpcfb16;
		break;
	}

	/*
	  XXX

	  これはあんまりだなぁ...
	*/
	video.w = (width - 256) / 2;
	video.h = (height - 212) / 2;


	for (i = 0; i < 32 * 32 * 32; i++)
	{
		switch (real_depth)
		{
		case 2:
			md_tbl_pallete15[i] = get_color((i & 0x1f) << 3, (i & 0x3e0) >> 2, (i & 0x7b00) >> 7);
			break;
		case 8:
			md_tbl_pallete15[i] = get_color((i & 0x1f) << 3, (i & 0x3e0) >> 2, (i & 0x7b00) >> 7);
			break;
		case 16:
			/* not implemented yet */
			break;
		}

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

#ifdef USE_PIXBUF
	if (alloc_pixbuf(width, height) == FALSE)
	{
		text_mode();
		perror("alloc: pixbuf");
		return FALSE;
	}
#endif /* USE_PIXBUF */

	kbmode_raw(0);
	init_tty();
	(*clear_hpcfb)(0);
	return TRUE;
}


void md_settitle(const char *title)
{
	return;
}



void md_video_fill(int x, int y, int w, int h, Uint32 c)
{
	md_video_rect_t	rect;

	rect.x = x;
	rect.w = w;
	rect.y = y;
	rect.h = h;

	md_video_fillrect(video.screen, &rect, c);
}


Uint8 *md_video_lockline(int x, int y, int w, int h)
{
	return (Uint8 *) (*video_lockline_hpcfb)(x, y, w, h);
}


void md_video_unlockline(void)
{
	(*put_refresh_line_hpcfb)();
	return;
}


void md_video_update(int n, md_video_rect_t *rp)
{
	return;
}


int md_video_pitch(void)
{
	return (*video_pixbytes_hpcfb)(width);
}

int md_video_bpp(void)
{
	return fake_depth;
}

int md_video_pixbytes(int n)
{
	return (*video_pixbytes_hpcfb)(n);
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
#ifdef USE_PIXBUF
	free_pixbuf();
#endif /* USE_PIXBUF */
	kbmode_normal(0);
	reset_tty();
	text_mode();
	return;
}

static inline void md_video_fillrect(md_video_surface_t *scr, md_video_rect_t *r, int col)
{
#if 0
	char *ptr;
	int bw, bh, x, y;

	ptr = fb;
	bw = (r->w - 256) / 2;
	bh = (r->h - 192) / 2;
	for (y = 0; y < bh; y++)
	{
		memset(ptr, col, (size_t) width + bw * 2);
		ptr += rowbytes;
	}

	ptr = fb + rowbytes * (bh  + height);
	for (y = 0; y < bh; y++)
	{
		memset(ptr, col, (size_t) width + bw * 2);
		ptr += rowbytes;
	}
#endif
	return;
}



static int (*md_timer_proc)(int) = NULL;
static struct itimerval md_timer;

static RETSIGTYPE md_timer_itimerproc(int n)
{
	if (md_timer_proc == NULL)
		return;

/*	md_timer_proc(0); XXX */
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
		key = get_key();
		if (key == 0)
			break;

		if (key & 0x80)
		{
			/* KEY UP */
			key &= 0x7f;

			/* joystick */
			switch (key)
			{
			case MGK_ALT:
				joystate &= 0xdf;
				break;
			case MGK_SPACE:
				joystate &= 0xef;
				break;
			case MGK_UP:
				joystate &= 0xce;
				break;
			case MGK_DOWN:
				joystate &= 0xcd;
				break;
			case MGK_LEFT:
				joystate &= 0xcb;
				break;
			case MGK_RIGHT:
				joystate &= 0xc7;
				break;
			}

			events[pckbd_keys[key]] = -1;
		} else 
		{
			/*  KEY DOWN */
			switch (key)
			{
			case MGK_F8:
			case MGK_ESC:
				events[QUIT_KEY] = conv[QUIT_KEY];
				break;

			/* joystick */
			case MGK_ALT:
				joystate |= 0x10;
				break;
			case MGK_SPACE:
				joystate |= 0x20;
				break;
			case MGK_UP:
				joystate |= 0x01;
				break;
			case MGK_DOWN:
				joystate |= 0x02;
				break;
			case MGK_LEFT:
				joystate |= 0x04;
				break;
			case MGK_RIGHT:
				joystate |= 0x08;
				break;
			}

			key = pckbd_keys[key];
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


static int alloc_pixbuf(int w, int h)
{
	pixbuf = (int *) malloc(w * h * sizeof(int));
	if (!pixbuf)
		return FALSE;

	return TRUE;
}

static void free_pixbuf(void)
{
	if (pixbuf)
		free(pixbuf);
}

static Uint8 *video_lockline_hpcfb2(int x, int y, int w, int h)
{
	ps_x = x;
	ps_y = y;

#ifdef USE_PIXBUF
	/* return local pixbuf's address */
	ps_w = w;
	ps_h = h;
	ps_pp = width - w;
	ps_pbuf = pixbuf + (y + video.h) * w + (x + video.w);
	return (Uint8 *) ps_pbuf;
#else
	/* return frame buffer's address */
	ps_w = width;
	ps_h = height;
	ps_pp = rowbytes;
	return (Uint8 *) fb + (y + video.h) * rowbytes + ((x + video.w) >> 2);
#endif /* USE_PIXBUF */
}

static Uint8 *video_lockline_hpcfb8(int x, int y, int w, int h)
{
	/* return frame buffer's address */
	ps_x = x;
	ps_y = y;
	ps_w = width;
	ps_h = height;
	ps_pp = rowbytes;
	return (Uint8 *) fb + (y + video.h) * rowbytes + (x + video.w);
}

static Uint8 *video_lockline_hpcfb16(int x, int y, int w, int h)
{
	/* not implimented yet. */
	return (Uint8 *) NULL;
}

static void clear_hpcfb2(int col)
{
	char *ptr;
	int y, pcol;

	pcol = (col & 0x03) | (col << 2) | (col << 4) | (col << 6);
	ptr = fb;
	for (y = 0; y < fb_height; y++)
	{
		memset(ptr, pcol, (size_t) (fb_width / 4));
		ptr += rowbytes;
	}
}

static void clear_hpcfb8(int col)
{
	char *ptr;
	int y;

	ptr = fb;
	for (y = 0; y < fb_height; y++)
	{
		memset(ptr, col, (size_t) fb_width);
		ptr += rowbytes;
	}
}

static void clear_hpcfb16(int col)
{
	int *ptr;
	int x, y;

	for (y = 0; y < fb_height; y++)
	{
		ptr = (int *) fb + rowbytes * y;
		for (x = 0; x < fb_width * 2; x++)
		{
			*ptr++ = col;
		}
		ptr += rowbytes;
	}
}

static void put_refresh_line_hpcfb2(void)
{
#ifdef USE_PIXBUF
	char *dstptr;
	int *srcptr;
	int i;
	char pix;

	/* indirect draw via pixbuf */

	dstptr = fb + rowbytes * ps_y + (ps_x >> 2);
	srcptr = ps_pbuf;

	for (i = 0; i < (ps_w >> 2); i++)
	{
#ifdef MD_LITTLE
		pix = (*srcptr++ << 6) & 0xc0;
		pix |= (*srcptr++ << 4) & 0x30;
		pix |= (*srcptr++ << 2) & 0x0c;
		pix |= *srcptr++ & 0x03;
#else
		pix = *srcptr++ & 0x03;
		pix |= (*srcptr++ << 2) & 0x0c;
		pix |= (*srcptr++ << 4) & 0x30;
		pix |= (*srcptr++ << 6) & 0xc0;
#endif /* MD_LITTLE */
		*dstptr++ = pix;
	}
#endif /* USE_PIXBUF */

	/* direct draw by v99x8_refresh.c */
}

static void put_refresh_line_hpcfb8(void)
{
	/* direct draw by v99x8_refresh.c */
}

static void put_refresh_line_hpcfb16(void)
{
	/* not implimented yet. */
}

static int video_pixbytes_hpcfb2(int n)
{
	return n >> 2;
}

static int video_pixbytes_hpcfb8(int n)
{
	return n;
}

static int video_pixbytes_hpcfb16(int n)
{
	return n << 1;
}

static int video_pixbytes_hpcfb32(int n)
{
	return n << 2;
}

static int open_mouse(void)
{
	if ((mouse_fd = open(MOUSEDEV, O_RDWR )) < 0)
		return -1;

	return 0;
}

static void close_mouse(void)
{
	if (mouse_fd == -1)
		return;
	close(mouse_fd);
	mouse_fd = -1;
}

static void mouse_event(void)
{
	struct wscons_event ev[16];
	fd_set fds;
	struct timeval to;
	int i, n, r;

	if (mouse_fd < 0) 
		return;

	FD_ZERO(&fds);
	FD_SET(mouse_fd,&fds);
	to.tv_sec = 0;
	to.tv_usec = 0;
	if (select(mouse_fd+1,&fds,0,0,&to) != 1)
		return;

	r = read(mouse_fd,ev,sizeof(ev));
	if (r < 0)
	{
		close(mouse_fd);
		mouse_fd = -1;
		return;
	}
	n = r/sizeof(ev[0]);
/* printf("event count = %d \n",n); */
	for (i=0; i<n; i++)
	{
		switch (ev[i].type)
		{
		case WSCONS_EVENT_MOUSE_DELTA_X:
			mouse_x += ev[i].value;
/* fprintf(stderr, "MOUSE_DELTAX %d\n",mouse_x); */
			break;

		case WSCONS_EVENT_MOUSE_DELTA_Y:
			mouse_y += ev[i].value;
/* fprintf(stderr, "MOUSE_DELTAY %d\n",mouse_y); */
			break;

		case WSCONS_EVENT_MOUSE_ABSOLUTE_X:
			mouse_x = ev[i].value;
/* fprintf(stderr, "MOUSE_ABSX %d\n",mouse_x); */
			break;

		case WSCONS_EVENT_MOUSE_ABSOLUTE_Y:
			mouse_y = ev[i].value;
/* fprintf(stderr, "MOUSE_ABSY %d\n",mouse_y); */
			break;
		}
	}
}

static void get_mouse_values(int *x, int *y, int *btn)
{
	*x = mouse_x;
	*y = mouse_y;
	*btn = 0;
}

static void text_mode(void)
{
	int dispmode;

	dispmode = WSDISPLAYIO_MODE_EMUL;
	if (ioctl(fb_fd, WSDISPLAYIO_SMODE, &dispmode) < 0)
	{
		perror("ioctl(WSDISPLAYIO_SMODE) WSDISPLAYIO_MODE_EMUL");
	}
}

static void graph_mode(void)
{
	int dispmode;

	dispmode = WSDISPLAYIO_MODE_MAPPED;
	if (ioctl(fb_fd, WSDISPLAYIO_SMODE, &dispmode) < 0)
	{
		perror("ioctl(WSDISPLAYIO_SMODE) WSDISPLAYIO_MODE_EMUL");
	}
}

static struct _platinfo *platmatch(char *name, int depth)
{
	struct _platinfo *p;

	p = &platinfo[0];
	while (p->depth)
	{
		if (p->depth == depth 
			&& strncmp(name, p->name, strlen(p->name)) == 0)
		{
			return p;
		}
		p++;
	}
	return NULL;
}

static void getplatform(void)
{
	char *ep,*cp;
	int mib[2];
	size_t len;

	platform = NULL;
	if ((ep = getenv("PLATFORM")) == NULL)
	{
		mib[0] = CTL_HW;
		mib[1] = HW_MODEL;
		sysctl(mib, 2, NULL, &len, NULL, 0);
		if ((cp = malloc(len)) != NULL)
		{
			sysctl(mib, 2, cp, &len, NULL, 0);
			while (*cp && *cp != ' ')       /* skip vender name */
				cp++;
			if (*cp)
			{
				cp++;
				platform = cp;
			}
		}
	} 
	else
	{
		platform = ep;
	}
	printf("md_hpcfbio getplatform: platform = %s \n", platform);
}

static int fbsetinfo_hpcfb(struct hpcfb_fbconf *fbconf)
{
	int fbsize = 0;

	printf("fbsetinfo_hpcfb: %dx%d (%dbytes/line) %dbit offset=0x%08lx\n",
		fbconf->hf_width,
		fbconf->hf_height,
		fbconf->hf_bytes_per_line,
		fbconf->hf_pixel_width,
		fbconf->hf_offset);

	fb_width = fbconf->hf_width;
	fb_height = fbconf->hf_height;

	switch (fbconf->hf_pixel_width)
	{
	case 2:
		real_depth = fbconf->hf_pixel_width;
		rowbytes = fbconf->hf_bytes_per_line;
		break;
	case 8:
		real_depth = fbconf->hf_pixel_width;
		rowbytes =fbconf->hf_bytes_per_line;
		break;
	case 16:
		real_depth = fbconf->hf_pixel_width;
		rowbytes = fbconf->hf_bytes_per_line;
		break;
	default:
		text_mode();
		printf("hpcmips fbsetinfo_hpcfb: unknown depth %d\n", fbconf->hf_bytes_per_line);	
		return -1;
		break;
	}

	fb_off = fbconf->hf_offset;

	printf("display: %dx%dx%d\n",
		fbconf->hf_width,
		fbconf->hf_height,
		fbconf->hf_pixel_width);

	fbsize = rowbytes * fbconf->hf_height;

	printf("fbsize: %d off:0x%08lx rowbytes:%d\n", fbsize, fb_off, rowbytes);

	if (fb_off)
		fbsize += fb_off;
	return fbsize;
}

static int fbsetinfo(struct wsdisplay_fbinfo *fbinfo)
{
	int fbsize = 0;
	struct _platinfo *p;

	fb_width = fbinfo->width;
	fb_height = fbinfo->height;
	real_depth = fbinfo->depth;
	switch (real_depth)
	{
	case 2:
		rowbytes = DEF_ROWBYTE2;
		break;
	case 8:
		rowbytes = DEF_ROWBYTE8;
		break;
	case 16:
		rowbytes = DEF_ROWBYTE16;
		break;
	default:
		text_mode();
		printf("hpcmips fbsetinfo: unknown depth %d\n", fbinfo->depth);
		return -1;
		break;
	}

	if (platform == NULL)
	{
		printf("hpcmips fbsetinfo: PLATFORM is not set; use default\n");
	} else if ((p = platmatch(platform, fbinfo->depth)) == NULL)
	{
		printf("hpcmips fbsetinfo: unknown platform %s\n", platform);   
		return -1;
	} else {
		rowbytes = p->rowbyte;
		fb_off = p->off;
	}
	printf("display: %dx%dx%d\n",
		fbinfo->width,
		fbinfo->height,
		fbinfo->depth);

	fbsize = rowbytes * fbinfo->height;

	printf("fbsize: %d off:%d rowbytes:%d\n", fbsize, fb_off, rowbytes);

	if (fb_off)
		fbsize += fb_off;
	return fbsize;
}

static void init_tty(void)
{
	tcgetattr(0, &save_tty);
	save_tty.c_iflag |= INLCR | ICRNL;
	save_tty.c_lflag |= ICANON | ECHO;
	save_tty.c_oflag |= OPOST | ONLCR;
	cur_tty = save_tty;

	cur_tty.c_iflag = IGNPAR | IGNBRK;
	cur_tty.c_oflag = ONLCR;
	cur_tty.c_cflag = CREAD | CS8;
	cur_tty.c_lflag &= ~(ICANON | IEXTEN | ECHO | ISIG);
	cur_tty.c_cc[VTIME] = 0;
	cur_tty.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &cur_tty);
}

static void reset_tty(void)
{
	tcsetattr(0, TCSANOW, &save_tty);
}

static int kbmode_raw(int fd)
{
	SET_KBMODE(fd, K_RAW);
}

static int kbmode_normal(int fd)
{
	SET_KBMODE(fd, K_XLATE);
}

static int get_key(void)
{
	fd_set fds;
	struct timeval to;
	int r;
	unsigned char dt;

	FD_ZERO(&fds);
	FD_SET(0,&fds);
	to.tv_sec = 0;
	to.tv_usec = 0;
	dt = 0;
	if (select(1,&fds,0,0,&to) != 1)
		return 0;
	r = read(0,&dt,sizeof(unsigned char));
	return (int)dt;
}

static inline int power2(int v)
{
	return v * v;
}


static int get_color(u_char r, u_char g, u_char b)
{
	int red16, green16, blue16;
	int i, index;
	int delta, odelta;

	index = 0;
	switch (real_depth)
	{
	case 16:
		red16 = r;
		green16 = g;
		blue16 = b;

#ifdef ORD_R5G5B5

		if (red16)
			red16 = red16 * 2 + 1;
		if (green16)
			green16 = green16 * 2 + 1;
		if (blue16)
			blue16 = blue16 * 2 + 1;

		index = ((red16 << 10) | (green16 << 5) | blue16);

#else

		if (red16)
			red16 = red16 * 2 + 1;
		if (green16)
			green16 = 0x3f + (green16 + 1) / 16;
		if (blue16)
			blue16 = blue16 * 2 + 1;

		index = ((red16 << 11) | (green16 << 5) | blue16);
		break;

#endif /* ORD_R5G5B5 */
	case 8:
		odelta = 65536;
		for (i = 0; i < 256; i++)
		{
			delta = (power2(red[i] - r))   +
			(power2(green[i] - g)) +
			(power2(blue[i] - b))  / 3;

			if (delta < odelta)
			{
				odelta = delta;
				index = i;
			if (delta == 0)
				break;
			}
		}
		break;

	case 2:
		index = ((r + g + b) / 3) / 64;
		break;
	}

	return index;
}
