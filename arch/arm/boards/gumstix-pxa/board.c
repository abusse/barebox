// SPDX-License-Identifier: GPL-2.0-or-later

#include <common.h>
#include <crc.h>
#include <driver.h>
#include <envfs.h>
#include <fcntl.h>
#include <fs.h>
#include <globalvar.h>
#include <init.h>
#include <net.h>
#include <of.h>
#include <asm/barebox-arm.h>
#include <asm/memory.h>
#include <mach/pxa/mfp-pxa27x.h>

#define GUMSTIX_SDRAM_BASE	0xa0000000

/* The Gumstix U-Boot's environment: the last 4 KiB of its two NOR blocks */
#define GUMSTIX_UBOOT_ENV_OFFSET	0x3f000
#define GUMSTIX_UBOOT_ENV_SIZE		0x1000

static bool gumstix_pxa_board(void)
{
	return of_machine_is_compatible("gumstix,verdex") ||
		of_machine_is_compatible("gumstix,connex");
}

/* SDRAM as sized by the PBL, which probed it for mirrors */
static int gumstix_pxa_mem_init(void)
{
	if (!gumstix_pxa_board())
		return 0;

	arm_add_mem_device("ram0", GUMSTIX_SDRAM_BASE,
			   arm_mem_endmem_get() - GUMSTIX_SDRAM_BASE);

	return 0;
}
mem_initcall(gumstix_pxa_mem_init);

/*
 * The Gumstix U-Boot's memsetup (the first-stage init) leaves four of the six
 * MMC pins as GPIOs; its mmcinit command set them at run time.
 */
static unsigned long gumstix_verdex_mmc_pins[] = {
	GPIO32_MMC_CLK,
	GPIO92_MMC_DAT_0,
	GPIO109_MMC_DAT_1,
	GPIO110_MMC_DAT_2,
	GPIO111_MMC_DAT_3,
	GPIO112_MMC_CMD,
};

static int gumstix_pxa_probe(struct device *dev)
{
	const char *model = NULL, *board = "gumstix-verdex";

	barebox_set_hostname("gumstix");

	if (of_machine_is_compatible("gumstix,verdex"))
		pxa2xx_mfp_config(gumstix_verdex_mmc_pins,
				  ARRAY_SIZE(gumstix_verdex_mmc_pins));

	/* the Linux device tree that matches this barebox build */
	of_property_read_string(of_get_root_node(), "model", &model);
	if (model && strstr(model, "LAN91C111"))
		board = "gumstix-verdex-lan91c111";
	else if (model && strstr(model, "connex"))
		board = "gumstix-connex";
	globalvar_add_simple("gumstix.board", board);
	globalvar_add_simple("gumstix.dtb", basprintf("%s.dtb", board));

	/* the NFS root and board directory, when not from DHCP */
	globalvar_add_simple("gumstix.nfsroot", NULL);
	globalvar_add_simple("gumstix.nfsstate", NULL);

	defaultenv_append_directory(defaultenv_gumstix);

	return 0;
}

static const struct of_device_id gumstix_pxa_of_match[] = {
	{ .compatible = "gumstix,verdex" },
	{ .compatible = "gumstix,connex" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, gumstix_pxa_of_match);

static struct driver gumstix_pxa_driver = {
	.name = "board-gumstix-pxa",
	.probe = gumstix_pxa_probe,
	.of_compatible = gumstix_pxa_of_match,
};
coredevice_platform_driver(gumstix_pxa_driver);

/*
 * The Ethernet chips on the Gumstix carriers have no MAC address of their
 * own. The Gumstix U-Boot makes one from the board serial number (the NOR
 * chip's unique number) and keeps both in its environment, "serial#" and
 * "ethaddr". When barebox is chain-loaded from that U-Boot, take them from
 * there, so that the board keeps the identity it has always had; the serial
 * number also reaches Linux (/serial-number in the device tree).
 */
static int gumstix_uboot_env(void)
{
	char *buf, *p, *end;
	u8 mac[ETH_ALEN];
	int fd, ret;

	if (!gumstix_pxa_board())
		return 0;

	fd = open("/dev/nor0", O_RDONLY);
	if (fd < 0)
		return 0;

	buf = xmalloc(GUMSTIX_UBOOT_ENV_SIZE);
	if (lseek(fd, GUMSTIX_UBOOT_ENV_OFFSET, SEEK_SET) != GUMSTIX_UBOOT_ENV_OFFSET)
		ret = -EIO;
	else
		ret = read(fd, buf, GUMSTIX_UBOOT_ENV_SIZE);
	close(fd);
	if (ret != GUMSTIX_UBOOT_ENV_SIZE)
		goto out;

	/* a CRC32 of the variables, then "name=value" strings, empty one last */
	if (crc32(0, buf + 4, GUMSTIX_UBOOT_ENV_SIZE - 4) !=
	    le32_to_cpu(*(__le32 *)buf)) {
		pr_info("gumstix: no U-Boot environment in NOR\n");
		goto out;
	}

	end = buf + GUMSTIX_UBOOT_ENV_SIZE;
	buf[GUMSTIX_UBOOT_ENV_SIZE - 1] = '\0';
	for (p = buf + 4; p < end && *p; p += strlen(p) + 1) {
		if (!strncmp(p, "serial#=", 8)) {
			barebox_set_serial_number(p + 8);
			pr_info("gumstix: serial number %s from the U-Boot environment\n",
				p + 8);
		} else if (!strncmp(p, "ethaddr=", 8) &&
			   !string_to_ethaddr(p + 8, mac) &&
			   is_valid_ether_addr(mac)) {
			eth_register_ethaddr(0, mac);
			pr_info("gumstix: MAC address %s from the U-Boot environment\n",
				p + 8);
		}
	}
out:
	free(buf);
	return 0;
}
late_initcall(gumstix_uboot_env);
