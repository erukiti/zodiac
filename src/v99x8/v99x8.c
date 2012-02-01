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

#include "../zodiac.h"
#include "v99x8.h"

#include "z80.h"	// XXX interrupt


static int vram_addr = 0;
static int vram_page = 0;

static bool f_out3 = FALSE;

bool f_scr = TRUE;
static bool f_mode = TRUE;

extern void v99x8_command(int m);	// v99x8_internal.h ??
extern void v99x8_cputovdp(int m);



/*
0: 000, 10 xx TMS 
x: 010, 10 xx     

1: 000, 00 xx TMS 
2: 001, 00 xx TMS 
3: 000, 01 xx TMS 
4: 010, 00 xx     

5: 011, 00 xx     1 
6: 100, 00 xx     2 
7: 101, 00 xx     1 
8: 111, 00 xx     0 

a: 111, 00 11     0
c: 111, 00 01     0

*/


/*
normal, sc7/8

00000h: 00000h
00001h: 10000h
00002h: 00001h
00003h: 10000h
0fffeh: 07fffh
0ffffh: 17fffh
10000h: 08000h
10001h: 08001h

*/

static void v99x8_interleave(void)
{
	static Uint8 *vram = NULL;
	Uint8 *p;
	size_t size;

	size = v99x8.pages * 0x4000;

	if (vram == NULL)
		vram = (Uint8 *)mem_alloc(size);

	p = vram;
	vram = v99x8.vram;
	v99x8.vram = p;

	if (v99x8.f_interleave)
	{
		int a, b, c;

		a = 0;
		b = size / 2;
		c = 0;

		while (c < size)
		{
			v99x8.vram[c]     = vram[a++];
			v99x8.vram[c + 1] = vram[b++];
			c += 2;
		}
	} else
	{
		int a, b, c;

		a = 0;
		b = size / 2;
		c = 0;

		while (c < size)
		{
			v99x8.vram[a++] = vram[c];
			v99x8.vram[b++] = vram[c + 1];
			c += 2;
		}
	}
}

struct scr
{
	bool f_tms;
	bool f_interleave;
};

static void v99x8_update(void)
{
/*
	struct scr scr[]=
	{
		{TRUE, FALSE},
		
	}
*/

	bool f;

	if (!f_mode)
		return;

	f = v99x8.f_interleave;

	switch(((v99x8.ctrl[0] & 0x0e) >> 1) | (v99x8.ctrl[1] & 0x18))
	{
case 0x10: v99x8.scr = 0; v99x8.f_tms = TRUE; v99x8.f_interleave = FALSE; break;
case 0x12: v99x8.scr = 9; v99x8.f_tms = FALSE; v99x8.f_interleave = FALSE; break;

case 0x00: v99x8.scr = 1; v99x8.f_tms = TRUE; v99x8.f_interleave = FALSE; break;
case 0x01: v99x8.scr = 2; v99x8.f_tms = TRUE; v99x8.f_interleave = FALSE; break;
case 0x08: v99x8.scr = 3; v99x8.f_tms = TRUE; v99x8.f_interleave = FALSE; break;

case 0x02: v99x8.scr = 4; v99x8.f_tms = FALSE; v99x8.f_interleave = FALSE; break;
case 0x03: v99x8.scr = 5; v99x8.f_tms = FALSE; v99x8.f_interleave = FALSE; break;
case 0x04: v99x8.scr = 6; v99x8.f_tms = FALSE; v99x8.f_interleave = FALSE; break;
case 0x05: v99x8.scr = 7; v99x8.f_tms = FALSE; v99x8.f_interleave = TRUE; break;
case 0x07: v99x8.scr = 8; v99x8.f_tms = FALSE; v99x8.f_interleave = TRUE; break;
	}

	if (f != v99x8.f_interleave)
		v99x8_interleave();

	f_mode = FALSE;
}






