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

#define	VAR

#include "../config.h"

#if defined(HAVE_SYS_UTSNAME_H)
#	include <sys/utsname.h>
#endif

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#	include <unistd.h>  /* getopt */
#endif

#ifdef USE_CW_PROFILER
#	include <Profiler.h>
#endif /* USE_CW_PROFILER */

#include "zodiac.h"
#include "audio.h"
#include "msx/msx.h"
#include "version.h"

#include "v99x8/v99x8.h" /* XXX */

const char *title = "Zodiac(MSX)";

int soundfreq = XXX_AUDIO_FREQ;
int soundbuf = XXX_AUDIO_BUFSIZE;
int rcf_r = XXX_AUDIO_RCF_R;
int rcf_c = XXX_AUDIO_RCF_C;


/* XXX move! MSX configuration */

msx_conf_t msx_conf =
{
	NULL,
	NULL,
	NULL,
	100,
	1
};


static var_t var_list[] =
{
	{"diska", VAR_TYPE_STR, &msx_conf.diska},
	{"diskb", VAR_TYPE_STR, &msx_conf.diskb},
	{"carta", VAR_TYPE_STR, &msx_conf.cart1},
	{"sync", VAR_TYPE_INT, &msx_conf.syncfreq},
	{"uperiod", VAR_TYPE_INT, &msx_conf.uperiod},

	{"soundbuf", VAR_TYPE_INT, &soundbuf},
	{"sound", VAR_TYPE_INT, &soundfreq},

	{"zoom", VAR_TYPE_INT, &v99x8.f_zoom},

	{NULL, VAR_NULL, NULL}
};









void zodiac_init(void)
{
	EMPTYIMAGE = (Uint8 *)mem_alloc(0x4000);
	memset(EMPTYIMAGE, 0xff, 0x4000);
}

static void version_info(void)
{
	struct utsname u;

	uname(&u);
	printf("%s version %s (%s)\n", PACKAGE, VERSION, MD_ARCH);
	printf("%s %s(%s)\n\n", u.sysname, u.release, u.machine);
	puts(COPYRIGHT);
}

static void usage(const char* progname)
{
	version_info();
	printf("\nusage: %s [OPTION]... [-c CONFIGURATION FILE]\n", progname);
	puts("Disk image:");

	puts("  -a DRIVEA              use DRIVEA file as drive `A:'.");
	puts("  -b DRIVEB              use DRIVEB file as drive `B:'.");
	puts("\n" "Sound:");
	puts("  -B MSEC                sound buffer size set to MSEC [milisec]. default is 50.");
	puts("  -f FREQ                sound frequency set to FREQ [Hz]. default is 22050.");
	puts("\n" "Synchronization:");
	puts("  -s RATE                synchronizeation rate set to RATE [%]. default is 100.");
	puts("  -u PERIOD              max frame skip count set to PERIOD. default is 2.");
	puts("\n" "Displaying:");
	puts("  -z                     enable zoom mode. default is disabled.");
	puts("             --help      display this message.");
	puts("             --version   display version info.");
	puts("");
}

