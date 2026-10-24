/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Gumstix verdex / verdex pro (PXA270) low-level init, run from NOR flash
 * at 0 without a stack. Port of board/gumstix/memsetup.S of the Gumstix
 * U-Boot 1.2.0 (Craig Hughes, Gumstix Inc.), which is what these modules
 * shipped with; register values unchanged.
 *
 * Returns with r0 = 1 if it initialised clocks and SDRAM, 0 if it was
 * already running from SDRAM (chain-loaded) and left them alone. Does not
 * return when the module wakes from sleep: it jumps to the resume address
 * in PSPR like every PXA27x boot loader must.
 */

#include <linux/linkage.h>

#define GPIO_BASE	0x40e00000
#define GPDR0		0x0c
#define GPDR3		0x10c
#define GPSR0		0x18
#define GPSR3		0x118
#define GPCR0		0x24
#define GPCR3		0x124
#define GAFR0_L		0x54

#define PWR_BASE	0x40f00000
#define PSSR		0x04
#define PSPR		0x08
#define RCSR		0x30
#define PSSR_PH		(1 << 4)
#define RCSR_HWR	(1 << 0)
#define RCSR_WDR	(1 << 1)
#define RCSR_SMR	(1 << 2)
#define RCSR_GPR	(1 << 3)

#define CLK_BASE	0x41300000
#define CCCR		0x00
#define OSCC		0x08
#define OSCC_OON	(1 << 1)

#define OST_BASE	0x40a00000
#define OSCR0		0x10
#define OSCR10		0x58
#define OSMR10		0x98
#define OMCR10		0xd8
#define OMCR_C		(1 << 7)
#define OMCR_P		(1 << 6)
#define OMCR_R		(1 << 3)

#define MEMC_BASE	0x48000000
#define MDCNFG		0x00
#define MDREFR		0x04
#define MSC0		0x08
#define MECR		0x14
#define MCMEM0		0x28
#define MCMEM1		0x2c
#define MCATT0		0x30
#define MCATT1		0x34
#define MCIO0		0x38
#define MCIO1		0x3c
#define MDMRS		0x40

#define MDREFR_K0RUN	(1 << 13)
#define MDREFR_E1PIN	(1 << 15)
#define MDREFR_K1RUN	(1 << 16)
#define MDREFR_K1DB2	(1 << 17)
#define MDREFR_K2RUN	(1 << 18)
#define MDREFR_K2DB2	(1 << 19)
#define MDREFR_APD	(1 << 20)
#define MDREFR_SLFRSH	(1 << 22)
#define MDREFR_K0FREE	(1 << 23)
#define MDREFR_K1FREE	(1 << 24)
#define MDREFR_K2FREE	(1 << 25)

#define MDCNFG_DE0	(1 << 0)
#define MDCNFG_DE1	(1 << 1)
#define MDCNFG_DE2	(1 << 16)
#define MDCNFG_DE3	(1 << 17)

#define SDRAM_BASE	0xa0000000

#define ICU_BASE	0x40d00000
#define ICMR		0x04
#define ICLR		0x08
#define ICMR2		0xa0
#define ICLR2		0xa4

/* "Large 1GB model", MA<24:10>, tRP=3, CL=3, tRCD=3, tRAS=7, tRC=10, 32 bit */
#define MDCNFG_13_10	0x8ad00ad0	/* 4 banks x 13 rows x 10 columns */
#define MDCNFG_13_9	0x8ac80ac8	/* 4 banks x 13 rows x 9 columns */

/*
 * L = 16, 2N = 4: 208 MHz run, 416 MHz turbo, what the Gumstix U-Boot of a
 * 400 MHz module sets (CCCR read back as 0x210 on a 64 MiB verdex). The
 * 600 MHz XL6P modules use 2N = 6 (0x310); 416 MHz is safe on those too.
 */
#ifndef GUMSTIX_VERDEX_CCCR
#define GUMSTIX_VERDEX_CCCR	0x00000210
#endif

	.section .text.gumstix_verdex_lowlevel_init
