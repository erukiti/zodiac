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

#if 0


#ifndef HAVE_GETTIMEOFDAY

void md_profile_clear(void) {;}
void md_profile_start(int n) {;}
void md_profile_stop(int n) {;}
int	md_profile_result(int n) {return -1;}
void md_profile_print(void) {;}

#else

#	ifdef	HAVE_UNISTD_H
#		include	<unistd.h>
#	endif
#	ifdef	HAVE_SYS_TIME_H
#		include	<sys/time.h>
#	endif

#define MAX_PROFILE	4

typedef struct
{
	int count;
	int time;

	struct timeval ltv;
} md_profile_t;

static struct timezone tzp;

md_profile_t md_profile[MAX_PROFILE];

void md_profile_clear(void)
{
	int i;

	for (i = 0; i < MAX_PROFILE; ++i)
	{
		md_profile[i].count = 0;
		md_profile[i].time = 0;
	}
}

void md_profile_start(int n)
{
//	if (n > MAX_PROFILE || n < 0)	// XXX
//		

	gettimeofday(&md_profile[n].ltv, &tzp);
}

void md_profile_stop(int n)
{
	struct timeval tv;

	gettimeofday(&tv, &tzp);

	md_profile[n].time += (tv.tv_sec - md_profile[n].ltv.tv_sec)
	                      * 1000 * 1000 + tv.tv_usec
	                      - md_profile[n].ltv.tv_usec;
	++md_profile[n].count;
}

int	md_profile_result(int n)
{
	if (md_profile[n].count == 0)
		return -1;
	return md_profile[n].time / md_profile[n].count;
}

void md_profile_print(void)
{
	int i;
	int n;

	for (i = 0; i < MAX_PROFILE; ++i)
	{
		n = md_profile_result(i);
		if (n != -1)
			printf("profile %d: %d usec.\n", i, n);
	}
}

#endif /* HAVE_GETTIMEOFDAY */


#endif /* MD_USE_PROFILE */
