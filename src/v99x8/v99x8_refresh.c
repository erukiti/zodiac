/*
 * Copyright (c) 2000-2002 SASAKI Shunsuke (eruchan@users.sourceforge.net).
 * Copyright (c) 2001-2002 The Zodiac project.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "../../config.h"

#include <stdlib.h>
#include <string.h>

#include "../zodiac.h"
#include "v99x8.h"

extern bool f_scr;


#ifdef MD_LITTLE
#	define UINT32_FROM16(n1, n2) ((n2) << 16 | (n1))
#	define UINT8_FROM4(n1, n2) ((n1) << 4 | (n2))
#else
#	define	UINT32_FROM16(n1, n2)	((n1) << 16 | (n2))
#	define	UINT8_FROM4(n1, n2)	((n2) << 4 | (n1))
#endif


typedef struct
{
	int width, height;
	int bpp;

} v99x8_refresh_t;

static v99x8_refresh_t v99x8_refresh;


  /* pallete */

typedef struct
{
	bool flag;
	Uint8 r, g, b;
	Uint32 color;
} v99x8_pallete_t;

static v99x8_pallete_t pal[16 + 1];
static Uint32 pal_8[256];
static Uint32 pal_m[256];

static bool f_pal;

void v99x8_pallete_set(Uint8 n, Uint8 r, Uint8 g, Uint8 b)
{
	if (n == 0)
		n = 16;

	pal[n].r = r >> 3;
	pal[n].g = g >> 3;
	pal[n].b = b >> 3;
	pal[n].flag = TRUE;

	f_pal = TRUE;
}

static void v99x8_pallete_update(void)
{
	static int bg = 0;

	int a;
	int i, j;

	if (f_pal)
		for (i = 1; i < 17; ++i)
			if (pal[i].flag)
				pal[i].color = md_maprgb15(pal[i].r, pal[i].g, pal[i].b);

	a = (v99x8.col_bg == 0 || (v99x8.ctrl[8] & 0x20)) ? 16 : v99x8.col_bg;
	if (bg != a || pal[a].flag)
	{
		f_pal = TRUE;
		bg = a;
		memcpy(&pal[0], &pal[a], sizeof(v99x8_pallete_t));
		pal[0].flag = TRUE;
	}

	if (!f_pal)
		return;

	if (!v99x8.f_zoom)
	{
		for (i = 0; i < 16; ++i)
		{
			for (j = 0; j < 16; ++j)
			{
				if (!pal[i].flag && !pal[j].flag)
					continue;

				a = UINT8_FROM4(i, j);

				if (i == j)
				{
					pal_m[a] = pal[i].color;
				} else
				{
					pal_m[a] = md_maprgb15((pal[i].r >> 1) + (pal[j].r >> 1),
					                       (pal[i].g >> 1) + (pal[j].g >> 1),
					                       (pal[i].b >> 1) + (pal[j].b >> 1));
				}
			}
		}
	}
	f_pal = FALSE;
	for (i = 0; i < 17; ++i)
		pal[i].flag = FALSE;
}

static void v99x8_pallete_init(void)
{
	Uint32 pal_black;
	int i;

	memset(pal, 0, sizeof(pal));
	f_pal = FALSE;

	pal_black = md_maprgb15(0, 0, 0);
	for (i = 0; i < 256; ++i)
	{
	             /* GGGRRRBB */
		pal_8[i] = md_maprgb15(i & 0x1c, (i >> 3) & 0x1c, (i & 3) << 3);
		pal_m[i] = pal_black;
	}
}


#define	V99X8_WIDTH  (256 + 15)
#define	V99X8_HEIGHT (212 + 15)


static Uint8 tbl_yjk_b[32 * 64 * 64], tbl_yjk_rg[62 + 32];
static Uint8 blackbuf[256];      /* sprite 非表示用バッファ */


void v99x8_refresh_init(void)
{
	int i;
	md_video_mode_t	mode;

	v99x8_refresh.width  = V99X8_WIDTH;
	v99x8_refresh.height = V99X8_HEIGHT;

	if (v99x8.f_zoom)
	{
		v99x8_refresh.width *= 2;
		v99x8_refresh.height *= 2;
	}

	mode.width  = v99x8_refresh.width;
	mode.height = v99x8_refresh.height;

#ifdef	MD_BPP
	mode.bpp = v99x8_refresh.bpp = MD_BPP;
#else
	mode.bpp = v99x8_refresh.bpp = 16;
#endif

	mode.option	= md_video_defaultopt();

	md_video_fixmode(&mode);
	if (!md_video_setmode(v99x8_refresh.width, v99x8_refresh.height, &mode, NULL))
		exit(EXIT_FAILURE);

	v99x8_refresh.bpp = md_video_bpp();

	v99x8_pallete_init();

	for (i = 0; i < 32; ++i)
	{
		int	n;

		int	j;
		for (j = 0; j < 64; ++j)
		{
			int	k;
			for (k = 0; k < 64; ++k)
			{
				n = (i * 5 - ((j & 0x20) ? j - 64 : j) * 2
				     - ((k & 0x20) ? k - 64 : k)) >> 2;
				if (n > 31)
					n = 31;
				if (n < 0)
					n = 0;

				tbl_yjk_b[(((j + 32) & 63) << 11)
				    + (((k + 32) & 63) << 5) + i] = n;
			}
		}

		tbl_yjk_rg[i] = 0;
		tbl_yjk_rg[i + 32] = i;
		tbl_yjk_rg[i + 64] = 31;
	}

/*
y5:k3 (low)
y5:k3 (high)
y5:j3 
y5:j3 

r: y + j
g: y + k
b: (5Y + 2J + K) >> 2
*/
	memset(blackbuf, 0, sizeof(blackbuf));
}

void v99x8_refresh_screen(void)
{
	md_video_update(0, NULL);
}

void v99x8_refresh_clear(void)
{
	md_video_fill(0, 0, v99x8_refresh.width, v99x8_refresh.height, pal[v99x8.col_bg].color);
}

static Uint8 *v99x8_refresh_start(int x, int w, int h)
{
	int a;

	v99x8_pallete_update();

	a = v99x8.f_zoom ? 2 : 1;

	return md_video_lockline((((7 - v99x8.ctrl[18]) & 0x0f) + x) * a
	                         , v99x8.scanline * a
	                         , w * a, h * a);
}

