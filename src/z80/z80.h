#ifndef __Z80_H
#define __Z80_H

#include <md.h>
#include "../z80_config.h"

#include "../zdb.h"


#ifndef Z80_PAGEBIT
#	define Z80_PAGEBIT 0
#endif

#define Z80_GETPAGE(n) ((n) >> (16 - Z80_PAGEBIT))
#define Z80_GETADDR(n) ((n) & ((1 << (16 - Z80_PAGEBIT)) - 1))
#define Z80_NPAGE (1 << Z80_PAGEBIT)

/* XXX
    o public と private をきっちりわける。
    o z80_* と z80 をうまくひとまとめにする。
*/


typedef struct
{
	pair16l_t AF, BC, DE, HL;
	pair16l_t IX, IY, PC, SP;
	pair16l_t AFD, BCD, DED, HLD;
	Uint8 I, R;


/* XXX private */
	int iflag, ivec;

	Uint8 (*in[256])(void);
	void (*out[256])(Uint8);

	int cycle, n_cycles;        /* XXX */

	zdb_t *zdb;
} z80_t;

typedef struct
{
	Uint16 addr;
	int (*func)(void);
} z80_patch_t;

typedef struct page
{
	Uint8 *mem;
	Uint8 (*read)(struct page *p, int page, Uint16 addr);
	void (*write)(struct page *p, int page, Uint16 addr, Uint8 n);
	const z80_patch_t *patch;
} z80_page_t;

extern z80_t z80;               /* XXX 隠蔽すべき？ */
extern z80_page_t z80_page[Z80_NPAGE];


#define Z80_NOINT 0xffff
#define Z80_INT 0x0038
#define Z80_NMI 0x0066

extern	void z80_init(int hz);

extern void z80_intreq(int a);
extern void z80_nmireq(void);
extern void z80_interrupt(void);

extern void z80_in_install(int n, Uint8(*func)(void));
extern void z80_out_install(int n, void (*func)(Uint8));
extern void z80_patch_install(void *base, const z80_patch_t *patch);
extern int z80_patch(void);
extern int z80_patch_nop(void);

extern void z80_cycle(int cycles);

extern void z80_di(void);
extern void z80_ei(void);






#define Z80_AF z80.AF.w
#define Z80_BC z80.BC.w
#define Z80_DE z80.DE.w
#define Z80_HL z80.HL.w
#define Z80_SP z80.SP.w
#define Z80_PC z80.PC.w
#define Z80_A  z80.AF.b.h
#define Z80_F  z80.AF.b.l
#define Z80_B  z80.BC.b.h
#define Z80_C  z80.BC.b.l
#define Z80_D  z80.DE.b.h
#define Z80_E  z80.DE.b.l
#define Z80_H  z80.HL.b.h
#define Z80_L  z80.HL.b.l
#define Z80_I  z80.I
#define Z80_R  z80.R

#define Z80_IX  z80.IX.w
#define Z80_IY  z80.IY.w
#define Z80_IXH z80.IX.b.h
#define Z80_IXL z80.IX.b.l
#define Z80_IYH z80.IY.b.h
#define Z80_IYL z80.IY.b.l

#define Z80_AFD z80.AFD.w
#define Z80_BCD z80.BCD.w
#define Z80_DED z80.DED.w
#define Z80_HLD z80.HLD.w

// Flags
#define Z80_SF 0x80
#define Z80_ZF 0x40
#define Z80_HF 0x10
#define Z80_PF 0x04
#define Z80_VF 0x04
#define Z80_NF 0x02
#define Z80_CF 0x01


#ifndef Z80_FASTMEM
	extern Uint8 z80_rdmem(int addr);
	extern void z80_wrmem(Uint16 addr, Uint8 n);
	extern Uint16 z80_rdmem2(int addr);
	extern void z80_wrmem2(Uint16 addr, Uint16 n);
#else
#	ifdef Z80_FASTMEM_USE
#		include "../z80_fastmem.h"
#	endif
#endif


/* z80 tiny debugger. 暫定簡易デバッガ */

#define TYDB_TP_MAX 10

struct tydb_params_t {
	int is_enable;
	int is_init;
	int is_external;

	int da_addr;
	int dp_addr;
	int tp_addr[TYDB_TP_MAX];
	int tp_nums;
	int cnt;

	char s[256];
	int sock_fd;
};

extern struct tydb_params_t *tydb_params;

//extern struct tydb_params_t *tydebug_init(void);
//extern int tydebug_main(int addr, struct tydb_params_t *params);



#define zdb_rdmem z80_rdmem
#define zdb_rdmem2 z80_rdmem2
#define zdb_wrmem z80_wrmem
#define zdb_wrmem2 z80_wrmem2
extern void zdb_rdmemn(char *p, int addr, size_t bytes);
extern void zdb_wrmemn(const char *p, int addr, size_t bytes);


//extern int extdebug_main(int addr, struct tydb_params_t *params);




#endif /* __Z80_H */

