/*
   XXX

	z80 tiny debugger. 暫定簡易デバッガ。

	fMSX に似てないのはいいけど。なんだか冗長・ボーロボロ風味。
*/

#include "../../config.h"

#include <ctype.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#	include <unistd.h>
#endif

#define Z80_FASTMEM_USE
#include "../zodiac.h"
#include "z80.h"
#include "z80_internal.h"
#include "../msx/msx.h"


void zdb_rdmemn(char *p, int addr, size_t bytes)
{
#ifdef MD_LITTLE
	while (bytes >= 2)
	{
		addr &= 0xffff;

		*(Uint16 *)p++ = zdb_rdmem2(addr);
		addr += 2;
	}
#endif

	while (bytes > 0)
	{
		addr &= 0xffff;

		*p++ = zdb_rdmem(addr);
		++addr;
	}
}

void zdb_wrmemn(const char *p, int addr, size_t bytes)
{
#ifdef MD_LITTLE
	while (bytes >= 2)
	{
		addr &= 0xffff;

		zdb_wrmem2(addr, *(Uint16 *)p++);
		addr += 2;
	}
#endif

	while (bytes > 0)
	{
		addr &= 0xffff;
		zdb_wrmem(addr++, *p++);
	}
}



static char *iff_modes[] = {
	"IM0",
	"IM1",
	"IM2",
	"IM2",
};

static char *flags = "SZ.H.PNC";


static char *nim_r_tbl[] = {
	"B",
	"C",
	"D",
	"E",
	"H",
	"L",
	"(HL)",
	"A"
};

static char *nim_ss_tbl[] = {
	"BC",
	"DE",
	"HL",
	"SP"
};

static char *nim_qq_tbl[] = {
	"BC",
	"DE",
	"HL",
	"AF"
};

static char *nim_pp_tbl[] = {
	"BC",
	"DE",
	"IX",
	"SP"
};

static char *nim_rr_tbl[] = {
	"BC",
	"DE",
	"IY",
	"SP"
};

static char *nim_bb_tbl[] = {
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7"
};

static char *nim_cc_tbl[] = {
	"NZ",
	"Z",
	"NC",
	"C",
	"PO",
	"PE",
	"P",
	"M",
};

static char *nim_rt_tbl[] = {
	"00H",
	"08H",
	"10H",
	"18H",
	"20H",
	"28H",
	"30H",
	"38H"
};

enum {
	OPR_BYTE = 1,
	OPR_WORD,
	OPR_RDISP,
	OPR_JRDISP,
};

struct tydb_nim_t {
	char *s;
	char nbyte;
	int loc;
	char oprds[2];
};

struct tydb_nimparams_t {
	char *reg1;
	char *reg2;
	char *bit;
	char *idx;
	char *cc;
	char *rst;

	char byte[10];
	char word[10];
	char rdisp[10];
	char raddr[10];

	struct tydb_nim_t nim;
};

struct tydb_params_t *tydb_params;	/* XXX これはどこかへ… */


static struct tydb_nim_t nim_indx_tbl[] = {
	{ "ADD %I, %P",			2, 0x09, { 0, 0 } },
	{ "RRC (%I%D) [%@]",		4, 0x0E, { OPR_RDISP, 0 } },
	{ "RL (%I%D)  [%@]",		4, 0x16, { OPR_RDISP, 0 } },
	{ "LD %I, %W",			4, 0x21, { OPR_WORD, 0 } },
	{ "LD (%W), %I",		4, 0x22, { OPR_WORD, 0 } },
	{ "INC %I",			2, 0x23, { 0, 0 } },
	{ "LD %I, (%W)",		4, 0x2A, { OPR_WORD,0 } },
	{ "DEC %I",			2, 0x2B, { 0, 0 } },
	{ "INC (%I%D) [%@]",		3, 0x34, { OPR_RDISP, 0 } },
	{ "DEC (%I%D) [%@]",		3, 0x35, { OPR_RDISP, 0 } },
	{ "LD (%I%D), %N [%@]",		4, 0x36, { OPR_RDISP, OPR_BYTE } },
	{ "LD %R, (%I%D) [%@]",		3, 0x46, { OPR_RDISP, 0 } },
	{ "LD (%I%D), %R [%@]",		3, 0x70, { OPR_RDISP, 0 } },
	{ "ADD A, (%I%D) [%@]",		3, 0x86, { OPR_RDISP, 0 } },
	{ "ADC A, (%I%D) [%@]",		3, 0x8E, { OPR_RDISP, 0 } },
	{ "SUB A, (%I%D) [%@]",		3, 0x96, { OPR_RDISP, 0 } },
	{ "SBC A, (%I%D) [%@]",		3, 0x9E, { OPR_RDISP, 0 } },
	{ "AND A, (%I%D) [%@]",		3, 0xA6, { OPR_RDISP, 0 } },
	{ "XOR A, (%I%D) [%@]",		3, 0xAE, { OPR_RDISP, 0 } },
	{ "OR A, (%I%D) [%@]",		3, 0xB6, { OPR_RDISP, 0 } },
	{ "CP A, (%I%D) [%@]",		3, 0xBE, { OPR_RDISP, 0 } },
	{ "POP %I",			2, 0xE1, { 0, 0 } },
	{ "EX (SP), %I",		2, 0xE3, { 0, 0 } },
	{ "PUSH %I",			2, 0xE5, { 0, 0 } },
	{ "JP (%I)",			2, 0xE9, { 0, 0 } },
	{ "LD SP, %I",			2, 0xF9, { 0, 0 } },
	{ "",				-1, 0x00, { 0, 0 } }
};