static __inline__ void v99x8_refresh_stop(void)
{
	md_video_unlockline();
}


#ifdef MD_BPP

#	if MD_BPP != 2

#		define pixel_put(pb, n, px) *((md_pixel_t *)(pb) + (n)) = (px)

#	else

/*

#		ifdef MD_LITTLE
#			define pixel_put(pb, n, px) *((Uint8 *)(pb) + (n) / 4)  \
			                              |= (px) << ((3 - (n)) * 2)
#		else
#			define pixel_put(pb, n, px) *((Uint8 *)(pb) + (n) / 4)  \
			                              |= (px) << ((n) * 2)
#		endif

*/

#	endif

#else
static __inline__ void pixel_put(void *pb, int n, Uint32 p1)
{
	Uint8 *p;
	int mask, pix2bpp;

	switch (v99x8_refresh.bpp)
	{
	case 16:
		*((Uint16 *)pb + n) = p1;
		break;
	case 8:
		*((Uint8 *)pb + n) = p1;
		break;
	case 32:
		*((Uint32 *)pb + n) = p1;
		break;
	case 2:
		p = (Uint8 *)pb + (n >> 2);
#ifdef MD_LITTLE
		mask = 0xc0 >> ((n & 3) * 2);
		pix2bpp = p1 << (6 - ((n & 3) * 2));
#else
		mask = 0x03 << ((n & 3) * 2);
		pix2bpp = p1 << ((n & 3) * 2);

#endif
		*p = (*p & ~mask) | (pix2bpp & mask);
		break;

/* XXX 2bpp 対応方法
 *
 *  1. まず対応する 2bit を & で 0 クリアする
 *  2. 次に、2bit を | で加える
 *
 * という手順が必要。対応は後日にて.... --Ｌ
 */

	}
}
#endif




static Uint8 sbuf[32 + 256 + 16];

typedef struct
{
	Uint8 y;
	Uint8 x;
	Uint8 n;
	Uint8 a;
} v99x8_sprite_t;



static Uint8 *v99x8_refresh_sprite1(Uint8 y)
{
	v99x8_sprite_t *ptr_a;
	int n, size;
	int i;
	Uint8 a, c, *ptr_g, *ptr_s, *tbl_sp;
	bool cf;

//	if (v99x8.ctrl[8] & 0x02)
//		return blackbuf;

//	tbl_sp = &v99x8.vram[(int)v99x8.ctrl[6] << 11];
	tbl_sp = v99x8.vram + ((int)v99x8.ctrl[6] << 11);

	memset(sbuf + 32, 0, 256);

	size = (v99x8.ctrl[1] & 0x02) ? 16 : 8;
//	if (SpritesMAG)
//		h *= 2;

	n = 0;
	cf = FALSE;
	ptr_a = (v99x8_sprite_t *)(v99x8.vram + ((int)v99x8.ctrl[11] << 15)
		                                  + ((int)v99x8.ctrl[5] << 7));

	for (i = 0 ;; ++i, ++ptr_a)
	{
		if (i >= 32 || n >= 4 || ptr_a->y == 208)
			break;

		a = (Uint8)(ptr_a->y - v99x8.ctrl[23]); // a>256-h? a-=256
		if (a >=y || a + size < y)
			continue;

		++n;
		a = y - (a + 1);

		c = ptr_a->a;
		ptr_s = sbuf + ptr_a->x + ((c & 0x80) ? 0 : 32);
		ptr_g = tbl_sp + a
		+ (((v99x8.ctrl[1] & 0x02) ? ptr_a->n & 0xfc : ptr_a->n) << 3);

//		cf = TRUE;
		c &= 0x0f;

		if (ptr_s[0] == 0 && (*ptr_g & 0x80))	ptr_s[0] = c;
		if (ptr_s[1] == 0 && (*ptr_g & 0x40))	ptr_s[1] = c;
		if (ptr_s[2] == 0 && (*ptr_g & 0x20))	ptr_s[2] = c;
		if (ptr_s[3] == 0 && (*ptr_g & 0x10))	ptr_s[3] = c;
		if (ptr_s[4] == 0 && (*ptr_g & 0x08))	ptr_s[4] = c;
		if (ptr_s[5] == 0 && (*ptr_g & 0x04))	ptr_s[5] = c;
		if (ptr_s[6] == 0 && (*ptr_g & 0x02))	ptr_s[6] = c;
		if (ptr_s[7] == 0 && (*ptr_g & 0x01))	ptr_s[7] = c;

		if (!(v99x8.ctrl[1] & 0x02))
			continue;

		ptr_s += 8;
		ptr_g += 16;

		if (ptr_s[0] == 0 && (*ptr_g & 0x80))	ptr_s[0] = c;
		if (ptr_s[1] == 0 && (*ptr_g & 0x40))	ptr_s[1] = c;
		if (ptr_s[2] == 0 && (*ptr_g & 0x20))	ptr_s[2] = c;
		if (ptr_s[3] == 0 && (*ptr_g & 0x10))	ptr_s[3] = c;
		if (ptr_s[4] == 0 && (*ptr_g & 0x08))	ptr_s[4] = c;
		if (ptr_s[5] == 0 && (*ptr_g & 0x04))	ptr_s[5] = c;
		if (ptr_s[6] == 0 && (*ptr_g & 0x02))	ptr_s[6] = c;
		if (ptr_s[7] == 0 && (*ptr_g & 0x01))	ptr_s[7] = c;
	}
	return sbuf + 32;
}


