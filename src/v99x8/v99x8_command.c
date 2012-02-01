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

#include <string.h>
#include <stdlib.h>

#include "../zodiac.h"
#include "v99x8.h"

typedef struct
{
	int xbytes;
	int xmask, xshift;
	int ymask, yshift;

	int sx, sy, dx, dy, nx, ny;
	int lop;

	Uint8 *src, *dst;
} vcom_t;

vcom_t vcom;

typedef struct
{
	int sx, sy, ex, ey;
	int x, y;
	int xsize;
} r44_t;

static r44_t r44;

static int getshift(int n)
{
	int i;

	for (i = 0; n & (1 << i); i++)
		;
	return i;
}

#define vcom_getsx() vcom.sx = (v99x8.ctrl[32] + (v99x8.ctrl[33] << 8)) & vcom.xmask
#define vcom_getsy() vcom.sy = (v99x8.ctrl[34] + (v99x8.ctrl[35] << 8)) & vcom.ymask
#define vcom_getdx() vcom.dx = (v99x8.ctrl[36] + (v99x8.ctrl[37] << 8)) & vcom.xmask
#define vcom_getdy() vcom.dy = (v99x8.ctrl[38] + (v99x8.ctrl[39] << 8)) & vcom.ymask
#define vcom_getnx() vcom.nx = ((v99x8.ctrl[40] + (v99x8.ctrl[41] << 8) - 1) & 511) + 1
#define vcom_getny() vcom.ny = ((v99x8.ctrl[42] + (v99x8.ctrl[43] << 8) - 1) & 1023) + 1

static void vcom_set(int base, int n)
{
	v99x8.ctrl[base]     = n & 0xff;
	v99x8.ctrl[base + 1] = n >> 8;
}

#define vcom_setsy(n) vcom_set(34, n)
#define vcom_setdy(n) vcom_set(38, n)
#define vcom_setny(n) vcom_set(42, n)

#define vcom_vram(x, y) (&v99x8.vram[((x) >> vcom.xshift) + ((y) << vcom.yshift)])


static int vcom_canonaddr(void)
{
	int ny;

	if (v99x8.ctrl[45] & 0x04)  /* Direction to left */
	{
		vcom.sx -= vcom.nx;
		vcom.dx -= vcom.nx;
	}
	vcom.sx = max(vcom.sx, 0);
	vcom.nx = min(vcom.nx, vcom.xmask + 1 - vcom.sx);
	vcom.dx = max(vcom.dx, 0);
	vcom.nx = min(vcom.nx, vcom.xmask + 1 - vcom.dx);

	ny = vcom.ny;
	if ((v99x8.ctrl[45] & 0x08) == 0)   /* Direction to down */
	{
		ny = min(ny, vcom.ymask + 1 - vcom.sy);
		ny = min(ny, vcom.ymask + 1 - vcom.dy);
	} else
	{
		ny = min(ny, vcom.sy + 1);
		ny = min(ny, vcom.dy + 1);
		ny = 0 - ny;
	}

//printf("can %d->%d *(%d,%d)\n", vcom.sx, vcom.dx, vcom.nx, ny);

	vcom.src = vcom_vram(vcom.sx, vcom.sy);
	vcom.dst = vcom_vram(vcom.dx, vcom.dy);
	vcom.nx >>= vcom.xshift;

	return ny;
}

static void vcom_hcopy(Uint8 *dst, Uint8 *src, int nx, int ny)
{
	if (ny < 0)
	{
		while (ny++ < 0)
		{
			memmove(dst, src, nx);
			src -= vcom.xbytes;
			dst -= vcom.xbytes;
		}
	} else
	{
		while (ny--)
		{
			memmove(dst, src, nx);
			src += vcom.xbytes;
			dst += vcom.xbytes;
		}
	}
}