ENTRY(gumstix_verdex_lowlevel_init)
	mov	r10, lr

	/* GPIO levels, directions and alternate functions */
	ldr	r0, =GPIO_BASE
	ldr	r1, =0xfffdef1b
	str	r1, [r0, #GPSR0]
	ldr	r1, =0xffefffff
	str	r1, [r0, #GPSR0 + 4]
	ldr	r1, =0xfffffdff
	str	r1, [r0, #GPSR0 + 8]
	ldr	r1, =0x00000000
	str	r1, [r0, #GPSR3]
	ldr	r1, =0x08023000
	str	r1, [r0, #GPCR0]
	ldr	r1, =0x00100200
	str	r1, [r0, #GPCR0 + 4]
	ldr	r1, =0x00010000
	str	r1, [r0, #GPCR0 + 8]
	ldr	r1, =0x00000800
	str	r1, [r0, #GPCR3]
	ldr	r1, =0xc1829218
	str	r1, [r0, #GPDR0]
	ldr	r1, =0x0012aaa1
	str	r1, [r0, #GPDR0 + 4]
	ldr	r1, =0x00010000
	str	r1, [r0, #GPDR0 + 8]
	ldr	r1, =0x00014800
	str	r1, [r0, #GPDR3]
	add	r2, r0, #GAFR0_L
	ldr	r1, =0x80040000
	str	r1, [r2], #4
	ldr	r1, =0xa5254010
	str	r1, [r2], #4
	ldr	r1, =0x69908010
	str	r1, [r2], #4
	ldr	r1, =0xaaa5a8aa
	str	r1, [r2], #4
	ldr	r1, =0xaaaaaaaa
	str	r1, [r2], #4
	ldr	r1, =0x00010000
	str	r1, [r2], #4
	ldr	r1, =0x10000000
	str	r1, [r2], #4
	ldr	r1, =0x00000001
	str	r1, [r2], #4

	/* release the GPIO pins from their sleep state */
	ldr	r0, =PWR_BASE
	mov	r1, #0x20
	str	r1, [r0, #PSSR]

	/* chain-loaded into SDRAM: clocks and SDRAM are already set up */
	mov	r9, #0
	cmp	pc, #SDRAM_BASE
	bhi	wakeup
	mov	r9, #1

	/* core clock, then a frequency change with turbo mode */
	ldr	r1, =CLK_BASE
	ldr	r2, =GUMSTIX_VERDEX_CCCR
	str	r2, [r1, #CCCR]
	mrc	p14, 0, r2, c6, c0, 0
	orr	r2, r2, #0x3
	mcr	p14, 0, r2, c6, c0, 0
	ldr	r0, [r1, #CCCR]
1:	mrc	p14, 0, r0, c6, c0, 0
	cmp	r2, r0
	bne	1b

	/* static memory: CS0 16 bit P30 flash, CS1 Ethernet */
	ldr	r3, =MEMC_BASE
	ldr	r2, =0x033c35d8
	str	r2, [r3, #MSC0]
	ldr	r2, [r3, #MSC0]

	/* expansion card interface timings */
	ldr	r2, =0x00000002
	str	r2, [r3, #MECR]
	ldr	r2, =0x0002851f
	str	r2, [r3, #MCMEM0]
	str	r2, [r3, #MCMEM1]
	str	r2, [r3, #MCATT0]
	str	r2, [r3, #MCATT1]
	ldr	r2, =0x00014291
	str	r2, [r3, #MCIO0]
	str	r2, [r3, #MCIO1]
	ldr	r2, [r3, #MCIO1]

	ldr	r0, =MDCNFG_13_10
sdram_setup:
	/* configure, but do not enable, the SDRAM partitions */
	str	r0, [r3, #MDCNFG]
	ldr	r0, [r3, #MDCNFG]

	/* valid refresh interval, no free running clocks */
	ldr	r2, [r3, #MDREFR]
	ldr	r1, =0xfff
	bic	r2, r2, r1
	orr	r2, r2, #0x001
	bic	r2, r2, #(MDREFR_K2FREE | MDREFR_K1FREE | MDREFR_K0FREE)
	str	r2, [r3, #MDREFR]

	/* self-refresh and clock-stop -> self-refresh -> power-down -> PWRDNX */
	orr	r2, r2, #(MDREFR_K1RUN | MDREFR_K2RUN)
	bic	r2, r2, #(MDREFR_K1DB2 | MDREFR_K2DB2)
	str	r2, [r3, #MDREFR]
	bic	r2, r2, #MDREFR_SLFRSH
	str	r2, [r3, #MDREFR]
	orr	r2, r2, #MDREFR_E1PIN
	str	r2, [r3, #MDREFR]
	nop

	str	r0, [r3, #MDCNFG]
	ldr	r0, [r3, #MDCNFG]

	/* NOP power-up period, about 200 us at 3.25 MHz */
	ldr	r4, =OST_BASE
	mov	r2, #0
	str	r2, [r4, #OSCR0]
	ldr	r5, =0x300
2:	ldr	r2, [r4, #OSCR0]
	cmp	r5, r2
	bgt	2b

	/* refresh cycles by accessing the disabled SDRAM */
	ldr	r4, =SDRAM_BASE
	mov	r2, #2
3:	str	r2, [r4]
	subs	r2, r2, #1
	bne	3b

	/* enable all four partitions */
	ldr	r2, [r3, #MDCNFG]
	orr	r2, r2, #(MDCNFG_DE0 | MDCNFG_DE1)
	orr	r2, r2, #(MDCNFG_DE2 | MDCNFG_DE3)
	str	r2, [r3, #MDCNFG]

	/* MRS: CAS latency 3, burst 4 */
	ldr	r2, =0x00320032
	str	r2, [r3, #MDMRS]

	/* auto-power-down */
	ldr	r2, [r3, #MDREFR]
	orr	r2, r2, #MDREFR_APD
	str	r2, [r3, #MDREFR]

	/* a mirror at 0x800 means 9 instead of 10 column address bits */
	mov	r0, #0x00000800
	mov	r1, #SDRAM_BASE
	str	r1, [r1]
	str	r0, [r1, r0]
	ldr	r1, [r1]
	cmp	r0, r1
	bne	wakeup
	ldr	r0, =MDCNFG_13_9
	b	sdram_setup

wakeup:
	/* resume from sleep at the address in PSPR */
	ldr	r0, =PWR_BASE
	ldr	r1, [r0, #RCSR]
	and	r1, r1, #(RCSR_GPR | RCSR_SMR | RCSR_WDR | RCSR_HWR)
	str	r1, [r0, #RCSR]
	teq	r1, #RCSR_SMR
	bne	1f
	mov	r1, #PSSR_PH
	str	r1, [r0, #PSSR]
	ldr	r1, [r0, #PSPR]
	mov	pc, r1
1:
	/*
	 * Mask all interrupts. Memory mapped instead of the CP6 accesses of
	 * the original code: those need CPAR set up and trap otherwise.
	 */
	ldr	r0, =ICU_BASE
	mov	r1, #0
	str	r1, [r0, #ICLR]
	str	r1, [r0, #ICLR2]
	str	r1, [r0, #ICMR]
	str	r1, [r0, #ICMR2]

	/* re-write MDREFR */
	ldr	r3, =MEMC_BASE
	ldr	r2, [r3, #MDREFR]
	str	r2, [r3, #MDREFR]

	/* 32.768 kHz oscillator for RTC and power manager */
	ldr	r1, =CLK_BASE
	ldr	r2, [r1, #OSCC]
	orr	r2, r2, #OSCC_OON
	str	r2, [r1, #OSCC]

	/* OS timer channel 10 at 32 kHz for CHOUT<0> */
	ldr	r1, =OST_BASE
	mov	r2, #(OMCR_C | OMCR_P | OMCR_R | 1)
	str	r2, [r1, #OMCR10]
	mov	r2, #1
	str	r2, [r1, #OSMR10]
	str	r2, [r1, #OSCR10]

	/* Ethernet reset (GPIO 107) was asserted above; release after ~200 us */
	mov	r2, #0
	str	r2, [r1, #OSCR0]
	ldr	r4, =0x300
4:	ldr	r2, [r1, #OSCR0]
	cmp	r4, r2
	bgt	4b
	ldr	r0, =GPIO_BASE
	ldr	r1, =0x00000800
	str	r1, [r0, #GPSR3]

	mov	r0, r9
	mov	pc, r10
ENDPROC(gumstix_verdex_lowlevel_init)
	.ltorg
