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
static const int cycles_ed[256] =
{
	  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,

	 14, 14, 17, 22, 10, 16, 10, 11,  14, 14, 17, 22,  0, 16,  0, 11,
	 14, 14, 17, 22,  0,  0, 10, 11,  14, 14, 17, 22,  0,  0, 10, 11,
	 14, 14, 17, 22,  0,  0,  0, 20,  17, 22, 17, 22,  0,  0,  0, 20,
	 14,  0, 17, 22,  0,  0,  0,  0,  14, 14, 17, 22,  0,  0,  0,  0,

	  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
      8, 18, 18, 18,  0,  0,  0,  0,  18, 18, 18, 18, 18,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,

	  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
};

#define CYCLES_REPEAT 23
#define CYCLES_REPEAT_F 5;      /* 23-5 */


static void Z80_INI(void)
{
	z80_wrmem(Z80_HL++, z80_in(Z80_C));
	--Z80_B;
	Z80_F |= Z80_NF;
	if (Z80_B == 0)
		Z80_F |= Z80_ZF;
	else
		Z80_F &= ~Z80_ZF;
}

static void Z80_IND(void)
{
	z80_wrmem(Z80_HL--, z80_in(Z80_C));
	--Z80_B;
	Z80_F |= Z80_NF;
	if (Z80_B == 0)
		Z80_F |= Z80_ZF;
	else
		Z80_F &= ~Z80_ZF;
}

static void Z80_OUTI(void)
{
	z80_out(Z80_C, z80_rdmem(Z80_HL++));
	--Z80_B;
	Z80_F |= Z80_NF;
	if (Z80_B == 0)
		Z80_F |= Z80_ZF;
	else
		Z80_F &= ~Z80_ZF;
}

static void Z80_OUTD(void)
{
	z80_out(Z80_C, z80_rdmem(Z80_HL--));
	--Z80_B;
	Z80_F |= Z80_NF;
	if (Z80_B == 0)
		Z80_F |= Z80_ZF;
	else
		Z80_F &= ~Z80_ZF;
}

static void Z80_LDI(void)
{
	z80_wrmem(Z80_DE++, z80_rdmem(Z80_HL++));
	--Z80_BC;
	Z80_F = (Z80_F & ~(Z80_NF | Z80_HF | Z80_PF))
	        | (Z80_BC == 0 ? 0 : Z80_PF);
}

static void Z80_LDD(void)
{
	z80_wrmem(Z80_DE--, z80_rdmem(Z80_HL--));
	--Z80_BC;
	Z80_F = (Z80_F & ~(Z80_NF | Z80_HF | Z80_PF))
	        | (Z80_BC == 0 ? 0 : Z80_PF);
}



