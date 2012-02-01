#ifndef __ZODIAC_H_
#define __ZODIAC_H_

#include <md.h>
#include "misc/ut.h"

VAR Uint8 *EMPTYIMAGE;

extern const char *title;       /* XXX */

typedef struct
{
	int hz;                     /* 32bit ���� 4.2 GHz �ޤǤ���ɽ���Բ�ǽ */

	int time_sec;
	int time_cycle;
	int time_cycleprev;
} context_t;

VAR volatile context_t context;          /* current context */

extern void context_new(int hz);

extern void context_timelock(void);
extern void context_timeunlock(void);



#if 0    /* ����ͽ����� */

typedef struct vtime
{
	unsigned long	sec;
	unsigned long	state;
} vtime_t;                      /* ���۴��� */

// #define MAX_VTIME 

typedef struct context
{
	int hz;                     /* 32bit ���� 4.2 GHz �ޤǤ���ɽ���Բ�ǽ */
	int vmul;                   /* ����å����۴������Ѵ���������ܿ� */
	int vcomp;                  /* ���۴����Ѵ��λ��������� */

	vtime_t vtime;              /* ���۴��� */

	struct context *master;     /* Ʊ����Ԥ���� */

	void (*proc)(void);         /* �ᥤ����� */
	void (*ctrl)(bool);         /* ����(start/stop) */
} context_t;

/*

���줾��Υ���ƥ����Ȥλ��ĥ���å����ܿ��򤫤����۴����֤��Ѵ���Ԥ���
������Ʊ�Τ���Ӥˤ�ꤽ�줾���Ʊ�����롣
����å��ι⤤ʪ�ϸ����礭���ʤ��ǽ���Ϥ��롣
�����ǡ�1��ñ�̤������ͤ�ø�������ˤ��������Ԥ���
��1 �äϤǤ�������

���۴����� ULONG_MAX/2 ���餤��������������
ULONG_MAX ���餤���ȷ׻�����Τ����ʤ����Ѥˤʤ��ǽ����ͭ�롣
���ȸ��ä��㤹����ȸ�����Τ������ʤäƤ��ޤ���

*/

#endif

extern void zodiac_audio_update(int ch);

#endif /* __ZODIAC_H_ */

