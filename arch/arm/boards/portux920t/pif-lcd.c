// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Anselm Busse <anselm.busse@outlook.com>
/*
 * Display of the taskit PIF extension board: a Batron BT41605 (HD44780,
 * 4x16 characters) on the PIF bus, data byte on NCS3, RS on PC10, R/W on
 * PC11 and an active-low gate for NCS3 on PC12. barebox shows the host
 * name, its version and what it boots (the "lcd" command, used by the boot
 * scripts); Linux has a driver of its own (hd44780-bus).
 */

#include <common.h>
#include <command.h>
#include <getopt.h>
#include <globalvar.h>
#include <init.h>
#include <io.h>
#include <clock.h>
#include <gpio.h>
#include <generated/utsrelease.h>
#include <mach/at91/at91rm9200.h>
#include <mach/at91/at91rm9200_mc.h>
#include <mach/at91/iomux.h>

#define PIF_LCD_DATA	IOMEM(0x40000000)
/* 8-bit, 16 wait states, TDF 2, ACSS 3, RW setup/hold 1 (as in Linux) */
#define PIF_SMC_CSR3	0x11034290
#define PIF_LCD_RS	AT91_PIN_PC10
#define PIF_LCD_RW	AT91_PIN_PC11
#define PIF_LCD_GATE	AT91_PIN_PC12

#define HD44780_BUSY	0x80
#define PIF_LCD_WIDTH	16
#define PIF_LCD_LINES	4

static bool pif_lcd_present;

static void pif_lcd_select(int rs, int read)
{
	gpio_set_value(PIF_LCD_GATE, 0);
	gpio_set_value(PIF_LCD_RS, rs);
	gpio_set_value(PIF_LCD_RW, read);
	ndelay(200);
}

static void pif_lcd_deselect(void)
{
	gpio_set_value(PIF_LCD_GATE, 1);
}

static u8 pif_lcd_status(void)
{
	u8 val;

	pif_lcd_select(0, 1);
	val = readb(PIF_LCD_DATA);
	pif_lcd_deselect();
	return val;
}

static int pif_lcd_wait(void)
{
	uint64_t start = get_time_ns();

	while (pif_lcd_status() & HD44780_BUSY)
		if (is_timeout(start, 10 * MSECOND))
			return -ETIMEDOUT;
	return 0;
}

static void pif_lcd_strobe(u8 val, int rs)
{
	pif_lcd_select(rs, 0);
	writeb(val, PIF_LCD_DATA);
	pif_lcd_deselect();
}

static int pif_lcd_write(u8 val, int rs)
{
	pif_lcd_strobe(val, rs);
	return pif_lcd_wait();
}

/* Write one line, padded with blanks. */
static void pif_lcd_line(int line, const char *s)
{
	/* DDRAM addresses of the four lines of a 16x4 HD44780 */
	static const u8 base[PIF_LCD_LINES] = { 0x00, 0x40, 0x10, 0x50 };
	int i;

	if (!pif_lcd_present || line < 0 || line >= PIF_LCD_LINES)
		return;
	pif_lcd_write(0x80 | base[line], 0);
	for (i = 0; i < PIF_LCD_WIDTH; i++)
		pif_lcd_write(*s ? *s++ : ' ', 1);
}

static int pif_lcd_init(void)
{
	char version[32];
	const char *host;
	size_t len;

	writel(PIF_SMC_CSR3, IOMEM(AT91RM9200_BASE_MC) + AT91RM9200_SMC_CSR(3));
	/* PC10-PC12 come out of reset as NCS4-NCS6 */
	at91_set_gpio_output(PIF_LCD_GATE, 1);
	at91_set_gpio_output(PIF_LCD_RS, 0);
	at91_set_gpio_output(PIF_LCD_RW, 0);

	/*
	 * 8-bit initialisation by instruction (the busy flag cannot be
	 * checked before the third function set), then two "lines" (four on
	 * this display), display on, no cursor, no blinking.
	 */
	pif_lcd_strobe(0x38, 0);
	mdelay(5);
	pif_lcd_strobe(0x38, 0);
	udelay(150);
	pif_lcd_strobe(0x38, 0);
	udelay(150);
	if (pif_lcd_write(0x38, 0) || pif_lcd_write(0x08, 0) ||
	    pif_lcd_write(0x01, 0) || pif_lcd_write(0x06, 0))
		return 0;

	/*
	 * Without the extension board the data bus floats: check that a
	 * character really moves the display's address counter.
	 */
	pif_lcd_write('-', 1);
	if ((pif_lcd_status() & 0x7f) != 1)
		return 0;
	pif_lcd_write(0x01, 0);
	pif_lcd_write(0x0c, 0);
	pif_lcd_present = true;

	host = barebox_get_hostname();
	snprintf(version, sizeof(version), "barebox %s", UTS_RELEASE);
	/* "barebox 2026.10.0" is one character too long: drop the ".0" */
	len = strlen(version);
	if (len > PIF_LCD_WIDTH && !strcmp(version + len - 2, ".0"))
		version[len - 2] = '\0';
	pif_lcd_line(0, host ? host : "");
	pif_lcd_line(2, version);
	pif_lcd_line(3, "starting");
	return 0;
}
postenvironment_initcall(pif_lcd_init);

static int do_lcd(int argc, char *argv[])
{
	char text[PIF_LCD_WIDTH + 1] = "";
	int line, i, opt;

	while ((opt = getopt(argc, argv, "c")) > 0) {
		switch (opt) {
		case 'c':
			for (i = 0; i < PIF_LCD_LINES; i++)
				pif_lcd_line(i, "");
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}
	if (optind == argc)
		return 0;
	line = simple_strtol(argv[optind++], NULL, 0);
	if (line < 0 || line >= PIF_LCD_LINES)
		return COMMAND_ERROR_USAGE;
	for (i = optind; i < argc; i++) {
		if (i > optind)
			strlcat(text, " ", sizeof(text));
		strlcat(text, argv[i], sizeof(text));
	}
	pif_lcd_line(line, text);
	return 0;
}

BAREBOX_CMD_HELP_START(lcd)
BAREBOX_CMD_HELP_TEXT("Write TEXT to LINE (0-3) of the PIF display. Does nothing")
BAREBOX_CMD_HELP_TEXT("without the PIF extension board.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-c", "clear the display first")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(lcd)
	.cmd		= do_lcd,
	BAREBOX_CMD_DESC("write to the PIF display")
	BAREBOX_CMD_OPTS("[-c] [LINE TEXT...]")
	BAREBOX_CMD_GROUP(CMD_GRP_CONSOLE)
	BAREBOX_CMD_HELP(cmd_lcd_help)
BAREBOX_CMD_END