static struct tydb_nim_t nim_indx_cb_tbl[] = {
	{ "RLC (%I%D) [%@]",		4, 0x06, { OPR_RDISP, 0 } },
	{ "RRC (%I%D) [%@]",		4, 0x0E, { OPR_RDISP, 0 } },
	{ "RL (%I%D) [%@]",		4, 0x16, { OPR_RDISP, 0 } },
	{ "RR (%I%D) [%@]",		4, 0x1E, { OPR_RDISP, 0 } },
	{ "SLA (%I%D) [%@]",		4, 0x26, { OPR_RDISP, 0 } },
	{ "SRA (%I%D) [%@]",		4, 0x2E, { OPR_RDISP, 0 } },
	{ "SRL (%I%D) [%@]",		4, 0x3E, { OPR_RDISP, 0 } },
	{ "BIT %B, (%I%D) [%@]",	4, 0x46, { OPR_RDISP, 0 } },
	{ "RES %B, (%I%D) [%@]",	4, 0x86, { OPR_RDISP, 0 } },
	{ "SET %B, (%I%D) [%@]",	4, 0xC6, { OPR_RDISP, 0 } },
	{ "",				-1, 0x00, { 0, 0 } }
};


static struct tydb_nim_t nim_ed_tbl[] = {
	{ "DB %W",		2, 0xFF, { OPR_WORD, 0 } },
	{ "IN %R, (C)",		2, 0x40, { 0, 0 } },
	{ "OUT (C), %R",	2, 0x41, { 0, 0 } },
	{ "SBC HL, %P",		2, 0x42, { 0, 0 } },
	{ "LD (%W), %P",	4, 0x43, { OPR_WORD, 0 } },
	{ "NEG A",		2, 0x44, { 0, 0 } },
	{ "RETN",		2, 0x45, { 0, 0 } },
	{ "IM 0",		2, 0x46, { 0, 0 } },
	{ "LD I, A",		2, 0x47, { 0, 0 } },
	{ "ADC HL, %P",		2, 0x4A, { 0, 0 } },
	{ "LD %P, (%W)",	4, 0x4B, { OPR_WORD, 0 } },
	{ "RETI",		2, 0x4D, { 0, 0 } },
	{ "LD R, A",		2, 0x4F, { 0, 0 } },
	{ "IM 1",		2, 0x56, { 0, 0 } },
	{ "LD A, I",		2, 0x57, { 0, 0 } },
	{ "IM 2",		2, 0x5E, { 0, 0 } },
	{ "LD A, R",		2, 0x5F, { 0, 0 } },
	{ "RRD",		2, 0x67, { 0, 0 } },
	{ "RLD",		2, 0x6F, { 0, 0 } },
	{ "LDI",		2, 0xA0, { 0, 0 } },
	{ "CPI",		2, 0xA1, { 0, 0 } },
	{ "INI",		2, 0xA2, { 0, 0 } },
	{ "OUTI",		2, 0xA3, { 0, 0 } },
	{ "LDD",		2, 0xA8, { 0, 0 } },
	{ "CPD",		2, 0xA9, { 0, 0 } },
	{ "INID",		2, 0xAA, { 0, 0 } },
	{ "OUTD",		2, 0xAB, { 0, 0 } },
	{ "LDIR",		2, 0xB0, { 0, 0 } },
	{ "CPIR",		2, 0xB1, { 0, 0 } },
	{ "INIR",		2, 0xB2, { 0, 0 } },
	{ "OTIR",		2, 0xB3, { 0, 0 } },
	{ "LDDR",		2, 0xB8, { 0, 0 } },
	{ "CPDR",		2, 0xB9, { 0, 0 } },
	{ "INDR",		2, 0xBA, { 0, 0 } },
	{ "OTDR",		2, 0xBB, { 0, 0 } },
	{ "",			-1, 0x00, { 0, 0 } }
};



