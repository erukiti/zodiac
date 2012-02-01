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

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../zodiac.h"
#include "../audio.h"
#include "../msx/msx.h"

#include "../ym2149/ym2149.h"
#include "../ym2413/ym2413.h"



/* ★ RTC */

static Uint8 msx_rtc[4][13];
static int msx_rtc_reg, msx_rtc_mode;
static bool msx_rtc_dirty;

static void msx_rtc_setup(void)
{
	static time_t latest=0;
	time_t t;
	struct tm tm;

	t = time(NULL);
	if (t == latest)
		return;

	tm = *localtime(&t);
	latest = t;

	msx_rtc[0][0]  = tm.tm_sec % 10;
	msx_rtc[0][1]  = tm.tm_sec / 10;
	msx_rtc[0][2]  = tm.tm_min % 10;
	msx_rtc[0][3]  = tm.tm_min / 10;
	msx_rtc[0][4]  = tm.tm_hour % 10;
	msx_rtc[0][5]  = tm.tm_hour / 10;
	msx_rtc[0][6]  = tm.tm_wday;
	msx_rtc[0][7]  = tm.tm_mday % 10;
	msx_rtc[0][8]  = tm.tm_mday / 10;
	msx_rtc[0][9]  = (tm.tm_mon + 1) % 10;
	msx_rtc[0][10] = (tm.tm_mon + 1) / 10;
	msx_rtc[0][11] = (tm.tm_year - 80) % 10;
	msx_rtc[0][12] = ((tm.tm_year - 80) / 10) % 10;
}

static Uint8 msx_in_b5(void)
{
	if (msx_rtc_reg == 13)
		return msx_rtc_mode | 0xf0;

	if (msx_rtc_reg > 13)
		return 0xff;

	if (msx_rtc_mode == 0)
		msx_rtc_setup();
	return msx_rtc[msx_rtc_mode][msx_rtc_reg] | 0xf0;
}

static void msx_out_b4(Uint8 n)
{
	msx_rtc_reg = n & 0x0f;
}

static void msx_out_b5(Uint8 n)
{
	n &= 0x0f;

	if (msx_rtc_reg == 13)
	{
		msx_rtc_mode = n & 3; 
	} else
	{
		if (msx_rtc_reg < 13 && msx_rtc[msx_rtc_mode][msx_rtc_reg] != n)
		{
			msx_rtc[msx_rtc_mode][msx_rtc_reg] = n;
			if (msx_rtc_mode > 1)
				msx_rtc_dirty = TRUE;
		}
	}
}

/* marshaling.... save/load より marshal/unmarshal ?? */

extern void *mem_readfile(const char *s, size_t off, size_t size);	/* XXX */
extern bool mem_writefile(const char *s, void *p, size_t size);	/* XXX */


void msx_rtc_load(void)
{
	Uint8 *p;

	p = (Uint8 *)mem_readfile(".msx.cmos", 0, 13 * 3);
	if (p == NULL)
		return;

	memcpy(msx_rtc[1], p, 13 * 3);
}

void msx_rtc_save(void)
{
	if (!msx_rtc_dirty)
		return;

	mem_writefile(".msx.cmos", msx_rtc[1], 13 * 3);
}


void msx_rtc_init(void)
{
	z80_out_install(0xb4, msx_out_b4);
	z80_out_install(0xb5, msx_out_b5);
	z80_in_install(0xb5, msx_in_b5);

	memset(msx_rtc, 0, sizeof(msx_rtc));
	msx_rtc_reg = 0;
	msx_rtc_mode = 0;
	msx_rtc_dirty = FALSE;

	msx_rtc_load();
}

void msx_rtc_fin(void)
{
	msx_rtc_save();
}


/* XXX */

