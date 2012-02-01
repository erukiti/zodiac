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

#include "../../config.h"

#ifdef HAVE_SYS_TYPES_H
#	include <sys/types.h>
#endif

#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_STDARG_H
#	include <stdarg.h>
#endif

#include <md.h>

#ifdef HAVE_LIBGEN_H
#	include <libgen.h>
#endif



#include "ut.h"

void convert_path(const char *, char *, const char *, char);


void *mem_alloc(size_t n)
{
	void *p;

	p = malloc(n);
	if (p == NULL)
		exit(EXIT_FAILURE);
	return p;
}

char *mem_strdup(char *s)
{
	return strcpy((char *)mem_alloc(strlen(s) + 1), s);
}



void var_set(var_t *vp, const char *name, const char *s)
{
/* printf("vset: [%s][%s]\n", name, s); */

	while(vp->name != NULL)
	{
		if (strcmp(vp->name, name) == 0)
		{
			switch(vp->type)
			{
			case VAR_TYPE_BOOL:
				*(bool *)vp->var = (strchr("ytYT1", *s) != NULL);
				break;
			case VAR_TYPE_INT:
				*(int *)vp->var = atoi(s);
				break;
			case VAR_TYPE_STR:
				*(void **)vp->var = mem_alloc(strlen(s) + 1);
				strcpy(*(char **)vp->var, s);
				break;

			case VAR_NULL:
				;
			}
			return;
		}
		++vp;
	}
}


// define MD_DIR_DRIVESEPARATOR none

#if MD_DIR_LAYOUT == MD_UNIX
#	define MD_DIR_SEPARATOR '/'
#	define MD_DIR_PATHSEPARATOR ':'

#elif MD_DIR_LAYOUT == MD_WINDOWS
#	define MD_DIR_SEPARATOR '\\'
#	define MD_DIR_PATHSEPARATOR ';'

#elif MD_DIR_LAYOUT == MD_MAC
#	define MD_DIR_SEPARATOR ':'
#	define MD_DIR_PATHSEPARATOR ';'

#endif


#ifndef MAXPATHLEN
#	ifdef MAX_PATH
#		define MAXPATHLEN MAX_PATH
#	elif defined(_MAX_PATH)
#		define MAXPATHLEN _MAX_PATH
#	else
#		define MAXPATHLEN 1024
#	endif
#endif


/* XXX to libmd */

#ifndef HAVE_DIRNAME

static int dirbase(const char *p, int n)
{
	if (n > 0 && p[n - 1] == MD_DIR_SEPARATOR)
		n--;

	while (n > 0 && p[--n] != MD_DIR_SEPARATOR)
		;

	return n;
}

char *dirname(const char *path)
{
	static char dir[MAXPATHLEN + 1];
	int n;

	n = strlen(path);
	if (n > MAXPATHLEN)
	{
#ifdef ENAMETOOLONG
		errno = ENAMETOOLONG;
#endif
		return NULL;
	}

	n = dirbase(path, n);

	strncpy(dir, path, n);
	dir[n] = '\0';

	return dir;
}

char *basename(const char *path)
{
	static char dir[MAXPATHLEN + 1];
	int n;

	n = strlen(path);
	if (n > MAXPATHLEN)
	{
#ifdef ENAMETOOLONG
		errno = ENAMETOOLONG;
#endif
		return NULL;
	}

	n = dirbase(path, n);
	if (path[n] == '\0')
		dir[0] = '\0';
	else
		strcpy(dir, path + n + 1);

	return dir;
}



#endif

char *progname, *progdir;

void misc_init(int argc, char *argv[])
{
	char *p;

	if (argc <= 0)
	{
		progname = "";
		progdir = "";
		return;
	}

	progname = mem_strdup(argv[0]);
	p = dirname(argv[0]);
	if (p == NULL)
		progdir = "";
	else
		progdir = mem_strdup(p);
}









#if 0 /* ¤È¤ê¤¢¤¨¤º disable */