static Uint8 *v99x8_refresh_sprite2(Uint8 y)
{
	v99x8_sprite_t *ptr_a;
	int n, size;
	int i;
	Uint8 a, c, *ptr_g, *ptr_c, *ptr_s, *tbl_sp;
	bool cf;

	if (v99x8.ctrl[8] & 0x02)
		return blackbuf;

	tbl_sp = v99x8.vram + ((int)v99x8.ctrl[6] << 11);

	memset(sbuf + 32, 0, 256);

	size = (v99x8.ctrl[1] & 0x02) ? 16 : 8;
//	if (SpritesMAG)
//		h *= 2;

	n = 0;
	cf = FALSE;

	ptr_c = v99x8.vram + ((int)v99x8.ctrl[11] << 15)
		               + ((int)(v99x8.ctrl[5] & 0xf8) << 7);
	ptr_a = (v99x8_sprite_t *)(ptr_c + 0x200);

	for (i = 0 ;; ++i, ++ptr_a, ptr_c += 16)
	{
		if (i >= 32 || n >= 8 || ptr_a->y == 216)
			break;

		a = (Uint8)(ptr_a->y - v99x8.ctrl[23]); // a>256-h? a-=256
		if (a >= y || a + size < y)
			continue;

		++n;
		a = y - (a + 1);

		c = ptr_c[a];
		ptr_s = sbuf + ptr_a->x + ((c & 0x80) ? 0 : 32);
		ptr_g = tbl_sp + a
		        + (((v99x8.ctrl[1] & 0x02) ? ptr_a->n & 0xfc : ptr_a->n) << 3);

		if ((c & 0x40) == 0)
		{
			cf = TRUE;
			c &= 0x0f;

			if (ptr_s[0] == 0 && (*ptr_g & 0x80))	ptr_s[0] = c;
			if (ptr_s[1] == 0 && (*ptr_g & 0x40))	ptr_s[1] = c;
			if (ptr_s[2] == 0 && (*ptr_g & 0x20))	ptr_s[2] = c;
			if (ptr_s[3] == 0 && (*ptr_g & 0x10))	ptr_s[3] = c;
			if (ptr_s[4] == 0 && (*ptr_g & 0x08))	ptr_s[4] = c;
			if (ptr_s[5] == 0 && (*ptr_g & 0x04))	ptr_s[5] = c;
			if (ptr_s[6] == 0 && (*ptr_g & 0x02))	ptr_s[6] = c;
			if (ptr_s[7] == 0 && (*ptr_g & 0x01))	ptr_s[7] = c;

			if (!(v99x8.ctrl[1] & 0x02))
				continue;

			ptr_s += 8; ptr_g += 16;
			if (ptr_s[0] == 0 && (*ptr_g & 0x80))	ptr_s[0] = c;
			if (ptr_s[1] == 0 && (*ptr_g & 0x40))	ptr_s[1] = c;
			if (ptr_s[2] == 0 && (*ptr_g & 0x20))	ptr_s[2] = c;
			if (ptr_s[3] == 0 && (*ptr_g & 0x10))	ptr_s[3] = c;
			if (ptr_s[4] == 0 && (*ptr_g & 0x08))	ptr_s[4] = c;
			if (ptr_s[5] == 0 && (*ptr_g & 0x04))	ptr_s[5] = c;
			if (ptr_s[6] == 0 && (*ptr_g & 0x02))	ptr_s[6] = c;
			if (ptr_s[7] == 0 && (*ptr_g & 0x01))	ptr_s[7] = c;
		} else
		{
			if (!cf)
				continue;

			c &= 0x0f;
			if (*ptr_g & 0x80)	ptr_s[0] |= c;
			if (*ptr_g & 0x40)	ptr_s[1] |= c;
			if (*ptr_g & 0x20)	ptr_s[2] |= c;
			if (*ptr_g & 0x10)	ptr_s[3] |= c;
			if (*ptr_g & 0x08)	ptr_s[4] |= c;
			if (*ptr_g & 0x04)	ptr_s[5] |= c;
			if (*ptr_g & 0x02)	ptr_s[6] |= c;
			if (*ptr_g & 0x01)	ptr_s[7] |= c;

			if (!(v99x8.ctrl[1] & 0x02))
				continue;

			ptr_s += 8; ptr_g += 16;
			if (*ptr_g & 0x80)	ptr_s[0] |= c;
			if (*ptr_g & 0x40)	ptr_s[1] |= c;
			if (*ptr_g & 0x20)	ptr_s[2] |= c;
			if (*ptr_g & 0x10)	ptr_s[3] |= c;
			if (*ptr_g & 0x08)	ptr_s[4] |= c;
			if (*ptr_g & 0x04)	ptr_s[5] |= c;
			if (*ptr_g & 0x02)	ptr_s[6] |= c;
			if (*ptr_g & 0x01)	ptr_s[7] |= c;
		}
	}
	return sbuf + 32;
}



void v99x8_refresh_sc0(int y, int h)
{
	Uint8 *pbuf;
	Uint32 fg, bg;
	int pp;


	if (f_scr)
	{
		v99x8.tbl_pg = v99x8.vram + (((int)v99x8.ctrl[4] & 0x3f) << 11);
		v99x8.tbl_pn = v99x8.vram + (((int)v99x8.ctrl[2] & 0x7f) << 10);
		f_scr = FALSE;
	}

	pbuf = v99x8_refresh_start(8, 240, h);
	pp = md_video_pitch() - md_video_pixbytes(240);

	fg = pal[v99x8.col_fg].color;
	bg = pal[v99x8.col_bg].color;

	while(h-- > 0)
	{
		int i;
		Uint8 *T,*G;

		T = v99x8.tbl_pn + (y >> 3) * 40;
		G = v99x8.tbl_pg + (y & 0x07);
		++y;

		for (i = 0; i < 40; ++i)
		{
			Uint8	a;

			a = G[(int)*T++ << 3];
			if  (v99x8.f_zoom)
			{
				pixel_put(pbuf, 0, (a & 0x80)? fg : bg);
				pixel_put(pbuf, 1, (a & 0x80)? fg : bg);
				pixel_put(pbuf, 2, (a & 0x40)? fg : bg);
				pixel_put(pbuf, 3, (a & 0x40)? fg : bg);
				pixel_put(pbuf, 4, (a & 0x20)? fg : bg);
				pixel_put(pbuf, 5, (a & 0x20)? fg : bg);
				pixel_put(pbuf, 6, (a & 0x10)? fg : bg);
				pixel_put(pbuf, 7, (a & 0x10)? fg : bg);
				pixel_put(pbuf, 8, (a & 0x08)? fg : bg);
				pixel_put(pbuf, 9, (a & 0x08)? fg : bg);
				pixel_put(pbuf, 10, (a & 0x04)? fg : bg);
				pixel_put(pbuf, 11, (a & 0x04)? fg : bg);
				pbuf += md_video_pixbytes(12);
	  		} else
	  		{
				pixel_put(pbuf, 0, (a & 0x80)? fg : bg);
				pixel_put(pbuf, 1, (a & 0x40)? fg : bg);
				pixel_put(pbuf, 2, (a & 0x20)? fg : bg);
				pixel_put(pbuf, 3, (a & 0x10)? fg : bg);
				pixel_put(pbuf, 4, (a & 0x08)? fg : bg);
				pixel_put(pbuf, 5, (a & 0x04)? fg : bg);
				pbuf += md_video_pixbytes(6);
			}
	  	}
		pbuf += pp;
	}

	v99x8_refresh_stop();
}

