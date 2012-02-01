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


// #define Z80_PROFILE 0

#define counter_update() z80.cycle=sc;
#define counter_reupdate() sc=z80.cycle;
//#define sc z80.cycle
//#define counter_update()
//#define counter_reupdate()

// XXX ??? table の方がよいのか？

#define	CYCLES_ADC_xHL      7+1
#define	CYCLES_ADC_r        4+1
#define	CYCLES_ADC_imm      7+1
#define	CYCLES_ADD_xHL      7+1
#define	CYCLES_ADD_r        4+1
#define	CYCLES_ADD_imm      7+1
#define	CYCLES_ADD_ss       11+1
#define	CYCLES_AND_xHL      7+1
#define	CYCLES_AND_r        4+1
#define	CYCLES_AND_imm      7+1
#define	CYCLES_CALL         17+1
#define	CYCLES_CALL_F       10+1
#define	CYCLES_CCF          4+1
#define	CYCLES_CP_xHL       7+1
#define	CYCLES_CP_r         4+1
#define	CYCLES_CP_imm       7+1
#define	CYCLES_CPL          4+1
#define	CYCLES_DAA          4+1
#define	CYCLES_DEC_xHL      11+1
#define	CYCLES_DEC_r        4+1
#define	CYCLES_DEC_ss       6+1
#define	CYCLES_DI           4+1
#define	CYCLES_DJNZ         13+1
#define	CYCLES_DJNZ_F       8+1
#define	CYCLES_EI           4+1
#define	CYCLES_EX_HL_xSP    19+1
#define	CYCLES_EX_ss_ss     4+1
#define	CYCLES_EXX          4+1
#define	CYCLES_HALT         4+1
#define	CYCLES_IN_nn        11+1
#define	CYCLES_INC_xHL      11+1
#define	CYCLES_INC_r        4+1
#define	CYCLES_INC_ss       6+1
#define	CYCLES_JP           10+1
#define	CYCLES_JP_xHL       4+1
#define	CYCLES_JR           12+1
#define	CYCLES_JR_F         7+1
#define	CYCLES_LD_xss_A     7+1
#define	CYCLES_LD_ximm16_A  13+1
#define	CYCLES_LD_ximm16_HL 16+1
#define	CYCLES_LD_A_xss     7+1
#define	CYCLES_LD_r_xHL     7+1
#define	CYCLES_LD_A_ximm16  13+1
#define	CYCLES_LD_r_r       4+1
#define	CYCLES_LD_xHL_r     7+1
#define	CYCLES_LD_r_imm     7+1
#define	CYCLES_LD_xHL_imm   10+1
#define	CYCLES_LD_ss_imm    10+1
#define	CYCLES_LD_HL_ximm16 16+1
#define	CYCLES_LD_SP_HL     6+1
#define	CYCLES_NOP          4+1
#define	CYCLES_OR_xHL       7+1
#define	CYCLES_OR_r         4+1
#define	CYCLES_OR_imm       7+1
#define	CYCLES_OUT_ximm_A   11+1
#define	CYCLES_POP_ss       10+1
#define	CYCLES_PUSH_ss      11+1
#define	CYCLES_RET          10+1
#define	CYCLES_RET_COND     11+1
#define	CYCLES_ROR_A        4+1
#define	CYCLES_RST          11+1
#define	CYCLES_SBC_xHL      7+1
#define	CYCLES_SBC_r        4+1
#define	CYCLES_SBC_imm      7+1
#define	CYCLES_SCF          4+1
#define	CYCLES_SUB_xHL      7+1
#define	CYCLES_SUB_r        4+1
#define	CYCLES_SUB_imm      7+1
#define	CYCLES_XOR_xHL      7+1
#define	CYCLES_XOR_r        4+1
#define	CYCLES_XOR_imm      7+1

#define Z80_1EX(r) do {int a; a = r; r = r##D; r##D = a;} while(0)

#define Z80_JR() Z80_PC += (Sint8)z80_rdmem(Z80_PC) + 1

#define	Z80_JR_COND(cond)	\
do                          \
{                           \
	if (cond)               \
	{                       \
		Z80_JR();           \
		sc -= CYCLES_JR;    \
	} else                  \
	{                       \
		++Z80_PC;           \
		sc -= CYCLES_JR_F;  \
	}                       \
} while(0)

#define Z80_CALL()               \
do                               \
{                                \
	pair16l_t a;                 \
                                 \
	a.w = Z80_PC + 2;            \
	Z80_PUSH(a);				 \
	Z80_PC = z80_rdmem2(Z80_PC); \
	sc -= CYCLES_CALL;           \
} while(0)

#define	Z80_RST(a)    \
do                    \
{                     \
	Z80_PUSH(z80.PC); \
	Z80_PC = (a);     \
	sc -= CYCLES_RST; \
} while(0)

#define	Z80_CALL_COND(cond)	  \
do                            \
{                             \
	if (cond)                 \
 	{                         \
 		Z80_CALL();           \
 	} else                    \
	{                         \
 		Z80_PC += 2;          \
 		sc -= CYCLES_CALL_F;  \
 	}                         \
} while(0)

#define	Z80_JP()				\
do                              \
{                               \
	Z80_PC=z80_rdmem2(Z80_PC);	\
	sc -= CYCLES_JP;            \
} while(0)

#define	Z80_JP_COND(cond)            \
do                                   \
{                                    \
	if (!(cond))                     \
		Z80_PC += 2;                 \
	else				             \
		Z80_PC = z80_rdmem2(Z80_PC); \
	sc -= CYCLES_JP;                 \
} while(0)

#define	Z80_RET()       \
do                      \
{                       \
	Z80_PC = Z80_POP(); \
	sc -= CYCLES_RET;   \
} while(0)

#define	Z80_RET_COND(cond)  \
do                          \
{                           \
	if (cond)               \
		Z80_PC = Z80_POP();	\
	sc -= CYCLES_RET_COND;  \
} while(0)





