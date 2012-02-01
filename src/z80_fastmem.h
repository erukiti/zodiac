#include "misc/ut.h"
#include "msx/msx.h"

static __inline__ void z80_wrmem(Uint16 addr, Uint8 n)
{
	int page;

	page = Z80_GETPAGE(addr);
	addr = Z80_GETADDR(addr);

	if (z80_page[page].write == NULL)
		z80_page[page].mem[addr] = n;
	else
		z80_page[page].write(&z80_page[page], page, addr, n);
}

static __inline__ Uint8 z80_rdmem(int addr)
{
	int page;

	page = Z80_GETPAGE(addr);
	addr = Z80_GETADDR(addr);


	if (addr != 0x1fff || z80_page[page].read == NULL)
		return z80_page[page].mem[addr];
	else
		return ~msx.ssl[msx.psl>>6];

/*	if (z80_page[page].read == NULL)
		return z80_page[page].mem[addr];
	else
		return z80_page[page].read(&z80_page[page], page, addr);
*/
}

#if 0
static __inline__ Uint8 *z80_rdmem4(int addr)
{
	int page;
	int a;

	page = Z80_GETPAGE(addr);
	addr = Z80_GETADDR(addr);

	a = 0x2000 - 4;
	if (z80_page[page].read != NULL)
		--a;

	if (addr <= a)
		return &z80_page[page].mem[addr];

//printf("%d %04x\n", page, addr);
	return NULL;
}

//#define z80_rdmem4(addr) NULL
#define z80_rdmem(ptr, addr, n) (((ptr)!=NULL)? ptr[n] : RdZ80(addr))
#endif

static __inline__ Uint16 z80_rdmem2(int addr)
{
	int page;
	int limit;
	pair16l_t a;

	page = Z80_GETPAGE(addr);
	addr = Z80_GETADDR(addr);

	limit = 0x1fff;
	if (z80_page[page].read != NULL)
		--limit;

	if (addr < limit)

#if defined(MD_LITTLE) && !defined(Z80_STRICTWIDE)
		return *(Uint16 *)(z80_page[page].mem + addr);
#else
	{
		a.b.l = z80_page[page].mem[addr];
		a.b.h = z80_page[page].mem[addr+1];
		return a.w;
	}
#endif

	if (z80_page[page].read == NULL)
	{
		if (page != 7)
		{
			a.b.l = z80_page[page].mem[0x1fff];
			a.b.h = z80_page[page+1].mem[0];
		} else
		{
			a.b.l = z80_page[7].mem[0x1fff];
			a.b.h = z80_page[0].mem[0];
		}

		return a.w;
	}

	if (addr != 0x1fff)
	{
		a.b.l = z80_page[page].mem[0x1ffe];
		a.b.h = ~msx.ssl[msx.psl>>6];
	} else
	{
		a.b.l = ~msx.ssl[msx.psl>>6];
		a.b.h = z80_page[0].mem[0];
	}
	return a.w;
}

static __inline__ void z80_wrmem2(int addr, Uint16 n)
{
	int page;
	pair16l_t a;

	page = Z80_GETPAGE(addr);
	addr = Z80_GETADDR(addr);
	a.w = n;

	if (addr != 0x1fff && z80_page[page].write == NULL)
	{
#if defined(MD_LITTLE) && !defined(Z80_STRICTWIDE)
		*(Uint16 *)(z80_page[page].mem+addr) = n;
#else
		z80_page[page].mem[addr] = a.b.l;
		z80_page[page].mem[addr+1] = a.b.h;
#endif
		return;
	}

	if (addr!=0x1fff)
	{
		z80_page[page].write(&z80_page[page], page, addr,   a.b.l);
		z80_page[page].write(&z80_page[page], page, addr + 1, a.b.h);
		return;
	}

	if (z80_page[page].write == NULL)
		z80_page[page].mem[addr] = a.b.l; else
		z80_page[page].write(&z80_page[page], page, addr, a.b.l);

	++page;
	if (page == Z80_NPAGE)
		page = 0;

	if (z80_page[page].write == NULL)
		z80_page[page].mem[0] = a.b.h; else
		z80_page[page].write(&z80_page[page], page, 0, a.b.h);
}