#define MI_DRIVE_SEPARATOR_CHAR ':'	/* XXX ¤¤¤º¤ì¤Ï md.h ¤« config.h ¤Ø? */
#define MI_DIR_POSITION_CHAR '.'	/*     Æ±¾å                          */
#define MI_DIR_SEPARATOR_CHAR '/'	/*     Æ±¾å                          */
#define MI_ENVPATH_SEPARATOR_CHAR ':'	/*     Æ±¾å                          */

/*
    Windows ¤Ê¤é ':' '\\' '.' '..'
    Mac ¤Ê¤é ':' '::'
    Unix ·Ï¤Ê¤é '/' '.' '..'
*/
#if defined(__WIN32__) || defined(WIN32)

  #define MD_DRIVE_SEPARATOR_CHAR ':'	/*     Æ±¾å (UNIX ¤Ç¤ÏÉ¬Í×¤Ê¤¤)      */
  #define MD_DIR_SEPARATOR_CHAR '\\'	/*     Æ±¾å                          */
  #define MD_DIR_POSITION_CHAR '.'	/*     Æ±¾å                          */
  #define MD_ENVPATH_SEPARATOR_CHAR ':'	/*     Æ±¾å                          */

#else

  #define MD_DRIVE_SEPARATOR_CHAR ':'	/*     Æ±¾å (UNIX ¤Ç¤ÏÉ¬Í×¤Ê¤¤)      */
  #define MD_DIR_SEPARATOR_CHAR '/'	/*     Æ±¾å                          */
  #define MD_DIR_POSITION_CHAR '.'	/*     Æ±¾å                          */
  #define MD_ENVPATH_SEPARATOR_CHAR ':'	/*     Æ±¾å                          */

#endif

#define STRDUP(p1, p2) { \
	(p1) = (char *) mem_alloc(sizeof(char) * (strlen((p2)) + 1)); \
	if ((p1) != NULL) { \
		sprintf((p1), "%s", (p2)); \
	} \
}

static void add_dirsep_chr(char **s)
{
       char *p;

       if (*s == NULL)
		return;

       if (*((*s) + strlen(*s) - 1) == MD_DIR_SEPARATOR_CHAR)
		return;

       p = (char *) mem_alloc(sizeof(char) * (strlen(*s) + 1));
       if (p == NULL)
		return;

       sprintf(p, "%s/", *s);
       mem_free(*s);
       *s = p;
}

char *dupconv_path(const char *s)
{
	char *p1, *p2;

	if (s == NULL)
		return NULL;

#if defined(__WIN32__) || defined(WIN32)
	p1 = (char *) s;
#else
	/* ¥É¥é¥¤¥Ö¥ì¥¿¡¼¤Ï¼Î¤Æ¤ë(UNIX ·Ï¤Ç¤Ï) */
	p1 = strchr(s, MI_DRIVE_SEPARATOR_CHAR);
	if (p1 == NULL) {
		p1 = (char *) s;
	} else {
		if (*(p1 + 1))
			p1++;

		/* ¥É¥é¥¤¥Ö¥ì¥¿¡¼¤ÎÎÙ¤Ë¤¢¤ë¥»¥Ñ¥ì¡¼¥¿¤â¼Î¤Æ¤ë */
/*
		if ((*p1 == MI_DIR_SEPARATOR_CHAR) && (*(p1 + 1)))
			p1++;
*/
	}
#endif

	STRDUP(p2, p1);
	if (p2 == NULL)
		return;

	for (p1 = p2; *p1; p1++) {
		if (*p1 == MI_DIR_SEPARATOR_CHAR)
			*p1 = MD_DIR_SEPARATOR_CHAR;

		if (*p1 == MI_DIR_POSITION_CHAR)
			*p1 = MD_DIR_POSITION_CHAR;
	}
	add_dirsep_chr(&p2);
	return p2;
}


void add_path(fpath_t **path, const char *s)
{
	fpath_t *np;
	struct stat st;
	char *sp;

	if ((sp = dupconv_path(s)) == NULL)
		return;

	if (stat(sp, &st) != 0)
		goto FAIL1;

	if ((st.st_mode & S_IFDIR) == 0)
		goto FAIL1;

	np = (fpath_t *) mem_alloc(sizeof(fpath_t));
	if (!np)
		goto FAIL1;

	STRDUP(np->s, sp);
	if (np->s == NULL)
		goto FAIL2;

	np->next = *path;
	*path = np;

FAIL1:
	mem_free(sp);
	return;
FAIL2:
	mem_free(sp);
	mem_free(np);
	return;
	}


