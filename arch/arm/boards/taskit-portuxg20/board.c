// SPDX-License-Identifier: GPL-2.0-or-later

#include <common.h>
#include <deep-probe.h>
#include <envfs.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <mach/at91/at91_rstc.h>
#include <mach/at91/at91sam9260.h>

#define PMC_PCER		IOMEM(0xfffffc10)
#define PIO_PER			0x00
#define PIO_OER			0x10
#define PIO_CODR		0x34
#define PIO_PUDR		0x60
#define PIO_PUER		0x64

/*
 * The DM9161 samples its MDIO address from the RMII receive lines while
 * NRST is low, so reset it with their pull-ups off (address 0) and its
 * 50 MHz reference oscillator on (PA26 low), as the U-Boot board code
 * does. The pulse is ERSTL(5), about 2 ms; the wait is a bounded poll, as
 * no timer may run yet.
 */
static void portuxg20_phy_reset(void)
{
	void __iomem *pioa = IOMEM(AT91SAM9260_BASE_PIOA);
	void __iomem *rstc = IOMEM(AT91SAM9260_BASE_RSTC);
	const u32 straps = BIT(14) | BIT(15) | BIT(17) | BIT(18) | BIT(28);
	u32 mr = readl(rstc + AT91_RSTC_MR) & ~AT91_RSTC_KEY;
	int i;

	writel(BIT(AT91SAM9260_ID_PIOA), PMC_PCER);
	writel(BIT(26), pioa + PIO_CODR);
	writel(BIT(26), pioa + PIO_OER);
	writel(BIT(26), pioa + PIO_PER);
	writel(straps, pioa + PIO_PUDR);

	writel(AT91_RSTC_KEY | (5 << 8) | AT91_RSTC_URSTEN, rstc + AT91_RSTC_MR);
	writel(AT91_RSTC_KEY | AT91_RSTC_EXTRST, rstc + AT91_RSTC_CR);
	for (i = 0; i < 10000000; i++) {
		u32 sr = readl(rstc + AT91_RSTC_SR);

		if (!(sr & AT91_RSTC_SRCMP) && (sr & AT91_RSTC_NRSTL))
			break;
	}

	writel(AT91_RSTC_KEY | mr, rstc + AT91_RSTC_MR);
	writel(straps, pioa + PIO_PUER);
}

static int portuxg20_probe(struct device *dev)
{
	portuxg20_phy_reset();
	defaultenv_append_directory(defaultenv_portuxg20);
	return 0;
}

static const struct of_device_id portuxg20_of_match[] = {
	{ .compatible = "taskit,portuxg20" },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(portuxg20_of_match);

static struct driver portuxg20_board_driver = {
	.name = "board-portuxg20",
	.probe = portuxg20_probe,
	.of_compatible = portuxg20_of_match,
};
coredevice_platform_driver(portuxg20_board_driver);
