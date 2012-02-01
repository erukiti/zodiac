/*
 * Copyright (c) 2001, 2002
 *    SASAKI Shunsuke (eruchan@users.sourceforge.net)
 * All rights reserved.
 */

/* XXX move to libmd?? */

#include "../config.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "zodiac.h"
#include "audio.h"



/* 
  MSXplug - Copyright (C) 2001 Mitsutaka Okazaki.
  http://www.angel.ne.jp/~okazaki/ym2413/

  This software is provided 'as-is', without any express or implied
  warranty. 

  Permission is granted to anyone to use this software for any purpose, 
  including commercial applications.
  To alter this software and redistribute it freely, if the origin of 
  this software is not misrepresented, and this notice is not removed
  from any source distribution.

	* volume
	* RCF (LPF)
*/

#define volume_apply(data, vol) ((data * vol) >> 12)

static int volume_set(double vol)
{
//	return (1 << 12) * pow(2, vol / 6.0);
	return (1 << 12) * pow(10, vol / 20.0);
}



typedef struct _tagRCF
{
	Uint32 enable;
	double a;
	double out;
} RCF;


static RCF *RCF_new(void)
{
	RCF *rcf = malloc(sizeof(RCF));
	return rcf;
}

static void RCF_reset(RCF *rcf, double rate, double R, double C)
{
	if (R == 0 || C == 0)
		rcf->enable = 0;
	else
	{
		rcf->a = 1.0 / R / C / rate;
		rcf->out = 0;
		rcf->enable = 1;
	}
}

static __inline__ Sint32 RCF_calc(RCF *rcf, Sint32 wav)
{
	if (rcf->enable)
	{
		rcf->out += rcf->a * ((double)wav - rcf->out);
		return (Sint32)rcf->out;
	}
	else
	{
		return wav;
	}
}

/*
static void RCF_delete(RCF *rcf)
{
	free(rcf);
}

static void RCF_disable(RCF *rcf)
{
	rcf->enable = 0;
}
*/



#define MAX_MIXER 8

typedef struct
{
	void (*calc)(int *, int);
	int volume;
	int latest_count, latest_sec;

	int *ringbuf;
	int latest_ring;

} audio_mixer_t;

typedef struct
{
	int freq;
	int buffer_size;

	int tap;

	audio_mixer_t mixer[MAX_MIXER];
	int mixer_num;

	int ring_size;
	int played_ring;

	int played_count, played_count_sec;
	int basetime;

	RCF *rcf;

	int delay;
/*
   発音開始時の余裕の処理。
   本来は開始時をもう少しちゃんと認識してそれに応じたディレイが必要。

   大きすぎると実際の音とゲームの中の時間がズレすぎる。
   小さすぎると buffer underrun が発生する可能性がふえる。
 */


} audio_t;

static audio_t audio;


static int ring_space(int ch)
{
	int a;

	a = audio.played_ring - audio.mixer[ch].latest_ring;
	if (a <= 0)
		a += audio.ring_size;

	return a;
}

static int calc2(int ch, int n)
{
	audio.mixer[ch].calc(audio.mixer[ch].ringbuf +
	                     audio.mixer[ch].latest_ring, n);

	audio.mixer[ch].latest_count += n;
	if (audio.mixer[ch].latest_count > audio.freq)
	{
		audio.mixer[ch].latest_count -= audio.freq;
		audio.mixer[ch].latest_sec += 1;
	}

	audio.mixer[ch].latest_ring += n;
	if (audio.mixer[ch].latest_ring >= audio.ring_size)
		audio.mixer[ch].latest_ring = 0;

	return n;
}

static void calc(int ch, int n)
{
	int a;

	if (n <= 0)
		return;

	a = ring_space(ch);
	if (a < n)
	{
		/* buffer overrun */
//printf("buffer overrun %d\n", n - a);

		audio.mixer[ch].latest_sec += (n - a) / audio.freq;
		audio.mixer[ch].latest_count += (n - a) % audio.freq;
		if (audio.mixer[ch].latest_count > audio.freq)
		{
			audio.mixer[ch].latest_count -= audio.freq;
			audio.mixer[ch].latest_sec += 1;
		}

		n = a;
	}

	if (audio.mixer[ch].latest_ring >= audio.played_ring)
	{
		n -= calc2(ch, min(n, audio.ring_size - audio.mixer[ch].latest_ring));
		if (n == 0)
			return;
	}

	calc2(ch, n);
}

void audio_update(int ch, int sec, int count)
{
	int i, a;

	if (audio.freq == 0)
		return;

	if (ch != -1)
		i = ch;
	else
	{
		i = 0;
		ch = audio.mixer_num - 1;
	}

	while (i <= ch)
	{
		a = (sec - audio.mixer[i].latest_sec) * audio.freq +
		    count - audio.mixer[i].latest_count;

		calc(i, a);

//if (a > bufsize)
//printf("audio :update %d\n", a);

		++i;
	}
}


