/*
 *   original: ste (Z80/R800 Simulator)  (C)2000 HRA!
 *
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

#define Z80_FASTMEM_USE
#include "../zodiac.h"
#include "z80.h"
#include "z80_internal.h"


static const int cycles_cb[256] =
{
	 10, 10, 10, 10, 10, 10, 17, 10,  10, 10, 10, 10, 10, 10, 17, 10,
	 10, 10, 10, 10, 10, 10, 17, 10,  10, 10, 10, 10, 10, 10, 17, 10,
	 10, 10, 10, 10, 10, 10, 17, 10,  10, 10, 10, 10, 10, 10, 17, 10,
	 10, 10, 10, 10, 10, 10, 17, 10,  10, 10, 10, 10, 10, 10, 17, 10,

	 10, 10, 10, 10, 10, 10, 14, 10,  10, 10, 10, 10, 10, 10, 14, 10,
	 10, 10, 10, 10, 10, 10, 14, 10,  10, 10, 10, 10, 10, 10, 14, 10,
	 10, 10, 10, 10, 10, 10, 14, 10,  10, 10, 10, 10, 10, 10, 14, 10,
	 10, 10, 10, 10, 10, 10, 14, 10,  10, 10, 10, 10, 10, 10, 14, 10,

	 10, 10, 10, 10, 10, 10, 17, 10,  10, 10, 10, 10, 10, 10, 17, 10,
	 10, 10, 10, 10, 10, 10, 17, 10,  10, 10, 10, 10, 10, 10, 17, 10,
	 10, 10, 10, 10, 10, 10, 17, 10,  10, 10, 10, 10, 10, 10, 17, 10,
	 10, 10, 10, 10, 10, 10, 17, 10,  10, 10, 10, 10, 10, 10, 17, 10,

	 10, 10, 10, 10, 10, 10, 17, 10,  10, 10, 10, 10, 10, 10, 17, 10,
	 10, 10, 10, 10, 10, 10, 17, 10,  10, 10, 10, 10, 10, 10, 17, 10,
	 10, 10, 10, 10, 10, 10, 17, 10,  10, 10, 10, 10, 10, 10, 17, 10,
	 10, 10, 10, 10, 10, 10, 17, 10,  10, 10, 10, 10, 10, 10, 17, 10,
};

static const int cycles_xxcb[256] =
{
	  0,  0,  0,  0,  0,  0, 25,  0,   0,  0,  0,  0,  0,  0, 25,  0,
	  0,  0,  0,  0,  0,  0, 25,  0,   0,  0,  0,  0,  0,  0, 25,  0,
	  0,  0,  0,  0,  0,  0, 25,  0,   0,  0,  0,  0,  0,  0, 25,  0,
	  0,  0,  0,  0,  0,  0, 25,  0,   0,  0,  0,  0,  0,  0, 25,  0,

	  0,  0,  0,  0,  0,  0, 22,  0,   0,  0,  0,  0,  0,  0, 22,  0,
	  0,  0,  0,  0,  0,  0, 22,  0,   0,  0,  0,  0,  0,  0, 22,  0,
	  0,  0,  0,  0,  0,  0, 22,  0,   0,  0,  0,  0,  0,  0, 22,  0,
	  0,  0,  0,  0,  0,  0, 22,  0,   0,  0,  0,  0,  0,  0, 22,  0,

	  0,  0,  0,  0,  0,  0, 25,  0,   0,  0,  0,  0,  0,  0, 25,  0,
	  0,  0,  0,  0,  0,  0, 25,  0,   0,  0,  0,  0,  0,  0, 25,  0,
	  0,  0,  0,  0,  0,  0, 25,  0,   0,  0,  0,  0,  0,  0, 25,  0,
	  0,  0,  0,  0,  0,  0, 25,  0,   0,  0,  0,  0,  0,  0, 25,  0,

	  0,  0,  0,  0,  0,  0, 25,  0,   0,  0,  0,  0,  0,  0, 25,  0,
	  0,  0,  0,  0,  0,  0, 25,  0,   0,  0,  0,  0,  0,  0, 25,  0,
	  0,  0,  0,  0,  0,  0, 25,  0,   0,  0,  0,  0,  0,  0, 25,  0,
	  0,  0,  0,  0,  0,  0, 25,  0,   0,  0,  0,  0,  0,  0, 25,  0,
};


static void Z80_CB(int a, Uint8 *r)
{
	int b = (a >> 3) & 7;

//printf("cb %02x\n", a);

	switch((a >> 6) & 3)
	{
	case 1:	/* BIT	b, r */
		Z80_F = (Z80_F & ~(Z80_NF | Z80_ZF)) | Z80_HF
		       | ((*r & (1 << b)) ? 0 : Z80_ZF);
		break;

	case 2:	/* RES	b, r */
		*r &= ~(1 << b);
		break;

	case 3:	/* SET	b, r */
		*r |= 1 << b;
		break;

	case 0:
		switch(b)
		{
		case 0:	/* RLC	r */
			*r = (*r << 1) | (*r >> 7);
			Z80_F = Z80_PZS[*r] | (*r & Z80_CF);
			break;

		case 1:	/* RRC	r */
			Z80_F = *r&Z80_CF;
			*r = (*r >> 1) | (*r << 7);
			Z80_F |= Z80_PZS[*r];
			break;

		case 2:	/* RL	r */
			{
				int 	n;

				n = *r >> 7;
				*r = (*r << 1) | (Z80_F & Z80_CF);
				Z80_F = Z80_PZS[*r] | n;
			}
			break;

		case 3:	/* RR	r */
			{
				int 	n;

				n = *r & Z80_CF;
				*r = (*r >> 1) | (Z80_F << 7);
				Z80_F = Z80_PZS[*r] | n;
			}
			break;

		case 4:	/* SLA	r */
			Z80_F = *r >> 7;
			*r <<= 1;
			Z80_F |= Z80_PZS[*r];
			break;

		case 5:	/* SRA	r */
			Z80_F = *r & Z80_CF;
			*r = (*r >> 1) | (*r & 0x80);
			Z80_F |= Z80_PZS[*r];
			break;

		case 6:	/* SLL	r  undefined operation */
			Z80_F = *r>>7;
			*r = (*r << 1) | (*r & 0x01);
			Z80_F |= Z80_PZS[*r];
			break;

		case 7:	/* SRL	r */
			Z80_F = *r & Z80_CF;
			*r >>= 1;
			Z80_F |= Z80_PZS[*r];
			break;
		}
		break;
	}
}


void z80_cycle_cb(void)
{
	int a;
	Uint8 xhl, *r, *reg[]={&Z80_B, &Z80_C, &Z80_D, &Z80_E,
	                       &Z80_H, &Z80_L, NULL, &Z80_A};

	a = z80_rdmem(Z80_PC++);
    z80.cycle -= cycles_cb[a];

//printf("cb %02x\n",a);

	r = reg[a & 7];
	if ((a & 7) == 6)
	{
		r = &xhl;
		xhl = z80_rdmem(Z80_HL);
	}

	Z80_CB(a, r);

	if ((a & 7) == 6)
		z80_wrmem(Z80_HL, xhl);
}

void z80_cycle_xxcb(Uint16 xx)
{
	int a;
	Uint8 xhl;

	a = z80_rdmem(Z80_PC++);
    z80.cycle -= cycles_xxcb[a];

//printf("xx cb %02x:  ",a);

//	if ((a & 7) != 6)

	xhl = z80_rdmem(xx);

	Z80_CB(a, &xhl);

	z80_wrmem(xx, xhl);
}

