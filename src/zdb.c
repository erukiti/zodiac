/*
   "Z" debugger thunk.

TODO:
  0. tdebugger を zdb tiny モード(z80) として実装
  1. zdb tiny モード(z80) を外部プロセス化
*/

#include "../config.h"
#include <stdlib.h>

#include "zodiac.h" /* misc/ut.h */
#include "zdb.h"

#ifndef TDEBUGGER
	extern int zdb_ext_init(zdb_t *);
	extern void zdb_ext_fin(zdb_t *);
	extern int zdb_ext_main(zdb_t *);
	extern void zdb_ext_onoff(zdb_t *p_zdb);
#else
	extern int zdb_tiny_init(zdb_t *);
	extern void zdb_tiny_fin(zdb_t *);
	extern int zdb_tiny_main(zdb_t *);
	extern void zdb_tiny_onoff(zdb_t *p_zdb);
#endif



void zdb_init(void)
{
}

void zdb_fin(void)
{
}

zdb_t *zdb_open(void)
{
	zdb_t *p_zdb;

	p_zdb = (zdb_t *)mem_alloc(sizeof(zdb_t));
	if (p_zdb == NULL)
		return NULL;


#ifndef TDEBUGGER
	if (zdb_ext_init(p_zdb) == -1)
		return NULL;
#else
	if (zdb_tiny_init(p_zdb) == -1)
		return NULL;
#endif

	return p_zdb;
}


void zdb_close(zdb_t *p_zdb)
{
#ifndef TDEBUGGER
	zdb_ext_fin(p_zdb);
#else
	zdb_tiny_fin(p_zdb);
#endif

	mem_free(p_zdb);
}


int zdb_main(zdb_t *p_zdb)
{
	int result;

#ifndef TDEBUGGER
	result = zdb_ext_main(p_zdb);
#else
	result = zdb_tiny_main(p_zdb);
#endif

	return result;
}


void zdb_onoff(zdb_t *p_zdb)
{
#ifndef TDEBUGGER
	zdb_ext_onoff(p_zdb);
#else
	zdb_tiny_onoff(p_zdb);
#endif
}


