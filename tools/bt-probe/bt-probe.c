/*
 * bt-probe — Bluetooth Tier-0 bring-up probe for the BCM43455 combo chip on
 * the Raspberry Pi 4, under Phoenix-RTOS. Standalone one-shot userspace tool
 * (mirrors tools/wifi-probe): mmaps the PL011 UART0 (0xfe201000, reserved for
 * BT by `dtoverlay=miniuart-bt`) and the VideoCore mailbox (0xfe00b880), raises
 * BT_REG_ON (expgpio[0], mailbox GPIO 128), then speaks H4 HCI over the UART:
 *
 *   1. HCI_RESET (opcode 0x0c03) — the BT ROM answers this with NO firmware
 *      patch, so it validates the whole UART<->controller path.
 *   2. READ_LOCAL_VERSION_INFORMATION (opcode 0x1001) — tells us the LMP
 *      version + which .hcd patch to bundle later (BCM4345C0 vs C5).
 *
 * Console-safe: with miniuart-bt the serial console is on the mini-UART, so
 * driving PL011 here does not touch the debug console. All fw-facing bytes are
 * H4-framed: 0x01=command, 0x02=ACL, 0x04=event.
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

/* ---- VideoCore mailbox (property channel) — BT_REG_ON via SET_GPIO_STATE --- */
#define RPI_PI4_MAILBOX_BASE 0xfe00b880u
#define VC_MBOX_READ 0x00u
#define VC_MBOX_STATUS 0x18u
#define VC_MBOX_WRITE 0x20u
#define VC_MBOX_STATUS_FULL 0x80000000u
#define VC_MBOX_STATUS_EMPTY 0x40000000u
#define VC_MBOX_RESP_OK 0x80000000u
#define VC_MBOX_PROP_CHANNEL 8u
#define VC_PROP_SET_GPIO_STATE 0x00038041u
#define EXPGPIO_BT_ON 128u /* expgpio[0] = "BT_ON" per Pi 4 DT (WL_ON=129=expgpio[1]) */

static uint32_t diag_mboxPower(uint32_t tag, uint32_t device_id, uint32_t state)
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
	msg[5] = device_id;
	msg[6] = state;
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

/* ---- PL011 UART0 (0xfe201000) ------------------------------------------- */
#define PL011_BASE 0xfe201000u
#define PL_DR 0x00u
#define PL_FR 0x18u
#define PL_IBRD 0x24u
#define PL_FBRD 0x28u
#define PL_LCRH 0x2Cu
#define PL_CR 0x30u
#define PL_IMSC 0x38u
#define PL_ICR 0x44u
#define FR_BUSY (1u << 3)
#define FR_RXFE (1u << 4) /* RX FIFO empty */
#define FR_TXFF (1u << 5) /* TX FIFO full */

/* 115200 8N1 on the 48 MHz UART clock: IBRD=48e6/(16*115200)=26, FBRD=round(.04*64)=3.
 * HW flow control (RTSEN|CTSEN) as the BT UART expects; callers use timeouts so
 * a deasserted CTS (chip not ready) is reported, not hung. */
static void pl011_init(volatile uint8_t *u)
{
	*(volatile uint32_t *)(u + PL_CR) = 0u;            /* disable */
	*(volatile uint32_t *)(u + PL_ICR) = 0x7FFu;       /* clear all irqs */
	*(volatile uint32_t *)(u + PL_IBRD) = 26u;
	*(volatile uint32_t *)(u + PL_FBRD) = 3u;
	*(volatile uint32_t *)(u + PL_LCRH) = 0x70u;       /* 8 bits, FIFO enable */
	*(volatile uint32_t *)(u + PL_IMSC) = 0u;
	*(volatile uint32_t *)(u + PL_CR) =
		(1u << 0) | (1u << 8) | (1u << 9) | (1u << 14) | (1u << 15); /* UARTEN|TXE|RXE|RTSEN|CTSEN */
}

/* Send one byte; returns 0 on success, -1 if TX FIFO stayed full (CTS never
 * asserted => controller not ready/absent). */
static int pl011_putc(volatile uint8_t *u, uint8_t c)
{
	int d;
	for (d = 0; d < 2000000; ++d) {
		if ((*(volatile uint32_t *)(u + PL_FR) & FR_TXFF) == 0u) {
			*(volatile uint32_t *)(u + PL_DR) = c;
			return 0;
		}
	}
	return -1;
}

/* Read one byte within ~timeout_ms; returns the byte (0..255) or -1 on timeout. */
static int pl011_getc(volatile uint8_t *u, int timeout_ms)
{
	int t;
	for (t = 0; t < timeout_ms * 20; ++t) {
		if ((*(volatile uint32_t *)(u + PL_FR) & FR_RXFE) == 0u) {
			return (int)(*(volatile uint32_t *)(u + PL_DR) & 0xffu);
		}
		usleep(50);
	}
	return -1;
}