static struct tydb_nim_t nim_cb_tbl[] = {
	{ "RLC %R",		2, 0x00, { 0, 0 } },
	{ "RRC %R",		2, 0x08, { 0, 0 } },
	{ "RL %R",		2, 0x10, { 0, 0 } },
	{ "RR %R",		2, 0x18, { 0, 0 } },
	{ "SLA %R",		2, 0x20, { 0, 0 } },
	{ "SRA %R",		2, 0x28, { 0, 0 } },
	{ "SRL %R",		2, 0x38, { 0, 0 } },
	{ "BIT %B, %R",		2, 0x40, { 0, 0 } },
	{ "RES %B, %R",		2, 0x80, { 0, 0 } },
	{ "SET %B, %R",		2, 0xC0, { 0, 0 } },
	{ "",			-1, 0x00, { 0, 0 } }
};


static struct tydb_nim_t nim_tbl[] = {
	{ "NOP",		1, 0x00, { 0, 0 } },
	{ "LD %P, %W",		3, 0x01, { OPR_WORD, 0 } },
	{ "LD (BC), A",		1, 0x02, { 0, 0 } },
	{ "INC %P",		1, 0x03, { 0, 0 } },
	{ "INC %R",		1, 0x04, { 0, 0 } },
	{ "DEC %R",		1, 0x05, { 0, 0 } },
	{ "LD %R, %N",		2, 0x06, { OPR_BYTE, 0 } },
	{ "RLCA",		1, 0x07, { 0, 0 } },
	{ "EX AF, 'AF",		1, 0x08, { 0, 0 } },
	{ "ADD HL, %P",		1, 0x09, { 0, 0 } },
	{ "LD A, (BC)",		1, 0x0A, { 0, 0 } },
	{ "DEC %P",		1, 0x0B, { 0, 0 } },
	{ "RRCA",		1, 0x0F, { 0, 0 } },
	{ "DJNZ %J",		2, 0x10, { OPR_JRDISP, 0 } },
	{ "LD (DE),A",		1, 0x12, { 0, 0 } },
	{ "RLA",		1, 0x17, { 0, 0 } },
	{ "JR %J",		2, 0x18, { OPR_JRDISP, 0 } },
	{ "LD A, (DE)",		1, 0x1A, { 0, 0 } },
	{ "RRA",		1, 0x1F, { 0, 0 } },
	{ "JR %C, %J",		2, 0x20, { OPR_JRDISP, 0 } },
	{ "LD (%W), HL",	3, 0x22, { OPR_WORD, 0 } },
	{ "DAA",		1, 0x27, { 0, 0 } },
	{ "LD HL, %W",		3, 0x2A, { OPR_WORD, 0 } },
	{ "CPL",		1, 0x2F, { 0, 0 } },
	{ "LD (%W), A",		3, 0x32, { OPR_WORD, 0 } },
	{ "INC (HL)",		1, 0x34, { 0, 0 } },
	{ "DEC (HL)",		1, 0x35, { 0, 0 } },
	{ "LD (HL), %N",	2, 0x36, { OPR_BYTE, 0 } },
	{ "SCF",		1, 0x37, { 0, 0 } },
	{ "LD A, (%W)",		3, 0x3A, { OPR_WORD, 0 } },
	{ "CCF",		1, 0x3F, { 0, 0 } },
	{ "LD %R, %Q",		1, 0x40, { 0, 0 } },
	{ "HALT",		1, 0x76, { 0, 0 } },
	{ "ADD A, %R",		1, 0x80, { 0, 0 } },
	{ "ADC A, %R",		1, 0x88, { 0, 0 } },
	{ "SUB A, %R",		1, 0x90, { 0, 0 } },
	{ "SBC A, %R",		1, 0x98, { 0, 0 } },
	{ "LD SP, HL",		1, 0xF9, { 0, 0 } },
	{ "AND %R",		1, 0xA0, { 0, 0 } },
	{ "XOR %R",		1, 0xA8, { 0, 0 } },
	{ "OR %R",		1, 0xB0, { 0, 0 } },
	{ "CP %R",		1, 0xB8, { 0, 0 } },
	{ "RET %C",		1, 0xC0, { 0, 0 } },
	{ "POP %P",		1, 0xC1, { 0, 0 } },
	{ "JP %C, %W",		3, 0xC2, { OPR_WORD, 0 } },
	{ "JP %W",		3, 0xC3, { OPR_WORD, 0 } },
	{ "CALL %C, %W",	3, 0xC4, { OPR_WORD, 0 } },
	{ "PUSH %P",		1, 0xC5, { 0, 0 } },
	{ "ADD A, %N",		2, 0xC6, { OPR_BYTE, 0 } },
	{ "RST %T",		1, 0xC7, { 0, 0 } },
	{ "RET",		1, 0xC9, { 0, 0 } },
	{ "CALL %W",		3, 0xCD, { OPR_WORD, 0 } },
	{ "ADC A, %N",		2, 0xCE, { OPR_BYTE, 0 } },
	{ "OUT (%N), A",	2, 0xD3, { OPR_BYTE, 0 } },
	{ "SUB A, %N",		2, 0xD6, { OPR_BYTE, 0 } },
	{ "EXX",		1, 0xD9, { 0, 0 } },
	{ "IN A, (%N)",		2, 0xDB, { OPR_BYTE, 0 } },
	{ "SBC A, %N",		2, 0xDE, { OPR_BYTE, 0 } },
	{ "EX (SP), HL",	1, 0xE3, { 0, 0 } },
	{ "AND %N",		2, 0xE6, { OPR_BYTE, 0 } },
	{ "JP (HL)",		1, 0xE9, { 0, 0 } },
	{ "EX DE, HL",		1, 0xEB, { 0, 0 } },
	{ "XOR %N",		2, 0xEE, { OPR_BYTE, 0 } },
	{ "DI",			1, 0xF3, { 0, 0 } },
	{ "OR %N",		2, 0xF6, { OPR_BYTE, 0 } },
	{ "EI",			1, 0xFB, { 0, 0 } },
	{ "CP %N",		2, 0xFE, { OPR_BYTE, 0 } },
	{ "",			-1, 0x00, { 0, 0 } }
};