static void ymmm(void)
{
	int n;

	vcom_getsx();
	vcom_getsy();
	vcom.dx = vcom.sx;
	vcom_getdy();
	vcom.nx = 512;
	vcom_getny();

//printf("ymmm: (%d,%d) %d*%d\n", vcom.sx, vcom.sy, vcom.dy, vcom.ny);
	n = vcom_canonaddr();
	vcom_hcopy(vcom.dst, vcom.src, vcom.nx, n);

	vcom_setsy(vcom.sy + n);
	vcom_setdy(vcom.dy + n);

	if (vcom.ny != abs(n))
		vcom_setny(vcom.ny - abs(n));
}

static void hmmm(void)
{
	int n;

	vcom_getsx();
	vcom_getsy();
	vcom_getdx();
	vcom_getdy();
	vcom_getnx();
	vcom_getny();

//printf("hmmm: (%d,%d)->(%d,%d) *(%d,%d)\n", vcom.sx, vcom.sy, vcom.dx, vcom.dy, vcom.nx, vcom.ny);
	n = vcom_canonaddr();
	vcom_hcopy(vcom.dst, vcom.src, vcom.nx, n);

	vcom_setsy(vcom.sy + n);
	vcom_setdy(vcom.dy + n);

	if (vcom.ny != abs(n))
		vcom_setny(vcom.ny - abs(n));
}

static void hmmv(void)
{
	int n, ny, clr;

	vcom.sx = 0;
	vcom.sy = 0;
	vcom_getdx();
	vcom_getdy();
	vcom_getnx();
	vcom_getny();
	clr = v99x8.ctrl[44];

	ny = n = vcom_canonaddr();
	if (n < 0)
	{
		while (ny++ < 0)
		{
			memset(vcom.dst, clr, vcom.nx);
			vcom.dst -= vcom.xbytes;
		}
	} else
	{
		while (ny--)
		{
			memset(vcom.dst, clr, vcom.nx);
			vcom.dst += vcom.xbytes;
		}
	}

	vcom_setsy(vcom.sy + n);
	vcom_setdy(vcom.dy + n);

	if (vcom.ny != abs(n))
		vcom_setny(vcom.ny - abs(n));
}

#define vcom_lset(dc, sc)                           \
switch(vcom.lop)                                    \
{                                                   \
	case 0x0: (dc)  = (sc); break;                  \
	case 0x1: (dc) &= (sc); break;                  \
	case 0x2: (dc) |= (sc); break;                  \
	case 0x3: (dc) ^= (sc); break;                  \
	case 0x4: (dc) =~ (sc); break;                  \
	case 0x8: if ((sc) != 0) (dc)  = (sc); break;   \
	case 0x9: if ((sc) != 0) (dc) &= (sc); break;   \
	case 0xa: if ((sc) != 0) (dc) |= (sc); break;   \
	case 0xb: if ((sc) != 0) (dc) ^= (sc); break;   \
	case 0xc: if ((sc) != 0) (dc) =~ (sc); break;   \
}

/*
	dc =  sc;
	dc &= sc;
	dc |= sc;
	dc ^= sc;
	dc =  ~sc;
*/

static void vcom_lmove(Uint8 *dst, Uint8 *src, int n)
{
#if 0
	if (v99x8.ctrl[45] & 0x04)  /* Direction to left */
#endif

	while(n>0)
	{
		vcom_lset(*dst, *src);
		++dst, ++src;
		--n;
	}
}

static void vcom_lcopy(Uint8 *dst, Uint8 *src, int nx, int ny)
{
	if (ny < 0)
	{
		while (ny++ < 0)
		{
			vcom_lmove(dst, src, nx);
			src -= vcom.xbytes;
			dst -= vcom.xbytes;
		}
	} else
	{
		while (ny--)
		{
			vcom_lmove(dst, src, nx);
			src += vcom.xbytes;
			dst += vcom.xbytes;
		}
	}
}