void z80_cycle(int cycle)
{
	int sc;
//int b;

#ifdef Z80_PROFILE
	profile_start(Z80_PROFILE);
#endif

	z80.cycle += cycle;
	z80.n_cycles = z80.cycle;
	counter_reupdate();

	if ((z80.iflag & Z80_IF_HALT) == Z80_IF_HALT)
		Z80_PC--;

	for (;;)
	{
#if defined(TDEBUGGER) || defined(EXTDEBUGGER)
/* context stop */
		if (zdb_main(z80.zdb))
			exit(1);
/* context start */
#endif


//DEBUGGER

//b=z80_rdmem(Z80_PC++);
//printf("%02x\n", b);

	switch(z80_rdmem(Z80_PC++))
		{
/* XXX */

case 0x00:	/* NOP */
{
	sc -= CYCLES_NOP;
}

	break;

case 0x01:	/* LD	BC, imm */
{
	Z80_BC = z80_rdmem2(Z80_PC);
	Z80_PC += 2;
	sc -= CYCLES_LD_ss_imm;
}

	break;

case 0x02:	/* LD	(BC), A */
{
	z80_wrmem(Z80_BC, Z80_A);
	sc -= CYCLES_LD_xss_A;
}

	break;

case 0x03:	/* INC	BC */
{
	++Z80_BC;
	sc -= CYCLES_INC_ss;
}

	break;

case 0x04:	/* INC	B */
{
	Z80_INC8(Z80_B);
	sc -= CYCLES_INC_r;
}

	break;

case 0x05:	/* DEC	B */
{
	Z80_DEC8(Z80_B);
	sc -= CYCLES_DEC_r;
}

	break;

case 0x06:	/* LD	B, imm */
{
	Z80_B = z80_rdmem(Z80_PC++);
	sc -= CYCLES_LD_r_imm;
}

	break;

case 0x07:	/* RLCA */
{
	Z80_F &= ~(Z80_HF | Z80_NF | Z80_CF);
	if ((Z80_A & 0x80) == 0)
	{
		Z80_A <<= 1;
	} else
	{
		Z80_F |= Z80_CF;
		Z80_A = (Z80_A << 1) | 1;
	}

	sc -= CYCLES_ROR_A;
}

	break;


case 0x08:	/* EX	AF, AF' */
{
	Z80_1EX(Z80_AF);
	sc -= CYCLES_EX_ss_ss;
}

	break;

case 0x09:	/* ADD	HL, BC */
{
	Z80_ADD16(Z80_HL, Z80_BC);
	sc -= CYCLES_ADD_ss;
}

	break;

case 0x0a:	/* LD	A, (BC) */
{
	Z80_A = z80_rdmem(Z80_BC);
	sc -= CYCLES_LD_A_xss;
}

	break;

case 0x0b:	/* DEC	BC */
{
	--Z80_BC;
	sc -= CYCLES_DEC_ss;
}

	break;

case 0x0c:	/* INC	C */
{
	Z80_INC8(Z80_C);
	sc -= CYCLES_INC_r;
}

	break;

case 0x0d:	/* DEC	C */
{
	Z80_DEC8(Z80_C);
	sc -= CYCLES_DEC_r;
}

	break;

case 0x0e:	/* LD	C, imm */
{
	Z80_C = z80_rdmem(Z80_PC++);
	sc -= CYCLES_LD_r_imm;
}

	break;

case 0x0f:	/* RRCA */
{
	Z80_F &= ~(Z80_HF | Z80_NF | Z80_CF);
	if ((Z80_A & 0x01) == 0)
	{
		Z80_A >>= 1;
	} else
	{
		Z80_F |= Z80_CF;
		Z80_A = (Z80_A >> 1) | 0x80;
	}

	sc -= CYCLES_ROR_A; 
}

	break;


case 0x10:	/* DJNZ */
{
	if(--Z80_B)
	{
		Z80_JR();
		sc -= CYCLES_DJNZ;
	} else
	{
		Z80_PC++;
		sc -= CYCLES_DJNZ_F;
	}
}

	break;

case 0x11:	/* LD	DE, imm */
{
	Z80_DE = z80_rdmem2(Z80_PC);
	Z80_PC += 2;
	sc -= CYCLES_LD_ss_imm;
}

	break;

case 0x12:	/* LD	(DE), A */
{
	z80_wrmem(Z80_DE, Z80_A);
	sc -= CYCLES_LD_xss_A;
}

	break;

case 0x13:	/* INC	DE */
{
	++Z80_DE;
	sc -= CYCLES_INC_ss;
}

	break;


case 0x14:	/* INC_D */
{
	Z80_INC8(Z80_D);
	sc -= CYCLES_INC_r;
}

	break;

case 0x15:	/* DEC_D */
{
	Z80_DEC8(Z80_D);
	sc -= CYCLES_DEC_r;
}

	break;

case 0x16:	/* LD	D, imm */
{
	Z80_D = z80_rdmem(Z80_PC++);
	sc -= CYCLES_LD_r_imm;
}

	break;

case 0x17:	/* RLA */
{
	int a;

	a = Z80_F&Z80_CF;
	Z80_F = (Z80_F & ~(Z80_HF | Z80_NF | Z80_CF)) | (Z80_A >> 7);
	Z80_A = (Z80_A << 1) | a;

	sc -= CYCLES_ROR_A;
}

	break;


case 0x18:	/* JR	imm */
{
	Z80_JR();
	sc -= CYCLES_JR;
}
	break;

case 0x19:	/* ADD	HL, DE */
{
	Z80_ADD16(Z80_HL, Z80_DE);
	sc -= CYCLES_ADD_ss;
}
	break;

case 0x1a:	/* LD	A, (DE) */
{
	Z80_A = z80_rdmem(Z80_DE);
	sc -= CYCLES_LD_A_xss;
}
	break;

case 0x1b:	/* DEC	DE */
{
	--Z80_DE;
	sc -= CYCLES_DEC_ss;
}
	break;

case 0x1c:	/* INC	E */
{
	Z80_INC8(Z80_E);
	sc -= CYCLES_INC_r;
}
	break;

case 0x1d:	/* DEC	E */
{
	Z80_DEC8(Z80_E);
	sc -= CYCLES_DEC_r;
}
	break;

case 0x1e:	/* LD	E, imm */
{
	Z80_E = z80_rdmem(Z80_PC++);
	sc -= CYCLES_LD_r_imm;
}
	break;

case 0x1f:	/* RRA */
{
	int a;

	a = (Z80_F & Z80_CF) << 7;
	Z80_F = (Z80_F & ~(Z80_HF | Z80_NF | Z80_CF)) | (Z80_A & Z80_CF);
	Z80_A = (Z80_A >> 1) | a;

	sc -= CYCLES_ROR_A;
}
	break;


case 0x20:	/* JR	NZ, imm */
{
	Z80_JR_COND(!(Z80_F & Z80_ZF));
}
	break;

case 0x21:	/* LD	HL, imm */
{
	Z80_HL = z80_rdmem2(Z80_PC);
	Z80_PC += 2;
	sc -= CYCLES_LD_ss_imm;
}
	break;

case 0x22:	/* LD	(imm), HL */
{
	z80_wrmem2(z80_rdmem2(Z80_PC), Z80_HL);
	Z80_PC += 2;
	sc -= CYCLES_LD_ximm16_HL;
}
	break;

case 0x23:	/* INC	HL */
{
	++Z80_HL;
	sc -= CYCLES_INC_ss;
}
	break;

case 0x24:	/* INC	H */
{
	Z80_INC8(Z80_H);
	sc -= CYCLES_INC_r;
}
	break;

case 0x25:	/* DEC	H */
{
	Z80_DEC8(Z80_H);
	sc -= CYCLES_DEC_r;
}
	break;

case 0x26:	/* LD	H, imm */
{
	Z80_H = z80_rdmem(Z80_PC++);
	sc -= CYCLES_LD_r_imm;
}
	break;

case 0x27:	/* DAA */
{
	if (Z80_F & Z80_NF)
	{
		if (!(Z80_F & Z80_HF))
		{
			if ((Z80_A & 0xf0) >= 0x70 && (Z80_F & Z80_CF)
			        && (Z80_A & 0x0f) <= 0x09)
				Z80_A += 0xa0;
		} else
		{
			if ((Z80_A & 0xf0) <= 0x80 && !(Z80_F & Z80_CF)
			        && (Z80_A & 0x0f) >= 0x06)
			{
				Z80_A += 0xfa;
			} else
			{
				if ((Z80_A & 0xf0) >= 0x60 && (Z80_F & Z80_CF)
				        && (Z80_A & 0x0f) >= 0x06)
					Z80_A += 0x9a;
			}
		}
	} else
	{
		if ((Z80_A & 0x0f) >= 10)
		{
			Z80_A += 0x06;
			Z80_F |= Z80_HF;
		} else
		{
			if (Z80_F & Z80_HF)
			{
				Z80_A += 0x06;
				if (Z80_A < 0x06)
					Z80_F |= Z80_CF;
			}
		}

		if (Z80_A > 0x9f || (Z80_F & Z80_CF))
			Z80_A += 0x60, Z80_F |= Z80_CF;
	}

#if 0
		if (Z80_F & Z80_HF)
		{
			if ((Z80_A & 0x0c) == 0)
			{
				if (Z80_F & Z80_CF)
				{
					if ((Z80_A & 0xc0) == 0)
						Z80_A += 0x66;
				} else
				{
					if ((Z80_A & 0xf0) <= 0x90)
						Z80_A += 0x06; else
						Z80_A += 0x66, Z80_F |= Z80_CF;
				}
			}
		} else
		{
			if ((Z80_A & 0x0f) <= 0x09)
			{
				if ( ( !(Z80_A & Z80_C) && (Z80_A & 0xf0) >= 0xa0 ) ||
					  (  (Z80_A & Z80_C) && (Z80_A & 0xf0) <= 0x20 ) )
					Z80_A += 0x60, Z80_F |= Z80_CF;
			} else
			{
				if (Z80_F & Z80_CF)
				{
					if ((Z80_A & 0xf0) <= 0x20)
						Z80_A += 0x66;
				} else
				{
					if ((Z80_A & 0xf0) <= 0x80)
						Z80_A += 0x06; else
						Z80_A += 0x66, Z80_F |= Z80_CF;
				}
			}
		}
#endif

	Z80_F = (Z80_F & (Z80_CF | Z80_HF)) | Z80_PZS[Z80_A];

	sc -= CYCLES_DAA;
}
	break;

/*
*/


case 0x28:	/* JR	Z, imm */
{
	Z80_JR_COND(Z80_F & Z80_ZF);
}
	break;

case 0x29:	/* ADD	HL, HL */
{
	Z80_ADD16(Z80_HL, Z80_HL);
	sc -= CYCLES_ADD_ss;
}
	break;

case 0x2a:	/* LD	HL, (imm) */
{
	Z80_HL = z80_rdmem2(z80_rdmem2(Z80_PC));
	Z80_PC += 2;
	sc -= CYCLES_LD_HL_ximm16;
}
	break;

case 0x2b:	/* DEC	HL */
{
	--Z80_HL;
	sc -= CYCLES_DEC_ss;
}
	break;

case 0x2c:	/* INC	L */
{
	Z80_INC8(Z80_L);
	sc -= CYCLES_INC_r;
}
	break;

case 0x2d:	/* DEC	L */
{
	Z80_DEC8(Z80_L);
	sc -= CYCLES_DEC_r;
}
	break;

case 0x2e:	/* LD	L, imm */
{
	Z80_L = z80_rdmem(Z80_PC++);
	sc -= CYCLES_LD_r_imm;
}
	break;

case 0x2f:	/* CPL */
{
	Z80_A = ~Z80_A;
	Z80_F |= Z80_NF | Z80_HF;
	sc -= CYCLES_CPL;
}
	break;


case 0x30:	/* JR	NC, imm */
{
	Z80_JR_COND(!(Z80_F & Z80_CF));
}
	break;

case 0x31:	/* LD	SP, imm */
{
	Z80_SP = z80_rdmem2(Z80_PC);
	Z80_PC += 2;
	sc -= CYCLES_LD_ss_imm;
}
	break;

case 0x32:	/* LD	(imm), A */
{
	z80_wrmem(z80_rdmem2(Z80_PC), Z80_A);
	Z80_PC += 2;
	sc -= CYCLES_LD_ximm16_A;
}
	break;

case 0x33:	/* INC	SP */
{
	++Z80_SP;
	sc -= CYCLES_INC_ss;
}
	break;

case 0x34:	/* INC	(HL) */
{
	int a;

	a = z80_rdmem(Z80_HL);
	Z80_INC8(a);
	z80_wrmem(Z80_HL, a);
	sc -= CYCLES_INC_xHL;
}
	break;

case 0x35:	/* DEC	(HL) */
{
	int a;

	a = z80_rdmem(Z80_HL);
	Z80_DEC8(a);
	z80_wrmem(Z80_HL, a);
	sc -= CYCLES_DEC_xHL;
}

	break;

case 0x36:	/* LD	(HL), imm */
{
	z80_wrmem(Z80_HL, z80_rdmem(Z80_PC++));
	sc -= CYCLES_LD_xHL_imm;
}

	break;

case 0x37:	/* SCF */
{
	Z80_F = (Z80_F & ~(Z80_HF | Z80_NF)) | Z80_CF;
	sc -= CYCLES_SCF;
}

	break;


case 0x38:	/* JR	C, imm */
{
	Z80_JR_COND(Z80_F & Z80_CF);
}

	break;

case 0x39:	/* ADD	HL, SP */
{
	Z80_ADD16(Z80_HL, Z80_SP);
	sc -= CYCLES_ADD_ss;
}

	break;

case 0x3a:	/* LD	A, (imm) */
{
	Z80_A = z80_rdmem(z80_rdmem2(Z80_PC));
	Z80_PC += 2;
	sc -= CYCLES_LD_A_ximm16;
}

	break;

case 0x3b:	/* DEC	SP */
{
	--Z80_SP;
	sc -= CYCLES_DEC_ss;
}

	break;

case 0x3c:	/* INC	A */
{
	Z80_INC8(Z80_A);
	sc -= CYCLES_INC_r;
}
	break;

case 0x3d:	/* DEC	A */
{
	Z80_DEC8(Z80_A);
	sc -= CYCLES_DEC_r;
}
	break;

case 0x3e:	/* LD	A, imm */
{
	Z80_A = z80_rdmem(Z80_PC++);
	sc -= CYCLES_LD_r_imm;
}
	break;

case 0x3f:	/* CCF XXX */
{
	Z80_F &= ~(Z80_NF | Z80_HF);
	Z80_F |= (Z80_F & Z80_CF) << 4; /* Z80_HF = Z80_CF */
	Z80_F ^= Z80_CF;

//	Z80_F = ((Z80_F ^ Z80_CF) & ~(Z80_NF | Z80_HF)) | ((Z80_F & Z80_CF) << 4);

	sc -= CYCLES_CCF; 
}
	break;


case 0x40:	/* LD	B, B */
{
	sc -= CYCLES_LD_r_r;
}
	break;

case 0x41:	/* LD	B, C */
{
	Z80_B = Z80_C;
	sc -= CYCLES_LD_r_r;
}
	break;

case 0x42:	/* LD	B, D */
{
	Z80_B = Z80_D;
	sc -= CYCLES_LD_r_r;
}
	break;

case 0x43:	/* LD	B, E */
{
	Z80_B = Z80_E;
	sc -= CYCLES_LD_r_r;
}
	break;

case 0x44:	/* LD	B, H */
{
	Z80_B = Z80_H;
	sc -= CYCLES_LD_r_r;
}
	break;

case 0x45:	/* LD	B, L */
{
	Z80_B = Z80_L;
	sc -= CYCLES_LD_r_r;
}
	break;

case 0x46:	/* LD	B, (HL) */
{
	Z80_B = z80_rdmem(Z80_HL);
	sc -= CYCLES_LD_r_xHL;
}
	break;

case 0x47:	/* LD	B, A */
{
	Z80_B = Z80_A;
	sc -= CYCLES_LD_r_r;
}
	break;


case 0x48:	/* LD	C, B */
{
	Z80_C = Z80_B;
	sc -= CYCLES_LD_r_r;
}
	break;

case 0x49:	/* LD	C, C */
{
	sc -= CYCLES_LD_r_r;
}
	break;

case 0x4a:	/* LD	C, D */
{
	Z80_C = Z80_D;
	sc -= CYCLES_LD_r_r;
}
	break;

case 0x4b:	/* LD	C, E */
{
	Z80_C = Z80_E;
	sc -= CYCLES_LD_r_r;
}
	break;

case 0x4c:	/* LD	C, H */
{
	Z80_C = Z80_H;
	sc -= CYCLES_LD_r_r;
}
	break;

case 0x4d:	/* LD	C, L */
{
	Z80_C = Z80_L;
	sc -= CYCLES_LD_r_r;
}
	break;

case 0x4e:	/* LD	C, (HL) */
{
	 Z80_C = z80_rdmem(Z80_HL);
	 sc -= CYCLES_LD_r_xHL;
}
	break;

case 0x4f:	/* LD	C, A */
{
	Z80_C = Z80_A;
	sc -= CYCLES_LD_r_r;
}
	break;


case 0x50:	/* LD	D, B */
{
	Z80_D = Z80_B;
	sc -= CYCLES_LD_r_r;
}
	break;

case 0x51:	/* LD	D, C */
{
	Z80_D = Z80_C;
	sc -= CYCLES_LD_r_r;
}
	break;

case 0x52:	/* LD	D, D */
{
	sc -= CYCLES_LD_r_r;
}
	break;

case 0x53:	/* LD	D, E */
{
	Z80_D = Z80_E;
	sc -= CYCLES_LD_r_r;
}
	break;

case 0x54:	/* LD	D, H */
{
	Z80_D = Z80_H;
	sc -= CYCLES_LD_r_r;
}
	break;

case 0x55:	/* LD	D, L */
{
	Z80_D = Z80_L;
	sc -= CYCLES_LD_r_r;
}
	break;

case 0x56:	/* LD	D, (HL) */
{
	 Z80_D = z80_rdmem(Z80_HL);
	 sc -= CYCLES_LD_r_xHL;
}
	 break;

case 0x57:	/* LD	D, A */
{
	Z80_D = Z80_A;
	sc -= CYCLES_LD_r_r;
}
	break;


case 0x58:	/* LD	E, B */
{
	Z80_E = Z80_B;
	sc -= CYCLES_LD_r_r;
}
	break;

case 0x59:	/* LD	E, C */
{
	Z80_E = Z80_C;
	sc -= CYCLES_LD_r_r;
}
	break;

case 0x5a:	/* LD	E, D */
{
	Z80_E = Z80_D;
	sc -= CYCLES_LD_r_r;
}
	break;

case 0x5b:	/* LD	E, E */
{
	sc -= CYCLES_LD_r_r;
}
	break;

case 0x5c:	/* LD	E, H */
{
	Z80_E = Z80_H;
	sc -= CYCLES_LD_r_r;
}

	break;

case 0x5d:	/* LD	E, L */
{
	Z80_E = Z80_L;
	sc -= CYCLES_LD_r_r;
}

	break;

case 0x5e:	/* LD	E, (HL) */
{
	 Z80_E = z80_rdmem(Z80_HL);
	 sc -= CYCLES_LD_r_xHL;
}

	 break;

case 0x5f:	/* LD	E, A */
{
	Z80_E = Z80_A;
	sc -= CYCLES_LD_r_r;
}

	break;


case 0x60:	/* LD	H, B */
{
	Z80_H = Z80_B;
	sc -= CYCLES_LD_r_r;
}

	break;

case 0x61:	/* LD	H, C */
{
	Z80_H = Z80_C;
	sc -= CYCLES_LD_r_r;
}

	break;

case 0x62:	/* LD	H, D */
{
	Z80_H = Z80_D;
	sc -= CYCLES_LD_r_r;
}

	break;

case 0x63:	/* LD	H, E */
{
	Z80_H = Z80_E;
	sc -= CYCLES_LD_r_r;
}

	break;

case 0x64:	/* LD	H, H */
{
	sc -= CYCLES_LD_r_r;
}

	break;

case 0x65:	/* LD	H, L */
{
	Z80_H = Z80_L;
	sc -= CYCLES_LD_r_r;
}

	break;

case 0x66:	/* LD	H, (HL) */
{
	 Z80_H = z80_rdmem(Z80_HL);
	 sc -= CYCLES_LD_r_xHL;
}

	 break;

case 0x67:	/* LD	H, A */
{
	Z80_H = Z80_A;
	sc -= CYCLES_LD_r_r;
}

	break;


case 0x68:	/* LD	L, B */
{
	Z80_L = Z80_B;
	sc -= CYCLES_LD_r_r;
}

	break;

case 0x69:	/* LD	L, C */
{
	Z80_L = Z80_C;
	sc -= CYCLES_LD_r_r;
}

	break;

case 0x6a:	/* LD	L, D */
{
	Z80_L = Z80_D;
	sc -= CYCLES_LD_r_r;
}

	break;

case 0x6b:	/* LD	L, E */
{
	Z80_L = Z80_E;
	sc -= CYCLES_LD_r_r;
}

	break;

case 0x6c:	/* LD	L, H */
{
	Z80_L = Z80_H;
	sc -= CYCLES_LD_r_r;
}

	break;

case 0x6d:	/* LD	L, L */
{
	sc -= CYCLES_LD_r_r;
}

	break;

case 0x6e:	/* LD	L, (HL) */
{
	 Z80_L = z80_rdmem(Z80_HL);
	 sc -= CYCLES_LD_r_xHL;
}

	 break;

case 0x6f:	/* LD	L, A */
{
	Z80_L = Z80_A;
	sc -= CYCLES_LD_r_r;
}

	break;


case 0x70:	/* LD	(HL), B */
{
	z80_wrmem(Z80_HL, Z80_B);
	sc -= CYCLES_LD_xHL_r;
}

	break;

case 0x71:	/* LD	(HL), C */
{
	z80_wrmem(Z80_HL, Z80_C);
	sc -= CYCLES_LD_xHL_r;
}

	break;

case 0x72:	/* LD	(HL), D */
{
	z80_wrmem(Z80_HL,Z80_D);
	sc -= CYCLES_LD_xHL_r;
}

	break;

case 0x73:	/* LD	(HL), E */
{
	z80_wrmem(Z80_HL,Z80_E);
	sc -= CYCLES_LD_xHL_r;
}

	break;

case 0x74:	/* LD	(HL), H */
{
	z80_wrmem(Z80_HL,Z80_H);
	sc -= CYCLES_LD_xHL_r;
}

	break;

case 0x75:	/* LD	(HL), L */
{
	z80_wrmem(Z80_HL,Z80_L);
	sc -= CYCLES_LD_xHL_r;
}

	break;

case 0x76:	/* HALT */
{
	z80.iflag |= Z80_IF_HALT;
	sc = 0;
}

	break;

case 0x77:	/* LD	(HL), A*/
{
	z80_wrmem(Z80_HL,Z80_A);
	sc -= CYCLES_LD_xHL_r;
}

	break;


case 0x78:	/* LD	A, B */
{
	Z80_A = Z80_B;
	sc -= CYCLES_LD_r_r;
}

	break;

case 0x79:	/* LD	A, C */
{
	Z80_A = Z80_C;
	sc -= CYCLES_LD_r_r;
}

	break;

case 0x7a:	/* LD	A, D */
{
	Z80_A = Z80_D;
	sc -= CYCLES_LD_r_r;
}

	break;

case 0x7b:	/* LD	A, E */
{
	Z80_A = Z80_E;
	sc -= CYCLES_LD_r_r;
}

	break;

case 0x7c:	/* LD	A, H */
{
	Z80_A = Z80_H;
	sc -= CYCLES_LD_r_r;
}

	break;

case 0x7d:	/* LD	A, L */
{
	Z80_A = Z80_L;
	sc -= CYCLES_LD_r_r;
}

	break;

case 0x7e:	/* LD	A, (HL) */
{
	 Z80_A = z80_rdmem(Z80_HL);
	 sc -= CYCLES_LD_r_xHL;
}

	 break;

case 0x7f:	/* LD	A, A */
{
	sc -= CYCLES_LD_r_r;
}

	break;



case 0x80:	/* ADD	A, B */
{
	Z80_ADD8(Z80_B, 0);
	sc -= CYCLES_ADD_r;
}

	break;

case 0x81:	/* ADD	A, C */
{
	Z80_ADD8(Z80_C, 0);
	sc -= CYCLES_ADD_r;
}
	break;

case 0x82:	/* ADD	A, D */
{
	Z80_ADD8(Z80_D, 0);
	sc -= CYCLES_ADD_r;
}
	break;

case 0x83:	/* ADD	A, E */
{
	Z80_ADD8(Z80_E, 0);
	sc -= CYCLES_ADD_r;
}
	break;

case 0x84:	/* ADD	A, H */
{
	Z80_ADD8(Z80_H, 0);
	sc -= CYCLES_ADD_r;
}
	break;

case 0x85:	/* ADD	A, L */
{
	Z80_ADD8(Z80_L, 0);
	sc -= CYCLES_ADD_r;
}
	break;

case 0x86:	/* ADD	A, (HL) */
{
	Z80_ADD8(z80_rdmem(Z80_HL), 0);
	sc -= CYCLES_ADD_xHL;
}
	break;

case 0x87:	/* ADD	A, A */
{
	Z80_ADD8(Z80_A, 0);
	sc -= CYCLES_ADD_r;
}
	break;


case 0x88:	/* ADC	A, B */
{
	Z80_ADD8(Z80_B, Z80_F&Z80_CF);
	sc -= CYCLES_ADC_r;
}
	break;

case 0x89:	/* ADC	A, C */
{
	Z80_ADD8(Z80_C, Z80_F&Z80_CF);
	sc -= CYCLES_ADC_r;
}
	break;

case 0x8a:	/* ADC	A, D */
{
	Z80_ADD8(Z80_D, Z80_F&Z80_CF);
	sc -= CYCLES_ADC_r;
}
	break;

case 0x8b:	/* ADC	A, E */
{
	Z80_ADD8(Z80_E, Z80_F&Z80_CF);
	sc -= CYCLES_ADC_r;
}
	break;

case 0x8c:	/* ADC	A, H */
{
	Z80_ADD8(Z80_H, Z80_F&Z80_CF);
	sc -= CYCLES_ADC_r;
}
	break;

case 0x8d:	/* ADC	A, L */
{
	Z80_ADD8(Z80_L, Z80_F&Z80_CF);
	sc -= CYCLES_ADC_r;
}
	break;

case 0x8e:	/* ADC	A, (HL) */
{
	Z80_ADD8(z80_rdmem(Z80_HL), Z80_F&Z80_CF);
	sc -= CYCLES_ADC_xHL;
}
	break;

case 0x8f:	/* ADC	A, A */
{
	Z80_ADD8(Z80_A, Z80_F&Z80_CF);
	sc -= CYCLES_ADC_r;
}
	break;


case 0x90:	/* SUB	A, B */
{
	Z80_SUB8(Z80_B, 0);
	sc -= CYCLES_SUB_r;
}
	break;

case 0x91:	/* SUB	A, C */
{
	Z80_SUB8(Z80_C, 0);
	sc -= CYCLES_SUB_r;
}
	break;

case 0x92:	/* SUB	A, D */
{
	Z80_SUB8(Z80_D, 0);
	sc -= CYCLES_SUB_r;
}
	break;

case 0x93:	/* SUB	A, E */
{
	Z80_SUB8(Z80_E, 0);
	sc -= CYCLES_SUB_r;
}
	break;

case 0x94:	/* SUB	A, H */
{
	Z80_SUB8(Z80_H, 0);
	sc -= CYCLES_SUB_r;
}
	break;

case 0x95:	/* SUB	A, L */
{
	Z80_SUB8(Z80_L, 0);
	sc -= CYCLES_SUB_r;
}
	break;

case 0x96:	/* SUB	A, (HL) */
{
	Z80_SUB8(z80_rdmem(Z80_HL), 0);
	sc -= CYCLES_SUB_xHL;
}
	break;

case 0x97:	/* SUB	A, A */
{
	Z80_A = 0;
	Z80_F = Z80_NF | Z80_ZF;
	sc -= CYCLES_SUB_r;
}
	break;


case 0x98:	/* SBC	A, B */
{
	Z80_SUB8(Z80_B, Z80_F&Z80_CF);
	sc -= CYCLES_SBC_r;
}
	break;

case 0x99:	/* SBC	A, C */
{
	Z80_SUB8(Z80_C, Z80_F&Z80_CF);
	sc -= CYCLES_SBC_r;
}
	break;

case 0x9a:	/* SBC	A, D */
{
	Z80_SUB8(Z80_D, Z80_F&Z80_CF);
	sc -= CYCLES_SBC_r;
}
	break;

case 0x9b:	/* SBC	A, E */
{
	Z80_SUB8(Z80_E, Z80_F&Z80_CF);
	sc -= CYCLES_SBC_r;
}
	break;

case 0x9c:	/* SBC	A, H */
{
	Z80_SUB8(Z80_H, Z80_F&Z80_CF);
	sc -= CYCLES_SBC_r;
}
	break;

case 0x9d:	/* SBC	A, L */
{
	Z80_SUB8(Z80_L, Z80_F&Z80_CF);
	sc -= CYCLES_SBC_r;
}
	break;

case 0x9e:	/* SBC	A, (HL) */
{
	Z80_SUB8(z80_rdmem(Z80_HL), Z80_F&Z80_CF);
	sc -= CYCLES_SBC_xHL;
}
	break;

case 0x9f:	/* SBC	A, A */
{
	Z80_SUB8(Z80_A, Z80_F&Z80_CF);
	sc -= CYCLES_SBC_r;
}
	break;


case 0xa0:	/* AND	A, B */
{
	Z80_AND8(Z80_B);
	sc -= CYCLES_AND_r;
}
	break;

case 0xa1:	/* AND	A, C */
{
	Z80_AND8(Z80_C);
	sc -= CYCLES_AND_r;
}
	break;

case 0xa2:	/* AND	A, D */
{
	Z80_AND8(Z80_D);
	sc -= CYCLES_AND_r;
}
	break;

case 0xa3:	/* AND	A, E */
{
	Z80_AND8(Z80_E);
	sc -= CYCLES_AND_r;
}
	break;

case 0xa4:	/* AND	A, H */
{
	Z80_AND8(Z80_H);
	sc -= CYCLES_AND_r;
}
	break;

case 0xa5:	/* AND	A, L */
{
	Z80_AND8(Z80_L);
	sc -= CYCLES_AND_r;
}
	break;

case 0xa6:	/* AND	A, (HL) */
{
	Z80_AND8(z80_rdmem(Z80_HL));
	sc -= CYCLES_AND_xHL;
}
	break;

case 0xa7:	/* AND	A, A */
{
	Z80_AND8(Z80_A);
	sc -= CYCLES_AND_r;
}
	break;


case 0xa8:	/* XOR	A, B */
{
	Z80_XOR8(Z80_B);
	sc -= CYCLES_XOR_r;
}
	break;

case 0xa9:	/* XOR	A, C */
{
	Z80_XOR8(Z80_C);
	sc -= CYCLES_XOR_r;
}
	break;

case 0xaa:	/* XOR	A, D */
{
	Z80_XOR8(Z80_D);
	sc -= CYCLES_XOR_r;
}
	break;

case 0xab:	/* XOR	A, E */
{
	Z80_XOR8(Z80_E);
	sc -= CYCLES_XOR_r;
}
	break;

case 0xac:	/* XOR	A, H */
{
	Z80_XOR8(Z80_H);
	sc -= CYCLES_XOR_r;
}
	break;

case 0xad:	/* XOR	A, L */
{
	Z80_XOR8(Z80_L);
	sc -= CYCLES_XOR_r;
}
	break;

case 0xae:	/* XOR	A, (HL) */
{
	Z80_XOR8(z80_rdmem(Z80_HL));
	sc -= CYCLES_XOR_xHL;
}
	break;

case 0xaf:	/* XOR	A, A */
{
	Z80_A = 0;
	Z80_F = Z80_PF | Z80_ZF;
	sc -= CYCLES_XOR_r;
}
	break;


case 0xb0:	/* OR	A, B */
{
	Z80_OR8(Z80_B);
	sc -= CYCLES_OR_r;
}
	break;

case 0xb1:	/* OR	A, C */
{
	Z80_OR8(Z80_C);
	sc -= CYCLES_OR_r;
}
	break;

case 0xb2:	/* OR	A, D */
{
	Z80_OR8(Z80_D);
	sc -= CYCLES_OR_r;
}
	break;

case 0xb3:	/* OR	A, E */
{
	Z80_OR8(Z80_E);
	sc -= CYCLES_OR_r;
}
	break;

case 0xb4:	/* OR	A, H */
{
	Z80_OR8(Z80_H);
	sc -= CYCLES_OR_r;
}
	break;

case 0xb5:	/* OR	A, L */
{
	Z80_OR8(Z80_L);
	sc -= CYCLES_OR_r;
}
	break;

case 0xb6:	/* OR	A, (HL) */
{
	Z80_OR8(z80_rdmem(Z80_HL));
	sc -= CYCLES_OR_xHL;
}
	break;

case 0xb7:	/* OR	A, A */
{
	Z80_OR8(Z80_A);
	sc -= CYCLES_OR_r;
}
	break;


case 0xb8:	/* CP	A, B */
{
	Z80_CP8(Z80_B);
	sc -= CYCLES_CP_r;
}
	break;

case 0xb9:	/* CP	A, C */
{
	Z80_CP8(Z80_C);
	sc -= CYCLES_CP_r;
}
	break;

case 0xba:	/* CP	A, D */
{
	Z80_CP8(Z80_D);
	sc -= CYCLES_CP_r;
}
	break;

case 0xbb:	/* CP	A, E */
{
	Z80_CP8(Z80_E);
	sc -= CYCLES_CP_r;
}
	break;

case 0xbc:	/* CP	A, H */
{
	Z80_CP8(Z80_H);
	sc -= CYCLES_CP_r;
}
	break;

case 0xbd:	/* CP	A, L */
{
	Z80_CP8(Z80_L);
	sc -= CYCLES_CP_r;
}
	break;

case 0xbe:	/* CP	A, (HL) */
{
	Z80_CP8(z80_rdmem(Z80_HL));
	sc -= CYCLES_CP_xHL;
}
	break;

case 0xbf:	/* CP	A, A */
{
	Z80_F = Z80_NF | Z80_ZF;
	sc -= CYCLES_CP_r;
}
	break;


case 0xc0:	/* RET	NZ */
{
	Z80_RET_COND(!(Z80_F & Z80_ZF)); 
}
	break;

case 0xc1:	/* POP	BC */
{
	Z80_BC = Z80_POP();
	sc -= CYCLES_POP_ss;
}
	break;

case 0xc2:	/* JP	NZ, imm */
{
	Z80_JP_COND(!(Z80_F & Z80_ZF)); 
}
	break;

case 0xc3:	/* JP	imm */
{
	Z80_JP();
}
	break;

case 0xc4:	/* CALL	NZ, imm */
{
	Z80_CALL_COND(!(Z80_F & Z80_ZF));
}
	break;

case 0xc5:	/* PUSH	BC */
{
	Z80_PUSH(z80.BC);
	sc -= CYCLES_PUSH_ss;
}
	break;

case 0xc6:	/* ADD	A, imm */
{
	Z80_ADD8(z80_rdmem(Z80_PC++), 0);
	sc -= CYCLES_ADD_imm;
}
	break;

case 0xc7:	/* RST	00 */
{
	Z80_RST(0x00);
}
	break;


case 0xc8:	/* RET_Z */
{
	Z80_RET_COND(Z80_F & Z80_ZF);    
}
	break;

case 0xc9:	/* RET */
{
	Z80_RET();
}
	break;

case 0xca:	/* JP_Z */
{
	Z80_JP_COND(Z80_F & Z80_ZF);
}
	break;

case 0xcb:	/* 0xCB */
{
	counter_update();
	z80_cycle_cb();
	counter_reupdate();
	break;
}

case 0xcc:	/* CALL_Z */
{
	Z80_CALL_COND(Z80_F & Z80_ZF);
}
	break;

case 0xcd:	/* CALL */
{
	Z80_CALL();
}

	break;

case 0xce:	/* ADC	A, imm */
{
	Z80_ADD8(z80_rdmem(Z80_PC++), Z80_F & Z80_CF);
 	sc -= CYCLES_ADC_imm;
}
 	break;

case 0xcf:	/* RST	08 */
{
	Z80_RST(0x08);
}
	break;


case 0xd0:	/* RET	NC */
{
	Z80_RET_COND(!(Z80_F & Z80_CF)); 
}
	break;

case 0xd1:	/* POP	DE */
{
	Z80_DE = Z80_POP();
	sc -= CYCLES_POP_ss;
}
	break;

case 0xd2:	/* JP	NC, imm */
{
	Z80_JP_COND(!(Z80_F & Z80_CF)); 
}
	break;

case 0xd3:	/* OUT	(imm), A */
{
	counter_update();
	z80_out(z80_rdmem(Z80_PC++), Z80_A);
	counter_reupdate();

	sc -= CYCLES_OUT_ximm_A;
}
	break;

case 0xd4:	/* CALL	NC */
{
	Z80_CALL_COND(!(Z80_F & Z80_CF));
}
	break;

case 0xd5:	/* PUSH	DE */
{
	Z80_PUSH(z80.DE);
	sc -= CYCLES_PUSH_ss;
}
	break;

case 0xd6:	/* SUB	A, imm */
{
	Z80_SUB8(z80_rdmem(Z80_PC++), 0);
	sc -= CYCLES_SUB_imm;
}
	break;

case 0xd7:	/* RST	10 */
{
	Z80_RST(0x10);
}
	break;


case 0xd8:	/* RET	C */
{
	Z80_RET_COND(Z80_F & Z80_CF);    
}
	break;

case 0xd9:	/* EXX */
{
	Z80_1EX(Z80_BC);
	Z80_1EX(Z80_DE);
	Z80_1EX(Z80_HL);
	sc -= CYCLES_EXX;
}
	break;

case 0xda:	/* JP	C, imm */
{
	Z80_JP_COND(Z80_F & Z80_CF); 
}
	break;

case 0xdb:	/* IN	A, (imm) */
{
	Z80_A=z80_in(z80_rdmem(Z80_PC++));
	sc -= CYCLES_IN_nn;
}
	break;

case 0xdc:	/* CALL	C, imm */
{
	Z80_CALL_COND(Z80_F & Z80_CF);
}
	break;

case 0xdd:	/* 0xdd */
{
	counter_update();
	z80_cycle_xx(&z80.IX);
	counter_reupdate();
}
	break;

case 0xde:	/* SBC	A, imm */
{
	Z80_SUB8(z80_rdmem(Z80_PC++), Z80_F & Z80_CF);
	sc -= CYCLES_SBC_imm;
}
	break;

case 0xdf:	/* RST18 */
{
	Z80_RST(0x18);
}
	break;

case 0xe0:	/* RET	PO */
{
	Z80_RET_COND(!(Z80_F & Z80_PF)); 
}
	break;

case 0xe1:	/* POP	HL */
{
	Z80_HL = Z80_POP();
	sc -= CYCLES_POP_ss;
}
	break;

case 0xe2:	/* JP	PO, imm */
{
	Z80_JP_COND(!(Z80_F & Z80_PF)); 
}
	break;

case 0xe3:	/*	EX	(SP), HL */
{
	int a;

	a = z80_rdmem2(Z80_SP);
	z80_wrmem2(Z80_SP, Z80_HL);
	Z80_HL = a;

	sc -= CYCLES_EX_HL_xSP;
}
	break;

case 0xe4:	/* CALL	PO, imm */
{
	Z80_CALL_COND(!(Z80_F & Z80_PF));
}
	break;

case 0xe5:	/* PUSH	HL */
{
	Z80_PUSH(z80.HL);
	sc -= CYCLES_PUSH_ss;
}
	break;

case 0xe6:	/* AND	imm */
{
	 Z80_AND8(z80_rdmem(Z80_PC++));
	 sc -= CYCLES_AND_imm;
}
	 break;

case 0xe7:	/* RST	20 */
{
	Z80_RST(0x20);
}
	break;


case 0xe8:	/* RET	PE */
{
	Z80_RET_COND(Z80_F & Z80_PF);    
}
	break;

case 0xe9:	/* JP	(HL) */
{
	Z80_PC = Z80_HL;
	sc -= CYCLES_JP_xHL;
}
	break;

case 0xea:	/* JP	PE, imm */
{
	Z80_JP_COND(Z80_F & Z80_PF); 
}
	break;

case 0xeb:	/* EX	DE, HL */
{
	int a;

	a = Z80_DE;
	Z80_DE = Z80_HL;
	Z80_HL = a;
	sc -= CYCLES_EX_ss_ss;
}
	break;

case 0xec:	/* CALL	PE, imm */
{
	Z80_CALL_COND(Z80_F & Z80_PF);
}
	break;

case 0xed:	/* 0xED */
{
	counter_update();
	z80_cycle_ed();
	counter_reupdate();
	break;
}

case 0xee:	/* XOR	A, imm */
{
	Z80_XOR8(z80_rdmem(Z80_PC++));
	sc -= CYCLES_XOR_imm;
}
	break;

case 0xef:	/* RST	28 */
{
	Z80_RST(0x28);
}
	break;


case 0xf0:	/* RET	P */
{
	Z80_RET_COND(!(Z80_F & Z80_SF)); 
}
	break;

case 0xf1:	/* POP	AF */
{
	Z80_AF = Z80_POP();
	sc -= CYCLES_POP_ss;
}
	break;

case 0xf2:	/* JP	P, imm */
{
	Z80_JP_COND(!(Z80_F & Z80_SF)); 
}
	break;

case 0xf3:	/* DI */
{
	z80_di();
	sc -= CYCLES_DI;
}
	break;

case 0xf4:	/* CALL	P, imm */
{
	Z80_CALL_COND(!(Z80_F & Z80_SF));
}
	break;

case 0xf5:	/* PUSH	AF */
{
	Z80_PUSH(z80.AF);
	sc -= CYCLES_PUSH_ss;
}
	break;

case 0xf6:	/* OR	A, imm */
{
	Z80_OR8(z80_rdmem(Z80_PC++));
	sc -= CYCLES_OR_imm;
}
	break;

case 0xf7:	/* RST	30 */
{
	Z80_RST(0x30);
}
	break;


case 0xf8:	/* RET	M */
{
	Z80_RET_COND(Z80_F & Z80_SF);
}
	break;

case 0xf9:	/* LD	SP, HL */
{
	Z80_SP = Z80_HL;
	sc -= CYCLES_LD_SP_HL;
}
	break;

case 0xfa:	/* JP	M, imm */
{
	Z80_JP_COND(Z80_F & Z80_SF); 
}
	break;

case 0xfb:	/* EI */
{
	z80_ei();
	if (z80.ivec != Z80_NOINT)
		z80_interrupt();
//  e = 2;
	sc -= CYCLES_EI; 
}
	break;

case 0xfc:	/* CALL	M, imm */
{
	Z80_CALL_COND(Z80_F & Z80_SF);
	sc -= 0;
}
	break;

case 0xfd:	/* 0xFD */
{
	counter_update();
	z80_cycle_xx(&z80.IY);
	counter_reupdate();
}
	break;

case 0xfe:	/* CP	A, imm */
{
	Z80_CP8(z80_rdmem(Z80_PC++));
	sc -= CYCLES_CP_imm;
}
	break;

case 0xff:	/* RST	38 */
{
	Z80_RST(0x38);
}
	break;

		}

/*
if (e > 0 && e-- == 0)
{
//printf("!");
	counter_update();
	z80_interrupt();
	counter_reupdate();
}
*/
		 if (sc <= 0)
		 	break;
	}

	counter_update();

	context_timelock();
	context.time_cycleprev += z80.n_cycles - z80.cycle;
	if (context.time_cycle >= context.hz)
	{
		++context.time_sec;
		context.time_cycleprev -= context.hz;
	}
	context.time_cycle = context.time_cycleprev;
	context_timeunlock();


#ifdef	Z80_PROFILE
	profile_stop(Z80_PROFILE);
#endif
}

