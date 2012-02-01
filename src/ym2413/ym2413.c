/* emu2413 wrapper */

#include "../../config.h"

#include <stdio.h>
#include <stdlib.h>

#include "../zodiac.h"
#include "../audio.h"

#include "emu2413.h"

static OPLL *opll = NULL;
static int opll_ch = 0;

/* Standard clock = MSX clock */
#define MSX_CLK 3579545

void ym2413_calc(int *p, int a)
{
	while (a-- > 0)
		*p++ = OPLL_calc(opll);
}


void ym2413_init(void)
{
	if (audio_freq() == 0 || opll != NULL)
		return;

	md_audio_lock();

	opll = OPLL_new(MSX_CLK, audio_freq());
	if (opll == NULL)
	{
		fprintf(stderr, "OPLL_new: failed.");
		md_audio_unlock();
		exit(EXIT_FAILURE);
	}

	OPLL_reset(opll);
//	OPLL_set_quality(opll, 1);
	OPLL_reset_patch(opll, 0); /* if use default voice data. */ 

	opll_ch = audio_add(ym2413_calc, XXX_VOL_OPLL, XXX_COFF_OPLL);
	md_audio_unlock();
}

void ym2413_fin(void)
{
/*	if (opll != NULL)
	{
//		audio_remove(opll_ch);

		OPLL_delete(opll);
		opll = NULL;
	}
*/
}

static int ym2413_reg = 0;

void ym2413_out_0(Uint8 n)
{
	ym2413_reg = n & 0x3f;
}

void ym2413_out_1(Uint8 n)
{
	md_audio_lock();

	zodiac_audio_update(opll_ch);
	OPLL_writeReg(opll, ym2413_reg, n);

	md_audio_unlock();
}

