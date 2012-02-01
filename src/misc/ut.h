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


#ifndef __UT_H_
#define __UT_H_



#ifndef VAR
#	define VAR extern
#endif

extern void *mem_alloc(size_t n);
#define mem_alloc_a mem_alloc
#define mem_alloc_u mem_alloc
#define mem_free free

extern char *mem_strdup(char *s);



typedef enum
{
	VAR_NULL = 0, VAR_TYPE_BOOL, VAR_TYPE_INT, VAR_TYPE_STR
} var_type_t;

typedef struct
{
	char *name;
	var_type_t type;

	void *var;
} var_t;

extern void var_set(var_t *vp, const char *name, const char *s);
extern void var_configfile(const char *fn, var_t *vp);

struct fpath
{
	char *s;
	struct fpath *next;
};
typedef struct fpath fpath_t;

/* XXX */
extern FILE *zodiac_path(const char *fn);
/*
extern char *dupconv_path(const char *s);
extern void add_path(fpath_t **path, const char *s);
extern fpath_t *init_path(void);
extern void setup_path(fpath_t **path, const char *s);
extern void free_path(fpath_t **path);
extern char *psearch_file(const fpath_t *path, const char *fn, struct stat *stp);
*/

#ifdef NDEBUG
	#define debug_printf(x)
	#define debug_puts(x)
#else
	extern void debug_printf2(const char *fmt, ...);
	#define debug_printf(x) debug_printf2 x
	extern void debug_puts(const char *s);
#endif

extern void panic_assert2(char *exp, char *file, char *line);
#define panic_assert(exp) if (!(exp)) panic_assert2(#exp, __FILE__, __LINE__)

extern void misc_init(int argc,char *argv[]);

#endif	/* __UT_H_ */