void v99x8_refresh_sc1(int y, int h)
{
	Uint8 *T,*G;
	int i, n;
	int pp;
	Uint8 *pbuf, *sp;

	if (f_scr)
	{
		v99x8.tbl_pg = v99x8.vram + (((int)v99x8.ctrl[4] & 0x3f) << 11);
		v99x8.tbl_pn = v99x8.vram + (((int)v99x8.ctrl[2] & 0x7f) << 10);
		v99x8.tbl_cl = v99x8.vram + (((int)v99x8.ctrl[10] & 0x07) << 14)
		                          + ((int)v99x8.ctrl[3] << 6);
		f_scr = FALSE;
	}

	pbuf = v99x8_refresh_start(0, 256, h);
	pp = md_video_pitch() - md_video_pixbytes(256);

	while (h-- > 0)
	{
		n = v99x8.ctrl[23] + y;
		G = v99x8.tbl_pg + (n & 0x07);
		T = v99x8.tbl_pn + ((n & 0xF8) << 2);
		sp = v99x8_refresh_sprite1(y++);

		for(i = 0; i < 32; ++i)
		{
			Uint8 a;
			Uint8 n;
			Uint32 fg, bg;

			n = v99x8.tbl_cl[*T >> 3];
			fg = pal[n >> 4].color;
			bg = pal[n & 0x0f].color;

			a = G[(int)*T++ << 3];
			if  (v99x8.f_zoom)
			{
				pixel_put(pbuf, 0, sp[0] ? pal[sp[0]].color : (a & 0x80) ? fg : bg);
				pixel_put(pbuf, 1, sp[0] ? pal[sp[0]].color : (a & 0x80) ? fg : bg);
				pixel_put(pbuf, 2, sp[1] ? pal[sp[1]].color : (a & 0x40) ? fg : bg);
				pixel_put(pbuf, 3, sp[1] ? pal[sp[1]].color : (a & 0x40) ? fg : bg);
				pixel_put(pbuf, 4, sp[2] ? pal[sp[2]].color : (a & 0x20) ? fg : bg);
				pixel_put(pbuf, 5, sp[2] ? pal[sp[2]].color : (a & 0x20) ? fg : bg);
				pixel_put(pbuf, 6, sp[3] ? pal[sp[3]].color : (a & 0x10) ? fg : bg);
				pixel_put(pbuf, 7, sp[3] ? pal[sp[3]].color : (a & 0x10) ? fg : bg);
				pixel_put(pbuf, 8, sp[4] ? pal[sp[4]].color : (a & 0x08) ? fg : bg);
				pixel_put(pbuf, 9, sp[4] ? pal[sp[4]].color : (a & 0x08) ? fg : bg);
				pixel_put(pbuf, 10, sp[5] ? pal[sp[5]].color : (a & 0x04) ? fg : bg);
				pixel_put(pbuf, 11, sp[5] ? pal[sp[5]].color : (a & 0x04) ? fg : bg);
				pixel_put(pbuf, 12, sp[6] ? pal[sp[6]].color : (a & 0x02) ? fg : bg);
				pixel_put(pbuf, 13, sp[6] ? pal[sp[6]].color : (a & 0x02) ? fg : bg);
				pixel_put(pbuf, 14, sp[7] ? pal[sp[7]].color : (a & 0x01) ? fg : bg);
				pixel_put(pbuf, 15, sp[7] ? pal[sp[7]].color : (a & 0x01) ? fg : bg);
				pbuf += md_video_pixbytes(16);
				sp += 8;
			} else
			{
				pixel_put(pbuf, 0, sp[0] ? pal[sp[0]].color : (a & 0x80) ? fg : bg);
				pixel_put(pbuf, 1, sp[1] ? pal[sp[1]].color : (a & 0x40) ? fg : bg);
				pixel_put(pbuf, 2, sp[2] ? pal[sp[2]].color : (a & 0x20) ? fg : bg);
				pixel_put(pbuf, 3, sp[3] ? pal[sp[3]].color : (a & 0x10) ? fg : bg);
				pixel_put(pbuf, 4, sp[4] ? pal[sp[4]].color : (a & 0x08) ? fg : bg);
				pixel_put(pbuf, 5, sp[5] ? pal[sp[5]].color : (a & 0x04) ? fg : bg);
				pixel_put(pbuf, 6, sp[6] ? pal[sp[6]].color : (a & 0x02) ? fg : bg);
				pixel_put(pbuf, 7, sp[7] ? pal[sp[7]].color : (a & 0x01) ? fg : bg);

				pbuf += md_video_pixbytes(8);
				sp += 8;
			}
		}
		pbuf += pp;
	}

	v99x8_refresh_stop();
}