fpath_t *init_path(void)
{
	fpath_t *path;
	char *p;

	path = (fpath_t *) mem_alloc(sizeof(fpath_t));
	if (path == NULL)
		return NULL;

#if defined(__WIN32__) || defined(WIN32)
	/* ??? */
#else
	p = getenv("HOME");
	if (p != NULL)
		add_path(&path, p);

	add_path(&path, DATADIR);
	add_path(&path, "./");
#endif

	return path;
}


void setup_path(fpath_t **path, const char *s)
{
	char *dsp, *p1, *p2, *p3;

	if (s == NULL || *s == '\0')
		return;

	STRDUP(dsp, s);
	p1 = dsp;
	while(1) {
		p2 = strchr(p1, MI_ENVPATH_SEPARATOR_CHAR);
		if (p2 != NULL)
			*p2 = '\0';

		p3 = p1;
		while(isspace(*p3))
			p3++;

		add_path(path, p3);

		if ((p2 == NULL) || (*(p2 + 1) == '\0'))
			break;

		p1 = p2 + 1;
	}
	mem_free(dsp);
}


void free_path(fpath_t **path)
{
	fpath_t *p1, *p2;

	if (*path == NULL)
		return;

	p1 = (*path)->next;
	while(p1) {
		p2 = p1->next;
		if (p1->s)
			mem_free(p1->s);
		mem_free(p1);
		p1 = p2;
	}
	mem_free(*path);
}


#ifndef PATH_MAX
#	define PATH_MAX 1024
#endif

char *psearch_file(const fpath_t *path, const char *fn, struct stat *stp)
{
	struct stat st;
	fpath_t *fpp;
	int i;

	if (stp == NULL)
		stp = &st;

	for (fpp = path; fpp->next; fpp = fpp->next)
	{
		char *p;
		int n;

		n = strlen(fpp->s) + strlen(fn);
		if (n > PATH_MAX)
			continue;

		p = (char *)mem_alloc(n + 1);
		sprintf(p, "%s%s", fpp->s, fn);

		if (stat(p, stp) == 0)
			return p;

		mem_free(p);
	}
	return NULL;
}


#if 0
int main() {
	fpath_t *fp, *p;

	fp = init_path();
	setup_path(&fp, "/usr/pkg/bin/:/usr/X11R6/bin/:/usr/pkg/etc");

	/* test */
	for(p = fp; p->next; p = p->next) {
		printf("ptr:%08x next:%08x s:\"%s\"\n", p, p->next, p->s);
	}

	printf("%s\n", psearch_file(fp, ".zodiac/config", NULL));
	free_path(&fp);
	return 0;
}
#endif



char *psearch_find(const char **path, const char *fn, struct stat *stp)
{
	struct stat st;
	int i;

	if (stp == NULL)
		stp = &st;

	for (i = 0; path[i] != NULL; ++i)
	{
		char *p;
		int n;

		n = strlen(path[i]) + strlen(fn);
		if (n > PATH_MAX)
			continue;

		p = (char *)mem_alloc(n + 1);
		sprintf(p, "%s%s", path[i], fn);

		if (stat(p, stp) == 0)
			return p;

		mem_free(p);
 	}

	return NULL;
}






