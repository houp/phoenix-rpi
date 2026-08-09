/*
 * bt-probe — Bluetooth Tier-0 bring-up probe for the BCM43455 combo chip on
 * the Raspberry Pi 4, under Phoenix-RTOS. Standalone one-shot userspace tool
 * (mirrors tools/wifi-probe).
 *
 * HW routing truth (established 2026-08-09 via the read-only dump below): the
 * VideoCore firmware routes NO UART to the BT chip -- GPIO30-33 are plain
 * inputs and the mini-UART is disabled; PL011 is the debug CONSOLE on GPIO14/15
 * (dtoverlay=miniuart-bt did not take at runtime). So this probe routes BT to
 * the AUX mini-UART itself (PL011 stays the console): sets GPIO32/33[+30/31] to
 * ALT5, enables+configures the AUX mini-UART (baud from the CORE clock read via
 * mailbox), raises BT_REG_ON (expgpio[0]=mbox GPIO 128), then speaks H4 HCI:
 *   1. HCI_RESET (0x0c03)  -- ROM answers with no .hcd => UART<->BT alive.
 *   2. READ_LOCAL_VERSION (0x1001) -- LMP subver picks the .hcd for Tier-2.
 *
 * Copyright 2026 Phoenix Systems
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <sys/mman.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ---- VideoCore mailbox (property channel) ------------------------------- */
#define RPI_PI4_MAILBOX_BASE 0xfe00b880u
#define VC_MBOX_READ 0x00u
#define VC_MBOX_STATUS 0x18u
#define VC_MBOX_WRITE 0x20u
#define VC_MBOX_STATUS_FULL 0x80000000u
#define VC_MBOX_STATUS_EMPTY 0x40000000u
#define VC_MBOX_RESP_OK 0x80000000u
#define VC_MBOX_PROP_CHANNEL 8u
#define VC_PROP_SET_GPIO_STATE 0x00038041u
#define VC_PROP_GET_CLOCK_RATE 0x00030002u
#define VC_CLK_CORE 4u                 /* the mini-UART is clocked by the CORE clock */
#define EXPGPIO_BT_ON 128u             /* expgpio[0]="BT_ON" (WL_ON=129=expgpio[1]) */

/* Generic property-channel call: msg[5]=arg0, msg[6]=arg1 in/out; returns
 * msg[6] (the second response word) on success, 0xFFFFFFFF on failure. */
static uint32_t diag_mbox2(uint32_t tag, uint32_t arg0, uint32_t arg1)
{
	addr_t pa_base = (addr_t)RPI_PI4_MAILBOX_BASE & ~(addr_t)(_PAGE_SIZE - 1);
	addr_t pa_offs = (addr_t)RPI_PI4_MAILBOX_BASE & (addr_t)(_PAGE_SIZE - 1);
	volatile uint32_t *mbox;
	uint32_t *msg;
	uintptr_t msg_pa;
	uint32_t request;
	uint32_t result = 0xFFFFFFFFu;
	void *mbox_page;
	void *msg_page;

	mbox_page = mmap(NULL, _PAGE_SIZE, PROT_READ | PROT_WRITE,
		MAP_DEVICE | MAP_UNCACHED | MAP_PHYSMEM | MAP_ANONYMOUS, -1, pa_base);
	if (mbox_page == MAP_FAILED) {
		return 0xFFFFFFFFu;
	}
	mbox = (volatile uint32_t *)((volatile uint8_t *)mbox_page + pa_offs);

	msg_page = mmap(NULL, _PAGE_SIZE, PROT_READ | PROT_WRITE,
		MAP_UNCACHED | MAP_CONTIGUOUS | MAP_ANONYMOUS, -1, 0);
	if (msg_page == MAP_FAILED) {
		munmap(mbox_page, _PAGE_SIZE);
		return 0xFFFFFFFFu;
	}
	msg = msg_page;

	msg[0] = 32;
	msg[1] = 0;
	msg[2] = tag;
	msg[3] = 8;
	msg[4] = 0;
	msg[5] = arg0;
	msg[6] = arg1;
	msg[7] = 0;

	msg_pa = (uintptr_t)va2pa(msg);
	if (msg_pa == (uintptr_t)-1) {
		munmap(msg_page, _PAGE_SIZE);
		munmap(mbox_page, _PAGE_SIZE);
		return 0xFFFFFFFFu;
	}
	request = ((uint32_t)msg_pa & ~0xFu) | VC_MBOX_PROP_CHANNEL;

	while ((mbox[VC_MBOX_STATUS / 4] & VC_MBOX_STATUS_FULL) != 0u) {
	}
	mbox[VC_MBOX_WRITE / 4] = request;
	for (;;) {
		while ((mbox[VC_MBOX_STATUS / 4] & VC_MBOX_STATUS_EMPTY) != 0u) {
		}
		if (mbox[VC_MBOX_READ / 4] == request) {
			break;
		}
	}
	if (msg[1] == VC_MBOX_RESP_OK) {
		result = msg[6];
	}
	munmap(msg_page, _PAGE_SIZE);
	munmap(mbox_page, _PAGE_SIZE);
	return result;
}