void vcom_lputc(int m)  // XXX 左方向、上方向のテストはしてません
{
	Uint8 *dst;
	int dot;

	dst = vcom_vram(r44.x, r44.y);

	switch (v99x8.scr)
	{
	case 5:
	case 7:
		m &= 0x0f;
		dot = (*dst >> ((1 - (r44.x & 1)) * 4)) & 0x0f;
		vcom_lset(dot, m);
		if ((r44.x & 1) == 0)
			*dst = (*dst & 0x0f) | (dot << 4);
		else
			*dst = (*dst & 0xf0) | dot;
		break;

	case 6:
		m &= 0x03;
		dot = (*dst >> ((1 - (r44.x & 3)) * 2)) & 0x03;
		vcom_lset(dot, m);
		switch (r44.x & 0x03)
		{
		case 0:
			*dst = (*dst & 0x3f) | (dot << 6);
			break;
		case 1:
			*dst = (*dst & 0xcf) | (dot << 4);
			break;
		case 2:
			*dst = (*dst & 0xf3) | (dot << 2);
			break;
		case 3:
			*dst = (*dst & 0xfc) | dot;
		}
		break;

	case 8:
		vcom_lset(*dst, m & 0xff);
		break;

	}

	if (r44.sx <= r44.ex)
	{
		if (++r44.x >= r44.ex)
		{
			r44.x = r44.sx;
			if (r44.sy < r44.ey)
			{
				if (++r44.y >= r44.ey)
					v99x8.status[2] &= ~0x01;
			} else
			{
				if (--r44.y < r44.ey)
					v99x8.status[2] &= ~0x01;
			}
		}
	} else
	{
		if (--r44.x < r44.ex)
		{
			r44.x = r44.sx;
			if (r44.sy <= r44.ey)
			{
				if (++r44.y > r44.ey)
					v99x8.status[2] &= ~0x01;
			} else
			{
				if (--r44.y < r44.ey)
					v99x8.status[2] &= ~0x01;
			}
		}
	}
}


void vcom_hputc(int m) // XXX 左方向、上方向のテストはしてません
{
	*vcom_vram(r44.x, r44.y) = m;

	if (r44.sx <= r44.ex)
	{
		r44.x += 1 << vcom.xshift;

		if (r44.x >= r44.ex)
		{
			r44.x = r44.sx;
			if (r44.sy <= r44.ey)
			{
				if ((++r44.y) >= r44.ey)
					v99x8.status[2] &= ~0x01;
			} else
			{
				if ((--r44.y) <= r44.ey)
					v99x8.status[2] &= ~0x01;
			}
		}
	} else
	{
		r44.x -= 1 << vcom.xshift;

		if (r44.x <= r44.ex)
		{
			r44.x = r44.sx;
			if (r44.sy <= r44.ey)
			{
				if ((++r44.y) >= r44.ey)
					v99x8.status[2] &= ~0x01;
			} else
			{
				if ((--r44.y) <= r44.ey)
					v99x8.status[2] &= ~0x01;
			}
		}
	}
}


void v99x8_cputovdp(int m)
{
	if ((v99x8.status[2] & 0x01) == 0)
		return;

	switch(v99x8.ctrl[46] >> 4)
	{
	case 0xb:
		vcom_lputc(m);
		break;
	case 0xf:
		vcom_hputc(m);
		break;
	}
}

static void lmmc(void)
{
	vcom_getdx();
	vcom_getdy();
	vcom_getnx();
	vcom_getny();


	r44.sx = r44.x = vcom.dx;
	r44.sy = r44.y = vcom.dy;
	r44.ex = vcom.dx + vcom.nx;
	r44.ey = vcom.dy + vcom.ny;

	vcom_lputc(v99x8.ctrl[44]);
}


static void vcom_lpset(int x,int y)
{
	Uint8 *dst;
	int src_dot, dst_dot;

	dst = vcom_vram(x, y);

	switch(v99x8.scr)
	{
	case 5:
	case 7:
		src_dot = v99x8.ctrl[44] & 0x0f;
		dst_dot = (*dst >> ((1 - (x & 1)) * 4)) & 0x0f;
		vcom_lset(dst_dot, src_dot);
		if ((x & 1) == 0)
			*dst = (*dst & 0x0f) | (dst_dot << 4);
		else
			*dst = (*dst & 0xf0) | dst_dot;
		break;

	case 6:
		src_dot = v99x8.ctrl[44] & 0x03;
		dst_dot = (*dst >> ((1 - (r44.x & 3)) * 2)) & 0x03;
		vcom_lset(dst_dot, src_dot);
		switch(x & 0x03)
		{
		case 0:
			*dst = (*dst & 0x3f) | (src_dot << 6);
			break;
		case 1:
			*dst = (*dst & 0xcf) | (src_dot << 4);
			break;
		case 2:
			*dst = (*dst & 0xf3) | (src_dot << 2);
			break;
		case 3:
			*dst = (*dst & 0xfc) | src_dot;
		}
		break;

	case 8:
		vcom_lset(*dst, v99x8.ctrl[44] & 0xff);
		break;
	}
}