char **psearch_default(const char *name)
{
	char **p;

#if defined(__WIN32__) || defined(WIN32)

	char *q;

	p = (char **)mem_alloc(sizeof(char *) * 3);

	p[0] = (char *)mem_alloc(1);
	*p[0] = '\0';

/* ¥Ð¥¤¥Ê¥ê¡¼¤Îµ¯Æ°¤·¤¿¥Ç¥£¥ì¥¯¥È¥ê¤«¤é¥Ñ¥¹¤òµá¤á¤ë */
	q = dirname(args[0]);
	p[1] = (char *)mem_alloc(strlen(q) + 1 + 1);
	sprintf(p[1], "%s\\", q);

	p[2] = NULL;

#else

	char *r;
	int n;

	p = (char **)mem_alloc(sizeof(char *) * 3);
	n = 0;

	r = getenv("HOME");
	if (r != NULL)
	{
		p[0] = (char *)mem_alloc(strlen(r) + 2 + strlen(name) + 1 + 1);
		sprintf(p[0], "%s/.%s/", r, name);
		++n;
	}

	p[n] = (char *)mem_alloc(strlen(DATADIR) + 1 + name + 2);
	sprintf(p[n], DATADIR "/%s", name);

	++n;

	p[n] = NULL;

#endif

	return p;
	
}

#endif


FILE *zodiac_path(const char *fn)
{
	FILE *fp;
	char buf[MAXPATHLEN + 1];

#if MD_DIR_LAYOUT != MD_UNIX

/*	convert_path(fn, buf, "", MD_DIR_SEPARATOR); */

	fp = fopen(fn, "rb");
	if (fp != NULL)
		return fp;

	sprintf(buf, "%s%c%s", progdir, MD_DIR_SEPARATOR, fn);
	return fopen(buf, "rb");

#else

	char *p;

	fp = fopen(fn, "rb");
	if (fp != NULL)
		return fp;

	p = getenv("HOME");
	if (p != NULL)
	{
		sprintf(buf, "%s/.zodiac/%s", p, fn);	/* XXX buffer overrun */
		fp = fopen(buf, "rb");
		if (fp != NULL)
			return fp;
	}

	sprintf(buf, DATADIR "/%s", fn);
	return fopen(buf, "rb");

#endif
}


void var_configfile(const char *fn, var_t *vp)
{
	FILE *fp;
	char *p, *q, buf[256+1], name[256+1];

	fp = zodiac_path(fn);
	if (fp == NULL)
		return;

	while(fgets(buf, sizeof(buf) - 1, fp) != NULL)
	{
		if (*buf == '\0' || *buf == '#')
			continue;
		p = buf + strlen(buf) - 1;
		if (*p == '\n')
			*p = '\0';

		p = buf;
		while(isspace(*p))
			++p;

		q = name;
		while(isalpha(*p))
			*q++ = *p++;
		*q = '\0';
		if (*name == '\0')
			continue;

		while(isspace(*p))
			++p;
		while(strchr(":=", *p))
			++p;
		while(isspace(*p))
			++p;

		var_set(vp, name, p);
	}

	fclose(fp);
}



void panic_assert2(char *exp, char *file, char *line)
{
	fprintf(stderr, "panic: %s failed in %s:%s\n", exp, file, line);
	exit(EXIT_FAILURE);
}

/*
   ---------------------------------------------------------------------------
	¥ convert_path
   ---------------------------------------------------------------------------
	Change to any from UNIX standerd path
*/

void convert_path(const char *inPath, char *outPath, const char *headPath, char sepChar)
{
	int	i, theCnt, theLen;

	if (sepChar)
	{
		/* convert path charactor */

		theLen = strlen(headPath);
		if (theLen > 0)
			strcpy(outPath, headPath);

		for (i = 0;; i++)
		{
			outPath[i + theLen] = inPath[i];
			if (outPath[i + theLen] == 0)
				break;
			else if (outPath[i + theLen] == '/')
				outPath[i + theLen] = sepChar;
		}
	}
	else
	{
		/* delete path infomation */

		theLen = strlen(inPath) - 1;
		for (i = theLen; i >= 0; i--)
		{
			if (inPath[i] == '/')
				break;
		}
		theCnt = i + 1;
		for(i = 0; ; i++)
		{
			if (!(outPath[i] = inPath[i + theCnt]))
				break;
		}
	}
}

#ifndef NDEBUG

void debug_printf2(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	fputs("debug: ", stderr);
	vfprintf(stderr, fmt, args);
	fputs("\n", stderr);
	va_end(args);
}

void debug_puts(const char *s)
{
	fputs(s, stderr);
}

#endif

