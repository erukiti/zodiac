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

#ifdef HAVE_MMAP
#	include <sys/types.h>
#	include <sys/mman.h>
#endif

#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#	include <unistd.h>
#endif



#define Z80_FASTMEM_USE

#include "../zodiac.h"
#include "../z80/z80.h"
#include "msx.h"

#define MAX_DRIVES 2
#define NUM_SECTORS 512

/* XXX 凄く適当 */
#define CYCLE_DSKIO 10
#define CYCLE_GETBPB 10
#define CYCLE_DSKCHG 10


	/* ★ */

typedef struct
{
	int size;

	int fd;
#ifdef HAVE_MMAP
	Uint8 *addr;
#endif
} diskimage_t;

static diskimage_t drive[MAX_DRIVES];

#ifndef HAVE_MMAP
	static Uint8 secbuf[NUM_SECTORS];
#endif

#ifndef O_BINARY
#	define O_BINARY 0
#endif

static bool disk_open(int n, const char *s)
{
	struct stat sb;

	if (n >= MAX_DRIVES || n < 0)
		return FALSE;

	drive[n].fd = open(s, O_RDWR | O_BINARY);
	if (drive[n].fd < 0)
		return FALSE;

	fstat(drive[n].fd, &sb);
	drive[n].size = sb.st_size;

#ifdef HAVE_MMAP
	drive[n].addr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, drive[n].fd, 0);
	if (drive[n].addr == MAP_FAILED)
	{
		perror("mmap");
		close(drive[n].fd);
		drive[n].fd = -1;
		return FALSE;
	}
#endif
	return TRUE;
}

static bool disk_check(int n)
{
	return n < MAX_DRIVES && n >= 0 && drive[n].fd >= 0;
}

static void disk_close(int n)
{
	if (!disk_check(n))
		return;

#ifdef	HAVE_MMAP
	munmap(drive[n].addr, drive[n].size);
#endif
	close(drive[n].fd);
	drive[n].fd = -1;
}

static Uint8 *disk_seek(int n, int a)
{
	off_t off;

	off = a * NUM_SECTORS;

	if (!disk_check(n) || off + NUM_SECTORS > drive[n].size)
		return NULL;

#ifdef HAVE_MMAP
	return drive[n].addr + off;
#else
	if (lseek(drive[n].fd, off, SEEK_SET) != off)
		return NULL;
	return secbuf;
#endif
}

static bool disk_rprotect(Uint8 *p)  /* XXX */
{
	return memcmp("_ErrSec_", p, 8) == 0;   /* fMSX98 方式 */

/* XXX 本当はセクタの中の残り 512-8 も memcmp する必要がある? */
}

static bool disk_read(int n, Uint8 *p)
{
/*
	singlepcm_play(data, size);
*/


#ifndef HAVE_MMAP
	if (read(drive[n].fd, secbuf, NUM_SECTORS) != NUM_SECTORS)
		return FALSE;
#endif
	return !disk_rprotect(p);
}

static bool disk_write(int n)
{
#ifdef HAVE_MMAP
	return TRUE;
#else
	return write(drive[n].fd, secbuf, NUM_SECTORS) == NUM_SECTORS;
#endif
}




	/* ★ */

bool msx_disk_change(int n, const char *s)
{
	if (disk_check(n))
		disk_close(n);

	if (s == NULL)
		return FALSE;

	return disk_open(n, s);
}

	/* ★ */

static int cycle_dskio(int n)
{
	if (n > 2)
		n = 2;

	return n * 227 * 2; /* XXX context を使ってちゃんとした速度を得ること */
}


static int disk_dskio(void)
{
	Uint8 *p;
	int i;
	int n_done;

	int psl, ssl;


/*
printf("disk_dskio (%c) %04x - %04x <- %x(%d)\n", 
 (Z80_F & Z80_CF) ? 'W' : 'R', Z80_HL, Z80_HL + Z80_B * 512, Z80_DE, Z80_B);
*/

	z80_di();

	if (!disk_check(Z80_A))
	{
		Z80_AF = 0x0201;
		return CYCLE_DSKIO;
	}

	psl = (msx.psl >> 2) & 0x03;
	ssl = (msx.ssl[psl] >> 2) & 0x03;
	msx_slot_change2(1, z80_rdmem(0xf342));

	n_done = 0;

	while (Z80_B-- > 0)
	{
		p = disk_seek(Z80_A, Z80_DE++);
		if (p == NULL)
		{
			Z80_B++;
			Z80_AF = 0x0801;
			break;
		}

		if (Z80_F & Z80_CF)     /* write */
		{
			for (i = 0; i < NUM_SECTORS; ++i)
				p[i] = z80_rdmem(Z80_HL++);

 			if (!disk_write(Z80_A))
			{
				Z80_B++;
				Z80_AF = 0x0a01;
				break;
			}
		} else
		{
			if (!disk_read(Z80_A, p))
			{
				Z80_B++;
				Z80_AF = 0x0401;
				break;
			}

			for (i = 0; i < NUM_SECTORS; ++i)
				z80_wrmem(Z80_HL++, p[i]);
		}

		++n_done;
	}

	msx_slot_change(1, psl, ssl);

	if (Z80_B == 0)
		Z80_F &= ~Z80_CF;
	return cycle_dskio(n_done);
}