static void line(void)
{
	int i, j, x, y;
	int maj, min;

	vcom_getdx();
	vcom_getdy();
	maj = (v99x8.ctrl[40] + (v99x8.ctrl[41] << 8)) & 1023;
	min = (v99x8.ctrl[42] + (v99x8.ctrl[43] << 8)) & 511;
	for (i = 0; i <= maj; i++)
	{
		j = (i * min) / maj;
		if (v99x8.ctrl[45] & 0x01)
		{
			y = vcom.dy + ((v99x8.ctrl[45] & 0x08) ? -i : i);
			x = vcom.dx + ((v99x8.ctrl[45] & 0x04) ? -j : j);
		} else
		{
			x = vcom.dx + ((v99x8.ctrl[45] & 0x04) ? -i : i);
			y = vcom.dy + ((v99x8.ctrl[45] & 0x08) ? -j : j);
		}
		vcom_lpset(x, y);
	}
}

static Uint8 vcom_point(int x, int y)
{
	Uint8 clr = *vcom_vram(x, y);

	switch (v99x8.scr)
	{
	case 5:
	case 7:
		clr = (clr >> (4 * (x & 1))) & 0x0F;
		break;
	case 6:
		clr = (clr >> (2 * (x & 3))) & 0x03;
		break;
	}

	return clr;
}

static void srch(void)
{
	int i;

	vcom_getsx();
	vcom_getsy();

	i = 0;
	v99x8.status[2] &= ~0x10;
	while ((0 <= (vcom.sx + i)) && ((vcom.sx + i) <= vcom.xmask))
	{
		Uint8 clr = vcom_point(vcom.sx + i, vcom.sy);
		if (v99x8.ctrl[45] & 0x02)
		{
			if (clr != v99x8.ctrl[44])
			{
				v99x8.status[2] |= 0x10;
				break;
			}
		} else
		{
			if (clr == v99x8.ctrl[44])
			{
				v99x8.status[2] |= 0x10;
				break;
			}
		}
		i = (v99x8.ctrl[45] & 0x04) ? (i - 1) : (i + 1);
	}

	if (v99x8.status[2] & 0x10)
	{
		v99x8.status[8] = (vcom.sx + i) & 0xff;
		v99x8.status[9] = (((vcom.sx + i) >> 8) & 0x03) | 0xfc;
	}
}


static void pset(void)
{
	vcom_getdx();
	vcom_getdy();
	vcom_lpset(vcom.dx, vcom.dy);
}


static void point(void)
{
	vcom_getsx();
	vcom_getsy();
	v99x8.status[7] = vcom_point(vcom.sx, vcom.sy);
}

static void hmmc(void)
{
	vcom_getdx();
	vcom_getdy();
	vcom_getnx();
	vcom_getny();


	r44.sx = r44.x = vcom.dx & ~1;
	r44.sy = r44.y = vcom.dy;
	r44.ex = (vcom.dx + vcom.nx) & ~1;
	r44.ey = vcom.dy + vcom.ny;
	r44.xsize = vcom.nx & ~1;

	vcom_hputc(v99x8.ctrl[44]);
}