/* ---- BCM2711 GPIO ------------------------------------------------------- */
#define GPIO_BASE 0xfe200000u
static void gpio_fsel(volatile uint8_t *g, unsigned pin, unsigned fn)
{
	volatile uint32_t *reg = (volatile uint32_t *)(g + (pin / 10u) * 4u);
	unsigned shift = (pin % 10u) * 3u;
	uint32_t v = *reg;
	v &= ~(0x7u << shift);
	v |= ((fn & 0x7u) << shift);
	*reg = v;
}
static unsigned gpio_getfsel(volatile uint8_t *g, unsigned pin)
{
	uint32_t v = *(volatile uint32_t *)(g + (pin / 10u) * 4u);
	return (v >> ((pin % 10u) * 3u)) & 0x7u;
}
static const char *fsel_name(unsigned f)
{
	switch (f) {
	case 0: return "IN";
	case 1: return "OUT";
	case 4: return "ALT0";
	case 5: return "ALT1";
	case 6: return "ALT2";
	case 7: return "ALT3";
	case 3: return "ALT4";
	case 2: return "ALT5";
	default: return "?";
	}
}
#define GPIO_FN_ALT5 2u  /* mini-UART (TXD1/RXD1/CTS1/RTS1) */

/* ---- AUX mini-UART (BCM2835 aux @ 0xfe215000) --------------------------- */
#define AUX_BASE 0xfe215000u
#define AUX_ENABLES 0x04u
#define AUX_MU_IO 0x40u
#define AUX_MU_IER 0x44u
#define AUX_MU_IIR 0x48u
#define AUX_MU_LCR 0x4Cu
#define AUX_MU_MCR 0x50u
#define AUX_MU_LSR 0x54u
#define AUX_MU_CNTL 0x60u
#define AUX_MU_BAUD 0x68u
#define LSR_RX_RDY (1u << 0)
#define LSR_TX_EMPTY (1u << 5)