void v99x8_refresh_sc4(int y, int h)
{
	int i;
	int n;
	Uint8 *T, *PGT, *CLT;
	int pp;
	Uint8 *pbuf, *sp;

	if (f_scr)
	{
		v99x8.tbl_pg = v99x8.vram + (((int)v99x8.ctrl[4] & 0x3c) << 11);
		v99x8.tbl_pn = v99x8.vram + (((int)v99x8.ctrl[2] & 0x7f) << 10);
		v99x8.tbl_cl = v99x8.vram + (((int)v99x8.ctrl[10] & 0x07) << 14)
		                          + (((int)v99x8.ctrl[3] & 0x80) << 6);
		f_scr = FALSE;
	}


	pbuf = v99x8_refresh_start(0, 256, h);
	pp = md_video_pitch() - md_video_pixbytes(256);

	while (h-- > 0)
	{
		n = v99x8.ctrl[23] + y;
		T = v99x8.tbl_pn + ((n & 0xf8) << 2);

		n = ((n & 0xc0) << 5) + (n & 0x07);
		PGT = v99x8.tbl_pg + n;
		CLT = v99x8.tbl_cl + n;

		if (v99x8.scr == 2)
			sp = v99x8_refresh_sprite1(y++);
		else
			sp = v99x8_refresh_sprite2(y++);

		for (i = 0; i < 32; ++i)
		{
			Uint8 a;
			Uint8 n;
			Uint32 fg, bg;

			n = CLT[*T << 3];
			fg = pal[n >> 4].color;
			bg = pal[n & 0x0f].color;

			a = PGT[(int)*T++ << 3];

			if  (v99x8.f_zoom)
			{
				pixel_put(pbuf, 0, sp[0] ? pal[sp[0]].color : (a & 0x80) ? fg : bg);
				pixel_put(pbuf, 1, sp[0] ? pal[sp[0]].color : (a & 0x80) ? fg : bg);
				pixel_put(pbuf, 2, sp[1] ? pal[sp[1]].color : (a & 0x40) ? fg : bg);
				pixel_put(pbuf, 3, sp[1] ? pal[sp[1]].color : (a & 0x40) ? fg : bg);
				pixel_put(pbuf, 4, sp[2] ? pal[sp[2]].color : (a & 0x20) ? fg : bg);
				pixel_put(pbuf, 5, sp[2] ? pal[sp[2]].color : (a & 0x20) ? fg : bg);
				pixel_put(pbuf, 6, sp[3] ? pal[sp[3]].color : (a & 0x10) ? fg : bg);
				pixel_put(pbuf, 7, sp[3] ? pal[sp[3]].color : (a & 0x10) ? fg : bg);
				pixel_put(pbuf, 8, sp[4] ? pal[sp[4]].color : (a & 0x08) ? fg : bg);
				pixel_put(pbuf, 9, sp[4] ? pal[sp[4]].color : (a & 0x08) ? fg : bg);
				pixel_put(pbuf, 10, sp[5] ? pal[sp[5]].color : (a & 0x04) ? fg : bg);
				pixel_put(pbuf, 11, sp[5] ? pal[sp[5]].color : (a & 0x04) ? fg : bg);
				pixel_put(pbuf, 12, sp[6] ? pal[sp[6]].color : (a & 0x02) ? fg : bg);
				pixel_put(pbuf, 13, sp[6] ? pal[sp[6]].color : (a & 0x02) ? fg : bg);
				pixel_put(pbuf, 14, sp[7] ? pal[sp[7]].color : (a & 0x01) ? fg : bg);
				pixel_put(pbuf, 15, sp[7] ? pal[sp[7]].color : (a & 0x01) ? fg : bg);
				pbuf += md_video_pixbytes(16); sp += 8;
			} else
			{
				pixel_put(pbuf, 0, sp[0] ? pal[sp[0]].color : (a & 0x80)? fg : bg);
				pixel_put(pbuf, 1, sp[1] ? pal[sp[1]].color : (a & 0x40)? fg : bg);
				pixel_put(pbuf, 2, sp[2] ? pal[sp[2]].color : (a & 0x20)? fg : bg);
				pixel_put(pbuf, 3, sp[3] ? pal[sp[3]].color : (a & 0x10)? fg : bg);
				pixel_put(pbuf, 4, sp[4] ? pal[sp[4]].color : (a & 0x08)? fg : bg);
				pixel_put(pbuf, 5, sp[5] ? pal[sp[5]].color : (a & 0x04)? fg : bg);
				pixel_put(pbuf, 6, sp[6] ? pal[sp[6]].color : (a & 0x02)? fg : bg);
				pixel_put(pbuf, 7, sp[7] ? pal[sp[7]].color : (a & 0x01)? fg : bg);

				pbuf += md_video_pixbytes(8); sp += 8;
			}
		}
		pbuf += pp;
	}

	v99x8_refresh_stop();
}

void v99x8_refresh_sc5(int y, int h)
{
	int i;
	Uint8 *T;
	int pp;
	Uint8 *pbuf, *sp;

	if (f_scr)
	{
		v99x8.tbl_pn = v99x8.vram + (((int)v99x8.ctrl[2] & 0x60) << 10);
		f_scr = FALSE;
	}

	pbuf = v99x8_refresh_start(0, 256, h);
	pp = md_video_pitch() - md_video_pixbytes(256);

	T = v99x8.tbl_pn + (((y + v99x8.ctrl[23]) & 0xff) << 7);

	while(h-- > 0)
	{
		sp = v99x8_refresh_sprite2(y++);
		for (i = 0; i < 32; ++i)
		{
			if  (v99x8.f_zoom)
			{
				pixel_put(pbuf, 0, pal[sp[0] ? sp[0] : T[0] >> 4].color);
				pixel_put(pbuf, 1, pal[sp[0] ? sp[0] : T[0] >> 4].color);
				pixel_put(pbuf, 2, pal[sp[1] ? sp[1] : T[0] & 0x0f].color);
				pixel_put(pbuf, 3, pal[sp[1] ? sp[1] : T[0] & 0x0f].color);
				pixel_put(pbuf, 4, pal[sp[2] ? sp[2] : T[1] >> 4].color);
				pixel_put(pbuf, 5, pal[sp[2] ? sp[2] : T[1] >> 4].color);
				pixel_put(pbuf, 6, pal[sp[3] ? sp[3] : T[1] & 0x0f].color);
				pixel_put(pbuf, 7, pal[sp[3] ? sp[3] : T[1] & 0x0f].color);
				pixel_put(pbuf, 8, pal[sp[4] ? sp[4] : T[2] >> 4].color);
				pixel_put(pbuf, 9, pal[sp[4] ? sp[4] : T[2] >> 4].color);
				pixel_put(pbuf, 10, pal[sp[5] ? sp[5] : T[2] & 0x0f].color);
				pixel_put(pbuf, 11, pal[sp[5] ? sp[5] : T[2] & 0x0f].color);
				pixel_put(pbuf, 12, pal[sp[6] ? sp[6] : T[3] >> 4].color);
				pixel_put(pbuf, 13, pal[sp[6] ? sp[6] : T[3] >> 4].color);
				pixel_put(pbuf, 14, pal[sp[7] ? sp[7] : T[3] & 0x0f].color);
				pixel_put(pbuf, 15, pal[sp[7] ? sp[7] : T[3] & 0x0f].color);

				pbuf += md_video_pixbytes(16); T += 4; sp += 8;
			} else {
				pixel_put(pbuf, 0, pal[sp[0] ? sp[0] : T[0] >> 4].color);
				pixel_put(pbuf, 1, pal[sp[1] ? sp[1] : T[0] & 0x0f].color);
				pixel_put(pbuf, 2, pal[sp[2] ? sp[2] : T[1] >> 4].color);
				pixel_put(pbuf, 3, pal[sp[3] ? sp[3] : T[1] & 0x0f].color);
				pixel_put(pbuf, 4, pal[sp[4] ? sp[4] : T[2] >> 4].color);
				pixel_put(pbuf, 5, pal[sp[5] ? sp[5] : T[2] & 0x0f].color);
				pixel_put(pbuf, 6, pal[sp[6] ? sp[6] : T[3] >> 4].color);
				pixel_put(pbuf, 7, pal[sp[7] ? sp[7] : T[3] & 0x0f].color);

				pbuf += md_video_pixbytes(8); T += 4; sp += 8;
			}
		}
		pbuf += pp;
	}

	v99x8_refresh_stop();
}

