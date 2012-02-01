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


/* XXX */
static const int cycles_xx[256] =
{
	  0,  0,  0,  0,  0,  0,  0,  0,   0, 17,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,   0, 17,  0,  0,  0,  0,  0,  0,
	  0, 16, 22, 12, 11, 11, 11,  0,   0, 17, 22, 12, 11, 11, 11,  0,
	  0,  0,  0,  0, 25, 25, 21,  0,   0, 17,  0,  0,  0,  0,  0,  0,

	  0,  0,  0,  0, 11, 11, 21,  0,   0,  0,  0,  0, 11, 11, 21,  0,
	  0,  0,  0,  0, 11, 11, 21,  0,   0,  0,  0,  0, 11, 11, 21,  0,
	 11, 11, 11, 11, 11, 11, 11, 11,  11, 11, 11, 11, 11, 11, 11, 11,
	 21, 21, 21, 21, 21, 21, 21, 21,   0,  0,  0,  0, 11, 11, 21,  0,

	  0,  0,  0,  0, 11, 11, 21,  0,   0,  0,  0,  0, 11, 11, 21,  0,
	  0,  0,  0,  0, 11, 11, 21,  0,   0,  0,  0,  0, 11, 11, 21,  0,
	  0,  0,  0,  0, 11, 11, 21,  0,   0,  0,  0,  0, 11, 11, 21,  0,
	  0,  0,  0,  0, 11, 11, 21,  0,   0,  0,  0,  0, 11, 11, 21,  0,

	  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
      0, 16,  0, 25,  0, 17,  0,  0,   0, 10,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,   0, 12,  0,  0,  0,  0,  0,  0,
};


#define	xxdisp() (xx->w + (Sint8)z80_rdmem(Z80_PC++))


