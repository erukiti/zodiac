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

#include "config.h"

#include <assert.h>


#include "../zodiac.h"
#include "msx.h"

// XXX
extern void *mem_readfile(const char *s, size_t off, size_t size);

extern void msx_slot_ssl(int n);



	/* MSX generic read/write handler  */

static void ignore_write(struct page *p, int page, Uint16 addr, Uint8 n)
{
assert(addr < 0x2000);
assert(page < 7);

	;
}

static void ignore_write_page7(struct page *p, int page, Uint16 addr, Uint8 n)
{
assert(addr < 0x2000);
assert(page < 8);

	if (addr == 0x1fff)
		msx_slot_ssl(n);
}

static Uint8 mem_read_page7(struct page *p, int page, Uint16 addr)
{
assert(addr < 0x2000);
assert(page < 8);

	if (addr != 0x1fff)
		return z80_page[7].mem[addr];
	return ~msx.ssl[msx.psl >> 6];
}

static void mem_write_page7(struct page *p, int page, Uint16 addr, Uint8 n)
{
assert(addr < 0x2000);
assert(page < 8);

	if (addr == 0x1fff)
		msx_slot_ssl(n);
	else
		z80_page[7].mem[addr] = n;
}





	/* cartridge implementaion */

void msx_empty_slot(msx_slot_t *p, Uint8 n)
{
assert(n < 4);

	n <<= 1;

	z80_page[n].mem   = EMPTYIMAGE;
	z80_page[n].read  = NULL;
	z80_page[n].write = ignore_write;

	++n;
	z80_page[n].mem   = EMPTYIMAGE;
	if (n != 7 || !msx.f_extslot[msx.psl >> 6])
	{
		z80_page[n].read  = NULL;
		z80_page[n].write = ignore_write;
	} else
	{
		z80_page[n].read  = mem_read_page7;
		z80_page[n].write = ignore_write_page7;
	}
}


static void rom_slot(msx_slot_t *p, Uint8 n)
{
assert(n < 4);

	n <<= 1;

	z80_page[n].patch = p->patch;
	z80_page[n].mem   = p->mem + ((Uint8 *)p->ptr)[n] * 0x2000;
	z80_page[n].read  = NULL;
	z80_page[n].write = ignore_write;

	++n;
	z80_page[n].patch = p->patch;
	z80_page[n].mem   = p->mem + ((Uint8 *)p->ptr)[n] * 0x2000;
	if (n != 7 || !msx.f_extslot[msx.psl >> 6])
	{
		z80_page[n].read  = NULL;
		z80_page[n].write = ignore_write;
	} else
	{
		z80_page[n].read  = mem_read_page7;
		z80_page[n].write = ignore_write_page7;
	}
/*printf("rom %p\n", p->mem); */
}

msx_slot_t *msx_rom_load(Uint8 psl, Uint8 ssl, int offset, int size, const char *s, const z80_patch_t *patch)
{
	msx_slot_t *p;
	Uint8 *q;
	int i;
	int n;

	q = (Uint8 *)mem_readfile(s, 0, size * 0x2000);
	if (q == NULL)
		return NULL;

	p = (msx_slot_t *)mem_alloc(sizeof(msx_slot_t));
//printf("rom %d %d %d: %s %p  (%p)\n", psl, ssl, offset, s, p, q);

	p->mem = q;
	p->change = rom_slot;
	p->offset = offset;
	p->size = size;
	p->ptr = mem_alloc(sizeof(Uint8) * 8);
	p->patch = patch;

	if (patch != NULL)
		z80_patch_install(q - offset * 0x2000, patch);

	n = (offset + size) / 2;
	if (n > 4)
		n = 4;

	for (i = offset / 2; i < n; ++i)
	{
		msx_slot(psl, ssl, i) = p;
		((Uint8 *)p->ptr)[i * 2]     = i * 2 - offset;
		((Uint8 *)p->ptr)[i * 2 + 1] = i * 2 - offset + 1;
	}
	return p;
}



static void msx_out_fc(Uint8 n)
{
	msx_slot_t *p;
	int a;

	msx.mapper[0] = n;
	a = msx.psl & 3;
	p = msx_slot(a, msx.ssl[a] & 3, 0);
	p->change(p, 0);
}

static void msx_out_fd(Uint8 n)
{
	msx_slot_t *p;
	int a;

	msx.mapper[1] = n;
	a = (msx.psl >> 2) & 3;
	p = msx_slot(a, (msx.ssl[a] >> 2) & 3, 1);
	p->change(p, 1);
}

static void msx_out_fe(Uint8 n)
{
	msx_slot_t *p;
	int a;

	msx.mapper[2] = n;
	a = (msx.psl >> 4) & 3;
	p = msx_slot(a, (msx.ssl[a] >> 4) & 3, 2);
	p->change(p, 2);
}

