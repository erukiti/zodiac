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

#include <assert.h>
#include <stdlib.h>
#include <string.h>


#include "../zodiac.h"
#include "../audio.h"
#include "msx.h"

extern void msx_empty_slot(msx_slot_t *p, Uint8 n);


int msx_slot_nnum(int page)
{
	int n, psl, ssl;

assert(page >= 0 && page < 4);

	n = page << 1;
	psl = (msx.psl >> n) & 3;
	if (!msx.f_extslot[psl])
		ssl = 0;
	else
		ssl = (msx.ssl[psl] >> n) & 3;

	return (psl * 16) + (ssl * 4) + page;
}


void msx_slot_update(int n)
{
	int i, a;

	for (i = 0; i < 4; ++i, n >>= 2)
	{
		if ((n & 3) == 0)
			continue;

		a = msx_slot_nnum(i);
		msx.slot_current[i] = msx.slot[a];

/*printf("slot %d-%d page %d. %2d: ", (msx.psl >> (i << 1)) & 3
 , (msx.ssl[(msx.psl >> (i << 1)) & 3] >> (i << 1)) & 3, i, a);

printf("%p %p\n", &msx.slot[a], msx.slot[a].change);
*/
		msx.slot[a]->change(msx.slot[a], (Uint8)i);
	}
}



static Uint8 msx_in_a8(void)
{
	return msx.psl;
}

static void msx_slot_psl(Uint8 n)
{
	int m;

	m = msx.psl;
	if (n == m)
		return;

	msx.psl = n;
	msx_slot_update(m ^ n);
}

void msx_slot_ssl(int n)
{
	int m;

	m = msx.ssl[msx.psl >> 6];
	if (n == m)
		return;

	msx.ssl[msx.psl >> 6] = (Uint8)n;
	msx_slot_update(m ^ n);
}

void msx_slot_change(int page, int psl, int ssl)
{
	int mask;
	int a;

//printf("%d-%d\n", psl, ssl);

assert(psl >= 0 && psl < 4);
assert(ssl >= 0 && ssl < 4);

	a = page * 2;
	mask = 3 << a;
	msx.psl = (msx.psl & ~mask) | (psl << a);
	msx.ssl[psl] = (msx.ssl[psl] & ~mask) | (ssl << a);

	msx_slot_update(mask);
}

void msx_slot_change2(int page, int slot)
{
/*
	int psl, ssl;

	psl = slot & 0x03;
	if ((slot & 0x80) == 0x80)
		ssl = (slot >> 2) & 0x03;
	else
		ssl = 0;
*/

/*
	f = (slot & 0x80) == 0x80;
	if (f ^ msx_slot_ext[psl])
*/

	msx_slot_change(page, slot & 0x03, (slot >> 2) & 0x03);
}



// XXX 仮実装

/*
   o misc/ut.c に移す？
   o memmap 対応
   o 真面目にエラー処理・分割対策などをする
*/

void *mem_readfile(const char *s, size_t off, size_t size)
{
	FILE *fp;
	void *p;

	if (s == NULL)
		return NULL;

	fp = zodiac_path(s); /* fopen(s, "rb"); */

	if (fp == NULL)
		return NULL;

	if (size == 0)
	{
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		rewind(fp);
		if (off > size)
		{
			fclose(fp);
			return NULL;
		}
	}

	p = mem_alloc(size);
	fseek(fp, off, SEEK_SET);
	fread(p, 1, size, fp);	// XXX
	fclose(fp);

	return p;
}

bool mem_writefile(const char *s, void *p, size_t size)
{
	FILE *fp;

	if (s == NULL)
		return FALSE;

	fp = fopen(s, "wb");

	fwrite(p, 1, size, fp); // XXX

	return TRUE;
}



msx_slot_t msx_slot_default=
{
	msx_empty_slot,
	NULL,
	0, 0,
	NULL,
	NULL
};


static void msx_slot_init(void)
{
	int i;

	for (i = 0; i < 4 * 4 * 4; ++i)
		msx.slot[i] = &msx_slot_default;

	msx.f_extslot[0] = TRUE;
	msx.f_extslot[1] = TRUE;
	msx.f_extslot[2] = TRUE;
	msx.f_extslot[3] = TRUE;

	z80_in_install(0xa8, msx_in_a8);
	z80_out_install(0xa8, msx_slot_psl);
}

void msx_init(void)
{

	md_refresh_init(60 * msx_conf.syncfreq / 100, msx_conf.uperiod);

	memset(&msx, 0, sizeof(msx_t));

//	z80_init(3573680);
	z80_init(228 * 60 * 262);

	msx_slot_init();

	msx_memmap_init();
	msx_memmap_load(3, 0, 16);

	msx_rtc_init();
	msx_ppi_init();
	msx_video_init(0x98);
	msx_psg_init();
	msx_tape_init();


	for(;;)
	{
		if (    msx_rom_load(0, 0, 0, 4, "MSX2.ROM",     NULL) != NULL 
		     && msx_rom_load(3, 1, 0, 2, "MSX2EXT.ROM",  NULL) != NULL)
			break;

		if (    msx_rom_load(0, 0, 0, 4, "MSX2P.ROM",    NULL) != NULL
		     && msx_rom_load(3, 1, 0, 2, "MSX2PEXT.ROM", NULL) != NULL)
			break;

		if (    msx_rom_load(0, 0, 0, 4, "MSX.ROM",      NULL) != NULL)
			break;
		fprintf(stderr, "MSX: BIOS image not found.\n");
		exit(EXIT_FAILURE);
	}

/*	if (msx_conf.cart1 != NULL && *msx_conf.cart1 != '\0') */

	msx_music_init(0, 2);
	msx_kanji_init();

	if (msx_disk_init(3, 2))
	{
		msx_dos2_load(2, 0, "MSXDOS2.ROM", NULL);

		msx_disk_change(0, msx_conf.diska);
		msx_disk_change(1, msx_conf.diskb);
	}

	if (msx_conf.cart1!=NULL && *msx_conf.cart1!='\0')
		msx_rom_load(1, 0, 2, 4, msx_conf.cart1, NULL) ;

	msx_slot_update(0xff);
}

void msx_fin(void)
{
	msx_rtc_fin();
	msx_disk_fin();
//	msx_music_fin();
	msx_tape_fin();
}

void msx_pause(bool f) /* start, stop */
{
	if (f)
	{
		audio_start();
		md_refresh_pause(1);
	} else
	{
		md_refresh_pause(0);
		audio_stop();
	}
}

static __inline__ void msx_hsync(void)
{
	int scanline;

	scanline = v99x8_hsync();
	if (scanline == 234)
		msx_event_keyscan();

	if (scanline == 233)
	{
		md_audio_lock();
		zodiac_audio_update(-1);
		md_audio_unlock();
	}
}


void msx_main(void)
{
	msx.f_mainloop = TRUE;

	msx_pause(0);

	for (;;)
	{
		z80_cycle(context.hz / 60 / 262);

		msx_hsync();
		if (!msx.f_mainloop)
			break;

		z80_interrupt();
	}

	msx_pause(1);

	printf("zodiac: exited at %04Xh.\n", Z80_PC);
}


#if 0

void context_main(void)
{
	
}


#endif

