/*
 * Copyright (c) 2000-2002 SASAKI Shunsuke (eruchan@users.sourceforge.net).
 * Copyright (c) 2001-2002 The Zodiac project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "md.h"

#include <assert.h>

static int fps_ticks;

md_fps_t md_fps;


typedef	struct
{
	int sync, frameskip;
	int base, next, skip, times;
	int adjust, adjust0;
} md_refresh_t;

static md_refresh_t md_refresh;


void md_refresh_init(int sync, int frameskip)
{
	md_refresh.sync = sync;
	md_refresh.frameskip = frameskip;
	md_refresh.times = 0;
	md_refresh.skip = 0;

	md_refresh.adjust = 0;
	md_refresh.adjust0 = 0;

	md_fps.time = 0;
	md_fps.skip = 0;
	md_fps.frame = 0;

assert(sync >= 0);
}

void md_refresh_pause(bool f)
{
	if (f)
	{
		md_fps.time += md_timer_getticks() - fps_ticks - md_refresh.adjust0;
	}
	else
	{
		fps_ticks=md_timer_getticks();

		if (md_refresh.sync>0)
		{
/*			md_refresh.next = md_refresh.base = fps_ticks
			                   + 1000 / md_refresh.sync;
*/
			md_refresh.base = fps_ticks;
			md_refresh.next = fps_ticks + 1000 / md_refresh.sync;
		}
	}
}

void md_refresh_adjust(int adjust)
{
	md_refresh.adjust = adjust;
}

bool md_refresh_sync(void)	// 戻り値が真なら表示。偽ならスキップ。
{
	int t, n;

	if (md_refresh.sync == 0)
	{
		if (md_refresh.skip > 0)
		{
			++md_fps.skip;
			--md_refresh.skip;
			return FALSE;
		}

		++md_fps.frame;
		md_refresh.skip = md_refresh.frameskip;
		return TRUE;
	}

	++md_refresh.times;

	if (md_refresh.skip > 0)
	{
		++md_fps.skip;
		--md_refresh.skip;
		return FALSE;
	}

	++md_fps.frame;

//printf("%d (%d)\n", md_refresh.next - md_refresh.base, md_refresh.adjust0);

	while((t = md_timer_getticks()) < md_refresh.next)
		;

	n = 0;
	for (;;)
	{
		if (md_refresh.adjust < md_refresh.adjust0)
			md_refresh.adjust0--;
		else if (md_refresh.adjust > md_refresh.adjust0)
			md_refresh.adjust0++;

		md_refresh.next = md_refresh.base + md_refresh.adjust0 +
		                  (md_refresh.times + n) * 1000 / md_refresh.sync;
		if (t < md_refresh.next || n >= md_refresh.frameskip)
			break;
		++n;
		++md_refresh.skip;
	}

	return TRUE;
}