static	Uint8	defaultkeys[]=
{
/* 0 	xx		xx		xx		xx		xx		xx		xx		xx */
		0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,

/* 8 	BS		TAB		xx		xx		CLEAR	RETURN	xx		xx */
		0x75,	0x73,	0xff,	0xff,	0xff,	0x77,	0xff,	0xff,

/* 16	xx		xx		PAUSE	xx		xx		xx		xx		xx */
		0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,

/* 24	xx		xx		xx		ESCAPE	xx		xx		xx		xx */
		0xff,	0xff,	0xff,	0x72,	0xff,	0xff,	0xff,	0xff,

/* 32	SPACE	!		"		#		$		xx		&		' */
		0x80,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0x20,

/* 40	(		)		*		+		COMMA	MINUS	PERIOD	SLASH */
		0xff,	0xff,	0xff,	0xff,	0x22, 	0x12, 	0x23, 	0x24,

/* 48	0		1		2		3		4		5		6		7 */
		0x00, 	0x01, 	0x02, 	0x03, 	0x04, 	0x05, 	0x06, 	0x07,

/* 56	8		9		COLON	;		<		=		>		? */
		0x10, 	0x11, 	0x20,	0x17, 	0xff,	0x13,	0xff,	0xff,

/* 64	@		--		--		--		--		--		--		-- */
		0x15,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,

/* 72	--		--		--		--		--		--		--		-- */
		0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,

/* 80	--		--		--		--		--		--		--		-- */
		0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,

/* 88	--		--		--		[		BSLASH	]		CARET	_ */
		0xff,	0xff,	0xff,	0x16,	0x14,	0x21,	0x13,	0x25,

/* 96	`		a		b		c		d		e		f		g */
		0x21,	0x26,	0x27,	0x30,	0x31,	0x32,	0x33,	0x34,

/* 104	h		i		j		k		l		m		n		o */
		0x35,	0x36,	0x37,	0x40,	0x41,	0x42,	0x43,	0x44,

/* 112	p		q		r		s		t		u		v		w */
		0x45,	0x46,	0x47,	0x50,	0x51,	0x52,	0x53,	0x54,

/* 120	x		y		z		--		--		--		--		DELETE */
		0x55,	0x56,	0x57,	0xff,	0xff,	0xff,	0xff,	0x83,

/* 128	--		--		--		--		--		--		--		-- */
		0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,

/* 136	--		--		--		--		--		--		--		-- */
		0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,

/* 144	--		--		--		--		--		--		--		-- */
		0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,

/* 152	--		--		--		--		--		--		--		-- */
		0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,

/* 160	--		--		--		--		--		--		--		-- */
		0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,

/* 168	--		--		--		--		--		--		--		-- */
		0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,

/* 176	--		--		--		--		--		--		--		-- */
		0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,

/* 184	--		--		--		--		--		--		--		-- */
		0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,

/* 192	--		--		--		--		--		--		--		-- */
		0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,

/* 200	--		--		--		--		--		--		--		-- */
		0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,

/* 208	--		--		--		--		--		--		--		-- */
		0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,

/* 216	--		--		--		--		--		--		--		-- */
		0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,

/* 224	--		--		--		--		--		--		--		-- */
		0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,

/* 232	--		--		--		--		--		--		--		-- */
		0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,

/* 240	--		--		--		--		--		--		--		-- */
		0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,

/* 248	--		--		--		--		--		--		--		-- */
		0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,

/* 256	KP0 	KP1 	KP2 	KP3 	KP4 	KP5 	KP6 	KP7 */
		0x93,	0x94,	0x95,	0x96,	0x97,	0xa0,	0xa1,	0xa2,

/* 264	KP8 	KP9 	KP. 	KP/ 	KP* 	KP- 	KP+ 	KPe */
		0xa3,	0xa4,	0xa7,	0x92,	0x93,	0xa5,	0x94,	0xff,

/* 272	KP= 	UP		DOWN	RIGHT	LEFT	INSERT	HOME	END */
		0xff,	0x85,	0x86,	0x87,	0x84,	0x82,	0x81,	0x76,

/* 280	PAGEUP	PAGEDN	F1		F2		F3		F4		F5		F6 */
		0x74,	0x64,	0x65,	0x66,	0x67,	0x70,	0x71,	0xff,

/* 288	F7		F8		F9		F10		F11		F12		F13		F14 */
		0xff,	0xff,	0xff,	0xff,	0xb1,	0xb0,	0xff,	0xff,

/* 296	F15		--		--		--		NUM		CAPS	SCR		RSHIFT */
		0xff,	0xff,	0xff,	0xff,	0xff,	0x63,	0xff,	0x60,

/* 304	LSHIFT	RCTRL	LCTRL	RALT	LALT	RMETA	LMETA	LSPUER */
		0x60,	0x61,	0x61,	0x62,	0x62,	0x62,	0x62,	0xff,

/* 312	RSUPER	MODE	xx		HELP	PRINT	SYSREQ	BREAK	MENU */
		0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,

/* 320	POWER	EURO	QUIT */
		0xff,	0xff,	0xb0
};


