/* emu2149 wrapper */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "../zodiac.h"
#include "../audio.h"

#include "emu2149.h"

static PSG *psg = NULL;
static int psg_ch = 0;

/* Standard clock = MSX clock */
#define MSX_CLK 3579545

/* YM2149 internal clock */
#define YM2149_CLK MSX_CLK


void ym2149_calc(int *p, int a)
{
// printf("ym %d\n", a);

	while (a-- > 0)
		*p++ = PSG_calc(psg);
}


void ym2149_init(void)
{
	if (audio_freq() == 0 || psg != NULL)
		return;

	md_audio_lock();

	psg = PSG_new(YM2149_CLK, audio_freq());
	if (psg == NULL)
	{
		fprintf(stderr, "PSG_new: failed.");
		exit(EXIT_FAILURE);
	}
//	PSG_setVolumeMode(psg, );

	PSG_reset(psg);
//	PSG_set_quality(psg, 1);

	psg_ch = audio_add(ym2149_calc, XXX_VOL_PSG, XXX_COFF_PSG);
	md_audio_unlock();
}

void ym2149_fin(void)
{
/*	if (psg != NULL)
	{
//		audio_remove(psg_ch);

		PSG_delete(psg);
		psg = NULL;
	}
*/
}

void ym2149_out(int port, Uint8 n)
{
	if (psg == NULL)
		return;

	md_audio_lock();

	zodiac_audio_update(psg_ch);
	PSG_writeReg(psg, port, n);

	md_audio_unlock();
}

