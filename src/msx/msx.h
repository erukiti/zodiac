#ifndef __MSX_H_
#define __MSX_H_

#include "../misc/ut.h"
#include "../z80/z80.h"
#include "../v99x8/v99x8.h"

typedef struct msx_slot
{
	void (*change)(struct msx_slot *self, Uint8 n);     /* page 書換 */
	Uint8 *mem;
	int offset, size;
	void *ptr;                                          /* 汎用 */
	const z80_patch_t *patch;
} msx_slot_t;


typedef struct
{
	Uint8 psl, ssl[4];

	msx_slot_t *slot[4 * 4 * 4];
	msx_slot_t *slot_current[4];
	bool f_extslot[4];

	Uint8 mapper[4];

	bool f_mainloop;                    /* ??? state にすべき？ */
} msx_t;

VAR msx_t msx;

#define msx_slot_num(psl, ssl, page) ((psl) * 16 + (page) \
                                  + (msx.f_extslot[psl] ? ((ssl) * 4) : 0))

#define msx_slot(psl, ssl, page) msx.slot[msx_slot_num((psl), (ssl), (page))]






extern void msx_init(void);
extern void msx_fin(void);
extern void msx_pause(bool f);
extern void msx_main(void);

extern void msx_slot_update(int n);

extern void msx_slot_change(int page, int psl, int ssl);
extern void msx_slot_change2(int page, int slot);


extern void msx_memmap_init(void);
extern void msx_memmap_load(Uint8 psl, Uint8 ssl, Uint8 size);
extern msx_slot_t *msx_rom_load(Uint8 psl, Uint8 ssl, int offset, int size, const char *s, const z80_patch_t *patch);
extern msx_slot_t *msx_dos2_load(Uint8 psl, Uint8 ssl, const char *s, const z80_patch_t *patch);



extern void msx_rtc_init(void);
extern void msx_rtc_fin(void);
extern void msx_ppi_init(void);
extern void msx_psg_init(void);

extern void msx_tape_init(void);
extern void msx_tape_fin(void);

extern bool msx_disk_change(int n, const char *s);
extern bool msx_disk_init(int psl, int ssl);
extern void msx_disk_fin(void);

extern void msx_music_init(int psl, int ssl);
extern void msx_music_fin(void);

extern void msx_event_keyscan(void);

extern void msx_kanji_init(void);

extern void msx_video_init(int port);


/* XXX move! */
typedef struct
{
	char *diska;
	char *diskb;
	char *cart1;

	int syncfreq;
	int uperiod;
} msx_conf_t;

extern msx_conf_t msx_conf;



#endif /* __MSX_H_ */