static int indx_tbl[256];
static int indx_cb_tbl[256];
static int ed_tbl[256];
static int cb_tbl[256];
static int nn_tbl[256];


#define MASKEQ(a, b, c) (((a) & (b)) == (c))

#define GETOPRD(s, addr, idxr) { \
	int i, rdisp; \
	for (i = 0; i < 2; i++) \
	{ \
		switch ((s).nim.oprds[i]) \
		{ \
		case OPR_BYTE: \
			sprintf((s).byte, "%02XH", zdb_rdmem(++(addr))); break; \
		case OPR_WORD: \
			sprintf((s).word, "%04XH", zdb_rdmem2(++(addr))); break; \
		case OPR_RDISP: \
			rdisp = zdb_rdmem(++(addr)); \
			if (rdisp & 0x80) \
				sprintf((s).rdisp, "-%02XH", 256 - rdisp); \
			else \
				sprintf((s).rdisp, "+%02XH", rdisp); \
			sprintf((s).raddr, "%04XH", idxr + (Sint8) rdisp); \
			break; \
		case OPR_JRDISP: \
			rdisp = ((addr) + 1) + (Sint8)zdb_rdmem(++(addr)); \
			sprintf((s).rdisp, "%04XH", rdisp); \
			break; \
		} \
	} \
}


#define CB_BIT { \
	s.bit = nim_bb_tbl[(c2 & 0x38) >> 3]; \
	s.nim = nim_cb_tbl[cb_tbl[c2 & 0xC0]]; \
	break; \
}

#define ED_REG_SS { \
	s.reg1 = nim_ss_tbl[(c2 & 0x30) >> 4]; \
	s.nim = nim_ed_tbl[ed_tbl[c2 & 0x4F]]; \
	break; \
}

#define ED_REG_R { \
	s.reg1 = nim_r_tbl[(c2 & 38) >> 3]; \
	s.nim = nim_ed_tbl[ed_tbl[0x41]]; \
	break; \
}

#define XDCB_BIT { \
	s.bit = nim_bb_tbl[(c3 & 0x38) >> 3]; \
	s.nim = nim_indx_cb_tbl[indx_cb_tbl[c3 & 0xC7]]; \
}

#define XD_REG_R { \
	s.reg1 = nim_r_tbl[(c2 & 0x38) >> 3]; \
	s.nim = nim_indx_tbl[indx_tbl[0x46]]; \
}

#define XD_REG_R2 { \
	s.reg1 = nim_r_tbl[c2 & 0x7]; \
	s.nim = nim_indx_tbl[indx_tbl[0x70]]; \
}

#define XD_REG_RR_PP { \
	s.reg1 = nim_rr_tbl[(c2 & 0x30) >> 4]; \
	if (c1 == 0xFD) \
		s.reg1 = nim_pp_tbl[(c2 & 0x30) >> 4]; \
	s.nim = nim_indx_tbl[indx_tbl[0x09]]; \
}


#define NOPR_CC_RT { \
	if (!MASKEQ(c1, 0xC7, 0xC7)) \
		s.cc = nim_cc_tbl[(c1 & 0x38) >> 3]; \
	else \
		s.rst = nim_rt_tbl[(c1 & 0x38) >> 3]; \
	s.nim = nim_tbl[nn_tbl[c1 & 0xC7]]; \
	break; \
}

#define NOPR_REG_QQ { \
	s.reg1 = nim_qq_tbl[(c1 & 0x30) >> 4]; \
	s.nim = nim_tbl[nn_tbl[c1 & 0xCF]]; \
	break; \
}

#define NOPR_REG_R { \
	s.reg1 = nim_r_tbl[c1 & 0x07]; \
	s.nim = nim_tbl[nn_tbl[c1 & 0xF8]]; \
	break; \
}