static int getshift(int n)
{
	int i;

	for (i = 0; (n & 1) == 1 && i < 8; i++, n >>= 1)
		;
	return i;
}


typedef struct
{
	Uint8 media_id;
	Uint16 sector_size;
	Uint8 dir_mask;
	Uint8 dir_shift;
	Uint8 cluster_mask;
	Uint8 cluster_shift;
	Uint16 fat_first;
	Uint8 fat_num;
	Uint8 dir_num;
	Uint16 data_first;
	Uint16 cluster_num;
	Uint8 fat_sec;
	Uint16 dir_first;
} msx_dpb_t;

#define XXX_SECTOR_SIZE 512

#define DIRENT_SIZE(n) ((n) >> 5)


#define BS_DUMMY           ((bs) + 0x00)

#define BS_MAKER_ID(bs)    ((bs) + 0x03)
#define BS_SECTOR_SIZE(bs) ((bs)[0x0b] + (bs)[0x0c] * 256)
#define BS_CLUSTER_SEC(bs) ((bs)[0x0d])
#define BS_UNUSED_SEC(bs)  ((bs)[0x0e] + (bs)[0x0f] * 256)
#define BS_FAT_NUM(bs)     ((bs)[0x10])
#define BS_DIR_NUM(bs)     ((bs)[0x11] + (bs)[0x12] * 256)
#define BS_SECTOR_NUM(bs)  ((bs)[0x13] + (bs)[0x14] * 256)
#define BS_MEDIA_ID(bs)    ((bs)[0x15])
#define BS_FAT_SEC(bs)     ((bs)[0x16] + (bs)[0x17] * 256)

#define BS_TRACK_SEC(bs)   ((bs)[0x18] + (bs)[0x19] * 256)
#define BS_HEAD_NUM(bs)    ((bs)[0x1a] + (bs)[0x1b] * 256)
#define BS_HIDDEN_SEC(bs)  ((bs)[0x1c] + (bs)[0x1d] * 256)


static bool dpb_conv(msx_dpb_t *dpb, Uint8 *bs, int size)
{
	dpb->media_id = BS_MEDIA_ID(bs);

	if (dpb->media_id < 0xf8)
		printf("disk: unknown media ID %02x\n", dpb->media_id);

	if (BS_SECTOR_SIZE(bs) != XXX_SECTOR_SIZE) /* XXX ? */
	{
		printf("disk: boot sector size != %d\n", XXX_SECTOR_SIZE);
		return FALSE;
	}

	if (BS_SECTOR_SIZE(bs) * BS_SECTOR_NUM(bs) > size)
	{
		printf("disk: image file is too small(%d)\n",
		       BS_SECTOR_SIZE(bs) * BS_SECTOR_NUM(bs));
	}

	if (BS_DIR_NUM(bs) >= 256)
	{
		printf("disk: directory entry >= 256\n");
		return FALSE;
	}


	dpb->sector_size = BS_SECTOR_SIZE(bs);

	dpb->dir_mask = DIRENT_SIZE(dpb->sector_size) - 1;
	dpb->dir_shift = getshift(dpb->dir_mask);

	dpb->cluster_mask = BS_CLUSTER_SEC(bs) - 1;
	dpb->cluster_shift = getshift(dpb->cluster_mask) + 1;

	dpb->fat_num = BS_FAT_NUM(bs);

	dpb->dir_num = BS_DIR_NUM(bs) & 0xff;
	dpb->fat_sec = BS_FAT_SEC(bs);

	dpb->fat_first = BS_UNUSED_SEC(bs);
	dpb->dir_first = dpb->fat_first + dpb->fat_sec * dpb->fat_num;
	dpb->data_first = dpb->dir_first + dpb->dir_num /
	                  DIRENT_SIZE(dpb->sector_size);

	dpb->cluster_num = (BS_SECTOR_NUM(bs) - dpb->data_first) /
	                   BS_CLUSTER_SEC(bs);

#if 0
printf("media_id: %02x\n", dpb->media_id);
printf("sector_size: %d\n", dpb->sector_size);
printf("dir_mask: %d\n", dpb->dir_mask);
printf("dir_shift: %d\n", dpb->dir_shift);
printf("cluster_mask: %d\n", dpb->cluster_mask);
printf("cluster_shift: %d\n", dpb->cluster_shift);
printf("fat_first: %d\n", dpb->fat_first);
printf("fat_num: %d\n", dpb->fat_num);
printf("dir_num: %d\n", dpb->dir_num);
printf("data_first: %d\n", dpb->data_first);
printf("cluster_num: %d\n", dpb->cluster_num);
printf("fat_sec: %d\n", dpb->fat_sec);
printf("dir_first: %d\n", dpb->dir_first);
#endif

	return TRUE;
}