/* callback */
static void audio_play(void *unused, Uint8 *p, int num)
{
	int i, j;
	int n;

	num >>= 1; /* buffer size -> buffer num */

	if (audio.delay > 0)
	{
		audio.delay--; // XXX

		memset(p, 0, num * 2);
		return;
	}


/*
context_timelock();
n = num - (context.time_sec * audio.freq + context.time_cycle/(context.hz/audio.freq));
context_timeunlock();
*/

	if (audio.basetime == -1)
		audio.basetime = md_timer_getticks();
	else if (audio.played_count + num >= audio.freq &&
	         audio.played_count_sec % 5 == 0)
	{
		n = md_timer_getticks();
		n = ((n - audio.basetime - audio.played_count_sec * 1000) * 
		    audio.freq - audio.played_count * 1000) / audio.freq;

		md_refresh_adjust(n);

#if 0
printf("(adjust %d) ", n);
n = context.time_sec * audio.freq + context.time_cycle/(context.hz/audio.freq);
printf("%d\n", n - (audio.played_count_sec * audio.freq + audio.played_count));
#endif

	}

	audio.played_count += num;
	if (audio.played_count >= audio.freq)
	{
		audio.played_count -= audio.freq;
		audio.played_count_sec++;
	}


	/* buffer underrun */
	audio_update(-1, audio.played_count_sec, audio.played_count);

/*	for (i = 0; i < audio.mixer_num; i++)
		calc(i, num - (audio.ring_size - ring_space(i)));
*/


	for (i = 0; i < num; i++)
	{
		int a;

		a = 0;
		for (j = 0; j < audio.mixer_num; j++)
			a += volume_apply(audio.mixer[j].ringbuf[audio.played_ring],
			                  audio.mixer[j].volume);

		a = RCF_calc(audio.rcf, a);

		if (a > 32767)
			a = 32767;
		if (a < -32768)
			a = -32768;

		*(Sint16 *)p = a;
		p += 2;

		if (++audio.played_ring >= audio.ring_size)
			audio.played_ring = 0;
	}
}





// XXX move to libmd
static int round2pow(int n)
{
	int i;

	i = 512;
	while ((i << 1) < n && i <= 8192)
		i <<= 1;

	return i;
}


bool audio_init(int freq, int size, int r, int c)
{
	audio.freq = freq;
	if (freq == 0)
		return FALSE;

	if (freq < 8000)
		audio.freq = freq = 8000;
	if (freq > 48000)
		audio.freq = freq = 48000;

	audio.buffer_size = round2pow((freq * size) / 1000);
	if (!md_audio_init(audio_play, audio.freq, audio.buffer_size))
	{
		audio.freq = 0;
		return FALSE;
	}

	audio.ring_size = audio.buffer_size * 8;

	audio.mixer_num = 0;
	audio.played_ring = 0;

	audio.played_count = 0;
	audio.played_count_sec = 0;
	audio.basetime = -1;

	audio.delay = XXX_AUDIO_DELAY;

	audio.rcf = RCF_new();
	RCF_reset(audio.rcf, audio.freq, r, c * 0.001e-6);

	return TRUE;
}

int audio_add(void (*calc)(int *, int), int volume, int cuttoff)
{
	if (audio.freq == 0)
		return -1;

	if (audio.mixer_num >= MAX_MIXER)
	{
		/* XXX */

		return -1;
	}

	audio.mixer[audio.mixer_num].latest_count = 0;
	audio.mixer[audio.mixer_num].latest_sec = 0;

	audio.mixer[audio.mixer_num].latest_ring = 0;

	audio.mixer[audio.mixer_num].calc = calc;
	audio.mixer[audio.mixer_num].volume = volume_set(volume);

	audio.mixer[audio.mixer_num].ringbuf = (int *)mem_alloc(audio.ring_size *
	                                                    sizeof(int));

//printf("audio: add %d\n", audio.mixer[audio.mixer_num].volume);

	return audio.mixer_num++;
}

void audio_fin(void)
{
}

void audio_start(void)
{
	md_audio_pause(1);

}

void audio_stop(void)
{
	md_audio_pause(0);
}

int audio_freq(void)
{
	return audio.freq;
}

#if 0

static int singlepcm_ch;
static Sint16 *singlepcm_data;
static int singlepcm_size, singlepcm_latest;

static void singlepcm_calc(int *p, int a)
{
	int n;

	n = min(singlepcm_size - singlepcm_latest, a);
	a -= n;

	while (n-- > 0)
		*p++ = singlepcm_data[singlepcm_latest++];

	if (a > 0)
		memset(p, 0, a * sizeof(int));
}

void singlepcm_init(void)
{
	md_audio_lock();
	singlepcm_size = 0;
	singlepcm_latest = 0;
	singlepcm_ch = audio_add(singlepcm_calc, XXX_VOL_SINGLEPCM, XXX_COFF_SINGLEPCM);
	md_audio_unlock();
}

void singlepcm_play(Sint16 *p, int n)
{
/*	if (p == NULL || n == 0)
		return;

	md_audio_lock();

	zodiac_audio_update(singlepcm_ch);

	singlepcm_data = p;
	singlepcm_size = n;
	singlepcm_latest = 0;

	md_audio_unlock();
*/
}

#endif