static void func_fin(void)
{
	msx.f_mainloop = FALSE;
}


#if defined(TDEBUGGER) || defined(EXTDEBUGGER)
static void func_debug_onoff(void)
{
	zdb_onoff(z80.zdb);
}
#endif


static Uint8 keymap[16];

void msx_event_keyscan(void)
{
#if defined(TDEBUGGER) || defined(EXTDEBUGGER)
	void (*func[])() = {func_fin, func_debug_onoff};
#else
	void (*func[])() = {func_fin, func_fin};
#endif
	int evbuf[16];
	int n;

	n = md_event_gets(evbuf, sizeof(evbuf) / sizeof(int));

	memset(keymap, 0xff, sizeof(keymap));

	while (n > 0)
	{
		--n;
		if (evbuf[n] < 0xb0)
			keymap[evbuf[n] >> 4] &= ~(1 << (evbuf[n] & 7));
		else
			func[evbuf[n] - 0xb0]();
	}
}

/* XXX i8255 */

static Uint8 ppi_reg, ppi_value;

static int pwm_ch;       /* 1bit D/A */
static Sint16 pwm_data;


static Uint8 msx_in_a9(void) {return keymap[ppi_value & 0x0f];}
static Uint8 msx_in_aa(void) {return ppi_value;}
static Uint8 msx_in_ab(void) {return ppi_reg;}

void msx_tape_motor(bool f);

static void ppi_out_c(Uint8 n)
{
	int m;

	m = ppi_value ^ n;

	if (m & 0x10)
		msx_tape_motor(!(n & 0x10));

/*
0x20: casette write
0x40: CAPS off/on
*/

	if (m & 0x80)
	{
		md_audio_lock();

		zodiac_audio_update(pwm_ch);
		pwm_data = (n & 0x80) << 2;

		md_audio_unlock();
	}

	ppi_value = n;
}

static void msx_out_aa(Uint8 n) {ppi_out_c(n);}

static void msx_out_ab(Uint8 n)
{
	if (n & 0x80)
	{
		ppi_reg = n ;
	} else
	{
		if (n & 1)
			ppi_out_c(ppi_value |   1 << (7 & (n >> 1)));
		else
			ppi_out_c(ppi_value & ~(1 << (7 & (n >> 1))));
	}
}



void msx_event_init(void)
{
	int i;

	md_event_init();
	for (i = 0; i < sizeof(defaultkeys); ++i)
	{
		 if (defaultkeys[i] != 0xff)
		 	md_event_set(i, defaultkeys[i]);
	}
}

static void pwm_calc(int *p, int a)
{
	while (a-->0)
		*p++ = pwm_data;
}

void msx_ppi_init(void)
{
	msx_event_init();
	memset(keymap, 0xff, sizeof(keymap));

	z80_in_install(0xa9, msx_in_a9);
	z80_in_install(0xaa, msx_in_aa);
	z80_in_install(0xab, msx_in_ab);
	z80_out_install(0xaa, msx_out_aa);
	z80_out_install(0xab, msx_out_ab);

	ppi_reg = 0;
	ppi_value = 0x10;

	if (audio_freq() == 0)
		return;

	md_audio_lock();
	pwm_ch = audio_add(pwm_calc, XXX_VOL_PWM, XXX_COFF_PWM);
	md_audio_unlock();
}




  /* XXX */

extern int md_joystick_getall(int n);

static Uint8 psg_ctrl[16];
static int psg_reg;

static Uint8 msx_in_a2(void)
{
	if (psg_reg == 14)
	{
		 int n;

		 n = (psg_ctrl[15] & 0x40) ? 1 : 0;
		 return ~md_joystick_getall(n) & 0x3f;
	}

	return psg_ctrl[psg_reg];
}

static void msx_out_a0(Uint8 n)
{
	psg_reg = n & 0x0f;
}