static void dpb_make(msx_dpb_t *dpb, int size)
{
	msx_dpb_t dpb_default[4] = 
	{
		{0xf8, 512, 15, 4, 1, 2, 1, 2, 112, 12, 354, 2, 5}, 
		{0xf9, 512, 15, 4, 1, 2, 1, 2, 112, 14, 713, 3, 7}, 
		{0xfa, 512, 15, 4, 1, 2, 1, 2, 112, 10, 315, 1, 3}, 
		{0xfb, 512, 15, 4, 1, 2, 1, 2, 112, 12, 634, 2, 5}
	};


	if (dpb->media_id < 0xf8)
	{
		switch(size)
		{
		case 368640:
			dpb->media_id = 0xf8;
			break;

		case 737280:
			dpb->media_id = 0xf9;
			break;

		case 327680:
			dpb->media_id = 0xfa;
			break;

		case 655360:
			dpb->media_id = 0xfb;
			break;

		default:
			dpb->media_id = 0xff;
		}
	}

	if (dpb->media_id <0xf8 || dpb->media_id > 0xfb)
	{
		dpb->media_id = 0xff;
		dpb->sector_size = 512;
		printf("disk: illegal media\n");
	}
	else
	{
		memcpy(dpb, &dpb_default[dpb->media_id - 0xf8], sizeof(msx_dpb_t));
	}
}

static int disk_getdpb(void)
{
	msx_dpb_t dpb;
	Uint8 *p;

//printf("disk_getdpb\n");

	if (!disk_check(Z80_A))
	{
		Z80_AF = 0x0201;
		return CYCLE_GETBPB;
	}

	p = disk_seek(Z80_A, 0);
	if (!disk_read(Z80_A, p))
	{
		Z80_AF = 0x0c01;
		return CYCLE_GETBPB;
	}

	if (!dpb_conv(&dpb, p, drive[Z80_A].size))
		dpb_make(&dpb, drive[Z80_A].size);

	z80_wrmem(Z80_HL + 1, dpb.media_id);
	z80_wrmem2(Z80_HL + 2, dpb.sector_size);
	z80_wrmem(Z80_HL + 4, dpb.dir_mask);
	z80_wrmem(Z80_HL + 5, dpb.dir_shift);
	z80_wrmem(Z80_HL + 6, dpb.cluster_mask);
	z80_wrmem(Z80_HL + 7, dpb.cluster_shift);
	z80_wrmem2(Z80_HL + 8, dpb.fat_first);
	z80_wrmem(Z80_HL + 10, dpb.fat_num);
	z80_wrmem(Z80_HL + 11, dpb.dir_num);
	z80_wrmem2(Z80_HL + 12, dpb.data_first);
	z80_wrmem2(Z80_HL + 14, dpb.cluster_num);
	z80_wrmem(Z80_HL + 16, dpb.fat_sec);
	z80_wrmem2(Z80_HL + 17, dpb.dir_first);

	Z80_F &= ~Z80_CF;

	return CYCLE_GETBPB;
}


static int disk_dskchg(void)
{
//printf("disk_dskchg\n");

	z80_di();

	if (!disk_check(Z80_A))
	{
		Z80_AF = 0x0201;
		return CYCLE_DSKCHG;
	}

	Z80_B = 0;
	disk_getdpb();

	return CYCLE_DSKCHG;
}

bool msx_disk_init(int psl, int ssl)
{
	int i;

	static const z80_patch_t disk_patch[] =
	{
		{0x4010, disk_dskio   },
		{0x4013, disk_dskchg  },
		{0x4016, disk_getdpb  },
		{0x401c, z80_patch_nop},
		{0x401f, z80_patch_nop},
		{0     , NULL}
	};

	for (i = 0; i < MAX_DRIVES; ++i)
		drive[i].fd = -1;

	return msx_rom_load(psl, ssl, 2, 2, "DISK.ROM", disk_patch) != NULL;
}

void msx_disk_fin(void)
{
	int i;

	for (i = 0; i < MAX_DRIVES; ++i)
		disk_close(i);
}

