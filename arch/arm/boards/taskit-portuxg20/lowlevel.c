// SPDX-License-Identifier: GPL-2.0-only
/*
 * taskit PortuxG20 (Stamp9G20, AT91SAM9G20): barebox as second stage.
 * Clocks, SDRAM and the DBGU pins are set up by the first stage
 * (U-Boot SPL from this project, at91bootstrap or the vendor U-Boot).
 */

#include <common.h>
#include <debug_ll.h>
#include <asm/reloc.h>
#include <mach/at91/barebox-arm.h>
#include <mach/at91/at91sam9_sdramc.h>
#include <mach/at91/at91sam9260.h>
#include <mach/at91/hardware.h>

#define PORTUXG20_PIOC		IOMEM(0xfffff800)
#define PIO_PER			0x00
#define PIO_OER			0x10
#define PIO_SODR		0x30

/* PC9 high enables the RS-232 transceiver in front of the DBGU */
static void portuxg20_rs232_enable(void)
{
	writel(BIT(9), PORTUXG20_PIOC + PIO_SODR);
	writel(BIT(9), PORTUXG20_PIOC + PIO_OER);
	writel(BIT(9), PORTUXG20_PIOC + PIO_PER);
}

AT91_ENTRY_FUNCTION(start_taskit_portuxg20, r0, r1, r2)
{
	extern char __dtb_z_at91_portuxg20_start[];

	arm_cpu_lowlevel_init();
	arm_setup_stack(AT91SAM9260_SRAM_END);

	portuxg20_rs232_enable();
	putc_ll('>');

	at91sam9260_barebox_entry(runtime_address(__dtb_z_at91_portuxg20_start));
}