#define NOPR_REG_R1_R2 { \
	s.reg1 = nim_r_tbl[(c1 & 0x38) >> 3]; \
	s.reg2 = nim_r_tbl[c1 & 0x07]; \
	s.nim = nim_tbl[nn_tbl[c1 & 0xC0]]; \
	break; \
}

#define NOPR_REG_SS { \
	s.reg1 = nim_ss_tbl[(c1 & 0x30) >> 4]; \
	s.nim = nim_tbl[nn_tbl[c1 & 0xCF]]; \
	break; \
}

#define NOPR_REG_R2 { \
	s.reg1 = nim_r_tbl[(c1 & 0x38) >> 3]; \
	s.nim = nim_tbl[nn_tbl[c1 & 0xC7]]; \
	break; \
}

#define NOPR_CC { \
	s.cc = nim_cc_tbl[(c1 & 0x18) >> 3]; \
	s.nim = nim_tbl[nn_tbl[c1 & 0x20]]; \
	break; \
}


struct tydb_params_t *tydebug_init(void);
static int tydebug_disasm(struct tydb_params_t *params);
static void tydebug_display_regs(struct tydb_params_t *params);
/* int tydebug_main(int addr, struct tydb_params_t *params); */



struct tydb_params_t *tydebug_init(void)
{
	int i;
	struct tydb_params_t *params;

	/* 
		ED で該当しないものは DB 扱いとする。
	*/
	memset(ed_tbl, 0x00, sizeof(int) * 256);

	for (i = 0; nim_indx_tbl[i].nbyte != -1; i++)
		indx_tbl[nim_indx_tbl[i].loc] = i;

	for (i = 0; nim_indx_cb_tbl[i].nbyte != -1; i++)
		indx_cb_tbl[nim_indx_cb_tbl[i].loc] = i;

	for (i = 0; nim_ed_tbl[i].nbyte != -1; i++)
		ed_tbl[nim_ed_tbl[i].loc] = i;

	for (i = 0; nim_cb_tbl[i].nbyte != -1; i++)
		cb_tbl[nim_cb_tbl[i].loc] = i;

	for (i = 0; nim_tbl[i].nbyte != -1; i++)
		nn_tbl[nim_tbl[i].loc] = i;

	params = mem_alloc(sizeof(struct tydb_params_t));
	memset(params, 0x00, sizeof(struct tydb_params_t));

	for(i = 0; i < TYDB_TP_MAX; i++)
		params->tp_addr[i] = -1;

	params->tp_addr[0] = 0x0;	/* XXX */

	params->tp_nums = TYDB_TP_MAX;
	params->is_init = 1;
	params->cnt = 0;
	return params;
}