void z80_cycle_ed(void)
{
	int n;

	n = z80_rdmem(Z80_PC++);
	z80.cycle -= cycles_ed[n];

//printf("ed %02x\n", n);
	switch (n)
	{
default:
	break;

/* 必要？ */
case 0xed:
case 0xcb:
case 0xfd:
case 0xdd:
	--Z80_PC;
	break;

case 0x4a:	/* ADC	HL, BC */
{
	Z80_ADC16(Z80_HL, Z80_BC);
}
	break;

case 0x5a:	/* ADC	HL, DE */
{
	Z80_ADC16(Z80_HL, Z80_DE);
}
	break;

case 0x6a:	/* ADC	HL, HL */
{
	Z80_ADC16(Z80_HL, Z80_HL);
}
	break;

case 0x7a:	/* ADC	HL, SP */
{
	Z80_ADC16(Z80_HL, Z80_SP);
}
	break;

case 0x42:	/* SBC	HL, BC */
{
	Z80_SBC16(Z80_BC);
}
	break;

case 0x52:	/* SBC	HL, DE */
{
	Z80_SBC16(Z80_DE);
}
	break;

case 0x62:	/* SBC	HL, HL */
{
	Z80_SBC16(Z80_HL);
}
	break;

case 0x72:	/* SBC	HL, SP */
{
	Z80_SBC16(Z80_SP);
}
	break;

case 0x43:	/*	LD	(imm), BC */
{
	z80_wrmem2(z80_rdmem2(Z80_PC), Z80_BC);
	Z80_PC += 2;
}
	break;

case 0x53:	/*	LD	(imm), DE */
{
	z80_wrmem2(z80_rdmem2(Z80_PC), Z80_DE);
	Z80_PC += 2;
}
	break;

case 0x63:	/*	LD	(imm), HL */
{
	z80_wrmem2(z80_rdmem2(Z80_PC), Z80_HL);
	Z80_PC += 2;
}
	break;

case 0x73:	/*	LD	(imm), SP */
{
	z80_wrmem2(z80_rdmem2(Z80_PC), Z80_SP);
	Z80_PC += 2;
}
	break;

case 0x4b:	/*	LD	BC, (imm) */
{
	Z80_BC = z80_rdmem2(z80_rdmem2(Z80_PC));
	Z80_PC += 2;
}
	break;

case 0x5b:	/*	LD	DE, (imm) */
{
	Z80_DE = z80_rdmem2(z80_rdmem2(Z80_PC));
	Z80_PC += 2;
}
	break;

case 0x6b:	/*	LD	HL, (imm) */
{
	Z80_HL = z80_rdmem2(z80_rdmem2(Z80_PC));
	Z80_PC += 2;
}
	break;

case 0x7b:	/*	LD	SP, (imm) */
{
	Z80_SP = z80_rdmem2(z80_rdmem2(Z80_PC));
	Z80_PC += 2;
}
	break;

case 0x67:	/* RRD */
{
	int a;

	a = z80_rdmem(Z80_HL);
	z80_wrmem(Z80_HL, (Z80_A << 4) | (a >> 4));
	Z80_A = (Z80_A & 0xf0) | (a & 0x0f);
	Z80_F = Z80_PZS[Z80_A] | (Z80_F & Z80_CF);
}
	break;

case 0x6f:	/* RLD */
{
	int a;

	a = z80_rdmem(Z80_HL);
	z80_wrmem(Z80_HL, (a << 4) | (Z80_A & 0x0f));
	Z80_A = (Z80_A & 0xf0) | (a >> 4);
	Z80_F = Z80_PZS[Z80_A] | (Z80_F & Z80_CF);
}
	break;

case 0x57:	/* LD	A, I */
{
	Z80_A = Z80_I;
	Z80_F = (Z80_F & Z80_CF) | Z80_ZS[Z80_A];
	if (z80.iflag & Z80_IFF1)
		Z80_F |= Z80_PF;
}
	break;

case 0x5f:	/* LD	A, R */
{
	Z80_R = z80.cycle; // XXX

	Z80_A = Z80_R;
	Z80_F = (Z80_F & Z80_CF) | Z80_ZS[Z80_A];
	if (z80.iflag & Z80_IFF1)
		Z80_F |= Z80_PF;
}
	break;

case 0x47:	/* LD	I, A */
{
	Z80_I = Z80_A;
}
	break;

case 0x4f:	/* LD	R, A */
{
	Z80_R = Z80_A;
}
	break;

case 0x46:	/* IM	0 */
{
	z80.iflag = (z80.iflag & ~Z80_IF_MODEM) | Z80_IF_MODE0;
}
	break;

case 0x56:	/* IM	1 */
{
	z80.iflag = (z80.iflag & ~Z80_IF_MODEM) | Z80_IF_MODE1;
}
	break;

case 0x5e:	/* IM	2 */
{
	z80.iflag = (z80.iflag & ~Z80_IF_MODEM) | Z80_IF_MODE2;
}
	break;

case 0x4d:	/* RETI */
{
	Z80_PC = Z80_POP();
/* XXX interrupt chain?? */
}
	break;

case 0x45:	/* RETN */
{
	if (z80.iflag & Z80_IFF2)
		z80.iflag |= Z80_IFF1;
	else
		z80.iflag &= ~Z80_IFF1;

	Z80_PC = Z80_POP();

/* XXX EI の時と同様の処理をすべき？ */
}
	break;

case 0x44:	/* NEG */
{
	Uint8 a;

	a = Z80_A;
	Z80_A = 0;
	Z80_SUB8(a, 0);
}
	break;


case 0xa2:	/* INI */
{
	Z80_INI();
}
	break;

case 0xb2:	/* INIR */
{
	do
	{
		Z80_INI();
		z80.cycle -= CYCLES_REPEAT;
	} while (Z80_B != 0);
	z80.cycle += CYCLES_REPEAT_F;
}
	break;

case 0xaa:	/* IND */
{
	Z80_IND();
}
	break;

case 0xba:	/* INDR */
{
	do
	{
		Z80_IND();
		z80.cycle -= CYCLES_REPEAT;
	} while (Z80_B != 0);
	z80.cycle += CYCLES_REPEAT_F;
}
	break;

case 0xa3:	/* OUTI */
{
	Z80_OUTI();
}
	break;

case 0xb3:	/* OTIR */
{
	do
	{
		Z80_OUTI();
		z80.cycle -= CYCLES_REPEAT;
	} while (Z80_B != 0);
	z80.cycle += CYCLES_REPEAT_F;
}
	break;

case 0xab:	/* OUTD */
{
	Z80_OUTD();
}
	break;

case 0xbb:	/* OTDR */
{
	do
	{
		Z80_OUTD();
		z80.cycle -= CYCLES_REPEAT;
	} while (Z80_B != 0);
	z80.cycle += CYCLES_REPEAT_F;
}
	break;

case 0xa0:	/* LDI */
{
	Z80_LDI();
}
	break;

case 0xb0:	/* LDIR */
{
	do
	{
		Z80_LDI();
		z80.cycle -= CYCLES_REPEAT;
	} while (Z80_BC != 0);
	z80.cycle += CYCLES_REPEAT_F;
}
	break;

case 0xa8:	/* LDD */
{
	Z80_LDD();
}
	break;

case 0xb8:	/* LDDR */
{
	do
	{
		Z80_LDD();
		z80.cycle -= CYCLES_REPEAT;
	} while (Z80_BC != 0);
	z80.cycle += CYCLES_REPEAT_F;
}
	break;

case 0xa1:	/* CPI */
{
	int a, b;

	a = z80_rdmem(Z80_HL++);
	b = (Uint8)(Z80_A - a);
	--Z80_BC;
	Z80_F = Z80_NF | (Z80_F & Z80_CF) | Z80_ZS[b]
	        | ((Z80_A ^ a ^ b) & Z80_HF) | ((Z80_BC == 0) ? 0 : Z80_PF);
}
	break;

case 0xb1:	/* CPIR */
{
	int a, b;

	do
	{
		a = z80_rdmem(Z80_HL++);
		b = (Uint8)(Z80_A - a);
		--Z80_BC;
		z80.cycle -= CYCLES_REPEAT;
	} while (Z80_BC != 0 && b != 0);
	z80.cycle += CYCLES_REPEAT_F;
	Z80_F = Z80_NF | (Z80_F & Z80_CF) | Z80_ZS[b]
	        | ((Z80_A ^ a ^ b) & Z80_HF);
	if (Z80_BC != 0)
		Z80_F |= Z80_PF;
}
	break;

case 0xa9:	/* CPD */
{
	int a, b;

	a = z80_rdmem(Z80_HL--);
	b = (Uint8)(Z80_A - a);
	--Z80_BC;
	Z80_F = Z80_NF | (Z80_F&Z80_CF) | Z80_ZS[b]
	        | ((Z80_A ^ a ^ b) & Z80_HF) | ((Z80_BC == 0) ? 0 : Z80_PF);
}
	break;

case 0xb9:	/* CPDR */
{
	int a, b;

	do
	{
		a = z80_rdmem(Z80_HL--);
		b =(Uint8)(Z80_A - a);
		--Z80_BC;
		z80.cycle -= CYCLES_REPEAT;
	} while (Z80_BC != 0 && b != 0);
	z80.cycle += CYCLES_REPEAT_F;
	Z80_F = Z80_NF | (Z80_F & Z80_CF) | Z80_ZS[b] 
	        | ((Z80_A ^ a ^ b) & Z80_HF) | ((Z80_BC == 0) ? 0 : Z80_PF);
}
	break;


case 0x40:	/* IN	B, (c) */
{
	Z80_B = Z80_IN8();
}
	break;

case 0x48:	/* IN	C, (c) */
{
	Z80_C = Z80_IN8();
}
	break;

case 0x50:	/* IN	D, (c) */
{
	Z80_D = Z80_IN8();
}
	break;

case 0x58:	/* IN	E, (c) */
{
	Z80_E = Z80_IN8();
}
	break;

case 0x60:	/* IN	H, (c) */
{
	Z80_H = Z80_IN8();
}
	break;

case 0x68:	/* IN	L, (c) */
{
	Z80_L = Z80_IN8();
}
	break;

case 0x70:	/* IN	F, (c) */
{
	Z80_IN8();
}
	break;

case 0x78:	/* IN	A, (c) */
{
	Z80_A = Z80_IN8();
}
	break;

case 0x41:	/* OUT	(c), B */
{
	z80_out(Z80_C, Z80_B);
}
	break;

case 0x49:	/* OUT	(c), C */
{
	z80_out(Z80_C, Z80_C);
}
	break;

case 0x51:	/* OUT	(c), D */
{
	z80_out(Z80_C, Z80_D);
}
	break;

case 0x59:	/* OUT	(c), E */
{
	z80_out(Z80_C, Z80_E);
}
	break;

case 0x61:	/* OUT	(c), H */
{
	z80_out(Z80_C, Z80_H);
}
	break;

case 0x69:	/* OUT	(c), L */
{
	z80_out(Z80_C, Z80_L);
}
	break;

case 0x79:	/* OUT	(c), A */
{
	z80_out(Z80_C, Z80_A);
}
	break;

case 0xfe:	/* patch invoke (fMSX 拡張) */
{
	static int state;
	int a;

	if (state == 0)
		state = z80_patch();

	a = min(state, z80.cycle);
	state -= a;
	z80.cycle -= a;

	if (state != 0)
		Z80_PC -= 2;
}

	break;


		}
}