static void msx_out_a1(Uint8 n)
{
	psg_ctrl[psg_reg] = n;

	if (audio_freq() != 0 && psg_reg < 14)
		ym2149_out(psg_reg, n);
}

void msx_psg_init(void)
{
	memset(psg_ctrl, 0, sizeof(psg_ctrl));

	if (audio_freq() != 0)
		ym2149_init();

	z80_in_install(0xa2, msx_in_a2);
	z80_out_install(0xa0, msx_out_a0);
	z80_out_install(0xa1, msx_out_a1);
}

void msx_music_init(int psl, int ssl)
{
	if (audio_freq() == 0)
		return;

	if (msx_rom_load(psl, ssl, 2, 2, "FMPAC.ROM", NULL) != NULL)
	{
		ym2413_init();
		z80_out_install(0x7c, ym2413_out_0);
		z80_out_install(0x7d, ym2413_out_1);
	}
}


/*
void	msx_music_fin(void)
{
	ym2413_fin();
}
*/




	/* kanji */

static Uint8 *kanji_font16;
static int kanji_reg = 0;


static Uint8 msx_in_kanji_1sui(void)
{
	return kanji_font16[kanji_reg++];
}

static Uint8 msx_in_kanji_2sui(void)
{
	return kanji_font16[0x20000 + kanji_reg++];
}

static void msx_out_kanji0(Uint8 n)
{
	kanji_reg = (kanji_reg & 0x1f800) | (((int)n & 0x3f) << 5);
}

static void msx_out_kanji1(Uint8 n)
{
	kanji_reg = (kanji_reg & 0x7e0) | (((int)n & 0x3f) << 11);
}


void msx_kanji_init(void)
{
	kanji_font16 = (Uint8*)mem_readfile("KANJI.ROM", 0, 0x40000);
	if (kanji_font16 == NULL)
		return;

	z80_in_install(0xd9, msx_in_kanji_1sui);
	z80_in_install(0xdb, msx_in_kanji_2sui);
	z80_out_install(0xd8, msx_out_kanji0);
	z80_out_install(0xd9, msx_out_kanji1);
	z80_out_install(0xda, msx_out_kanji0);
	z80_out_install(0xdb, msx_out_kanji1);

	msx_rom_load(3, 1, 2, 4, "KNJDRV.ROM", NULL);
}


void msx_video_init(int port)
{
	v99x8_init();

	z80_in_install(port + 0, v99x8_in_0);
	z80_in_install(port + 1, v99x8_in_1);

	z80_out_install(port + 0, v99x8_out_0);
	z80_out_install(port + 1, v99x8_out_1);
	z80_out_install(port + 2, v99x8_out_2);
	z80_out_install(port + 3, v99x8_out_3);
}



/* XXX move to msx_tape.c ?? */
#if 0
static size_t motor_on_size, motor_off_size;
static Sint16 *motor_on_data, *motor_off_data;

Sint16 *wav_load(const char *s, size_t *size)
{
	unsigned char *p;

/* XXX !!! ファイルサイズを get できるようにしないと危険！危険！危険！ */
	p = (unsigned char *)mem_readfile(s, 0, 0);
	if (p == NULL)
		return NULL;

	*size = (*(p + 0x28) + ((size_t)*(p + 0x29) << 8)
	    + ((size_t)*(p + 0x2a) << 16) + ((size_t)*(p + 0x2b) << 24) ) / 2;

//printf ( "size=%d\n", *size );

/*
{
int i;

for (i = 0; i < *size; i+=2)
	printf("%04x\n", ((Sint16 *)(p+0x2c))[i]);
}
*/

	return (Sint16 *)(p + 0x2c);
}
#endif

void msx_tape_init(void)
{
/*
	if (audio_freq() == 0)
		return;

	singlepcm_init();

	motor_on_data = wav_load("motor_on.wav", &motor_on_size);
	motor_off_data = wav_load("motor_off.wav", &motor_off_size);
*/
}

void msx_tape_fin(void)
{
}

void msx_tape_motor(bool f)
{
/*
	if (f)
		singlepcm_play(motor_on_data, motor_on_size);
	else
		singlepcm_play(motor_off_data, motor_off_size);
*/
}