void z80_cycle_xx(pair16l_t *xx)
{
	int n;

	n = z80_rdmem(Z80_PC++);
	z80.cycle -= cycles_xx[n];

//printf("xx %02x\n", n);

	switch (n)
	{

/* XXX */
default:
//printf("%02x\n", n);
	--Z80_PC;

	break;

case 0xfd:
case 0xdd:
	--Z80_PC;
	break;

case 0xcb:
{
	z80_cycle_xxcb(xxdisp());
}
	break;


case 0x09:	/* ADD	xx, BC */
{
	Z80_ADD16(xx->w, Z80_BC);
}
	break;


case 0x19:	/* ADD	xx, DE */
{
	Z80_ADD16(xx->w, Z80_DE);
}
	break;


case 0x21:	/* LD	xx, imm */
{
	xx->w = z80_rdmem2(Z80_PC);
	Z80_PC += 2;
}
	break;

case 0x22:	/* LD	(imm), xx */
{
	z80_wrmem2(z80_rdmem2(Z80_PC), xx->w);
	Z80_PC += 2;
}
	break;

case 0x23:	/* INC	xx */
{
	++xx->w;
}
	break;

case 0x24:	/* INC	xxH */
{
	Z80_INC8(xx->b.h);
}
	break;

case 0x25:	/* DEC	xxH */
{
	Z80_DEC8(xx->b.h);
}
	break;

case 0x26:	/* LD	xxH, imm */
{
	xx->b.h = z80_rdmem(Z80_PC++);
}
	break;


case 0x29:	/* ADD	xx, xx */
{
	Z80_ADD16(xx->w, xx->w);
}
	break;

case 0x2a:	/* LD	xx, (nn) */
{
	xx->w = z80_rdmem2(z80_rdmem2(Z80_PC));
	Z80_PC += 2;
}
	break;

case 0x2b:	/* DEC	xx */
{
	--xx->w;
}
	break;

case 0x2c:	/* INC	xxL */
{
	Z80_INC8(xx->b.l);
}
	break;

case 0x2d:	/* DEC	xxL */
{
	Z80_DEC8(xx->b.l);
}
	break;

case 0x2e:	/* LD	xxL, imm */
{
	xx->b.l = z80_rdmem(Z80_PC++);
}

	break;

case 0x34:	/* INC	(xx) */
{
	int a, xhl;

	xhl = xxdisp();
	a = z80_rdmem(xhl);
	Z80_INC8(a);
	z80_wrmem(xhl, a);
}
	break;

case 0x35:	/* DEC	(xx) */
{
	int a, xhl;

	xhl = xxdisp();
	a = z80_rdmem(xhl);
	Z80_DEC8(a);
	z80_wrmem(xhl, a);
}
	break;

case 0x36:	/* LD	(xx+d), imm */
{
	int xhl;

	xhl = xxdisp();
	z80_wrmem(xhl, z80_rdmem(Z80_PC++));
}
	break;

case 0x39:	/* ADD	xx, SP */
{
	Z80_ADD16(xx->w, Z80_SP);
}
	break;

case 0x44:	/* LD	B, xxH */
{
	Z80_B = xx->b.h;
}

	break;

case 0x45:	/* LD	B, xxL */
{
	Z80_B = xx->b.l;
}

	break;

case 0x46:	/* LD	B, (xx) */
{
	Z80_B = z80_rdmem(xxdisp());
}
	break;

case 0x4c:	/* LD	C, xxH */
{
	Z80_C = xx->b.h;
}

	break;

case 0x4d:	/* LD	C, xxL */
{
	Z80_C = xx->b.l;
}

	break;

case 0x4e:	/* LD	C, (xx) */
{
	Z80_C = z80_rdmem(xxdisp());
}
	break;

case 0x54:	/* LD	D, xxH */
{
	Z80_D = xx->b.h;
}

	break;

case 0x55:	/* LD	D, xxL */
{
	Z80_D = xx->b.l;
}

	break;

case 0x56:	/* LD	D, (xx) */
{
	Z80_D = z80_rdmem(xxdisp());
}
	break;

case 0x5c:	/* LD	E, xxH */
{
	Z80_E = xx->b.h;
}

	break;

case 0x5d:	/* LD	E, xxL */
{
	Z80_E = xx->b.l;
}

	break;

case 0x5e:	/* LD	E, (xx) */
{
	Z80_E = z80_rdmem(xxdisp());
}
	break;

case 0x60:	/* LD	xxH, B */
{
	xx->b.h = Z80_B;
}

	break;

case 0x61:	/* LD	xxH, C */
{
	xx->b.h = Z80_C;
}

	break;

case 0x62:	/* LD	xxH, D */
{
	xx->b.h = Z80_D;
}

	break;

case 0x63:	/* LD	xxH, E */
{
	xx->b.h = Z80_E;
}

	break;

case 0x64:	/* LD	xxH, xxH */
{
}

	break;

case 0x65:	/* LD	xxH, xxL */
{
	xx->b.h = xx->b.l;
}

	break;

case 0x66:	/* LD	H, (xx) */
{
	Z80_H = z80_rdmem(xxdisp());
}
	break;

case 0x67:	/* LD	xxH, A */
{
	xx->b.h = Z80_A;
}

	break;



case 0x68:	/* LD	xxL, B */
{
	xx->b.l = Z80_B;
}

	break;

case 0x69:	/* LD	xxL, C */
{
	xx->b.l = Z80_C;
}

	break;

case 0x6a:	/* LD	xxL, D */
{
	xx->b.l = Z80_D;
}

	break;

case 0x6b:	/* LD	xxL, E */
{
	xx->b.l = Z80_E;
}

	break;

case 0x6c:	/* LD	xxL, xxH */
{
	xx->b.l = xx->b.h;
}

	break;

case 0x6d:	/* LD	xxL, xxL */
{
}

	break;

case 0x6e:	/* LD	L, (xx) */
{
	Z80_L = z80_rdmem(xxdisp());
}
	break;

case 0x6f:	/* LD	xxL, A */
{
	xx->b.l = Z80_A;
}

	break;

case 0x70:	/* LD	(xx), B */
{
	z80_wrmem(xxdisp(), Z80_B);
}
	break;

case 0x71:	/* LD	(xx), C */
{
	z80_wrmem(xxdisp(), Z80_C);
}
	break;

case 0x72:	/* LD	(xx), D */
{
	z80_wrmem(xxdisp(), Z80_D);
}
	break;

case 0x73:	/* LD	(xx), E */
{
	z80_wrmem(xxdisp(), Z80_E);
}
	break;

case 0x74:	/* LD	(xx), H */
{
	z80_wrmem(xxdisp(), Z80_H);
}
	break;

case 0x75:	/* LD	(xx), L */
{
	z80_wrmem(xxdisp(), Z80_L);
}
	break;

case 0x77:	/* LD	(xx), A */
{
	z80_wrmem(xxdisp(), Z80_A);
}
	break;

case 0x7c:	/* LD	A, xxH */
{
	Z80_A = xx->b.h;
}

	break;

case 0x7d:	/* LD	A, xxL */
{
	Z80_A = xx->b.l;
}

	break;
case 0x7e:	/* LD	A, (xx) */
{
	Z80_A = z80_rdmem(xxdisp());
}
	break;


case 0x84:	/* ADD	A, xxH */
{
	Z80_ADD8(xx->b.h, 0);
}
	break;

case 0x85:	/* ADD	A, xxL */
{
	Z80_ADD8(xx->b.l, 0);
}
	break;

case 0x86:	/* ADD	A, (xx+d) */
{
	Z80_ADD8(z80_rdmem(xxdisp()), 0);
}
	break;

case 0x8c:	/* ADC	A, xxH */
{
	Z80_ADD8(xx->b.h, Z80_F & Z80_CF);
}
	break;

case 0x8d:	/* ADC	A, xxL */
{
	Z80_ADD8(xx->b.l, Z80_F & Z80_CF);
}
	break;

case 0x8e:	/* ADC	A, (xx+d) */
{
	Z80_ADD8(z80_rdmem(xxdisp()), Z80_F & Z80_CF);
}
	break;

case 0x94:	/* SUB	A, xxH */
{
	Z80_SUB8(xx->b.h, 0);
}
	break;

case 0x95:	/* SUB	A, xxL */
{
	Z80_SUB8(xx->b.l, 0);
}
	break;

case 0x96:	/* SUB	A, (xx) */
{
	Z80_SUB8(z80_rdmem(xxdisp()), 0);
}
	break;

case 0x9c:	/* SBC	A, xxH */
{
	Z80_SUB8(xx->b.h, Z80_F & Z80_CF);
}
	break;

case 0x9d:	/* SBC	A, xxL */
{
	Z80_SUB8(xx->b.l, Z80_F & Z80_CF);
}
	break;

case 0x9e:	/* SBC	A, (xx) */
{
	Z80_SUB8(z80_rdmem(xxdisp()), Z80_F & Z80_CF);
}
	break;

case 0xa4:	/* AND	A, xxH */
{
	Z80_AND8(xx->b.h);
}
	break;

case 0xa5:	/* AND	A, xxL */
{
	Z80_AND8(xx->b.l);
}
	break;

case 0xa6:	/* AND	A, (xx) */
{
	Z80_AND8(z80_rdmem(xxdisp()));
}
	break;

case 0xac:	/* XOR	A, xxH */
{
	Z80_XOR8(xx->b.h);
}
	break;

case 0xad:	/* XOR	A, xxL */
{
	Z80_XOR8(xx->b.l);
}
	break;

case 0xae:	/* XOR	A, (xx) */
{
	Z80_XOR8(z80_rdmem(xxdisp()));
}
	break;

case 0xb4:	/* OR	A, xxH */
{
	Z80_OR8(xx->b.h);
}
	break;

case 0xb5:	/* OR	A, xxL */
{
	Z80_OR8(xx->b.l);
}
	break;

case 0xb6:	/* OR	A, (xx) */
{
	Z80_OR8(z80_rdmem(xxdisp()));
}
	break;

case 0xbc:	/* CP	A, xxH */
{
	Z80_CP8(xx->b.h);
}
	break;

case 0xbd:	/* CP	A, xxL */
{
	Z80_CP8(xx->b.l);
}
	break;

case 0xbe:	/* CP	A, (xx) */
{
	Z80_CP8(z80_rdmem(xxdisp()));
}
	break;

case 0xe1:	/* POP	xx */
{
	xx->w = Z80_POP();
}
	break;

case 0xe3:	/*	EX	(SP), xx */
{
	int a;

	a = z80_rdmem2(Z80_SP);
	z80_wrmem2(Z80_SP, xx->w);
	xx->w = a;
}
	break;

case 0xe5:	/* PUSH	xx */
{
	Z80_PUSH(*xx);
}
	break;

case 0xe9:	/* JP	(xx) */
{
	Z80_PC = xx->w;
}
	break;

case 0xf9:	/* LD	SP, xx */
{
	Z80_SP = xx->w;
}
	break;

		}
}
