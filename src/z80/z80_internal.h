extern void z80_cycle_cb(void);
extern void z80_cycle_xx(pair16l_t *xx);
extern void z80_cycle_ed(void);
extern void z80_cycle_xxcb(Uint16 xx);

extern const Uint8 Z80_ZS[256];
extern const Uint8 Z80_PZS[256];


// IFF
#define Z80_IFF1 0x80
#define Z80_IFF2 0x40
#define Z80_IF_HALT 0x20
#define Z80_IF_NMI 0x10

#define Z80_IF_MODE0 0x00
#define Z80_IF_MODE1 0x01
#define Z80_IF_MODE2 0x02

#define Z80_IF_MODEM 0x03    /* mask */






#define	z80_contextsync()                            \
do                                                   \
{                                                    \
	int	a;                                           \
                                                     \
	a = z80.n_cycles - z80.cycle;                    \
                                                     \
	context_timelock();                              \
	context.time_cycle = context.time_cycleprev + a; \
	context_timeunlock();                            \
} while(0)

static __inline__ Uint8 z80_in(Uint8 n)
{
	z80_contextsync();
	return z80.in[n]();
}

#define z80_out(n, a)                \
do                                   \
{                                    \
	z80_contextsync();               \
	z80.out[(n) & 0xff]((a) & 0xff); \
} while(0)


#define	Z80_INC8(a)                              \
do                                               \
{                                                \
	(a) = (Uint8)((a) + 1);                  \
	Z80_F = (Z80_F & Z80_CF) | Z80_ZS[a]         \
	        | ((((a) & 0x0f) == 0) ? Z80_HF : 0) \
	        | (((a) == 0x80) ? Z80_PF : 0);      \
} while(0)

#define Z80_DEC8(a)                                 \
do                                                  \
{                                                   \
	(a) = (Uint8)((a) - 1);                  \
	Z80_F = (Z80_F & Z80_CF) | Z80_ZS[a] | Z80_NF   \
	        | ((((a) & 0x0f) == 0x0f) ? Z80_HF : 0) \
	        | (((a) == 0x7f) ? Z80_PF : 0);         \
} while(0)

#define	Z80_ADD16(a, b)                             \
do                                                  \
{                                                   \
	int n;                                          \
                                                    \
	n = a + b;                                      \
	Z80_F = (Z80_F & ~(Z80_HF | Z80_NF | Z80_CF))   \
	        | (((a ^ b ^ n) >> 8) & Z80_HF)         \
	        | (n >> 16);                            \
	a = (Uint16)n;                                  \
} while(0)
													

static __inline__ void Z80_ADD8(Uint8 a, int b)
{
	int  n;

	n = Z80_A + a + b;
	Z80_F = ((Z80_A ^ a ^ n) & Z80_HF) | (n >> 8) | Z80_ZS[(Uint8)n] |
	    (((~(Z80_A ^ a) & (n ^ a)) >> 5) & Z80_VF);
	Z80_A = n;
}

static __inline__ void Z80_AND8(Uint8 a)
{
	Z80_A &= a;
	Z80_F  = Z80_PZS[Z80_A] | Z80_HF;
}

static __inline__ void Z80_PUSH(pair16l_t a)
{
	z80_wrmem(--Z80_SP, a.b.h);
	z80_wrmem(--Z80_SP, a.b.l);
}

static __inline__ Uint16 Z80_POP(void)
{
	Uint16 a;

	a = z80_rdmem2(Z80_SP);
	Z80_SP += 2;
	return a;
}


static __inline__ void Z80_CP8(Uint8 a)
{
	int  n;

	n = Z80_A - a;
	Z80_F = ((Z80_A ^ a ^ n) & Z80_HF) | ((n >> 8) & Z80_CF)
	        | Z80_ZS[(Uint8)n] | (((~(Z80_A ^ a) & (n ^ a)) >> 5) & Z80_VF)
	        | Z80_NF;
}

static  __inline__  void  Z80_OR8(Uint8 a)
{
	Z80_A |= a;
	Z80_F = Z80_PZS[Z80_A];
}

#define Z80_ADC16(a, b)                                     \
do                                                          \
{                                                           \
	int n;                                                  \
                                                            \
	n = a + b + (Z80_F & Z80_CF);                           \
	Z80_F = (((~(a ^ b) & (b ^ n)) >> 13) & Z80_VF)         \
	        | ((n == 0) ? 0 : Z80_ZF) | ((n >> 8) & Z80_SF) \
	        | (((a ^ b ^ n) >> 8) & Z80_HF) | (n >> 16);    \
	a = n;                                                  \
   } while(0)

static __inline__ void Z80_SUB8(Uint8 a, int b)
{
	int n;

	n = Z80_A - a - b;
	Z80_F = ((Z80_A ^ a ^ n) & Z80_HF) | ((n >> 8) & Z80_CF)
	        | Z80_ZS[(Uint8)n] | (((~(Z80_A ^ a) & (n ^ a)) >> 5) & Z80_VF)
	        | Z80_NF;
	Z80_A = n;
}

static __inline__ void Z80_XOR8(Uint8 a)
{
	Z80_A ^= a;
	Z80_F = Z80_PZS[Z80_A];
}

static __inline__ void Z80_SBC16(Uint16 a)
{
	int n;

	n = Z80_HL - a - (Z80_F & Z80_CF);
	Z80_F = ((((Z80_HL ^ a) & (Z80_HL ^ n)) >> 13) & Z80_VF)
	        | ((n == 0) ? Z80_ZF : 0) | ((n >> 8) & Z80_SF)
	        | (((Z80_HL ^ a ^ n) >> 8) & Z80_HF) | ((n >> 16) & Z80_CF);
	Z80_HL = n;
}

static __inline__ Uint8 Z80_IN8(void)
{
	int a;

	a = z80_in(Z80_C);
	Z80_F = Z80_PZS[a] | (Z80_F & Z80_CF);
	return a;
}