static void aux_init(volatile uint8_t *a, uint32_t baud_reg)
{
	uint32_t en = *(volatile uint32_t *)(a + AUX_ENABLES);
	*(volatile uint32_t *)(a + AUX_ENABLES) = en | 1u;    /* enable mini-UART */
	*(volatile uint32_t *)(a + AUX_MU_CNTL) = 0u;         /* disable TX/RX during setup */
	*(volatile uint32_t *)(a + AUX_MU_IER) = 0u;
	*(volatile uint32_t *)(a + AUX_MU_LCR) = 3u;          /* 8-bit (BCM erratum: 0x3) */
	*(volatile uint32_t *)(a + AUX_MU_MCR) = 0u;
	*(volatile uint32_t *)(a + AUX_MU_IIR) = 0xC6u;       /* clear RX+TX FIFOs */
	*(volatile uint32_t *)(a + AUX_MU_BAUD) = baud_reg;
	*(volatile uint32_t *)(a + AUX_MU_CNTL) = 3u;         /* enable TX+RX (no auto-flow) */
}
static int aux_putc(volatile uint8_t *a, uint8_t c)
{
	int d;
	for (d = 0; d < 2000000; ++d) {
		if ((*(volatile uint32_t *)(a + AUX_MU_LSR) & LSR_TX_EMPTY) != 0u) {
			*(volatile uint32_t *)(a + AUX_MU_IO) = c;
			return 0;
		}
	}
	return -1;
}
static int aux_getc(volatile uint8_t *a, int timeout_ms)
{
	int t;
	for (t = 0; t < timeout_ms * 20; ++t) {
		if ((*(volatile uint32_t *)(a + AUX_MU_LSR) & LSR_RX_RDY) != 0u) {
			return (int)(*(volatile uint32_t *)(a + AUX_MU_IO) & 0xffu);
		}
		usleep(50);
	}
	return -1;
}

/* Send an H4 HCI command and collect the reply. Returns bytes received, or -1
 * if the command could not be sent. */
static int hci_cmd(volatile uint8_t *a, uint16_t opcode, const uint8_t *params,
	uint8_t plen, uint8_t *resp, int cap)
{
	int i, b, n = 0;
	if (aux_putc(a, 0x01u) != 0) {
		return -1;
	}
	(void)aux_putc(a, (uint8_t)(opcode & 0xffu));
	(void)aux_putc(a, (uint8_t)((opcode >> 8) & 0xffu));
	(void)aux_putc(a, plen);
	for (i = 0; i < (int)plen; ++i) {
		(void)aux_putc(a, params[i]);
	}
	b = aux_getc(a, 500);
	while (b >= 0 && n < cap) {
		resp[n++] = (uint8_t)b;
		b = aux_getc(a, 40);
	}
	return n;
}

static void hexdump(const char *label, const uint8_t *p, int n)
{
	int i;
	printf("%s (%d):", label, n);
	for (i = 0; i < n; ++i) {
		printf(" %02x", p[i]);
	}
	printf("\n");
}