static int tydebug_disasm(struct tydb_params_t *params)
{
	char buf[256], *p, *p1, *p2;
	unsigned char c1, c2, c3;
	struct tydb_nimparams_t s;
	int idxr = 0;
	int addr;

	addr = params->da_addr;
	c1 = zdb_rdmem(addr);

	switch (c1) {
	case 0xCB:
		c2 = zdb_rdmem(++addr);

		s.reg1 = nim_r_tbl[c2 & 0x07];
		/* ====
			01 bbb rrr	BIT b, r
			10 bbb rrr	RES b, r
			11 bbb rrr	SET b, r
		   ==== */
		if (MASKEQ(c2, 0xC0, 0x40) ||
		    MASKEQ(c2, 0xC0, 0x80) ||
		    MASKEQ(c2, 0xC0, 0xC0))
			CB_BIT;

		s.nim = nim_cb_tbl[cb_tbl[c2 & 0xF8]];
		break;

	case 0xED:
		c2 = zdb_rdmem(++addr);

		/* ====
			01 ss 1011	LD ss, (nn)
			01 ss 1010	ADC HL, ss
			01 ss 0011	LD (nn), ss
			01 ss 0010	SBC HL, ss
		   ==== */
		if (MASKEQ(c2, 0xCF, 0x4B) ||
		    MASKEQ(c2, 0xCF, 0x4A) ||
		    MASKEQ(c2, 0xCF, 0x43) ||
		    MASKEQ(c2, 0xCF, 0x42))
		{
			ED_REG_SS;
		}
		/* ====
			01 rrr 001	OUT (C), r
			01 rrr 000	IN r, (C)
		   ==== */
		else if (MASKEQ(c2, 0xC7, 0x41) ||
			 MASKEQ(c2, 0xC7, 0x40))
			ED_REG_R;
	
		/* ==== others ==== */
		s.nim = nim_ed_tbl[ed_tbl[c2]];
		break;

	case 0xDD:					/* FALLTHROUGH */
	case 0xFD:
		s.idx = nim_pp_tbl[2];		/* IX */
		idxr = Z80_IX;
		if (c1 == 0xFD)
		{
			s.idx = nim_rr_tbl[2];	/* IY */
			idxr = Z80_IY;
		}

		c2 = zdb_rdmem(++addr);
		/* ==== '0xCB' 2nd prefix ==== */
		if (c2 == 0xCB)
		{
			/* rotate or shift or bit oprs */
			c3 = zdb_rdmem(addr + 2);

			/* bit oprs */
			if (c3 & 0xC0)
				XDCB_BIT;

			break;
		}

		/* ==== 0111 0 rrr	LD (IXY+d), r ==== */
		if (MASKEQ(c2, 0xF8, 0x70))
		{
			XD_REG_R2;
		}
		/* ==== 01 rrr 110	LD r, (IXY+d) ==== */
		else if (MASKEQ(c2, 0xC7, 0x46))
		{
			XD_REG_R;
		}
		/* ==== 00 ss 1001	ADD IXY, ss ==== */
		else if (MASKEQ(c2, 0xCF, 0x09))
		{
			XD_REG_RR_PP;
		}
		/* ==== others ==== */
		else
			s.nim = nim_indx_tbl[indx_tbl[c2]];

		break;

	default:
		/* ==== 11 ttt 111	RST t ==== */
		if (MASKEQ(c1, 0xC7, 0xC7))
		{
			NOPR_CC_RT;
		}
		/* ==== 11 rr 0101	PUSH ss ==== */
		else if (MASKEQ(c1, 0xCF, 0xC5))
		{
			NOPR_REG_QQ;
		}
		/* ====
			11 ccc 100	CALL cc, nn
			11 ccc 010	JP cc, nn
		   ==== */
		else if (MASKEQ(c1, 0xC7, 0xC4) || MASKEQ(c1, 0xC7, 0xC2))
		{
			NOPR_CC_RT;
		}
		/* ==== 11 rr 0001	POP ss ==== */
		else if (MASKEQ(c1, 0xCF, 0xC1))
		{
			NOPR_REG_QQ;
		}
		/* ==== 11 ccc 000	RET cc ==== */
		else if (MASKEQ(c1, 0xC7, 0xC0))
		{
			NOPR_CC_RT;
		}
		/* ====
			1011 1 rrr	CP r
			1011 0 rrr	OR r
			1010 1 rrr	XOR r
			1010 0 rrr	AND r
			1001 1 rrr	SBC r
			1001 0 rrr	SUB r
			1000 1 rrr	ADC r
			1000 0 rrr	ADD r
		   ==== */
		else if (MASKEQ(c1, 0xF8, 0xB8) ||
			 MASKEQ(c1, 0xF8, 0xB0) ||
			 MASKEQ(c1, 0xF8, 0xA8) ||
			 MASKEQ(c1, 0xF8, 0xA0) ||
			 MASKEQ(c1, 0xF8, 0x98) ||
			 MASKEQ(c1, 0xF8, 0x90) ||
			 MASKEQ(c1, 0xF8, 0x88) ||
			 MASKEQ(c1, 0xC0, 0x80))
		{
			NOPR_REG_R;
		}
		/* ==== 01 rrr ppp	LD r, p ==== */
		else if (MASKEQ(c1, 0xC0, 0x40))
		{
			NOPR_REG_R1_R2;
		}
		/* ==== 001 cc 0000	JR c, d ==== */
		else if (MASKEQ(c1, 0xE7, 0x20))
		{
			NOPR_CC;
		}
		/* ====
			00 rr 1011	DEC ss
			00 rr 1001	ADD HL, ss
		   ==== */
		else if (MASKEQ(c1, 0xCF, 0x0B) ||
			 MASKEQ(c1, 0xCF, 0x09))
		{
			NOPR_REG_SS;
		}
		/* ==== 00 rrr 110	LD r, n ==== */
		else if (MASKEQ(c1, 0xC7, 0x06))
		{
			NOPR_REG_R2;
		}
		/* ====
			00 rrr 101	DEC r
			00 rrr 100	INC r
		   ==== */
		else if (MASKEQ(c1, 0xC7, 0x05) || MASKEQ(c1, 0xC7, 0x04))
		{
			NOPR_REG_R2;
		}
		/* ==== 00 rr 0011	INC ss ==== */
		else if	(MASKEQ(c1, 0xCF, 0x03))
		{
			NOPR_REG_SS;
		}
		/* ==== 00 ss 0001	LD ss, nn ==== */
		else if (MASKEQ(c1, 0xCF, 0x01))
		{
			NOPR_REG_SS;
		}
		/* ==== others ==== */
		else
			s.nim = nim_tbl[nn_tbl[c1]];

		break;
	}

	GETOPRD(s, addr, idxr);
	p = s.nim.s;
	buf[0] = '\0';

	while(1) {
		p1 = strchr(p, '%');
		if (!p1) {
			strcat(buf, p);
			break;
		}

		switch (*(p1 + 1))
		{
		case 'N': p2 = s.byte; break;
		case 'W': p2 = s.word; break;
		case 'R': p2 = s.reg1; break;
		case 'J': p2 = s.rdisp; break;
		case 'P': p2 = s.reg1; break;
		case 'Q': p2 = s.reg2; break;
		case 'I': p2 = s.idx; break;
		case 'D': p2 = s.rdisp; break;
		case 'B': p2 = s.bit; break;
		case 'T': p2 = s.rst; break;
		case 'C': p2 = s.cc; break;
		case '@': p2 = s.raddr; break;
		}
		strncat(buf, p, p1 - p);
		strcat(buf, p2);
		p = p1 + 2;
	}

	sprintf(params->s, "%s", buf);
	return s.nim.nbyte;
}