static void lmmm(void)
{
	int n;

	vcom_getsx();
	vcom_getsy();
	vcom_getdx();
	vcom_getdy();
	vcom_getnx();
	vcom_getny();

//printf("hmmm: (%d,%d)->(%d,%d) *(%d,%d)\n", vcom.sx, vcom.sy, vcom.dx, vcom.dy, vcom.nx, vcom.ny);
	n = vcom_canonaddr();
	vcom_lcopy(vcom.dst, vcom.src, vcom.nx, n);

	vcom_setsy(vcom.sy + n);
	vcom_setdy(vcom.dy + n);

	if (vcom.ny != abs(n))
		vcom_setny(vcom.ny - abs(n));
}


static void vcom_lfill(Uint8 *p, int clr, int n)
{
	if (n == 0)     /* バイト数単位で処理をおこなうため */
		n = 1;      /* 1dotが無視されてしまう対応 */

	switch(vcom.lop)
	{
	case 0x8:
		if (clr == 0)
			break;
	case 0x0:
		while (n-- > 0)
			*p++ = clr;
		break;

	case 0x9:
		if (clr == 0)
			break;
	case 0x1:
		while (n-- > 0)
			*p++ &= clr;
		break;

	case 0xa:
		if (clr == 0)
			break;
	case 0x2:
		while (n-- > 0)
			*p++ |= clr;
		break;

	case 0xb:
		if (clr == 0)
			break;
	case 0x3:
		while (n-- > 0)
			*p++ ^= clr;
		break;

	case 0xc:
		if (clr == 0)
			break;
	case 0x4:
		while (n-- > 0)
			*p++ = ~clr;
		break;
	}

}

static void lmmv(void)   /* XXX byte 単位で処理してるのは手抜き(^^; */
{
	int n, ny, clr;

	vcom.sx = 0;
	vcom.sy = 0;
	vcom_getdx();
	vcom_getdy();
	vcom_getnx();
	vcom_getny();
	clr = v99x8.ctrl[44];
	switch (vcom.xshift)
	{
	case 2:
		clr |= clr << 2;
	case 1:
		clr |= clr << 4;
	}

	ny = n = vcom_canonaddr();
	if (n < 0)
	{
		while (ny++ < 0)
		{
			vcom_lfill(vcom.dst, clr, vcom.nx);
			vcom.dst -= vcom.xbytes;
		}
	} else
	{
		while (ny--)
		{
			vcom_lfill(vcom.dst, clr, vcom.nx);
			vcom.dst += vcom.xbytes;
		}
	}

	vcom_setsy(vcom.sy + n);
	vcom_setdy(vcom.dy + n);

	if (vcom.ny != abs(n))
		vcom_setny(vcom.ny - abs(n));
}


void v99x8_command(int m)
{
                   /* sc  0  1  2  3  4  5    6    7    8 */
	const int xsize[] =  {0, 0, 0, 0, 0, 256, 512, 512, 256};
	const int xshift[] = {0, 0, 0, 0, 0, 1,   2,   1,   0};

	if (v99x8.scr < 5 || v99x8.scr >8)
		return;

	vcom.xbytes = xsize[v99x8.scr] >> xshift[v99x8.scr];

	vcom.xmask  = xsize[v99x8.scr] - 1;
	vcom.xshift = xshift[v99x8.scr];

	vcom.ymask  = v99x8.pages * 0x4000 / vcom.xbytes - 1;
	vcom.yshift = getshift(vcom.xbytes - 1);

	v99x8.status[2] |= 0x01;

	vcom.lop = m & 0xf;

	switch(m >> 4)
	{
	case 0xf:	hmmc(); break;
	case 0xe:	ymmm(); break;
	case 0xd:	hmmm(); break;
	case 0xc:	hmmv(); break;
	case 0xb:	lmmc(); break;
//	case 0xa:	lmcm(); break;
	case 0x9:	lmmm(); break;
	case 0x8:	lmmv(); break;
	case 0x7:	line(); break;
	case 0x6:	srch(); break;
	case 0x5:	pset(); break;
	case 0x4:	point(); break;
//	case 0x0:	stop(); break;
	}

	v99x8.ctrl[46] &= 0x0f;
	if ((m >> 4) != 0xb && (m >> 4) != 0xf)
		v99x8.status[2] &= ~0x01;
}

