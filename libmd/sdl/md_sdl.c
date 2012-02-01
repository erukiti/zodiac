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

#include <stdlib.h>
#include <string.h>
#include "../md.h"

static char *md_title = NULL;


md_video_t video;

void md_video_init(void)
{
	video.screen = NULL;
}

void md_video_fixmode(md_video_mode_t *mp)
{
	int i;
	SDL_Rect **rect;

	rect = SDL_ListModes(NULL, mp->option);
	if (rect == NULL || rect == (void *)-1)
		return;
	i = 0;
	while (rect[i] != NULL)
		++i;

	while (i-- > 0)
	{
		if (rect[i]->w >= mp->width && rect[i]->h >= mp->height && rect[i]->w >= rect[i]->h)
			break;
	}
	mp->width  = rect[i]->w;
	mp->height = rect[i]->h;
}

Uint32 md_tbl_pallete15[32 * 32 * 32];

/* SDL_Surface *video_screen; */

bool md_video_setmode(int width, int height, md_video_mode_t *mp, const char *title)
{
	int i, j;

/* printf("video: set video mode %d x %d (%d x %d).\n", width, height, mp->width, mp->height); */
	video.screen = SDL_SetVideoMode(mp->width, mp->height, mp->bpp, mp->option);
	if (video.screen == NULL)
	{
		fprintf(stderr, "video: %s\n", SDL_GetError());
		return FALSE;
	}

#ifdef	MD_BPP
	if (video.screen->format->BitsPerPixel != MD_BPP)
	{
		fprintf(stderr, "video: bpp %d != %d", video.screen->format->BitsPerPixel, MD_BPP);
		return FALSE;
	}
#endif

	if (md_title != NULL)
		SDL_WM_SetCaption(md_title, NULL);

/* 	SDL_ShowCursor(FALSE); */

	if (video.screen->w < width)
	{
		fprintf(stderr, "video: width(%d/%d) is too small\n",video.screen->w, width);
		return FALSE;
	}
	video.w = (video.screen->w - width) / 2;

	if (video.screen->h < height)
	{
		fprintf(stderr, "video: height(%d/%d) is too small\n",video.screen->h, height);
		return FALSE;
	}
	video.h = (video.screen->h - height) / 2;


	if (video.screen->format->BitsPerPixel != 8)
	{
		for (i = 0; i < 32 * 32 * 32; ++i)
			md_tbl_pallete15[i] = SDL_MapRGB(video.screen->format, (i & 0x1f) << 3, (i & 0x3e0) >> 2, (i & 0x7c00) >> 7);

/*
0000 0000 1111 1000 0x00f7

0000 0000 0001 1111 0x001f <<3
0000 0011 1110 0000 0x03e0 >>2
0111 1100 0000 0000 0x7c00 >>7
*/
	} else
	{
		for (i = 0; i < 8 * 8 * 8; ++i)
		{
			Uint32 a;
			int r,g,b, n;

			r = i & 7;
			g = i & 0x38;
			b = i & 0x1c0;
			n = (r << 2) + (g << 4) + (b << 6);

			a = SDL_MapRGB(video.screen->format, r << 5, g << 2, b >> 1);

			for (j = 0; j < 4; ++j)
			{
				md_tbl_pallete15[n + j + 0x000] = a;
				md_tbl_pallete15[n + j + 0x020] = a;
				md_tbl_pallete15[n + j + 0x040] = a;
				md_tbl_pallete15[n + j + 0x060] = a;

				md_tbl_pallete15[n + j + 0x400] = a;
				md_tbl_pallete15[n + j + 0x420] = a;
				md_tbl_pallete15[n + j + 0x440] = a;
				md_tbl_pallete15[n + j + 0x460] = a;

				md_tbl_pallete15[n + j + 0x800] = a;
				md_tbl_pallete15[n + j + 0x820] = a;
				md_tbl_pallete15[n + j + 0x840] = a;
				md_tbl_pallete15[n + j + 0x860] = a;

				md_tbl_pallete15[n + j + 0xc00] = a;
				md_tbl_pallete15[n + j + 0xc20] = a;
				md_tbl_pallete15[n + j + 0xc40] = a;
				md_tbl_pallete15[n + j + 0xc60] = a;
			}
		}
	}

/*	video_screen = SDL_AllocSurface(mp->option, mp->width, mp->height
	                , mp->bpp, 
	                video.screen->format->Rmask, 
	                video.screen->format->Gmask, 
	                video.screen->format->Bmask, 
	                video.screen->format->Amask);
*/

	return TRUE;
}


void md_settitle(const char *title)
{
	if (md_title != NULL) {
		free(md_title);
		md_title = NULL;
	}

	if (title != NULL)
	{
		md_title = (char *)malloc(strlen(title) + 1);
		if (md_title != NULL)
			strcpy(md_title, title);

		if (video.screen != NULL)
			SDL_WM_SetCaption(title, NULL);
	}
}


void md_video_fill(int x, int y, int w, int h, Uint32 c)
{
	md_video_rect_t rect;

	rect.x = video.w + x;
	rect.w = w;
	rect.y = video.h + y;
	rect.h = h;

	md_video_fillrect(video.screen, &rect, c);
}

Uint8 *md_video_lockline(int x, int y, int w, int h)
{
	if (SDL_MUSTLOCK(video.screen))
		SDL_LockSurface(video.screen); /* 戻り値チェック？ */

	return (Uint8 *)video.screen->pixels
	       + video.screen->format->BytesPerPixel * (video.w + x)
	       + video.screen->pitch * (y + video.h);
}

void md_video_unlockline(void)
{
	if (SDL_MUSTLOCK(video.screen))
		SDL_UnlockSurface(video.screen);
}

