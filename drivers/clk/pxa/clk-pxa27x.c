// SPDX-License-Identifier: GPL-2.0-only
/*
 * Marvell PXA27x family clocks
 *
 * Copyright (C) 2014 Robert Jarzmik
 *
 * Ported from the Linux kernel (drivers/clk/pxa/clk-pxa27x.c). Only the
 * peripheral clocks barebox can use are kept; the core, bus and memory
 * clocks and frequency changing are left out, the PBL sets the operating
 * point once.
 */
#include <common.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <of.h>

#include <dt-bindings/clock/pxa-clock.h>
#include "clk-pxa.h"
#include "clk-pxa2xx.h"

#define MHz (1000 * 1000)

static void __iomem *clk_regs;

static bool pxa27x_is_ppll_disabled(void)
{
	unsigned long ccsr = readl(clk_regs + CCSR);

	return ccsr & (1 << CCCR_PPDIS_BIT);
}

#define PXA27X_CKEN(dev_id, con_id, parents, mult_hp, div_hp,		\
		    bit, is_lp, flags)					\
	PXA_CKEN(dev_id, con_id, bit, parents, 1, 1, mult_hp, div_hp,	\
		 is_lp,  CKEN, CKEN_ ## bit, flags)
#define PXA27X_PBUS_CKEN(dev_id, con_id, bit, mult_hp, div_hp, delay)	\
	PXA27X_CKEN(dev_id, con_id, pxa27x_pbus_parents, mult_hp,	\
		    div_hp, bit, pxa27x_is_ppll_disabled, 0)

PARENTS(pxa27x_pbus) = { "osc_13mhz", "ppll_312mhz" };
PARENTS(pxa27x_32Mhz_bus) = { "osc_32_768khz", "osc_32_768khz" };

#define PXA27X_CKEN_1RATE(dev_id, con_id, bit, parents, delay)		\
	PXA_CKEN_1RATE(dev_id, con_id, bit, parents,			\
		       CKEN, CKEN_ ## bit, 0)

static struct desc_clk_cken pxa27x_clocks[] = {
	PXA27X_PBUS_CKEN("pxa2xx-uart.0", NULL, FFUART, 2, 42, 1),
	PXA27X_PBUS_CKEN("pxa2xx-uart.1", NULL, BTUART, 2, 42, 1),
	PXA27X_PBUS_CKEN("pxa2xx-uart.2", NULL, STUART, 2, 42, 1),
	PXA27X_PBUS_CKEN("pxa2xx-i2c.0", NULL, I2C, 2, 19, 0),
	PXA27X_PBUS_CKEN("pxa27x-udc", NULL, USB, 2, 13, 5),
	PXA27X_PBUS_CKEN("pxa2xx-mci.0", NULL, MMC, 2, 32, 0),
	PXA27X_PBUS_CKEN("pxa27x-ohci", NULL, USBHOST, 2, 13, 0),
	PXA27X_PBUS_CKEN("pxa2xx-i2c.1", NULL, PWRI2C, 1, 24, 0),
	PXA27X_PBUS_CKEN("pxa27x-ssp.0", NULL, SSP1, 1, 24, 0),
	PXA27X_PBUS_CKEN("pxa27x-ssp.1", NULL, SSP2, 1, 24, 0),
	PXA27X_PBUS_CKEN("pxa27x-ssp.2", NULL, SSP3, 1, 24, 0),
	PXA27X_PBUS_CKEN("pxa27x-pwm.0", NULL, PWM0, 1, 24, 0),
	PXA27X_PBUS_CKEN("pxa27x-pwm.1", NULL, PWM1, 1, 24, 0),
	PXA27X_PBUS_CKEN(NULL, "OSTIMER0", OSTIMER, 1, 96, 0),

	PXA27X_CKEN_1RATE("pxa27x-keypad", NULL, KEYPAD,
			  pxa27x_32Mhz_bus_parents, 0),
};

static void pxa27x_register_plls(void)
{
	clk_register_fixed_rate("osc_13mhz", NULL, CLK_GET_RATE_NOCACHE,
				13 * MHz);
	clkdev_pxa_register(CLK_OSC32k768, "osc_32_768khz", NULL,
			    clk_register_fixed_rate("osc_32_768khz", NULL,
						    CLK_GET_RATE_NOCACHE,
						    32768));
	clk_register_fixed_rate("clk_dummy", NULL, 0, 0);
	clk_register_fixed_factor(NULL, "ppll_312mhz", "osc_13mhz", 0, 24, 1);
}

static int pxa27x_dt_clocks_init(struct device_node *np)
{
	int ret;

	clk_regs = IOMEM(PXA2XX_CLK_BASE);

	pxa27x_register_plls();

	ret = clk_pxa_cken_init(pxa27x_clocks, ARRAY_SIZE(pxa27x_clocks),
				clk_regs);
	if (ret)
		return ret;

	return clk_pxa_dt_common_init(np);
}
CLK_OF_DECLARE(pxa_clks, "marvell,pxa270-clocks", pxa27x_dt_clocks_init);
