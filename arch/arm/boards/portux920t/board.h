/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * taskit Portux 920T: AT91RM9200, 18.432 MHz main crystal, 64 MiB SDRAM on
 * NCS1, 16 MiB 16-bit NOR (S29GL128N) on NCS0. The register values below are
 * the ones the vendor U-Boot 1.1.2 programs, read back from a running board.
 */

#ifndef __BOARD_H
#define __BOARD_H

#define MASTER_PLL_MUL		39
#define MASTER_PLL_DIV		4

/* clocks: PLLA 179.712 MHz, MCK = PLLA/3 = 59.904 MHz, PLLB 96 MHz (USB /2) */
#define CONFIG_SYS_MOR_VAL						\
		(AT91_PMC_MOSCEN |					\
		 (255 << 8))		/* Main Oscillator Start-up Time */
#define CONFIG_SYS_PLLAR_VAL						\
		(AT91_PMC_PLLA_WR_ERRATA | /* Bit 29 must be 1 when prog */ \
		 (0x3e << 8) |		/* PLL Counter */		\
		 (0 << 14) |		/* Divider A */			\
		 ((MASTER_PLL_MUL - 1) << 16) | (MASTER_PLL_DIV))
#define CONFIG_SYS_PLLBR_VAL	0x10483E0E

/* PCK/2 = MCK Master Clock from SLOW */
#define	CONFIG_SYS_MCKR2_VAL1		\
		(AT91_PMC_CSS_SLOW |	\
		 AT91RM9200_PMC_MDIV_2)

/* PCK/3 = MCK Master Clock = 59.904 MHz from PLLA */
#define	CONFIG_SYS_MCKR2_VAL2		\
		(AT91_PMC_CSS_PLLA |	\
		 AT91_PMC_PRES_1 |	\
		 AT91RM9200_PMC_MDIV_3 |\
		 AT91_PMC_PDIV_1)

/* NOR on NCS0: 16-bit, byte select, TDF 2, 6 wait states (vendor: 0x3286) */
#define CONFIG_SYS_EBI_CFGR_VAL	0x00000000
#define CONFIG_SYS_SMC_CSR0_VAL							\
		(AT91RM9200_SMC_NWS_(6) |	/* Number of Wait States */		\
		 AT91RM9200_SMC_WSEN |	/* Wait State Enable */			\
		 AT91RM9200_SMC_TDF_(2) |	/* Data Float Time */			\
		 AT91RM9200_SMC_BAT |		/* Byte Access Type */			\
		 AT91RM9200_SMC_DBW_16)	/* Data Bus Width */

/* SDRAM: D16-D31 on PIOC, NCS1 = SDRAM controller */
#define CONFIG_SYS_PIOC_ASR_VAL	0xFFFF0000
#define CONFIG_SYS_PIOC_BSR_VAL	0x00000000
#define CONFIG_SYS_PIOC_PDR_VAL	0xFFFF0000
#define CONFIG_SYS_EBI_CSA_VAL	AT91RM9200_EBI_CS1A_SDRAMC

/* SDRC_CR / SDRC_TR exactly as the vendor U-Boot programs them (64 MiB) */
#define CONFIG_SYS_SDRC_CR_VAL	0x21914159
#define CONFIG_SYS_SDRC_TR_VAL	0x000002E0

#endif	/* __BOARD_H */
