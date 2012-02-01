#ifndef __V99X8_H
#define __V99X8_H

#include "../misc/ut.h"
#include <md.h>


#define V99X8_NREG 48
#define V99X8_NSTAT 10

typedef struct
{
	Uint8 ctrl[V99X8_NREG], status[V99X8_NSTAT];

	Uint8 col_fg, col_bg;
	bool f_tms;
	int scr;
	Uint8 *tbl_pg, *tbl_pn, *tbl_cl;
	bool f_interleave; /* sc7/8 等における特別なマッピングモード */

	int pages;	/* VRAM memory size */
	bool f_zoom;

	Uint8 *vram;

	int scanline, n_scanlines; // 処理中の scanline と scanline 数。
	                               // ??? もっとよいネーミング？
} v99x8_t;

VAR v99x8_t v99x8;


extern void v99x8_init(void);
extern void v99x8_ctrl(int n, Uint8 m);
extern int v99x8_hsync(void);

extern Uint8 v99x8_in_0(void);	/* VRAM read */
extern Uint8 v99x8_in_1(void);	/* status in */

extern void v99x8_out_0(Uint8 n);	/* VRAM write */
extern void v99x8_out_1(Uint8 n);	/* ctrl out */
extern void v99x8_out_2(Uint8 n);	/* palette out */
extern void v99x8_out_3(Uint8 n);	/* ctrl out */


extern void v99x8_pallete_set(Uint8 n, Uint8 r, Uint8 g, Uint8 b);
extern void v99x8_refresh_init(void);
extern void v99x8_refresh_screen(void);
extern void v99x8_refresh_clear(void);
extern void v99x8_refresh_sc0(int y, int h);
extern void v99x8_refresh_sc1(int y, int h);
extern void v99x8_refresh_sc2(int y, int h);
extern void v99x8_refresh_sc3(int y, int h);
extern void v99x8_refresh_sc4(int y, int h);
extern void v99x8_refresh_sc5(int y, int h);
extern void v99x8_refresh_sc6(int y, int h);
extern void v99x8_refresh_sc7(int y, int h);
extern void v99x8_refresh_sc8(int y, int h);
extern void v99x8_refresh_sca(int y, int h);
extern void v99x8_refresh_scc(int y, int h);
extern void v99x8_refresh_scx(int y, int h);

extern Uint8 vram_read(int addr);
extern void vram_write(int addr, Uint8 n);

/*
#define VRAM_ADDR(addr) (v99x8.f_interleave ? \
                          (addr >> 1) | ((addr & 1) << 16) : \
                          addr)
*/

#endif /* __V99X8_H */