int main(void)
{
	void *gpio_page, *aux_page;
	volatile uint8_t *g, *a;
	uint8_t resp[64];
	uint32_t core_hz, baud_reg;
	int n, bton;

	printf("PHX-BT/0 tier0-hci-probe (mini-UART)\n");

	gpio_page = mmap(NULL, _PAGE_SIZE, PROT_READ | PROT_WRITE,
		MAP_DEVICE | MAP_UNCACHED | MAP_PHYSMEM | MAP_ANONYMOUS, -1, GPIO_BASE);
	aux_page = mmap(NULL, _PAGE_SIZE, PROT_READ | PROT_WRITE,
		MAP_DEVICE | MAP_UNCACHED | MAP_PHYSMEM | MAP_ANONYMOUS, -1, AUX_BASE);
	if (gpio_page == MAP_FAILED || aux_page == MAP_FAILED) {
		printf("error: mmap failed\n.\n");
		return 1;
	}
	g = (volatile uint8_t *)gpio_page;
	a = (volatile uint8_t *)aux_page;

	printf("routing before: GPIO14=%s 15=%s  30=%s 31=%s 32=%s 33=%s\n",
		fsel_name(gpio_getfsel(g, 14)), fsel_name(gpio_getfsel(g, 15)),
		fsel_name(gpio_getfsel(g, 30)), fsel_name(gpio_getfsel(g, 31)),
		fsel_name(gpio_getfsel(g, 32)), fsel_name(gpio_getfsel(g, 33)));

	/* Core clock -> mini-UART baud divisor for 115200. */
	core_hz = diag_mbox2(VC_PROP_GET_CLOCK_RATE, VC_CLK_CORE, 0u);
	if (core_hz == 0u || core_hz == 0xFFFFFFFFu) {
		core_hz = 500000000u; /* fall back to a common Pi4 core freq */
		printf("GET_CLOCK_RATE(core) failed; assuming %u Hz\n", core_hz);
	}
	baud_reg = core_hz / (8u * 115200u);
	if (baud_reg > 0u) {
		baud_reg -= 1u;
	}
	printf("core_clk=%u Hz -> AUX_MU_BAUD=%u (target 115200)\n", core_hz, baud_reg);

	/* Raise BT_REG_ON (expgpio[0]) and let the BT ROM boot. */
	(void)diag_mbox2(VC_PROP_SET_GPIO_STATE, EXPGPIO_BT_ON, 0u);
	usleep(50 * 1000);
	bton = (int)diag_mbox2(VC_PROP_SET_GPIO_STATE, EXPGPIO_BT_ON, 1u);
	usleep(250 * 1000);
	printf("BT_REG_ON(expgpio0/mbox128) set -> %d\n", bton);

	/* Route the BT-chip UART pins to the mini-UART (ALT5) and bring it up. */
	gpio_fsel(g, 30, GPIO_FN_ALT5); /* CTS1 */
	gpio_fsel(g, 31, GPIO_FN_ALT5); /* RTS1 */
	gpio_fsel(g, 32, GPIO_FN_ALT5); /* TXD1 */
	gpio_fsel(g, 33, GPIO_FN_ALT5); /* RXD1 */
	aux_init(a, baud_reg);
	printf("routing after:  30=%s 31=%s 32=%s 33=%s  AUX_ENABLES=0x%08x CNTL=0x%02x\n",
		fsel_name(gpio_getfsel(g, 30)), fsel_name(gpio_getfsel(g, 31)),
		fsel_name(gpio_getfsel(g, 32)), fsel_name(gpio_getfsel(g, 33)),
		*(volatile uint32_t *)(a + AUX_ENABLES),
		*(volatile uint32_t *)(a + AUX_MU_CNTL) & 0xffu);

	while (aux_getc(a, 5) >= 0) {
	}

	/* 1. HCI_RESET (0x0c03) -> Command Complete 04 0e 04 01 03 0c 00. */
	n = hci_cmd(a, 0x0c03u, NULL, 0u, resp, sizeof(resp));
	if (n < 0) {
		printf("HCI_RESET: TX blocked (mini-UART TX FIFO never empty?)\n");
	}
	else {
		hexdump("HCI_RESET reply", resp, n);
		if (n >= 7 && resp[0] == 0x04u && resp[1] == 0x0eu &&
			resp[3] == 0x03u && resp[4] == 0x0cu && resp[6] == 0x00u) {
			printf("  -> RESET OK: Command Complete, status=0 -- mini-UART<->BT controller ALIVE!\n");
		}
		else if (n > 0) {
			printf("  -> got %d bytes, not a clean RESET Command-Complete (baud? see hex)\n", n);
		}
		else {
			printf("  -> no response (controller silent)\n");
		}
	}

	/* 2. READ_LOCAL_VERSION_INFORMATION (0x1001). */
	n = hci_cmd(a, 0x1001u, NULL, 0u, resp, sizeof(resp));
	if (n > 0) {
		hexdump("READ_LOCAL_VERSION reply", resp, n);
		if (n >= 15 && resp[0] == 0x04u && resp[1] == 0x0eu) {
			unsigned hci_ver = resp[7];
			unsigned lmp_ver = resp[10];
			unsigned manuf = (unsigned)resp[11] | ((unsigned)resp[12] << 8);
			unsigned lmp_sub = (unsigned)resp[13] | ((unsigned)resp[14] << 8);
			printf("  -> HCI_ver=%u LMP_ver=%u manufacturer=0x%04x LMP_subver=0x%04x "
				"(0x000f=Broadcom; LMP_subver picks the .hcd)\n",
				hci_ver, lmp_ver, manuf, lmp_sub);
		}
	}
	else {
		printf("READ_LOCAL_VERSION: no response\n");
	}

	munmap(aux_page, _PAGE_SIZE);
	munmap(gpio_page, _PAGE_SIZE);
	printf(".\n");
	return 0;
}