void	v99x8_refresh_sc6(int y, int h)
{
	int i;
	Uint8 *T;
	int pp;
	Uint8 *pbuf, *sp;

	if (f_scr)
	{
		v99x8.tbl_pn = v99x8.vram + (((int)v99x8.ctrl[2] & 0x60) << 10);
		f_scr = FALSE;
	}


	pbuf = v99x8_refresh_start(0, 256, h);
	pp = md_video_pitch() - md_video_pixbytes(256);

	T = v99x8.tbl_pn + (((y + v99x8.ctrl[23]) & 0xff) << 7);

	while(h-- > 0)
	{
		sp = v99x8_refresh_sprite2(y++);
		for(i = 0; i < 32; ++i)
		{
			if  (v99x8.f_zoom)
			{
				pixel_put(pbuf, 0, pal[sp[0] ? sp[0] : T[0] >> 6].color);
				pixel_put(pbuf, 1, pal[sp[0] ? sp[0] : (T[0] >> 4) & 3].color);
				pixel_put(pbuf, 2, pal[sp[1] ? sp[1] : (T[0] >> 2) & 3].color);
				pixel_put(pbuf, 3, pal[sp[1] ? sp[1] : T[0] & 3].color);
				pixel_put(pbuf, 4, pal[sp[2] ? sp[2] : T[1] >> 6].color);
				pixel_put(pbuf, 5, pal[sp[2] ? sp[2] : (T[1] >> 4) & 3].color);
				pixel_put(pbuf, 6, pal[sp[3] ? sp[3] : (T[1] >> 2) & 3].color);
				pixel_put(pbuf, 7, pal[sp[3] ? sp[3] : T[1] & 3].color);
				pixel_put(pbuf, 8, pal[sp[4] ? sp[4] : T[2] >> 6].color);
				pixel_put(pbuf, 9, pal[sp[4] ? sp[4] : (T[2] >> 4) & 3].color);
				pixel_put(pbuf, 10, pal[sp[5] ? sp[5] : (T[2] >> 2) & 3].color);
				pixel_put(pbuf, 11, pal[sp[5] ? sp[5] : T[2] & 3].color);
				pixel_put(pbuf, 12, pal[sp[6] ? sp[6] : T[3] >> 6].color);
				pixel_put(pbuf, 13, pal[sp[6] ? sp[6] : (T[3] >> 4) & 3].color);
				pixel_put(pbuf, 14, pal[sp[7] ? sp[7] : (T[3] >> 2) & 3].color);
				pixel_put(pbuf, 15, pal[sp[7] ? sp[7] : T[3] & 3].color);
				pbuf += md_video_pixbytes(16); T += 4; sp += 8;
			} else
			{
				pixel_put(pbuf, 0, sp[0] ? pal[sp[0]].color : pal_m[(T[0] >> 6) | (T[0] & 0x30)]);
				pixel_put(pbuf, 1, sp[1] ? pal[sp[1]].color : pal_m[((T[0] & 0x0c) << 2) | (T[0] & 0x03)]);
				pixel_put(pbuf, 2, sp[2] ? pal[sp[2]].color : pal_m[(T[1] >> 6) | (T[1] & 0x30)]);
				pixel_put(pbuf, 3, sp[3] ? pal[sp[3]].color : pal_m[((T[1] & 0x0c) << 2) | (T[1] & 0x03)]);
				pixel_put(pbuf, 4, sp[4] ? pal[sp[4]].color : pal_m[(T[2] >> 6) | (T[2] & 0x30)]);
				pixel_put(pbuf, 5, sp[5] ? pal[sp[5]].color : pal_m[((T[2] & 0x0c) << 2) | (T[2] & 0x03)]);
				pixel_put(pbuf, 6, sp[6] ? pal[sp[6]].color : pal_m[(T[3] >> 6) | (T[3] & 0x30)]);
				pixel_put(pbuf, 7, sp[7] ? pal[sp[7]].color : pal_m[((T[3] & 0x0c) << 2) | (T[3] & 0x03)]);
				pbuf += md_video_pixbytes(8); T += 4; sp += 8;
			}
		}
		pbuf += pp;
	}

	v99x8_refresh_stop();
}

