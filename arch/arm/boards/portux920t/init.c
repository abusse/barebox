// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2009-2011 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
// SPDX-FileCopyrightText: 2026 Anselm Busse <anselm.busse@outlook.com>
/*
 * taskit Portux 920T: AT91RM9200 module with 64 MiB SDRAM, 16 MiB NOR on
 * NCS0, Davicom DM9161E RMII PHY at MDIO address 0, DBGU console, two LEDs
 * on PC0/PC1 (active low), USB host (1 port) and USB device (VBUS sense PC5,
 * pull-up PB17). No DataFlash is fitted; the SD slot is left to Linux (no
 * RM9200 MCI support in barebox).
 */

#include <common.h>
#include <net.h>
#include <init.h>
#include <environment.h>
#include <asm/armlinux.h>
#include <asm/mach-types.h>
#include <fs.h>
#include <gpio.h>
#include <fcntl.h>
#include <io.h>
#include <envfs.h>
#include <linux/sizes.h>
#include <mach/at91/hardware.h>
#include <mach/at91/at91_pmc.h>
#include <mach/at91/board.h>
#include <mach/at91/iomux.h>
#include <linux/ctype.h>
#include <globalvar.h>

static struct macb_platform_data ether_pdata = {
	.phy_interface = PHY_INTERFACE_MODE_RMII,
	.phy_addr = 0,
};

static int portux920t_mem_init(void)
{
	at91_add_device_sdram(0);

	return 0;
}
mem_initcall(portux920t_mem_init);

static struct at91_usbh_data portux_usbh_data = {
	.ports		= 1,
	.vbus_pin	= { -EINVAL, -EINVAL },
};

#ifdef CONFIG_LED_GPIO
struct gpio_led portux_leds[] = {
	{
		.gpio		= AT91_PIN_PC0,
		.active_low	= 1,
		.led = {
			.name = "led1",
		},
	}, {
		.gpio		= AT91_PIN_PC1,
		.active_low	= 1,
		.led = {
			.name = "led2",
		},
	},
};

static void portux_device_add_leds(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(portux_leds); i++) {
		at91_set_gpio_output(portux_leds[i].gpio, portux_leds[i].active_low);
		led_gpio_register(&portux_leds[i]);
	}
	led_set_trigger(LED_TRIGGER_HEARTBEAT, &portux_leds[1].led);
}
#else
static void portux_device_add_leds(void) {}
#endif

#if defined(CONFIG_USB_GADGET_DRIVER_AT91)
static struct at91_udc_data __initdata portux_udc_data = {
	.vbus_pin	= AT91_PIN_PC5,
	.pullup_pin	= AT91_PIN_PB17,
};

static void portux_add_device_udc(void)
{
	at91_add_device_udc(&portux_udc_data);
}
#else
static void portux_add_device_udc(void) {}
#endif

/*
 * The vendor U-Boot keeps "ethaddr=xx:xx:xx:xx:xx:xx" in its environment
 * sector (NOR 0x20000, 4-byte CRC then NUL-separated "name=value" strings).
 * Use it as the default MAC while that sector is still intact, and keep it
 * in global.portux.vendor_ethaddr for bin/portux-install.
 */
static void portux920t_vendor_ethaddr(void)
{
	const char *p = (const char *)(AT91_CHIPSELECT_0 + 0x20000 + 4);
	const char *end = p + 0x20000 - 4;
	u8 mac[6];

	while (p < end && *p) {
		if (!strncmp(p, "ethaddr=", 8) && !string_to_ethaddr(p + 8, mac)) {
			eth_register_ethaddr(0, mac);
			globalvar_add_simple("portux.vendor_ethaddr", p + 8);
			return;
		}
		p += strlen(p) + 1;
	}
}

static int portux920t_devices_init(void)
{
	portux920t_vendor_ethaddr();
	at91_add_device_eth(0, &ether_pdata);
	add_cfi_flash_device(0, AT91_CHIPSELECT_0, SZ_16M, 0);
	at91_add_device_usbh_ohci(&portux_usbh_data);
	portux_device_add_leds();
	portux_add_device_udc();

#if defined(CONFIG_DRIVER_CFI) || defined(CONFIG_DRIVER_CFI_OLD)
	/*
	 * NOR: 128 KiB sectors; barebox gets the first three, its env the
	 * fourth. The rest holds the rescue system for the "nor" boot target,
	 * as uImages (see bin/nor-install); same layout as in the Linux DT.
	 */
	devfs_add_partition("nor0", 0x000000, 0x060000, DEVFS_PARTITION_FIXED, "self");
	devfs_add_partition("nor0", 0x060000, 0x020000, DEVFS_PARTITION_FIXED, "env0");
	devfs_add_partition("nor0", 0x080000, 0x680000, DEVFS_PARTITION_FIXED, "nor0.kernel");
	devfs_add_partition("nor0", 0x700000, 0x020000, DEVFS_PARTITION_FIXED, "nor0.oftree");
	devfs_add_partition("nor0", 0x720000, 0x8e0000, DEVFS_PARTITION_FIXED, "nor0.root");
	default_environment_path_set("/dev/env0");
#endif

	/* no machine id of its own; only device-tree kernels are expected */
	armlinux_set_architecture(MACH_TYPE_AT91RM9200EK);

	defaultenv_append_directory(defaultenv_portux920t);

	return 0;
}
device_initcall(portux920t_devices_init);

static int portux920t_console_init(void)
{
	barebox_set_model("taskit Portux 920T");
	barebox_set_hostname("portux920t");

	at91_register_uart(0, 0);
	return 0;
}
console_initcall(portux920t_console_init);

static int portux920t_main_clock(void)
{
	at91_set_main_clock(18432000);
	return 0;
}
pure_initcall(portux920t_main_clock);
