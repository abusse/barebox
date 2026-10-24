// SPDX-License-Identifier: GPL-2.0-only
/*
 * Marvell PXA25x family clocks
 *
 * Copyright (C) 2014 Robert Jarzmik
 *
 * Ported from the Linux kernel (drivers/clk/pxa/clk-pxa25x.c). Only the
 * peripheral clocks barebox can use are kept; the core clocks and
 * frequency changing are left out, the PBL sets the operating point once.
 * The OS timer has its own fixed-clock node in pxa25x.dtsi.
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

PARENTS(pxa25x_pbus95) = { "ppll_95_85mhz", "ppll_95_85mhz" };
PARENTS(pxa25x_pbus147) = { "ppll_147_46mhz", "ppll_147_46mhz" };
PARENTS(pxa25x_osc3) = { "osc_3_6864mhz", "osc_3_6864mhz" };

#define PXA25X_CKEN(dev_id, con_id, parents, mult, div,			\
		    bit, is_lp, flags)					\
	PXA_CKEN(dev_id, con_id, bit, parents, mult, div, mult, div,	\
		 is_lp,  CKEN, CKEN_ ## bit, flags)
#define PXA25X_PBUS95_CKEN(dev_id, con_id, bit, mult_hp, div_hp, delay)	\
	PXA25X_CKEN(dev_id, con_id, pxa25x_pbus95_parents, mult_hp,	\
		    div_hp, bit, NULL, 0)
#define PXA25X_PBUS147_CKEN(dev_id, con_id, bit, mult_hp, div_hp, delay)\
	PXA25X_CKEN(dev_id, con_id, pxa25x_pbus147_parents, mult_hp,	\
		    div_hp, bit, NULL, 0)
#define PXA25X_OSC3_CKEN(dev_id, con_id, bit, mult_hp, div_hp, delay)	\
	PXA25X_CKEN(dev_id, con_id, pxa25x_osc3_parents, mult_hp,	\
		    div_hp, bit, NULL, 0)

static struct desc_clk_cken pxa25x_clocks[] = {
	PXA25X_PBUS95_CKEN("pxa2xx-mci.0", NULL, MMC, 1, 5, 0),
	PXA25X_PBUS95_CKEN("pxa2xx-i2c.0", NULL, I2C, 1, 3, 0),
	PXA25X_PBUS95_CKEN("pxa25x-udc", NULL, USB, 1, 2, 5),
	PXA25X_PBUS147_CKEN("pxa2xx-uart.0", NULL, FFUART, 1, 10, 1),
	PXA25X_PBUS147_CKEN("pxa2xx-uart.1", NULL, BTUART, 1, 10, 1),
	PXA25X_PBUS147_CKEN("pxa2xx-uart.2", NULL, STUART, 1, 10, 1),
	PXA25X_PBUS147_CKEN("pxa2xx-uart.3", NULL, HWUART, 1, 10, 1),
	PXA25X_OSC3_CKEN("pxa25x-ssp.0", NULL, SSP, 1, 1, 0),
	PXA25X_OSC3_CKEN("pxa25x-nssp.1", NULL, NSSP, 1, 1, 0),
	PXA25X_OSC3_CKEN("pxa25x-nssp.2", NULL, ASSP, 1, 1, 0),
	PXA25X_OSC3_CKEN("pxa25x-pwm.0", NULL, PWM0, 1, 1, 0),
	PXA25X_OSC3_CKEN("pxa25x-pwm.1", NULL, PWM1, 1, 1, 0),
};

static void pxa25x_register_plls(void)
{
	clk_register_fixed_rate("osc_3_6864mhz", NULL, CLK_GET_RATE_NOCACHE,
				3686400);
	clkdev_pxa_register(CLK_OSC32k768, "osc_32_768khz", NULL,
			    clk_register_fixed_rate("osc_32_768khz", NULL,
						    CLK_GET_RATE_NOCACHE,
						    32768));
	clk_register_fixed_rate("clk_dummy", NULL, 0, 0);
	clk_register_fixed_factor(NULL, "ppll_95_85mhz", "osc_3_6864mhz",
				  0, 26, 1);
	clk_register_fixed_factor(NULL, "ppll_147_46mhz", "osc_3_6864mhz",
				  0, 40, 1);
}

static int pxa25x_dt_clocks_init(struct device_node *np)
{
	int ret;

	pxa25x_register_plls();

	ret = clk_pxa_cken_init(pxa25x_clocks, ARRAY_SIZE(pxa25x_clocks),
				IOMEM(PXA2XX_CLK_BASE));
	if (ret)
		return ret;

	return clk_pxa_dt_common_init(np);
}
CLK_OF_DECLARE(pxa25x_clks, "marvell,pxa250-core-clocks",
	       pxa25x_dt_clocks_init);