void v99x8_refresh_sc7(int y, int h)
{
	Uint8 *pbuf, *sp;
	Uint8 *T;
	int pp;
	int i;

	if (f_scr)
	{
		v99x8.tbl_pn = v99x8.vram + (((int)v99x8.ctrl[2] & 0x20) << 11);
		f_scr = FALSE;
	}

	pbuf = v99x8_refresh_start(0, 256, h);
	pp = md_video_pitch() - md_video_pixbytes(256);

//printf("%d/%d %d  %d\n", y, v99x8.ctrl[23], (y+v99x8.ctrl[23])&0xff, v99x8.ctrl[19]+1);
	T = v99x8.tbl_pn + (((y + v99x8.ctrl[23]) & 0xff) << 8);

	while(h-- > 0)
	{
		sp = v99x8_refresh_sprite2(y);
		{
			for (i = 0; i < 32; ++i)
			{
			if  (v99x8.f_zoom)
				{
					pixel_put(pbuf, 0, pal[sp[0] ? sp[0] : T[0] >> 4].color);
					pixel_put(pbuf, 1, pal[sp[0] ? sp[0] : T[0] & 0x0f].color);
					pixel_put(pbuf, 2, pal[sp[1] ? sp[1] : T[1] >> 4].color);
					pixel_put(pbuf, 3, pal[sp[1] ? sp[1] : T[1] & 0x0f].color);
					pixel_put(pbuf, 4, pal[sp[2] ? sp[2] : T[2] >> 4].color);
					pixel_put(pbuf, 5, pal[sp[2] ? sp[2] : T[2] & 0x0f].color);
					pixel_put(pbuf, 6, pal[sp[3] ? sp[3] : T[3] >> 4].color);
					pixel_put(pbuf, 7, pal[sp[3] ? sp[3] : T[3] & 0x0f].color);
					pixel_put(pbuf, 8, pal[sp[4] ? sp[4] : T[4] >> 4].color);
					pixel_put(pbuf, 9, pal[sp[4] ? sp[4] : T[4] & 0x0f].color);
					pixel_put(pbuf, 10, pal[sp[5] ? sp[5] : T[5] >> 4].color);
					pixel_put(pbuf, 11, pal[sp[5] ? sp[5] : T[5] & 0x0f].color);
					pixel_put(pbuf, 12, pal[sp[6] ? sp[6] : T[6] >> 4].color);
					pixel_put(pbuf, 13, pal[sp[6] ? sp[6] : T[6] & 0x0f].color);
					pixel_put(pbuf, 14, pal[sp[7] ? sp[7] : T[7] >> 4].color);
					pixel_put(pbuf, 15, pal[sp[7] ? sp[7] : T[7] & 0x0f].color);
					pbuf += md_video_pixbytes(16); T += 8; sp += 8;
				} else
				{
					pixel_put(pbuf,0, sp[0]? pal[sp[0]].color : pal_m[T[0]]);
					pixel_put(pbuf,1, sp[1]? pal[sp[1]].color : pal_m[T[1]]);
					pixel_put(pbuf,2, sp[2]? pal[sp[2]].color : pal_m[T[2]]);
					pixel_put(pbuf,3, sp[3]? pal[sp[3]].color : pal_m[T[3]]);
					pixel_put(pbuf,4, sp[4]? pal[sp[4]].color : pal_m[T[4]]);
					pixel_put(pbuf,5, sp[5]? pal[sp[5]].color : pal_m[T[5]]);
					pixel_put(pbuf,6, sp[6]? pal[sp[6]].color : pal_m[T[6]]);
					pixel_put(pbuf,7, sp[7]? pal[sp[7]].color : pal_m[T[7]]);

					pbuf += md_video_pixbytes(8); T += 8; sp += 8;
				}
			}
		}
		++y;
		pbuf += pp;
	}

	v99x8_refresh_stop();
}

void v99x8_refresh_sc8(int y, int h)
{
	Uint8 *pbuf, *sp;
	Uint8 *T;
	int pp;
	int i;
	Uint8 sc8spr[16] =
	{
		0x00,0x02,0x10,0x12,0x80,0x82,0x90,0x92,
		0x49,0x4B,0x59,0x5B,0xC9,0xCB,0xD9,0xDB
	};

	if (f_scr)
	{
		v99x8.tbl_pn = v99x8.vram + (((int)v99x8.ctrl[2] & 0x20) << 11);
		f_scr = FALSE;
	}

	pbuf = v99x8_refresh_start(0, 256, h);
	pp = md_video_pitch() - md_video_pixbytes(256);

	T = v99x8.tbl_pn + (((y + v99x8.ctrl[23]) & 0xff) << 8);

	while(h-- > 0)
	{
		sp = v99x8_refresh_sprite2(y);
		{
			for (i = 0; i < 32; ++i)
			{
				if  (v99x8.f_zoom)
				{
					pixel_put(pbuf, 0, pal_8[sp[0] ? sc8spr[sp[0]] : T[0]]);
					pixel_put(pbuf, 1, pal_8[sp[0] ? sc8spr[sp[0]] : T[0]]);
					pixel_put(pbuf, 2, pal_8[sp[1] ? sc8spr[sp[1]] : T[1]]);
					pixel_put(pbuf, 3, pal_8[sp[1] ? sc8spr[sp[1]] : T[1]]);
					pixel_put(pbuf, 4, pal_8[sp[2] ? sc8spr[sp[2]] : T[2]]);
					pixel_put(pbuf, 5, pal_8[sp[2] ? sc8spr[sp[2]] : T[2]]);
					pixel_put(pbuf, 6, pal_8[sp[3] ? sc8spr[sp[3]] : T[3]]);
					pixel_put(pbuf, 7, pal_8[sp[3] ? sc8spr[sp[3]] : T[3]]);
					pixel_put(pbuf, 8, pal_8[sp[4] ? sc8spr[sp[4]] : T[4]]);
					pixel_put(pbuf, 9, pal_8[sp[4] ? sc8spr[sp[4]] : T[4]]);
					pixel_put(pbuf, 10, pal_8[sp[5] ? sc8spr[sp[5]] : T[5]]);
					pixel_put(pbuf, 11, pal_8[sp[5] ? sc8spr[sp[5]] : T[5]]);
					pixel_put(pbuf, 12, pal_8[sp[6] ? sc8spr[sp[6]] : T[6]]);
					pixel_put(pbuf, 13, pal_8[sp[6] ? sc8spr[sp[6]] : T[6]]);
					pixel_put(pbuf, 14, pal_8[sp[7] ? sc8spr[sp[7]] : T[7]]);
					pixel_put(pbuf, 15, pal_8[sp[7] ? sc8spr[sp[7]] : T[7]]);
					pbuf += md_video_pixbytes(16); T += 8; sp += 8;
				} else
				{
					pixel_put(pbuf,0, pal_8[sp[0] ? sc8spr[sp[0]] : T[0]]);
					pixel_put(pbuf,1, pal_8[sp[1] ? sc8spr[sp[1]] : T[1]]);
					pixel_put(pbuf,2, pal_8[sp[2] ? sc8spr[sp[2]] : T[2]]);
					pixel_put(pbuf,3, pal_8[sp[3] ? sc8spr[sp[3]] : T[3]]);
					pixel_put(pbuf,4, pal_8[sp[4] ? sc8spr[sp[4]] : T[4]]);
					pixel_put(pbuf,5, pal_8[sp[5] ? sc8spr[sp[5]] : T[5]]);
					pixel_put(pbuf,6, pal_8[sp[6] ? sc8spr[sp[6]] : T[6]]);
					pixel_put(pbuf,7, pal_8[sp[7] ? sc8spr[sp[7]] : T[7]]);
					pbuf += md_video_pixbytes(8); T += 8; sp += 8;
				}
			}
		}
		++y;
		pbuf += pp;
	}

	v99x8_refresh_stop();
}

