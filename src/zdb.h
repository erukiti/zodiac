/*
   "Z" debugger header

*/

typedef struct
{
	void *internal; /* debugger 本体が自由に使うポインター */

} zdb_t;


extern void zdb_init(void);
extern void zdb_fin(void);

extern zdb_t *zdb_open(void);
extern void zdb_close(zdb_t *p_zdb);
extern int zdb_main(zdb_t *p_zdb);

extern void zdb_onoff(zdb_t *p_zdb);
