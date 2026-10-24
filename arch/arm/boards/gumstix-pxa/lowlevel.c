// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Gumstix PXA modules: verdex / verdex pro (PXA270) and basix / connex
 * (PXA255).
 *
 * verdex: barebox can be the first stage in NOR at 0 (clocks, SDRAM and
 * pins as the Gumstix U-Boot set them) or be chain-loaded into SDRAM.
 * connex: second stage only, the PXA255 SDRAM setup of the original
 * U-Boot 1.1.4 is not available, so a previous loader must have done it.
 *
 * The entry functions are naked and run without a stack until
 * arm_setup_stack(); everything else happens in ordinary noinline
 * functions called after that.
 */

#include <common.h>
#include <debug_ll.h>
#include <linux/sizes.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <asm/reloc.h>
#include <asm/io.h>
#include <mach/pxa/debug_ll.h>

#define GUMSTIX_SDRAM_BASE	0xa0000000

static noinline void gumstix_start(void *dtb, unsigned long maxsize)
{
	unsigned long size;

	pxa_debug_ll_init();
	putc_ll('>');

	size = get_ram_size((long *)GUMSTIX_SDRAM_BASE, maxsize);

	barebox_arm_entry(GUMSTIX_SDRAM_BASE, size, dtb);
}

#ifdef CONFIG_MACH_GUMSTIX_VERDEX
int gumstix_verdex_lowlevel_init(void);

static noinline void gumstix_verdex_start(void)
{
	extern char __dtb_z_gumstix_verdex_start[];

	gumstix_start(runtime_address(__dtb_z_gumstix_verdex_start), SZ_256M);
}

ENTRY_FUNCTION(start_gumstix_verdex, r0, r1, r2)
{
	arm_cpu_lowlevel_init();

	/* no stack yet: pure assembly, returns with SDRAM usable */
	gumstix_verdex_lowlevel_init();

	arm_setup_stack(GUMSTIX_SDRAM_BASE + SZ_16M);

	gumstix_verdex_start();
}

static noinline void gumstix_verdex_lan91c111_start(void)
{
	extern char __dtb_z_gumstix_verdex_lan91c111_start[];

	gumstix_start(runtime_address(__dtb_z_gumstix_verdex_lan91c111_start),
		      SZ_256M);
}

ENTRY_FUNCTION(start_gumstix_verdex_lan91c111, r0, r1, r2)
{
	arm_cpu_lowlevel_init();

	gumstix_verdex_lowlevel_init();

	arm_setup_stack(GUMSTIX_SDRAM_BASE + SZ_16M);

	gumstix_verdex_lan91c111_start();
}
#endif

#ifdef CONFIG_MACH_GUMSTIX_CONNEX
static noinline void gumstix_connex_start(void)
{
	extern char __dtb_z_gumstix_connex_start[];
	unsigned long pc = get_pc();

	if (pc < GUMSTIX_SDRAM_BASE || pc >= GUMSTIX_SDRAM_BASE + SZ_256M) {
		/* no SDRAM setup for PXA255 yet: refuse to run from NOR */
		pxa_debug_ll_init();
		putc_ll('!');
		for (;;)
			;
	}

	gumstix_start(runtime_address(__dtb_z_gumstix_connex_start), SZ_64M);
}

ENTRY_FUNCTION(start_gumstix_connex, r0, r1, r2)
{
	arm_cpu_lowlevel_init();

	/*
	 * A previous loader set up SDRAM. When started from NOR instead the
	 * stack below is not usable on real hardware, but nothing is pushed
	 * before gumstix_connex_start() has refused to continue.
	 */
	arm_setup_stack(GUMSTIX_SDRAM_BASE + SZ_16M);

	gumstix_connex_start();
}
#endif