void md_video_update(int n, md_video_rect_t *rp)
{
/* DOUBLEBUF の時は UpdateRect のかわりに SDL_Flip を行う */

/* 	SDL_BlitSurface(video_screen, NULL, video.screen, NULL); */

	if (n == 0 || rp == NULL)
		SDL_UpdateRect(video.screen, 0, 0, 0, 0);
	else
		SDL_UpdateRects(video.screen, n, rp);

}

int md_video_pitch(void)
{
	return video.screen->pitch;
}

#ifndef	MD_BPP
int md_video_bpp(void)
{
	return video.screen->format->BitsPerPixel;
}
#endif


bool md_init(const char *title)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER
#ifdef	SDL_INIT_JOYSTICK
			| SDL_INIT_JOYSTICK
#endif
			| SDL_INIT_AUDIO
	)<0)
	{
		fprintf(stderr, "SDL: Couldn't initialize: %s\n", SDL_GetError());
		return FALSE;
	}
	atexit(SDL_Quit);

	md_video_init();

	md_settitle(title);

	return TRUE;
}


bool md_audio_init(void (*func)(void *, Uint8 *, int),int n,int m)
{
	SDL_AudioSpec audio;

	audio.freq     = n;
	audio.format   = AUDIO_S16SYS;
	audio.channels = 1;
	audio.samples  = m;
	audio.callback = func;
	audio.userdata = NULL;

	if (SDL_OpenAudio(&audio, NULL) < 0)
	{
		fprintf(stderr, "audio: %s\n", SDL_GetError());
		return FALSE;
	}

	return TRUE;
}



	static	int 			joy_num=0;

#ifndef SDL_INIT_JOYSTICK

void md_joystick_init(void) {;}
int md_joystick_getall(int N) { return 0; }

#else

	static SDL_Joystick *joy[2]={NULL, NULL};

void md_joystick_init(void)
{
	int i;

	joy_num = SDL_NumJoysticks();

/* printf("joy %d\n", joy_num); */

	if (joy_num == 0)
	{
//		printf("joystick is not found.\n");
		return ;
	}

	for (i = 0; i < joy_num; i++)
	{
		joy[i] = SDL_JoystickOpen(i);
		if (joy[i] == NULL)
			fprintf(stderr, "joystick%d: %s\n", i, SDL_GetError());
	}
}

int md_joystick_getall(int N)
{
	int x, y;

	if (joy_num == 0)
		return 0;

	x=SDL_JoystickGetAxis(joy[N], 0);
	y=SDL_JoystickGetAxis(joy[N], 1);

	return  (x>0 ? 8:0) | (x<0 ? 4:0) | (y>0 ? 2:0) | (y<0 ? 1:0)
		| (SDL_JoystickGetButton(joy[N], 0) ? 0x10:0)
		| (SDL_JoystickGetButton(joy[N], 1) ? 0x20:0);
}

#endif


static int conv[SDLK_LAST+1];
static int events[SDLK_LAST+1]; /* XXX key はこの方法をやめる */

void md_event_init(void)
{
	memset(conv, 0xff, sizeof(conv));
	memset(events, 0xff, sizeof(events));

	md_joystick_init();
}

void md_event_set(int n, int m) /* XXX */
{
	if (n < sizeof(conv) / sizeof(int))
		conv[n] = m;
}

int md_event_gets(int *p, int max)
{
	int i;
	int a;
	SDL_Event ev;

	while (SDL_PollEvent(&ev) != 0)
	{
		switch(ev.type)
		{
		case SDL_KEYUP:
			if (ev.key.keysym.scancode == 0x7b) /* XXX japanese keyboad dirty hack(T_T) */
				a = 95;
			else
				a = ev.key.keysym.sym;
			events[a] = -1;
			break;

		case SDL_KEYDOWN:
			if (ev.key.keysym.scancode == 0x7b) /* XXX japanese keyboad dirty hack(T_T) */
				a = 95; else
				a = ev.key.keysym.sym;
			events[a] = conv[a];
			break;

		case SDL_QUIT:
			events[322] = conv[322];
		}
	}

#if 0
	if (joy_num > 0)
		{
		 int x,y;

		 x = SDL_JoystickGetAxis(joy[N], 0);
		 y = SDL_JoystickGetAxis(joy[N], 1);

		 if (a < max && x > 0)
		 if (a < max && x < 0)
		 if (a < max && y > 0)
		 if (a < max && y < 0)
		 if (a < max && SDL_JoystickGetButton(joy[N], 0))
		 if (a < max && SDL_JoystickGetButton(joy[N], 1))
		}
#endif

	a = 0;
	for (i = 0; i < sizeof(events) / sizeof(int); ++i)
	{
		if (events[i] == -1)
			continue;
		if (a>=max)
			break;

		p[a++] = events[i];
	}

	return a;
}


static Uint32 (*md_timer_proc)(Uint32) = NULL;
static bool md_timer_flag = FALSE;
static int md_timer;

static Uint32 md_timer_itimerproc(Uint32 n)
{
	if (!md_timer_flag)
		return 0;

	if (md_timer_proc == NULL)
		return n;

	md_timer_proc(n);

	return n;
}

void md_timer_settimer(Uint32 n, Uint32 (*proc)(Uint32))
{
	md_timer_proc = proc;
	md_timer = n;
}

void md_timer_starttimer(void)
{
	md_timer_flag = TRUE;
	SDL_SetTimer(md_timer, md_timer_itimerproc);
}

void md_timer_stoptimer(void)
{
	md_timer_flag = FALSE;
	SDL_SetTimer(0, NULL);
}

