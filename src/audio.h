#ifndef __AUDIO_H_
#define __AUDIO_H_

extern bool audio_init(int freq, int size, int r, int c);
extern void audio_fin(void);
extern void audio_start(void);
extern void audio_stop(void);

extern int audio_freq(void);


/* for audio module. */
extern int audio_add(void (*calc)(int *, int), int volume, int cuttoff);
extern void audio_update(int ch, int sec, int count);

extern void singlepcm_init(void);
extern void singlepcm_play(Sint16 *p, int n);

#define VOL_BITS 9
#define VOL_RANGE 48.0
#define VOL_STEP (VOL_RANGE / (1 << VOL_BITS))

#define XXX_VOL_PSG  7.13
#define XXX_VOL_OPLL  6.00
#define XXX_VOL_PWM 5.00
#define XXX_VOL_SINGLEPCM 0

#define XXX_COFF_PSG 8000
#define XXX_COFF_OPLL 8000
#define XXX_COFF_PWM 0
#define XXX_COFF_SINGLEPCM 0

#define XXX_AUDIO_DELAY 1

#define XXX_AUDIO_FREQ 22050
#define XXX_AUDIO_BUFSIZE 50

#define XXX_AUDIO_RCF_R 4700
#define XXX_AUDIO_RCF_C 10



#endif /* __AUDIO_H_ */