void v99x8_ctrl(int n, Uint8 m)
{
//printf ("v99x8_ctrl %2d <= %02x\n", n, m);

	if (n >= V99X8_NREG)
		n = V99X8_NREG - 1;


	switch(n)
	{
	case 0:
//		if ((m & 0x0e) != (v99x8.ctrl[0] & 0x0e))

		if (((m ^ v99x8.ctrl[0]) & 0x0e) != 0)
		{
			f_mode = TRUE;
			f_scr = TRUE;
		}
		break;

	case 1:
		if (((m ^ v99x8.ctrl[1]) & 0x18) != 0)
		{
			f_mode = TRUE;
			f_scr = TRUE;
		}
		break;

	case 2:
	case 3:
	case 4:
	case 10:
		f_scr = TRUE;
		break;

	case 7:
		v99x8.col_fg = m >> 4;
		v99x8.col_bg = m & 0x0f;
		break;

	case 14:
		m &= v99x8.pages - 1;
		vram_page = (int)m << 14;
		break;

	case 15:
		if (m >= V99X8_NSTAT)
		m = V99X8_NSTAT - 1;
		break;

	case 16:
		m &= 0x0f;
		break;

	case 17:
		f_out3 = !(m & 0x80);
		m &= 0x3f;
		break;

    case 44: 
    	v99x8_update();
		v99x8_cputovdp(m);
    	break;
    case 46: 
    	v99x8_update();
		v99x8_command(m);
    	break;

    /* XXX

    */
    case 6:
	m &= 0x3f;
	break;

    case 11:
	m &= 0x03;
	break;
	}

	v99x8.ctrl[n] = m;
}


static void vram_incaddr(void)
{
	vram_addr = (vram_addr + 1) & 0x3fff;
	if (vram_addr == 0 && !v99x8.f_tms)
		v99x8_ctrl(14, v99x8.ctrl[14] + 1);
}

Uint8 vram_read(int addr)
{
	return v99x8.vram[addr];
}

void vram_write(int addr, Uint8 n)
{
	v99x8.vram[addr] = n;
}


Uint8 v99x8_in_0(void)	/* VRAM read */
{
	int n;

	v99x8_update();

	n = vram_read(vram_addr + vram_page);
	vram_incaddr();
	return n;
}

void v99x8_out_0(Uint8 n)	// VRAM write
{
	v99x8_update();

	vram_write(vram_addr + vram_page, n);
	vram_incaddr();
}

Uint8 v99x8_in_1(void)	// status in
{
	int n;
	int a, b;

	v99x8_update();

	n = v99x8.status[v99x8.ctrl[15]];

//if (z80.ivec!=Z80_NOINT)
//	{
//printf("* IFF:%d H:%d V:%d\n", z80.IFF&1, (v99x8.ctrl[0]&0x10), (v99x8.ctrl[1]&0x20));
//	}
//	z80_intreq(Z80_NOINT);
//	VKey=1;

	switch(v99x8.ctrl[15])
	{
	case 0:
		v99x8.status[0] &= ~0xa0;
		break;
	case 1:
		v99x8.status[1] &= ~0x01;
		break;
	case 7:
//		v99x8.status[7] = v99x8.ctrl[44] = v99x8_vdptocpu();
		break;

	case 2:
		context_timelock();
		a = context.hz / 60 / 262;
		b = (context.time_cycle % a) * 100 / a;
		context_timeunlock();

		if (b > 73)
			n |= 0x20;
		else
			n &= ~0x20;
		break;
	}

	return n;
}

void	v99x8_out_1(Uint8 n)	// ctrl out
{
	static int latch = -1;

	if (latch == -1)
	{
		latch = n;
	} else
	{
		if (n & 0x80)
		{
			if ((n & 0x40) == 0)
				v99x8_ctrl(n & 0x3f, latch);
		} else
		{
// ??? read/write の区別
			vram_addr = ((int)(n & 0x3f) << 8) + latch;
		}
		latch = -1;
	}
}

void v99x8_out_2(Uint8 n)	// palette out
{
	static int latch = -1;

	if (latch == -1)
	{
		latch = n;
	} else
	{
		int a;

		a = v99x8.ctrl[16];
		v99x8_pallete_set(a, (latch & 0x70) << 1, (n & 0x07) << 5, (latch & 0x07) << 5);
		v99x8_ctrl(16, a + 1);

		latch = -1;
	}
}

void v99x8_out_3(Uint8 n)	// ctrl out
{
	if (v99x8.ctrl[17] != 17)
		v99x8_ctrl(v99x8.ctrl[17], n);

	if (f_out3)
		v99x8_ctrl(17, v99x8.ctrl[17] + 1);
}