static void msx_out_ff(Uint8 n)
{
	msx_slot_t *p;
	int a;

	msx.mapper[3] = n;
	a = (msx.psl >> 6) & 3;
	p = msx_slot(a, (msx.ssl[a] >> 6) & 3, 3);
	p->change(p, 3);
}

void msx_memmap_init(void)
{
	z80_out_install(0xfc, msx_out_fc);
	z80_out_install(0xfd, msx_out_fd);
	z80_out_install(0xfe, msx_out_fe);
	z80_out_install(0xff, msx_out_ff);
	msx.mapper[0] = 3;
	msx.mapper[1] = 2;
	msx.mapper[2] = 1;
//	msx.mapper[3] = 0;
}

static void memmap_slot(msx_slot_t *p, Uint8 n)
{
	Uint8 *q;
	int m;

	m = msx.mapper[n];
	if (m >= p->size)
	{
		msx_empty_slot(p, n);
		return;
	}

	n <<= 1;

	q = p->mem + m * 0x4000;
	z80_page[n].patch = p->patch;
	z80_page[n].mem   = q;
	z80_page[n].read  = NULL;
	z80_page[n].write = NULL;

	++n;
	z80_page[n].patch = p->patch;
	z80_page[n].mem   = q + 0x2000;
	if (n != 7 || !msx.f_extslot[msx.psl >> 6])
	{
		z80_page[n].read  = NULL;
		z80_page[n].write = NULL;
	} else
	{
		z80_page[n].read  = mem_read_page7;
		z80_page[n].write = mem_write_page7;
	}
/* printf("mem (%d/%d) %p\n", m, p->size, p->mem); */
}

/* ○ ROM を読み込んだりメモリを確保し、slot に登録する */

void msx_memmap_load(Uint8 psl, Uint8 ssl, Uint8 size)
{
	msx_slot_t *p;
	int i;

	p = (msx_slot_t *)mem_alloc(sizeof(msx_slot_t));

	p->change = memmap_slot;
	p->mem = (Uint8 *)mem_alloc(size * 0x4000);
	p->offset = 0;
	p->size = size;
	p->ptr = NULL;
	p->patch = NULL;

	for (i = 0; i < 4; ++i)
		msx_slot(psl, ssl, i) = p;
}



static void dos2_write(struct page *p, int page, Uint16 addr, Uint8 n)
{
if (n >= 4)
	debug_printf(("dos2_write: %d\n", n));

//printf("dos2 set %d:%4x %d\n", page, addr, n);
	n &= 3;
	*(Uint8 *)msx.slot_current[1]->ptr = n;
	msx_slot_update(0x0c);
}

static void dos2_slot(msx_slot_t *p, Uint8 n)
{
	Uint8 *q;
	Uint8 m;

//printf("dos2 %d:\n", n);
	if (n == 2)
	{
		z80_page[4].patch = p->patch;
		z80_page[4].mem   = p->mem + 0x4000;
		z80_page[4].read  = NULL;
		z80_page[4].write = ignore_write;

		z80_page[5].patch = p->patch;
		z80_page[5].mem   = p->mem; //+ 0x6000;
		z80_page[5].read  = NULL;
		z80_page[5].write = ignore_write;

		return ;
	}

	m = *(Uint8 *)p->ptr;
	q = p->mem + m * 0x4000;

	z80_page[2].patch = p->patch;
	z80_page[2].mem   = q;
	z80_page[2].read  = NULL;
	z80_page[2].write = ignore_write;

	z80_page[3].patch = p->patch;
	z80_page[3].mem   = q + 0x2000;
	z80_page[3].read  = NULL;
	z80_page[3].write = dos2_write;
//printf("rom %p\n", p->mem);
}

msx_slot_t *msx_dos2_load(Uint8 psl, Uint8 ssl, const char *s, const z80_patch_t *patch)
{
	msx_slot_t *p;
	Uint8 *q;

	q = (Uint8 *)mem_readfile(s, 0, 0x10000);
	if (q == NULL)
		return NULL;

	p = (msx_slot_t *)mem_alloc(sizeof(msx_slot_t));
//printf("rom %d %d %d: %s %p  (%p)\n", psl, ssl, offset, s, p, q);

	p->mem    = q;
	p->change = dos2_slot;
	p->offset = 1;
	p->size   = 4;
	p->ptr    = mem_alloc(sizeof(Uint8));
	p->patch  = patch;

	if (patch != NULL)
		z80_patch_install(q - 0x4000, patch);

	msx_slot(psl, ssl, 1) = p;
	msx_slot(psl, ssl, 2) = p;
	*(Uint8 *)p->ptr   = 0;

	return p;
}
