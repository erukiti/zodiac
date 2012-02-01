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

#include "config.h"
#include <string.h>

#if defined(WIN32) || defined(__WIN32__)
#	include <windows.h>
#endif


#include "md.h"


#ifndef HAVE_GETOPT

char *optarg;
int optind=1;
int optopt;
int optreset;

//int opterr=1;

int getopt(int argc, char * const *argv, const char *opt)
{
	static char *ptr = "";
	const char *p;

	if (optreset || *ptr == '\0')
	{
		optreset = 0;
		if (optind >= argc)
		{
			ptr = "";
			return -1;
		}

		ptr = argv[optind];
		if (*ptr++ != '-')
		{
			ptr = "";
			return -1;
		}
		if (*ptr == '-')
		{
			++optind;
			ptr = "";
			return -1;
		}
	}

	optopt = *ptr++;
	p = strchr(opt, optopt);

	++p;
	if (*p != ':') // no opt
	{
		optarg = NULL;
		if (*p != '\0')
			++optind;
		return optopt;
	}

	if (*ptr != '\0')
	{
		optarg = ptr;
	} else
	{
		++optind;
		if (optind >= argc)
			return ':';

		optarg = argv[optind];
	}
	++optind;
	ptr = "";
	return optopt;
}

#endif

#ifndef HAVE_UNAME

int uname(struct utsname *u)
{
#if defined(WIN32) || defined(__WIN32__)
	DWORD len;
	SYSTEM_INFO sys_info;
    OSVERSIONINFO os_info;

	len = SYS_NMLN;
	GetComputerName(u->nodename, &len);

	GetSystemInfo(&sys_info);
	switch (sys_info.wProcessorArchitecture)
	{
	case PROCESSOR_ARCHITECTURE_INTEL:
		strcpy(u->machine, "x86"); // nProcLevel
		break;

//	case PROCESSOR_ARCHITECTURE_MIPS:
//	case PROCESSOR_ARCHITECTURE_ALPHA:
//	case PROCESSOR_ARCHITECTURE_PPC:

	default:
		strcpy(u->machine, "unknown");
	}

	os_info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&os_info);

	switch (os_info.dwPlatformId)
	{
	case VER_PLATFORM_WIN32s:
		strcpy(u->release, "Win32s");
		break;

	case VER_PLATFORM_WIN32_WINDOWS:

		switch(((LPBYTE)&os_info.dwBuildNumber)[1])
		{
		case 0:
			strcpy(u->release, "95");
			if (strcmp(os_info.szCSDVersion, "C") == 0)
				strcat(u->release, " OSR2");
			break;

		case 10:
			strcpy(u->release, "98");
			if (strcmp(os_info.szCSDVersion, "A") == 0)
				strcat(u->release, " SE");
			break;

		case 90:
			strcpy(u->release, "Me");
			break;

		default:
			strcpy(u->release, "unknown");
		}

		break;

	case VER_PLATFORM_WIN32_NT:
		if (os_info.dwMajorVersion < 5)
			strcpy(u->release, "NT");
		else if (os_info.dwMajorVersion == 5 && os_info.dwMinorVersion == 0)
			strcpy(u->release, "2000");
		else if (os_info.dwMajorVersion == 5 && os_info.dwMinorVersion == 1)
			strcpy(u->release, "XP");
		else
			strcpy(u->release, "NT");

		sprintf(u->release + strlen(u->release), " %s ", os_info.szCSDVersion);
		break;

	default:
		strcpy(u->release, "unknown");
	}

	strcpy(u->sysname, "Windows");
	sprintf(u->version, "Windows%s %s (%ld.%ld build %ld)", u->release,
	        os_info.szCSDVersion, os_info.dwMajorVersion,
	        os_info.dwMinorVersion, os_info.dwBuildNumber);

	return 0;
#else

	strcpy(u->sysname, "unknown");
	strcpy(u->nodename, "");
	strcpy(u->release, "");
	strcpy(u->version, "unknown");
	strcpy(u->machine, "unknown");

	return 0;
#endif

}

#endif