static void tydebug_display_regs(struct tydb_params_t *params)
{
	char fstr[9];
	int i, b;
	unsigned char sl, esl;

	for (i = 0, b = Z80_F; i < 8; i++, b <<= 1)
		fstr[i] = (b & 0x80) ? (flags[i]) : ('.');
	
	fstr[8] = '\0';

	printf("AF :%04X  BC :%04X  DE :%04X  HL :%04X\n", Z80_AF, Z80_BC, Z80_DE, Z80_HL);
	printf("AF':%04X  BC':%04X  DE':%04X  HL':%04X\n", Z80_AFD, Z80_BCD, Z80_DED, Z80_HLD);
	printf("IX :%04X  IY :%04X  SP :%04X  PC :%04X  I :%02X\n", Z80_IX, Z80_IY, Z80_SP, Z80_PC, Z80_I);

	sl = msx.psl;
	esl = zdb_rdmem(0xffff);
	printf("SLOT :#%d-%d #%d-%d #%d-%d #%d-%d\n",
			(sl & 0xC0) >> 6,	(esl & 0xC0) >> 6,
			(sl & 0x30) >> 4,	(esl & 0x30) >> 4,
			(sl & 0x0C) >> 2,	(esl & 0x0C) >> 2,
			 sl & 0x03,		 esl & 0x03);

	tydebug_disasm(params);
	printf("@PC: [%04X:%02X -> \"%s\"]\n", Z80_PC, zdb_rdmem(Z80_PC), params->s);
	printf("@SP: [%04X]    FLAGS:[%s]    IFF: [%s:%s]\n\n",
		z80_rdmem2(Z80_SP),
		fstr,
		iff_modes[z80.iflag & Z80_IF_MODEM],
		(z80.iflag & Z80_IFF1) ? ("EI") : ("DI"));
}


#define LIMIT_ADDR(adr) ((adr) & 0xffff)

int tydebug_main(int addr, struct tydb_params_t *params)
{
	char ibuf[256];
	int i, j, k;

	params->da_addr = addr;
	tydebug_display_regs(params);

	/* XXX */
	while(1)
	{
		printf("poka-n> ");
		fflush(stdout);
		fflush(stdin);

		fgets(ibuf, 100, stdin);
		for (i = 0; ibuf[i]; ibuf[i] = toupper(ibuf[i]), i++);
		switch (ibuf[0])
		{
		/* help */
		case 'H':
		case '?':
			puts("\n basic opr.");
			puts("   <CR>          : step next 1 instruction");
			puts("   ? or H        : (this) help");
			puts("   Q             : quit");

			puts("\n running opr.");
			puts("   G             : continue");
			puts("   J addr        : jump to address");
			puts("   S n           : step n instructions.");
			
			puts("\n trap opr.");
			puts("   T n addr      : set trap address at bank n");
			puts("   C n           : clear trap address at bank n");
			puts("   @             : list trap addresses");

			puts("\n listing opr.");
			puts("   D addr n      : disassemble");
			puts("   M addr n      : memory dump");
			break;

		/* quit */
		case 'Q':
			/* return 1; */
			exit(1);
			break;

		/* trap */
		case 'T':
			if (strlen(ibuf) >= 2)
			{
				int sel, taddr;
		
				if (sscanf(ibuf + 1, "%d %X", &sel, &taddr) == 2)
				{
					sel %= params->tp_nums;
					params->tp_addr[sel] = LIMIT_ADDR(taddr);
				}
			}
			break;
		case 'C':
			if (strlen(ibuf) >= 2)
			{
				int sel;

				sel = (sscanf(ibuf + 1, "%d", &sel) == 1) ? (sel % params->tp_nums) : (0);
				params->tp_addr[sel] = -1;
			}
			break;
		case '@':
			for (i = 0; i < params->tp_nums; i++)
				printf("TRAP:#%02d -> %04XH\n", i, params->tp_addr[i]);
			break;

		/* go */
		case 'G':
			params->is_enable = 0;
			return 0;
			break;

		/* jump */
		case 'J':
			if (strlen(ibuf) >= 2)
			{
				int jaddr;

				if (sscanf(ibuf + 1, "%X", &jaddr) == 1)
				{
					Z80_PC = LIMIT_ADDR(jaddr);
					params->is_enable = 0;
					return 0;
				}
			}
			break;

		/* step */
		case 'S':
			if (strlen(ibuf) >= 2)
			{
				int n;

				params->cnt = (sscanf(ibuf + 1, "%d", &n) == 1) ? n : 1;
				return 0;
			}
			break;

		/* disasm */
		case 'D':
			if (strlen(ibuf) >= 2)
			{
				int n, daddr;

				n = 1;
				if (sscanf(ibuf + 1, "%X %d", &daddr, &n) == 0)
					break;

				/* daddr = LIMIT_ADDR(daddr); */
				for (i = 0; i < n; i++)
				{
					params->da_addr = LIMIT_ADDR(daddr);
					daddr += tydebug_disasm(params);
					printf("%04X: %s\n", params->da_addr, params->s);
				}
			}
			break;

		/* mem dump */
		case 'M':
			if (strlen(ibuf) >= 2)
			{
				int n, daddr;

				n = 1;
				if (sscanf(ibuf + 1, "%X %d", &daddr, &n) == 0)
					break;

				daddr = LIMIT_ADDR(daddr);
				for (i = 0; i < n; i++)
					for(j = 0; j < 16; j++)
					{
						printf("%04X: ", LIMIT_ADDR(daddr));
						for(k = 0; k < 16; k++, daddr++)
							printf("%02X ", zdb_rdmem(LIMIT_ADDR(daddr)));

						printf(" | ");

						for(daddr -= 16, k = 0; k < 16; k++, daddr++)
							putchar(isprint(zdb_rdmem(LIMIT_ADDR(daddr))) ? 
								zdb_rdmem(LIMIT_ADDR(daddr)) : '.');

						printf("\n");
					}
			}
			break;

		case '\n':						/* XXX */
		case '\0':
			return 0;
			break;
		}
	}
	return 0;
}





