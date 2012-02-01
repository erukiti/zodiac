#ifndef __ZODIAC_H_
#define __ZODIAC_H_

#include <md.h>
#include "misc/ut.h"

VAR Uint8 *EMPTYIMAGE;

extern const char *title;       /* XXX */

typedef struct
{
	int hz;                     /* 32bit だと 4.2 GHz までしか表現不可能 */

	int time_sec;
	int time_cycle;
	int time_cycleprev;
} context_t;

VAR volatile context_t context;          /* current context */

extern void context_new(int hz);

extern void context_timelock(void);
extern void context_timeunlock(void);



#if 0    /* 改良予定候補 */

typedef struct vtime
{
	unsigned long	sec;
	unsigned long	state;
} vtime_t;                      /* 仮想基準時 */

// #define MAX_VTIME 

typedef struct context
{
	int hz;                     /* 32bit だと 4.2 GHz までしか表現不可能 */
	int vmul;                   /* クロックを仮想基準時に変換する時の倍数 */
	int vcomp;                  /* 仮想基準時変換の時の補正値 */

	vtime_t vtime;              /* 仮想基準時 */

	struct context *master;     /* 同期を行う相手 */

	void (*proc)(void);         /* メイン処理 */
	void (*ctrl)(bool);         /* 制御(start/stop) */
} context_t;

/*

それぞれのコンテキストの持つクロックに倍数をかけ仮想基準時間に変換を行う。
基準時間同士の比較によりそれぞれを同期する。
クロックの高い物は誤差が大きくなる可能性はある。
そこで、1秒単位で補正値を加減する事により補正を行う。
＃1 秒はでかすぎ？

仮想基準時は ULONG_MAX/2 くらいがいいだろうか。
ULONG_MAX くらいだと計算するのがかなり大変になる可能性が有る。
かと言って低すぎると誤差がものすごくなってしまう。

*/

#endif

extern void zodiac_audio_update(int ch);

#endif /* __ZODIAC_H_ */