/* Send an H4 HCI command (0x01 + opcode LE16 + plen + params) and collect the
 * reply bytes into `resp` (up to cap) with an inter-byte idle timeout. Returns
 * the number of bytes received (>=0), or -1 if the command could not be sent. */
static int hci_cmd(volatile uint8_t *u, uint16_t opcode, const uint8_t *params,
	uint8_t plen, uint8_t *resp, int cap)
{
	int i, b, n = 0;

	if (pl011_putc(u, 0x01u) != 0) {
		return -1; /* TX blocked (CTS low) */
	}
	(void)pl011_putc(u, (uint8_t)(opcode & 0xffu));
	(void)pl011_putc(u, (uint8_t)((opcode >> 8) & 0xffu));
	(void)pl011_putc(u, plen);
	for (i = 0; i < (int)plen; ++i) {
		(void)pl011_putc(u, params[i]);
	}
	/* First byte can take a while (controller processing); later bytes are
	 * back-to-back — use a generous first timeout then a short inter-byte one. */
	b = pl011_getc(u, 500);
	while (b >= 0 && n < cap) {
		resp[n++] = (uint8_t)b;
		b = pl011_getc(u, 40);
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
	void *pl_page;
	volatile uint8_t *u;
	uint8_t resp[64];
	int n, bton;

	printf("PHX-BT/0 tier0-hci-probe\n");

	/* Raise BT_REG_ON (expgpio[0]) via the VideoCore mailbox: power-cycle it
	 * like the WiFi WL_REG_ON, then let the BT ROM boot. */
	(void)diag_mboxPower(VC_PROP_SET_GPIO_STATE, EXPGPIO_BT_ON, 0u);
	usleep(50 * 1000);
	bton = (int)diag_mboxPower(VC_PROP_SET_GPIO_STATE, EXPGPIO_BT_ON, 1u);
	usleep(250 * 1000); /* BT ROM boot */
	printf("BT_REG_ON(expgpio0/mbox128) set -> %d\n", bton);

	pl_page = mmap(NULL, _PAGE_SIZE, PROT_READ | PROT_WRITE,
		MAP_DEVICE | MAP_UNCACHED | MAP_PHYSMEM | MAP_ANONYMOUS, -1, PL011_BASE);
	if (pl_page == MAP_FAILED) {
		printf("error: mmap PL011 failed\n.\n");
		return 1;
	}
	u = (volatile uint8_t *)pl_page;
	pl011_init(u);

	/* Drain any stale RX bytes. */
	while (pl011_getc(u, 5) >= 0) {
	}

	/* 1. HCI_RESET (0x0c03), no params -> Command Complete (04 0e 04 01 03 0c 00). */
	n = hci_cmd(u, 0x0c03u, NULL, 0u, resp, sizeof(resp));
	if (n < 0) {
		printf("HCI_RESET: TX BLOCKED (CTS not asserted -- controller not ready/powered?)\n");
	}
	else {
		hexdump("HCI_RESET reply", resp, n);
		if (n >= 7 && resp[0] == 0x04u && resp[1] == 0x0eu &&
			resp[3] == 0x03u && resp[4] == 0x0cu && resp[6] == 0x00u) {
			printf("  -> RESET OK: Command Complete, status=0 -- UART<->BT controller ALIVE!\n");
		}
		else if (n > 0) {
			printf("  -> got %d bytes but not a clean RESET Command-Complete (see hex)\n", n);
		}
		else {
			printf("  -> no response (controller silent)\n");
		}
	}

	/* 2. READ_LOCAL_VERSION_INFORMATION (0x1001). Reply payload after the 6-byte
	 * event/CmdComplete header: status, hci_ver, hci_rev(2), lmp_ver,
	 * manuf(2), lmp_subver(2). */
	n = hci_cmd(u, 0x1001u, NULL, 0u, resp, sizeof(resp));
	if (n > 0) {
		hexdump("READ_LOCAL_VERSION reply", resp, n);
		if (n >= 15 && resp[0] == 0x04u && resp[1] == 0x0eu) {
			unsigned hci_ver = resp[7];
			unsigned lmp_ver = resp[10];
			unsigned manuf = (unsigned)resp[11] | ((unsigned)resp[12] << 8);
			unsigned lmp_sub = (unsigned)resp[13] | ((unsigned)resp[14] << 8);
			printf("  -> HCI_ver=%u LMP_ver=%u manufacturer=0x%04x LMP_subver=0x%04x "
				"(manuf 0x000f=Broadcom; LMP_subver picks the .hcd)\n",
				hci_ver, lmp_ver, manuf, lmp_sub);
		}
	}
	else {
		printf("READ_LOCAL_VERSION: no response\n");
	}

	munmap(pl_page, _PAGE_SIZE);
	printf(".\n");
	return 0;
}