void zodiac_arg(int argc,char *argv[])
{
	int c;
	bool f_conf = FALSE;
	bool miss_option = FALSE;

	v99x8.f_zoom = FALSE;

	/* usage */
	for (c = 0; c < argc; c++)
	{
		if (strcmp(argv[c], "--help") == 0)
		{
			usage(argv[0]);
			exit(EXIT_SUCCESS);
		}
	}

	/* version info */
	for (c = 0; c < argc; c++)
	{
		if (strcmp(argv[c], "--version") == 0)
		{
			version_info();
			exit(EXIT_SUCCESS);
		}
	}

	for (;;)
	{
		c = getopt(argc, argv, "a:b:s:u:f:c:B:R:C:z");
		if (c == EOF)
			break;

/* printf("%d %02x %02x %s\n", optind, c, optopt, optarg); */

		switch(c)
		{
		case '?':
		case ':':
			miss_option = TRUE;
			break;
		case 'c':
			var_configfile(optarg, var_list);
			f_conf = TRUE;
			break;

		case 's':
			msx_conf.syncfreq = atoi(optarg);
			break;
		case 'u':
			msx_conf.uperiod = atoi(optarg) - 1;
			break;

		case 'f':
			soundfreq = atoi(optarg);
			break;
		case 'B':
			soundbuf = atoi(optarg);
			break;
		case 'R':
			rcf_r = atoi(optarg);
			break;
		case 'C':
			rcf_c = atoi(optarg);
			break;

		case 'a':
			var_set(var_list, "diska", optarg);
			break;
		case 'b':
			var_set(var_list, "diskb", optarg);
			break;

		case 'z':
			v99x8.f_zoom = TRUE;
			break;
		}
	}
	if (miss_option)
	{
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	argc -= optind;
	argv += optind;

	if (argc > 0)
		var_set(var_list, "carta", *argv);

	if (msx_conf.syncfreq < 0)
		msx_conf.syncfreq = 0;

	if (msx_conf.uperiod < 0 || msx_conf.uperiod > 9)
		msx_conf.uperiod = 0;

	if (!f_conf)
		var_configfile("config", var_list);

#if 0
	if (diska != NULL)
		printf("disk A:%s\n", diska);
	if (diskb != NULL)
		printf("disk B:%s\n", diskb);
	if (cart1 != NULL)
		printf("SLOT1 ROM A:%s\n", cart1);
	if (soundfreq != 0)
		printf("sound: %dHz\n", soundfreq);
#endif
}

int main(int argc,char *argv[])
{
#ifdef USE_CW_PROFILER
	ProfilerInit(collectDetailed, bestTimeBase, 100, 8);
#endif /* USE_CW_PROFILER */

	misc_init(argc, argv);


	zodiac_init();
	zodiac_arg(argc, argv);

	if (!md_init(title))
		exit(EXIT_FAILURE);

	audio_init(soundfreq, soundbuf, rcf_r, rcf_c);


	msx_init();
	msx_main();

	audio_fin();
	msx_fin();


	if (md_fps.time > 0)
	{
		printf("zodiac: %d fps.\n", (md_fps.frame * 1000 + md_fps.time - 1) / md_fps.time);
		if (md_fps.skip > 0)
			printf("zodiac: %d skip/sec.\n", (md_fps.skip * 1000 + md_fps.time - 1) / md_fps.time);
	}

#ifdef USE_CW_PROFILER
	ProfilerDump("\pProfileData.prof");
	ProfilerTerm();
#endif /* USE_CW_PROFILER */
	exit(EXIT_SUCCESS);
}

static md_mutex_t context_time_mutex;

void context_new(int hz)
{
	context.hz = hz;

	context.time_sec = 0;
	context.time_cycle = 0;

	context_time_mutex = md_mutex_new();
}

void context_timelock(void)
{
/*	md_mutex_lock(context_time_mutex); */
}

void context_timeunlock(void)
{
/*	md_mutex_unlock(context_time_mutex); */
}

void context_del(void)
{
	md_mutex_del(context_time_mutex);
}


#if 0

void context_init(context_t *cp, int hz, context_t *mp, void (*proc)(void))
{
	cp->hz = hz;
	cp->time_sec = 0;
	cp->time_state = 0;
	cp->master = mp;
	cp->proc = proc;
}

void context_update(context_t *cp, int state)
{
	cp->time_sec += state / cp->hz;
	cp->time_state += state % cp->hz;
}

void context_start(void)
{
}

void context_stop(void)
{
}

#endif

void zodiac_audio_update(int ch)
{
//printf("u %d %d\n", context.time_sec, context.time_cycle);
	if (audio_freq() == 0)
		return;

	context_timelock();
	audio_update(ch, context.time_sec, context.time_cycle / (context.hz / audio_freq()));
	context_timeunlock();
}