static __inline__ Uint32 v99x8_refresh_MapYJK(int n, int j, int k, int jk)
{
	return md_maprgb15(tbl_yjk_rg[n + j], tbl_yjk_rg[n + k], tbl_yjk_b [n + jk]);
}


void v99x8_refresh_scc(int y, int h)
{
	Uint8 *pbuf, *sp;
	Uint8 *T;
	int pp;
	int i;
	Uint8 sc8spr[16] =
	{
		0x00, 0x02, 0x10, 0x12, 0x80, 0x82, 0x90, 0x92,
		0x49, 0x4B, 0x59, 0x5B, 0xC9, 0xCB, 0xD9, 0xDB
	};

	if (f_scr)
	{
		v99x8.tbl_pn = v99x8.vram + (((int)v99x8.ctrl[2] & 0x20) << 11);
		f_scr = FALSE;
	}

	pbuf = v99x8_refresh_start(0, 256, h);
	pp = md_video_pitch() - md_video_pixbytes(256);

	T = v99x8.tbl_pn + (((y + v99x8.ctrl[23]) & 0xff) << 8);

	while(h-- > 0)
	{
		sp = v99x8_refresh_sprite2(y);
		for (i = 0; i < 64; ++i)
		{
			int yjk_k, yjk_j, yjk_jk;

			yjk_k = (((T[0] & 7) | ((T[1] & 7) << 3)) + 32) & 0x3f;
			yjk_j = (((T[2] & 7) | ((T[3] & 7) << 3)) + 32) & 0x3f;

			yjk_jk = (yjk_j << 11) + (yjk_k << 5);

			pixel_put(pbuf, 0, sp[0] ? pal_8[sc8spr[sp[0]]] : v99x8_refresh_MapYJK(T[0] >> 3, yjk_j, yjk_k, yjk_jk));
			pixel_put(pbuf, 1, sp[1] ? pal_8[sc8spr[sp[1]]] : v99x8_refresh_MapYJK(T[1] >> 3, yjk_j, yjk_k, yjk_jk));
			pixel_put(pbuf, 2, sp[2] ? pal_8[sc8spr[sp[2]]] : v99x8_refresh_MapYJK(T[2] >> 3, yjk_j, yjk_k, yjk_jk));
			pixel_put(pbuf, 3, sp[3] ? pal_8[sc8spr[sp[3]]] : v99x8_refresh_MapYJK(T[3] >> 3, yjk_j, yjk_k, yjk_jk));

			pbuf += md_video_pixbytes(4); T += 4; sp += 4;
		}
		++y;
		pbuf += pp;
	}

	v99x8_refresh_stop();
}

void	v99x8_refresh_sc2(int y, int h)	{;}
void	v99x8_refresh_sc3(int y, int h)	{;}
void	v99x8_refresh_sca(int y, int h)	{;} // sc10/sc11
void	v99x8_refresh_scx(int y, int h)
{
	Uint8 *pbuf;
	int pp;
	Uint32 fg, bg, m[4];

	if (f_scr)
	{
		v99x8.tbl_pg = v99x8.vram + (((int)v99x8.ctrl[4] & 0x3f) << 11);
		v99x8.tbl_pn = v99x8.vram + (((int)v99x8.ctrl[2] & 0x7c) << 10);
		v99x8.tbl_cl = v99x8.vram + (((int)v99x8.ctrl[10] & 0x07) << 14)
		                          + (((int)v99x8.ctrl[3] & 0xf8) << 6);
		f_scr = FALSE;
	}

	pbuf = v99x8_refresh_start(8, 240, h);
	pp = md_video_pitch() - md_video_pixbytes(240);

	fg = pal[v99x8.col_fg].color;
	bg = pal[v99x8.col_bg].color;
	m[0] = pal_m[(v99x8.col_bg << 4) | v99x8.col_bg];
	m[1] = pal_m[(v99x8.col_bg << 4) | v99x8.col_fg];
	m[2] = pal_m[(v99x8.col_fg << 4) | v99x8.col_bg];
	m[3] = pal_m[(v99x8.col_fg << 4) | v99x8.col_fg];

	while(h-- > 0)
	{
		int i;
		Uint8 *T,*G;

		T = v99x8.tbl_pn + (y >> 3) * 80;
		G = v99x8.tbl_pg + (y & 0x07);
		++y;

		for (i = 0; i < 80; ++i)
		{
			Uint8 a;

			a = G[(int)*T++ << 3];
			if  (v99x8.f_zoom)
			{
				pixel_put(pbuf, 0, (a & 0x80) ? fg : bg);
				pixel_put(pbuf, 1, (a & 0x40) ? fg : bg);
				pixel_put(pbuf, 2, (a & 0x20) ? fg : bg);
				pixel_put(pbuf, 3, (a & 0x10) ? fg : bg);
				pixel_put(pbuf, 4, (a & 0x08) ? fg : bg);
				pixel_put(pbuf, 5, (a & 0x04) ? fg : bg);
				pbuf += md_video_pixbytes(6);
			} else
			{
				pixel_put(pbuf, 0, m[a >> 6]);
				pixel_put(pbuf, 1, m[(a >> 4)& 0x03]);
				pixel_put(pbuf, 2, m[(a >> 2)& 0x03]);

				pbuf += md_video_pixbytes(3);
			}
		}
		pbuf += pp;
	}

	v99x8_refresh_stop();
}