int zdb_tiny_init(zdb_t *p_zdb)
{
	p_zdb->internal = tydebug_init();
	return 0;
}

void zdb_tiny_fin(zdb_t *p_zdb)
{
}


int zdb_check(int addr, struct tydb_params_t *params)
{
	int i;

	if (params->cnt > 0)
	{
		params->cnt--;
		return (params->cnt == 0) ? 1 : 0;
	}

	if (tydb_params->is_enable)
		return 1;

	for (i = 0; i < tydb_params->tp_nums; ++i)
	{
		if (Z80_PC == tydb_params->tp_addr[i])
			return 1;
	}

	return 0;
}

int zdb_tiny_main(zdb_t *p_zdb)
{
	if (!zdb_check(Z80_PC, p_zdb->internal))
		return 0;

	return tydebug_main(Z80_PC, p_zdb->internal);
}

void zdb_tiny_onoff(zdb_t *p_zdb)
{
	struct tydb_params_t *params;

	params = p_zdb->internal;
	params->is_enable = ~(params->is_enable);
}




/*
 *************************************************************

    XXX ext debugger.

    とりあえずここに書いてみるの試験。いずれどこかへ移すやも。

 *************************************************************
*/
struct tydb_params_t *extdebug_init(void);


struct tydb_params_t *extdebug_init(void)
{
	int i;
	struct tydb_params_t *params;

	/* setup debugger params */
	params = mem_alloc(sizeof(struct tydb_params_t));
	memset(params, 0x00, sizeof(struct tydb_params_t));

	for(i = 0; i < TYDB_TP_MAX; i++)
		params->tp_addr[i] = -1;

	params->tp_addr[0] = 0x0;	/* XXX */

	params->tp_nums = TYDB_TP_MAX;
	params->is_init = 1;
	params->cnt = 0;
	return params;
}

int extdebug_main(int addr, struct tydb_params_t *params)
{
	return 0;
}

void extdebug_fin(struct tydb_params_t *params)
{
}


int zdb_ext_init(zdb_t *p_zdb)
{
#ifdef EXTDEBUGGER
	p_zdb->internal = extdebug_init();
	return (p_zdb->internal == NULL) ? -1 : 0;
#else
	return 0;
#endif
}

void zdb_ext_fin(zdb_t *p_zdb)
{
#ifdef EXTDEBUGGER
	extdebug_fin(p_zdb->internal);
#endif
}


int zdb_ext_main(zdb_t *p_zdb)
{
#ifdef EXTDEBUGGER
	if (!zdb_check(Z80_PC, p_zdb->internal))
		return 0;

	return extdebug_main(Z80_PC, p_zdb->internal);
#else
	return 0;
#endif
}

void zdb_ext_onoff(zdb_t *p_zdb)
{
#ifdef EXTDEBUGGER
	struct tydb_params_t *params;

	params = p_zdb->internal;
	params->is_enable = ~(params->is_enable);
#endif
}