void v99x8_init(void)
{
	int i;

	static Uint8 inipal[16][3] =
	{
		{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x20, 0xc0, 0x20},
		{0x60, 0xE0, 0x60},	{0x20, 0x20, 0xe0}, {0x40, 0x60, 0xe0},
		{0xa0, 0x20, 0x20}, {0x40, 0xC0, 0xE0}, {0xe0, 0x20, 0x20},
		{0xe0, 0x60, 0x60}, {0xc0, 0xc0, 0x20}, {0xC0, 0xC0, 0x80},
		{0x20, 0x80, 0x20}, {0xC0, 0x40, 0xa0}, {0xa0, 0xa0, 0xa0},
		{0xE0, 0xE0, 0xE0}
	};

	v99x8.n_scanlines = 262;
	v99x8.pages = 8;

	memset(v99x8.ctrl, 0, sizeof(v99x8.ctrl));
	memset(v99x8.status, 0, sizeof(v99x8.status));
	v99x8.status[0] = 0x9f;
	v99x8.status[2] = 0x4c;

//	v99x8.f_zoom = FALSE;

	v99x8.vram = (Uint8 *)mem_alloc(v99x8.pages * 0x4000);
	memset(v99x8.vram, 0xff, v99x8.pages * 0x4000);

	v99x8.scr = 1;

	v99x8.col_fg = 0;
	v99x8.col_bg = 0;
	v99x8.f_tms = TRUE;


	v99x8_refresh_init();

	for(i = 0; i < 16; i++)
		v99x8_pallete_set(i, inipal[i][0], inipal[i][1], inipal[i][2]);

}


int v99x8_hsync(void)
{
	static bool flag_frame = FALSE;
	int a, line;

	if (v99x8.scanline < 226)
	{
		if (flag_frame)
		{
			line = v99x8.scanline + (((v99x8.ctrl[18] >> 4) + 8) & 0x0f) - 8 - 7;
			if (v99x8.ctrl[9] & 0x80)
				a = 212; else
				a = 192, line -= 10;

			if (v99x8.ctrl[1] & 0x40 && line >= 0 && line < a)
			{
				v99x8_update();

				switch(v99x8.scr)
				{
				case 0:	v99x8_refresh_sc0(line, 1); break;
				case 1:	v99x8_refresh_sc1(line, 1); break;
				case 2:	v99x8_refresh_sc4(line, 1); break;
				case 3:	v99x8_refresh_sc3(line, 1); break;
				case 4:	v99x8_refresh_sc4(line, 1); break;
				case 5:	v99x8_refresh_sc5(line, 1); break;
				case 6:	v99x8_refresh_sc6(line, 1); break;
				case 7:	v99x8_refresh_sc7(line, 1); break;
				case 8:	
					if (v99x8.ctrl[25] == 8)
						v99x8_refresh_scc(line, 1);
					else
						v99x8_refresh_sc8(line, 1);
					break;
				case 9:	v99x8_refresh_scx(line, 1); break;
				}
			}
		}

		if (((v99x8.scanline + v99x8.ctrl[23] 
		    - ((v99x8.ctrl[9] & 0x80) ? 8 : 18)) & 0xff) == v99x8.ctrl[19])
		{
			if (v99x8.ctrl[0] & 0x10)
			{
				v99x8.status[1] |= 0x01; /* H-sync */
				z80_intreq(Z80_INT);
			}
		} else
		{
			if (!(v99x8.ctrl[0] & 0x10))
				v99x8.status[1] &= ~0x01;   /* ?? H-sync off*/
		}
	} else
	{
		switch(v99x8.scanline)
		{
		case 234:
			if (flag_frame)
				v99x8_refresh_screen();

			flag_frame = md_refresh_sync();

			v99x8.status[2] |= 0x40;    /* VBlank on */

			v99x8.status[1] &= ~0x01;   /* ?? H-sync off*/
			z80_intreq(Z80_NOINT);      /* ?? H-sync を clear */

// XXX sprite check
			break;

		case 237:
//			v99x8.status[1] &= ~0x01;   /* ?? H-sync off*/
			if (v99x8.ctrl[1] & 0x20)
			{
				v99x8.status[0] |= 0x80;    /* V-sync int */
				z80_intreq(Z80_INT);
			}
			break;

		case 261:
			v99x8.status[2] &= ~0x40;   /* VBlank off */
			v99x8.status[0] &= ~0x40;   /* 5sprit off */
			v99x8.status[0] &= ~0x80;   /* Vsync off */
			z80_intreq(Z80_NOINT);      /* ?? V-sync を clear */

			if (flag_frame)
				v99x8_refresh_clear();
		}


/* NTSC timing

  3/  3: sync signal
 13/ 13: top erase
 26/ 16: top border
192/212: line
 25/ 15: bottom border

  3/  3: bottom erase

*/
	}
	return v99x8.scanline = (v99x8.scanline + 1) % v99x8.n_scanlines;
}



