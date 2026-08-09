/*
 * Phoenix-RTOS --- Raspberry Pi 4 WiFi (BCM43455 SDIO) bring-up probe
 *
 * Standalone userspace probe that reproduces the 2026-06-04 WiFi
 * firmware-download + ARM-CR4-release baseline on the Pi 4's BCM43455
 * SDIO chip, then reports whether the firmware came alive.
 *
 * PROVENANCE
 * ----------
 * The SDIO/SDHCI/GPIO/mailbox helpers and the firmware-release sequence
 * below were extracted VERBATIM from the lwip-port diagnostic UDP
 * responder (`port/diag-udp.c`) as it existed at commit a078a5c — the
 * last commit before the whole live WiFi bring-up path was deleted in
 * f0973b5. The original ran this sequence from a UDP 'G' command handler
 * (`diag_format_sdio_fwrelease`), which had to run inside the lwip-port
 * process because post-fbcon Pi 4 boots did not capture userspace stdout
 * over the pl011 UART. That coupling — a second owner of the UART/xHCI
 * path sharing the lwip process — is exactly what motivated the removal.
 *
 * This probe drops the UDP responder entirely and instead runs the
 * bring-up ONCE from `main()`, printing the identical telemetry to
 * stdout. It has NO lwip dependency: the WiFi path only ever used
 * mmap()/va2pa()/usleep()/snprintf() plus the two firmware C-arrays, so
 * it extracts cleanly into a self-contained binary. Run it from the psh
 * prompt on the Pi and read the report over the console.
 *
 * NB: the original 'G' reply was capped at one UDP datagram (1472 B),
 * which truncated the later telemetry lines. This probe uses a large
 * heap buffer, so its output is a SUPERSET of the old 'G' — same values,
 * nothing truncated.
 *
 * MMIO / GPIO TOUCHED (all via userspace mmap of physical pages, the
 * same MAP_PHYSMEM|MAP_DEVICE|MAP_UNCACHED pattern the ported
 * thermal/hwrng/vcmbox drivers use):
 *   - SDHCI (Arasan) @ 0xfe300000     — the controller the 43455 sits on
 *   - BCM2711 GPIO   @ 0xfe200000     — routes GPIO 34..39 to ALT3 (SDIO)
 *   - VideoCore mbox @ 0xfe00b880     — SET_GPIO_STATE(WL_ON) power cycle
 *
 * Copyright 2026 Phoenix Systems
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "wifi-fw-43455.h"
#include "wifi-nvram-43455.h"
#include "cr4tiny_blob.h"

#include <sys/mman.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/* ------------------------------------------------------------------ */
/* BCM2711 GPIO block (function-select for the SDIO alt-function). */

#define BCM2711_GPIO_BASE   0xfe200000u
#define GPIO_GPFSEL0        0x00u   /* +4*n for GPFSEL1..5 */


/* Set pin function-select (3 bits). pin: 0..53, fn: 0..7. Read-
 * modify-write of GPFSEL(pin/10). Routes GPIO 34..39 to ALT3 for SDIO. */
static void diag_gpioSetFsel(volatile uint8_t *base, unsigned pin, unsigned fn)
{
	unsigned bank = pin / 10u;
	unsigned shift = (pin % 10u) * 3u;
	volatile uint32_t *reg = (volatile uint32_t *)(base + GPIO_GPFSEL0 + bank * 4u);
	uint32_t v = *reg;
	v &= ~(0x7u << shift);
	v |= ((fn & 0x7u) << shift);
	*reg = v;
}


/* ------------------------------------------------------------------ */
/* VideoCore mailbox (property channel). Used only for the WL_ON expander
 * GPIO power cycle. Pi 4 mailbox base hardcoded (the port has no
 * board_config.h include path). */

#define RPI_PI4_MAILBOX_BASE  0xfe00b880u

#define VC_MBOX_READ          0x00u
#define VC_MBOX_STATUS        0x18u
#define VC_MBOX_WRITE         0x20u
#define VC_MBOX_STATUS_FULL   0x80000000u
#define VC_MBOX_STATUS_EMPTY  0x40000000u
#define VC_MBOX_RESP_OK       0x80000000u
#define VC_MBOX_PROP_CHANNEL  8u

#define VC_PROP_SET_GPIO_STATE  0x00038041u

#define EXPGPIO_WL_ON           129u  /* expgpio[1] = "WL_ON" per Pi 4 DT */


/* Get / set VideoCore device power state (here: an expander GPIO via
 * SET_GPIO_STATE). Returns the resulting state on success, 0xFFFFFFFF on
 * failure. */
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
		MAP_DEVICE | MAP_UNCACHED | MAP_PHYSMEM | MAP_ANONYMOUS,
		-1, pa_base);
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

	/* GET takes (device_id) and returns (device_id, state).
	 * SET takes (device_id, state) and returns (device_id, state). */
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
		result = msg[6];  /* returned state */
	}

	munmap(msg_page, _PAGE_SIZE);
	munmap(mbox_page, _PAGE_SIZE);
	return result;
}


/* Cold-power-cycle the BCM43455 WiFi chip via its WL_REG_ON line (a Pi 4
 * expander GPIO driven through the VideoCore mailbox): drop it, wait,
 * re-assert, settle. NB: a 20x-longer power-down was tested and did NOT
 * make the 43455 firmware execute (the fw-exec gate is not a reset-timing
 * issue); 50/150 ms is the established, enumeration-tested baseline. */
static void diag_wifiPowerCycle(void)
{
	(void)diag_mboxPower(VC_PROP_SET_GPIO_STATE, EXPGPIO_WL_ON, 0u);
	usleep(50 * 1000);
	(void)diag_mboxPower(VC_PROP_SET_GPIO_STATE, EXPGPIO_WL_ON, 1u);
	usleep(150 * 1000);
}


/* ------------------------------------------------------------------ */
/* SDHCI 3.0 controller (Arasan @ 0xfe300000). Register offsets and
 * command/response encodings per the SD Host Controller Simplified
 * Specification 3.0. */

#define SDHCI_ARGUMENT_1   0x08u
#define SDHCI_TRANS_CMD    0x0Cu
#define SDHCI_RESPONSE_0   0x10u
#define SDHCI_PRES_STATE   0x24u
#define SDHCI_INT_STATUS   0x30u

#define SDHCI_PRES_CMD_INHIBIT  0x00000001u
#define SDHCI_INT_CMD_COMPLETE  0x00000001u
#define SDHCI_INT_ERR_ANY       0x00008000u  /* ERR_INT bits live in the upper 16 */

/* SOFT_RESET_* live in bits 24..26 of the 32-bit dword at offset 0x2C
 * (CLOCK_CTL + TIMEOUT_CTL + SOFT_RESET). Write 1 to start the reset;
 * the bit clears when done. */
#define SDHCI_CLK_TIMEOUT_RESET 0x2Cu
#define SDHCI_SOFT_RESET_ALL    (1u << 24)
#define SDHCI_SOFT_RESET_CMD    (1u << 25)
#define SDHCI_SOFT_RESET_DAT    (1u << 26)

/* Command-register RESPONSE_TYPE + check-bit encodings (bits 0..5 of the
 * COMMAND half of the TRANS_CMD dword):
 *   R0  (no resp)  = 0x00
 *   R1             = 0x1a  (resp=2, CRC, index)
 *   R1b            = 0x1b
 *   R3  (CMD41)    = 0x02  (resp=2, no CRC, no index)
 *   R4  (CMD5)     = 0x02
 *   R5  (CMD52,53) = 0x1a
 *   R6  (CMD3)     = 0x1a */
#define SDHCI_RESP_R0   0x00u
#define SDHCI_RESP_R1   0x1au
#define SDHCI_RESP_R1b  0x1bu
#define SDHCI_RESP_R3   0x02u
#define SDHCI_RESP_R4   0x02u
#define SDHCI_RESP_R5   0x1au
#define SDHCI_RESP_R6   0x1au

#define SDHCI_BLOCK_SIZE_CNT  0x04u  /* BLOCK_SIZE (low 16) + BLOCK_COUNT (high 16) */
#define SDHCI_DATA_PORT       0x20u  /* PIO FIFO */
#define SDHCI_INT_XFER_COMPLETE  0x00000002u
#define SDHCI_INT_BUF_RD_READY   0x00000020u
#define SDHCI_INT_BUF_WR_READY   0x00000010u


/* Program SDHCI to a target SD-bus clock by dividing the 250 MHz base.
 * Per SDHCI 3.0 §2.2.13: divisor is 10-bit, output_hz = base / (2*N). */
static int diag_sdhciSetClockKHz(volatile uint8_t *base, unsigned target_khz)
{
	uint32_t base_hz = 250000000u;
	uint32_t target_hz = (uint32_t)target_khz * 1000u;
	uint32_t divisor;
	uint32_t clkctl;
	uint32_t i;

	if (target_hz == 0u || target_hz > base_hz) {
		return -1;
	}
	divisor = (base_hz + (2u * target_hz) - 1u) / (2u * target_hz);
	if (divisor > 0x3FFu) {
		divisor = 0x3FFu;
	}

	/* Disable SD clock first. RMW the low 16 (CLOCK_CTL) only. */
	clkctl = *(volatile uint32_t *)(base + SDHCI_CLK_TIMEOUT_RESET);
	clkctl &= 0xFFFF0000u;
	*(volatile uint32_t *)(base + SDHCI_CLK_TIMEOUT_RESET) = clkctl;

	/* Build new CLOCK_CTL: INTERNAL_CLOCK_EN=1, SD_CLOCK_EN=0 for now,
	 * divisor high bits [9:8] at [7:6], low bits [7:0] at [15:8]. */
	{
		uint16_t cctl = (uint16_t)(
			(uint16_t)(divisor & 0xFFu) << 8 |
			(uint16_t)((divisor >> 8) & 0x3u) << 6 |
			(1u << 0));
		uint32_t hi = *(volatile uint32_t *)(base + SDHCI_CLK_TIMEOUT_RESET) &
			0xFFFF0000u;
		*(volatile uint32_t *)(base + SDHCI_CLK_TIMEOUT_RESET) =
			hi | (uint32_t)cctl;
	}

	/* Wait for INTERNAL_CLOCK_STABLE (bit 1). */
	for (i = 0; i < 100000u; ++i) {
		uint32_t v = *(volatile uint32_t *)(base + SDHCI_CLK_TIMEOUT_RESET);
		if ((v & (1u << 1)) != 0u) {
			break;
		}
	}
	if (i == 100000u) {
		return -2;
	}

	/* Enable SD_CLOCK (bit 2). */
	{
		uint32_t v = *(volatile uint32_t *)(base + SDHCI_CLK_TIMEOUT_RESET);
		v |= (1u << 2);
		*(volatile uint32_t *)(base + SDHCI_CLK_TIMEOUT_RESET) = v;
	}

	return 0;
}


/* Soft-reset the CMD and DAT lines without disturbing CLOCK_CTL /
 * TIMEOUT_CTL (which firmware has already set up). 32-bit RMW. */
static int diag_sdhciResetCmdDat(volatile uint8_t *base)
{
	uint32_t orig = *(volatile uint32_t *)(base + SDHCI_CLK_TIMEOUT_RESET);
	uint32_t deadline = 100000u;
	uint32_t i;

	*(volatile uint32_t *)(base + SDHCI_CLK_TIMEOUT_RESET) =
		(orig & 0x00FFFFFFu) | SDHCI_SOFT_RESET_CMD | SDHCI_SOFT_RESET_DAT;

	for (i = 0; i < deadline; ++i) {
		uint32_t v = *(volatile uint32_t *)(base + SDHCI_CLK_TIMEOUT_RESET);
		if ((v & (SDHCI_SOFT_RESET_CMD | SDHCI_SOFT_RESET_DAT)) == 0u) {
			return 0;
		}
	}
	return -1;
}


/* Issue an SDHCI command. Returns 0 on success, negative on error. On
 * success, response_out[0..3] is filled from RESPONSE_0..3 (caller must
 * allocate a 4-element array). */
static int diag_sdhciCmd(volatile uint8_t *base, uint8_t cmd_index,
	uint32_t arg, uint16_t resp_type, uint32_t response_out[4])
{
	uint32_t deadline = 100000u;
	uint32_t i;

	/* Clear stale INT_STATUS bits (W1C). */
	*(volatile uint32_t *)(base + SDHCI_INT_STATUS) = 0xFFFFFFFFu;

	/* Wait for CMD_INHIBIT clear. */
	for (i = 0; i < deadline; ++i) {
		if ((*(volatile uint32_t *)(base + SDHCI_PRES_STATE) &
				SDHCI_PRES_CMD_INHIBIT) == 0u) {
			break;
		}
	}
	if (i == deadline) {
		return -1;  /* CMD_INHIBIT stuck */
	}

	/* Program ARGUMENT then COMMAND. 32-bit write to TRANS_CMD (offset
	 * 0x0C): low 16 = TRANSFER_MODE = 0 (no data), high 16 = COMMAND.
	 * The Arasan controller requires the combined 32-bit write. COMMAND
	 * layout in the upper dword: CMD_NUMBER at 31:24, RESPONSE_TYPE +
	 * check bits at 21:16. */
	*(volatile uint32_t *)(base + SDHCI_ARGUMENT_1) = arg;
	{
		uint32_t cmd_word =
			((uint32_t)resp_type << 16) |
			((uint32_t)cmd_index << 24);
		*(volatile uint32_t *)(base + SDHCI_TRANS_CMD) = cmd_word;
	}

	/* Wait for CMD_COMPLETE (or any error bit). */
	for (i = 0; i < deadline; ++i) {
		uint32_t st = *(volatile uint32_t *)(base + SDHCI_INT_STATUS);
		if ((st & SDHCI_INT_ERR_ANY) != 0u) {
			return -2;  /* error reported */
		}
		if ((st & SDHCI_INT_CMD_COMPLETE) != 0u) {
			break;
		}
	}
	if (i == deadline) {
		return -3;  /* cmd_complete didn't assert */
	}

	if (response_out != NULL) {
		response_out[0] = *(volatile uint32_t *)(base + SDHCI_RESPONSE_0 + 0x0);
		response_out[1] = *(volatile uint32_t *)(base + SDHCI_RESPONSE_0 + 0x4);
		response_out[2] = *(volatile uint32_t *)(base + SDHCI_RESPONSE_0 + 0x8);
		response_out[3] = *(volatile uint32_t *)(base + SDHCI_RESPONSE_0 + 0xC);
	}

	/* W1C the CMD_COMPLETE bit. */
	*(volatile uint32_t *)(base + SDHCI_INT_STATUS) = SDHCI_INT_CMD_COMPLETE;

	return 0;
}


/* CMD52 (IO_RW_DIRECT). arg layout: bit31 R/W, bits30:28 FN, bits25:9
 * 17-bit REG, bits7:0 DATA. resp_out must be a 4-element uint32_t array
 * (diag_sdhciCmd unconditionally dumps all four response slots). */
static int diag_sdioCmd52(volatile uint8_t *sdhci, int write, int fn,
	uint32_t reg, uint8_t data, uint32_t *resp_out)
{
	uint32_t arg = 0;

	arg |= (write ? 1u : 0u) << 31;
	arg |= ((uint32_t)fn & 7u) << 28;
	arg |= ((uint32_t)reg & 0x1ffffu) << 9;
	if (write) {
		arg |= (uint32_t)data;
	}
	return diag_sdhciCmd(sdhci, 52u, arg, SDHCI_RESP_R5, resp_out);
}


/* Switch SDIO to High-Speed (25 MHz) on a 4-bit data bus. Call after
 * CMD0/5/3/7 + F1 enable + IORDY. Sequence per BCM43455c0 / SDIO 2.0:
 * CCCR 0x13 SHS check + EHS set, CCCR 0x07 4-bit width, SDHCI HCTL1
 * 4BIT+HIGH_SPEED, reprogram clock to 25 MHz. */
static int diag_sdioGoHighSpeed(volatile uint8_t *sdhci)
{
	uint32_t hs_resp[4] = {0};
	uint32_t bic_resp[4] = {0};
	int rc;

	rc = diag_sdioCmd52(sdhci, 0, 0, 0x13u, 0u, hs_resp);
	if (rc != 0) {
		return -1;
	}
	if ((hs_resp[0] & 0x01u) == 0u) {
		return -2;  /* SHS not set */
	}

	rc = diag_sdioCmd52(sdhci, 1, 0, 0x13u,
		(uint8_t)((hs_resp[0] | 0x02u) & 0xffu), NULL);
	if (rc != 0) {
		return -3;
	}

	rc = diag_sdioCmd52(sdhci, 0, 0, 0x07u, 0u, bic_resp);
	if (rc != 0) {
		return -4;
	}
	rc = diag_sdioCmd52(sdhci, 1, 0, 0x07u,
		(uint8_t)((bic_resp[0] & 0xFCu) | 0x02u), NULL);
	if (rc != 0) {
		return -5;
	}

	{
		uint32_t hctl = *(volatile uint32_t *)(sdhci + 0x28u);
		hctl &= 0xFFFFFF00u;
		hctl |= (1u << 1) | (1u << 2);
		*(volatile uint32_t *)(sdhci + 0x28u) = hctl;
	}

	rc = diag_sdhciSetClockKHz(sdhci, 25000u);
	if (rc != 0) {
		return -6;
	}
	return 0;
}


/* CMD53 (IO_RW_EXTENDED) block-mode READ via SDHCI PIO. buf must point
 * to a 4-byte-aligned destination of at least block_count*block_size
 * bytes. */
static int diag_sdioCmd53Read(volatile uint8_t *sdhci, int fn,
	int incr_addr, uint32_t reg_addr,
	uint32_t block_count, uint32_t block_size,
	uint8_t *buf)
{
	uint32_t arg, cmd_word;
	uint32_t st;
	uint32_t bytes_total = block_count * block_size;
	uint32_t words_total = bytes_total / 4u;
	uint32_t block_words = block_size / 4u;
	uint32_t bytes_in_block = 0;
	uint32_t i;
	int deadline;

	/* Wait for CMD line idle. */
	for (deadline = 100000; deadline > 0; --deadline) {
		if ((*(volatile uint32_t *)(sdhci + SDHCI_PRES_STATE) &
			SDHCI_PRES_CMD_INHIBIT) == 0u) {
			break;
		}
	}
	if (deadline == 0) {
		return -1;
	}

	*(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS) = 0xFFFFFFFFu;

	*(volatile uint32_t *)(sdhci + SDHCI_BLOCK_SIZE_CNT) =
		(block_count << 16) | (block_size & 0xFFFu);

	arg = (0u << 31) |
		((uint32_t)(fn & 7u) << 28) |
		(1u << 27) |  /* block_mode */
		((incr_addr ? 1u : 0u) << 26) |
		((reg_addr & 0x1FFFFu) << 9) |
		(block_count & 0x1FFu);
	*(volatile uint32_t *)(sdhci + SDHCI_ARGUMENT_1) = arg;

	/* TRANSFER_MODE + COMMAND dword at 0x0C: BLOCK_COUNT_EN, DAT_XFER_DIR
	 * = read, MULTI_BLK if >1, R5 resp + CRC/index, DATA_PRESENT, CMD53. */
	cmd_word =
		(1u << 1) |
		(1u << 4) |
		((block_count > 1u ? 1u : 0u) << 5) |
		((uint32_t)0x3Au << 16) |
		((uint32_t)53u << 24);
	*(volatile uint32_t *)(sdhci + SDHCI_TRANS_CMD) = cmd_word;

	for (deadline = 100000; deadline > 0; --deadline) {
		st = *(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS);
		if ((st & SDHCI_INT_ERR_ANY) != 0u) {
			return -2;
		}
		if ((st & SDHCI_INT_CMD_COMPLETE) != 0u) {
			break;
		}
	}
	if (deadline == 0) {
		return -3;
	}
	*(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS) = SDHCI_INT_CMD_COMPLETE;

	/* PIO read loop: drain DATA_PORT one word at a time; clear
	 * BUFFER_READ_READY after each block-worth. */
	for (i = 0; i < words_total; ++i) {
		for (deadline = 100000; deadline > 0; --deadline) {
			st = *(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS);
			if ((st & SDHCI_INT_ERR_ANY) != 0u) {
				return -4;
			}
			if ((st & SDHCI_INT_BUF_RD_READY) != 0u) {
				break;
			}
		}
		if (deadline == 0) {
			return -5;
		}

		{
			uint32_t data = *(volatile uint32_t *)(sdhci + SDHCI_DATA_PORT);
			if (buf != NULL) {
				buf[i * 4 + 0] = (uint8_t)(data & 0xffu);
				buf[i * 4 + 1] = (uint8_t)((data >> 8) & 0xffu);
				buf[i * 4 + 2] = (uint8_t)((data >> 16) & 0xffu);
				buf[i * 4 + 3] = (uint8_t)((data >> 24) & 0xffu);
			}
		}

		bytes_in_block += 4u;
		if (bytes_in_block >= block_size) {
			bytes_in_block = 0u;
			*(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS) = SDHCI_INT_BUF_RD_READY;
		}
		(void)block_words;
	}

	for (deadline = 100000; deadline > 0; --deadline) {
		st = *(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS);
		if ((st & SDHCI_INT_ERR_ANY) != 0u) {
			return -6;
		}
		if ((st & SDHCI_INT_XFER_COMPLETE) != 0u) {
			break;
		}
	}
	if (deadline == 0) {
		return -7;
	}
	*(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS) = 0xFFFFFFFFu;
	return 0;
}


/* CMD53 (IO_RW_EXTENDED) BYTE-mode transfers: a single transaction of `nbytes`
 * (<=512), no SDIO block-count. brcmf/MMC use byte mode for sub-block control
 * frames (a block-mode CMD53 whose size mismatches the function's configured
 * block size stalls the data phase). Differs from the block helpers only in:
 * arg bit27(block_mode)=0 + byte count in arg[8:0]; TRANSFER_MODE has no
 * BLOCK_COUNT_EN / MULTI_BLK. nbytes is rounded up to 4 for the PIO word loop. */
static int diag_sdioCmd53ReadByteMode(volatile uint8_t *sdhci, int fn,
	int incr_addr, uint32_t reg_addr, uint32_t nbytes, uint8_t *buf)
{
	uint32_t arg, cmd_word, st, data;
	uint32_t words_total = (nbytes + 3u) / 4u;
	uint32_t i;
	int deadline;

	for (deadline = 100000; deadline > 0; --deadline) {
		if ((*(volatile uint32_t *)(sdhci + SDHCI_PRES_STATE) & SDHCI_PRES_CMD_INHIBIT) == 0u) {
			break;
		}
	}
	if (deadline == 0) {
		return -1;
	}
	*(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS) = 0xFFFFFFFFu;
	*(volatile uint32_t *)(sdhci + SDHCI_BLOCK_SIZE_CNT) = (1u << 16) | (nbytes & 0xFFFu);
	arg = (0u << 31) | ((uint32_t)(fn & 7u) << 28) | /* block_mode bit27 = 0 */
		((incr_addr ? 1u : 0u) << 26) |
		((reg_addr & 0x1FFFFu) << 9) | (nbytes & 0x1FFu);
	*(volatile uint32_t *)(sdhci + SDHCI_ARGUMENT_1) = arg;
	cmd_word = (1u << 4) | ((uint32_t)0x3Au << 16) | ((uint32_t)53u << 24); /* read dir, no BLK_CNT_EN */
	*(volatile uint32_t *)(sdhci + SDHCI_TRANS_CMD) = cmd_word;

	for (deadline = 100000; deadline > 0; --deadline) {
		st = *(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS);
		if ((st & SDHCI_INT_ERR_ANY) != 0u) {
			return -2;
		}
		if ((st & SDHCI_INT_CMD_COMPLETE) != 0u) {
			break;
		}
	}
	if (deadline == 0) {
		return -3;
	}
	*(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS) = SDHCI_INT_CMD_COMPLETE;

	for (deadline = 100000; deadline > 0; --deadline) {
		st = *(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS);
		if ((st & SDHCI_INT_ERR_ANY) != 0u) {
			return -4;
		}
		if ((st & SDHCI_INT_BUF_RD_READY) != 0u) {
			break;
		}
	}
	if (deadline == 0) {
		return -5;
	}
	for (i = 0; i < words_total; ++i) {
		data = *(volatile uint32_t *)(sdhci + SDHCI_DATA_PORT);
		if (buf != NULL) {
			buf[i * 4 + 0] = (uint8_t)(data & 0xffu);
			buf[i * 4 + 1] = (uint8_t)((data >> 8) & 0xffu);
			buf[i * 4 + 2] = (uint8_t)((data >> 16) & 0xffu);
			buf[i * 4 + 3] = (uint8_t)((data >> 24) & 0xffu);
		}
	}
	for (deadline = 100000; deadline > 0; --deadline) {
		st = *(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS);
		if ((st & SDHCI_INT_ERR_ANY) != 0u) {
			return -6;
		}
		if ((st & SDHCI_INT_XFER_COMPLETE) != 0u) {
			break;
		}
	}
	if (deadline == 0) {
		return -7;
	}
	*(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS) = 0xFFFFFFFFu;
	return 0;
}

static int diag_sdioCmd53WriteByteMode(volatile uint8_t *sdhci, int fn,
	int incr_addr, uint32_t reg_addr, uint32_t nbytes, const uint8_t *buf)
{
	uint32_t arg, cmd_word, st, data;
	uint32_t words_total = (nbytes + 3u) / 4u;
	uint32_t i;
	int deadline;

	for (deadline = 100000; deadline > 0; --deadline) {
		if ((*(volatile uint32_t *)(sdhci + SDHCI_PRES_STATE) & SDHCI_PRES_CMD_INHIBIT) == 0u) {
			break;
		}
	}
	if (deadline == 0) {
		return -1;
	}
	*(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS) = 0xFFFFFFFFu;
	*(volatile uint32_t *)(sdhci + SDHCI_BLOCK_SIZE_CNT) = (1u << 16) | (nbytes & 0xFFFu);
	arg = (1u << 31) | ((uint32_t)(fn & 7u) << 28) | /* write; block_mode bit27 = 0 */
		((incr_addr ? 1u : 0u) << 26) |
		((reg_addr & 0x1FFFFu) << 9) | (nbytes & 0x1FFu);
	*(volatile uint32_t *)(sdhci + SDHCI_ARGUMENT_1) = arg;
	cmd_word = ((uint32_t)0x3Au << 16) | ((uint32_t)53u << 24); /* write dir, no BLK_CNT_EN */
	*(volatile uint32_t *)(sdhci + SDHCI_TRANS_CMD) = cmd_word;

	for (deadline = 100000; deadline > 0; --deadline) {
		st = *(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS);
		if ((st & SDHCI_INT_ERR_ANY) != 0u) {
			return -2;
		}
		if ((st & SDHCI_INT_CMD_COMPLETE) != 0u) {
			break;
		}
	}
	if (deadline == 0) {
		return -3;
	}
	*(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS) = SDHCI_INT_CMD_COMPLETE;

	for (deadline = 100000; deadline > 0; --deadline) {
		st = *(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS);
		if ((st & SDHCI_INT_ERR_ANY) != 0u) {
			return -4;
		}
		if ((st & SDHCI_INT_BUF_WR_READY) != 0u) {
			break;
		}
	}
	if (deadline == 0) {
		return -5;
	}
	for (i = 0; i < words_total; ++i) {
		data = (uint32_t)buf[i * 4 + 0] | ((uint32_t)buf[i * 4 + 1] << 8) |
			((uint32_t)buf[i * 4 + 2] << 16) | ((uint32_t)buf[i * 4 + 3] << 24);
		*(volatile uint32_t *)(sdhci + SDHCI_DATA_PORT) = data;
	}
	for (deadline = 100000; deadline > 0; --deadline) {
		st = *(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS);
		if ((st & SDHCI_INT_ERR_ANY) != 0u) {
			return -6;
		}
		if ((st & SDHCI_INT_XFER_COMPLETE) != 0u) {
			break;
		}
	}
	if (deadline == 0) {
		return -7;
	}
	*(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS) = 0xFFFFFFFFu;
	return 0;
}

/* CMD53 (IO_RW_EXTENDED) block-mode WRITE via SDHCI PIO. Mirror of the
 * read: arg bit31 = 1, TRANSFER_MODE bit4 = 0, polls BUFFER_WRITE_READY,
 * writes DATA_PORT. Source is a little-endian byte buffer of at least
 * block_count*block_size bytes. */
static int diag_sdioCmd53Write(volatile uint8_t *sdhci, int fn,
	int incr_addr, uint32_t reg_addr,
	uint32_t block_count, uint32_t block_size,
	const uint8_t *buf)
{
	uint32_t arg, cmd_word;
	uint32_t st;
	uint32_t bytes_total = block_count * block_size;
	uint32_t words_total = bytes_total / 4u;
	uint32_t bytes_in_block = 0;
	uint32_t i;
	int deadline;

	for (deadline = 100000; deadline > 0; --deadline) {
		if ((*(volatile uint32_t *)(sdhci + SDHCI_PRES_STATE) &
			SDHCI_PRES_CMD_INHIBIT) == 0u) {
			break;
		}
	}
	if (deadline == 0) {
		return -1;
	}

	*(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS) = 0xFFFFFFFFu;
	*(volatile uint32_t *)(sdhci + SDHCI_BLOCK_SIZE_CNT) =
		(block_count << 16) | (block_size & 0xFFFu);

	arg = (1u << 31) |
		((uint32_t)(fn & 7u) << 28) |
		(1u << 27) |
		((incr_addr ? 1u : 0u) << 26) |
		((reg_addr & 0x1FFFFu) << 9) |
		(block_count & 0x1FFu);
	*(volatile uint32_t *)(sdhci + SDHCI_ARGUMENT_1) = arg;

	cmd_word =
		(1u << 1) |
		((block_count > 1u ? 1u : 0u) << 5) |
		((uint32_t)0x3Au << 16) |
		((uint32_t)53u << 24);
	*(volatile uint32_t *)(sdhci + SDHCI_TRANS_CMD) = cmd_word;

	for (deadline = 100000; deadline > 0; --deadline) {
		st = *(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS);
		if ((st & SDHCI_INT_ERR_ANY) != 0u) {
			return -2;
		}
		if ((st & SDHCI_INT_CMD_COMPLETE) != 0u) {
			break;
		}
	}
	if (deadline == 0) {
		return -3;
	}
	*(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS) = SDHCI_INT_CMD_COMPLETE;

	for (i = 0; i < words_total; ++i) {
		for (deadline = 100000; deadline > 0; --deadline) {
			st = *(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS);
			if ((st & SDHCI_INT_ERR_ANY) != 0u) {
				return -4;
			}
			if ((st & SDHCI_INT_BUF_WR_READY) != 0u) {
				break;
			}
		}
		if (deadline == 0) {
			return -5;
		}

		{
			uint32_t data = (uint32_t)buf[i * 4 + 0] |
				((uint32_t)buf[i * 4 + 1] << 8) |
				((uint32_t)buf[i * 4 + 2] << 16) |
				((uint32_t)buf[i * 4 + 3] << 24);
			*(volatile uint32_t *)(sdhci + SDHCI_DATA_PORT) = data;
		}

		bytes_in_block += 4u;
		if (bytes_in_block >= block_size) {
			bytes_in_block = 0u;
			*(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS) = SDHCI_INT_BUF_WR_READY;
		}
	}

	for (deadline = 100000; deadline > 0; --deadline) {
		st = *(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS);
		if ((st & SDHCI_INT_ERR_ANY) != 0u) {
			return -6;
		}
		if ((st & SDHCI_INT_XFER_COMPLETE) != 0u) {
			break;
		}
	}
	if (deadline == 0) {
		return -7;
	}
	*(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS) = 0xFFFFFFFFu;
	return 0;
}


/* ------------------------------------------------------------------ */
/* ---- #91 EROM (DMP) walk -------------------------------------------------
 * Replicates brcmfmac's brcmf_chip_dmp_erom_scan (external/linux .../chip.c)
 * to enumerate the chip's cores over the SDIO backplane, replacing the probe's
 * remaining HARDCODED core-address hypotheses (CR4 wrapper 0x18102000, SDIOD
 * mailbox 0x18005000, ram-top 0x238000) with the chip's own EROM answers.
 * Read-only. The bases it reports feed the fw-precondition bursts: the CR4
 * CORE base (=> ARMCR4_CAP/BANKINFO ramsize) and the SDIO-DEV core base
 * (=> the intstatus clear brcmf_sdio_buscore_activate does + the true HMB
 * mailbox). */
#define SI_ENUM_BASE_43455 0x18000000u
#define CC_EROMPTR_OFF 0x000000fcu
#define DMP_DESC_TYPE_MSK 0x0000000Fu
#define DMP_DESC_EMPTY 0x00000000u
#define DMP_DESC_VALID 0x00000001u
#define DMP_DESC_COMPONENT 0x00000001u
#define DMP_DESC_MASTER_PORT 0x00000003u
#define DMP_DESC_ADDRESS 0x00000005u
#define DMP_DESC_ADDRSIZE_GT32 0x00000008u
#define DMP_DESC_EOT 0x0000000Fu
#define DMP_COMP_PARTNUM 0x000FFF00u
#define DMP_COMP_PARTNUM_S 8
#define DMP_COMP_REVISION 0xFF000000u
#define DMP_COMP_REVISION_S 24
#define DMP_COMP_NUM_SWRAP 0x00F80000u
#define DMP_COMP_NUM_SWRAP_S 19
#define DMP_COMP_NUM_MWRAP 0x0007C000u
#define DMP_COMP_NUM_MWRAP_S 14
#define DMP_SLAVE_ADDR_BASE 0xFFFFF000u
#define DMP_SLAVE_TYPE 0x000000C0u
#define DMP_SLAVE_TYPE_S 6
#define DMP_SLAVE_TYPE_SLAVE 0u
#define DMP_SLAVE_TYPE_SWRAP 2u
#define DMP_SLAVE_TYPE_MWRAP 3u
#define DMP_SLAVE_SIZE_TYPE 0x00000030u
#define DMP_SLAVE_SIZE_TYPE_S 4
#define DMP_SLAVE_SIZE_4K 0u
#define DMP_SLAVE_SIZE_8K 1u
#define DMP_SLAVE_SIZE_DESC 3u
#define BCMA_ID_PMU 0x827u
#define BCMA_ID_GCI 0x840u
#define BCMA_ID_ARM_CR4 0x83Eu
#define BCMA_ID_SDIO_DEV 0x829u
#define BCMA_ID_INTERNAL_MEM 0x80Eu
#define BCMA_ID_CHIPCOMMON 0x800u

#define EROM_MAX_CORES 40

static int g_erom_ncores = -1; /* -1 = walk not run/failed */
static uint16_t g_erom_id[EROM_MAX_CORES];
static uint8_t g_erom_rev[EROM_MAX_CORES];
static uint32_t g_erom_base[EROM_MAX_CORES];
static uint32_t g_erom_wrap[EROM_MAX_CORES];
static uint32_t g_erom_ptr = 0u; /* the eromptr value we read */

/* Read one backplane byte at chip-internal `addr`, windowing per-byte so a
 * 32-bit read that straddles a 32 KiB SBADDR window boundary is still correct. */
static uint8_t diag_bpRead8(volatile uint8_t *sdhci, uint32_t addr)
{
	uint32_t resp[4] = {0};
	uint8_t lo = (uint8_t)(((addr >> 15) & 1u) ? 0x80u : 0x00u);
	uint8_t mid = (uint8_t)((addr >> 16) & 0xffu);
	uint8_t hi = (uint8_t)((addr >> 24) & 0xffu);
	uint32_t f1 = addr & 0x7FFFu;
	(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Au, lo, NULL);
	(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Bu, mid, NULL);
	(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Cu, hi, NULL);
	(void)diag_sdioCmd52(sdhci, 0, 1, f1, 0u, resp);
	return (uint8_t)(resp[0] & 0xffu);
}

static uint32_t diag_bpRead32(volatile uint8_t *sdhci, uint32_t addr)
{
	return (uint32_t)diag_bpRead8(sdhci, addr) |
		((uint32_t)diag_bpRead8(sdhci, addr + 1u) << 8) |
		((uint32_t)diag_bpRead8(sdhci, addr + 2u) << 16) |
		((uint32_t)diag_bpRead8(sdhci, addr + 3u) << 24);
}

/* Write one backplane byte at chip-internal `addr` (per-byte windowing). */
static void diag_bpWrite8(volatile uint8_t *sdhci, uint32_t addr, uint8_t v)
{
	uint8_t lo = (uint8_t)(((addr >> 15) & 1u) ? 0x80u : 0x00u);
	uint8_t mid = (uint8_t)((addr >> 16) & 0xffu);
	uint8_t hi = (uint8_t)((addr >> 24) & 0xffu);
	uint32_t f1 = addr & 0x7FFFu;
	(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Au, lo, NULL);
	(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Bu, mid, NULL);
	(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Cu, hi, NULL);
	(void)diag_sdioCmd52(sdhci, 1, 1, f1, v, NULL);
}

static void diag_bpWrite32(volatile uint8_t *sdhci, uint32_t addr, uint32_t v)
{
	diag_bpWrite8(sdhci, addr, (uint8_t)(v & 0xffu));
	diag_bpWrite8(sdhci, addr + 1u, (uint8_t)((v >> 8) & 0xffu));
	diag_bpWrite8(sdhci, addr + 2u, (uint8_t)((v >> 16) & 0xffu));
	diag_bpWrite8(sdhci, addr + 3u, (uint8_t)((v >> 24) & 0xffu));
}

/* Compute the ARMCR4 TCM RAM size from bankinfo (brcmf_chip_tcm_ramsize).
 * cr4_core = the CR4 CORE base (NOT the wrapper). Returns bytes, 0 on failure. */
#define ARMCR4_CAP_OFF 0x04u
#define ARMCR4_BANKIDX_OFF 0x40u
#define ARMCR4_BANKINFO_OFF 0x44u
#define ARMCR4_TCBANB_MASK 0x0000000Fu
#define ARMCR4_TCBBNB_MASK 0x000000F0u
#define ARMCR4_TCBBNB_SHIFT 4
#define ARMCR4_BSZ_MASK 0x0000007Fu
#define ARMCR4_BSZ_MULT 8192u
#define ARMCR4_BLK_1K_MASK 0x00000200u
static uint32_t diag_cr4RamSize(volatile uint8_t *sdhci, uint32_t cr4_core)
{
	uint32_t corecap, memsize = 0u, blksize, bxinfo;
	uint32_t nab, nbb, totb, idx;

	if (cr4_core == 0u) {
		return 0u;
	}
	corecap = diag_bpRead32(sdhci, cr4_core + ARMCR4_CAP_OFF);
	nab = (corecap & ARMCR4_TCBANB_MASK);
	nbb = (corecap & ARMCR4_TCBBNB_MASK) >> ARMCR4_TCBBNB_SHIFT;
	totb = nab + nbb;
	for (idx = 0u; idx < totb && idx < 64u; ++idx) {
		diag_bpWrite32(sdhci, cr4_core + ARMCR4_BANKIDX_OFF, idx);
		bxinfo = diag_bpRead32(sdhci, cr4_core + ARMCR4_BANKINFO_OFF);
		blksize = ARMCR4_BSZ_MULT;
		if (bxinfo & ARMCR4_BLK_1K_MASK) {
			blksize >>= 3; /* 1024 */
		}
		memsize += ((bxinfo & ARMCR4_BSZ_MASK) + 1u) * blksize;
	}
	return memsize;
}

/* get one EROM descriptor, advancing the cursor; classify ADDRESS variants. */
static uint32_t diag_dmpGetDesc(volatile uint8_t *sdhci, uint32_t *ea, uint8_t *type)
{
	uint32_t val = diag_bpRead32(sdhci, *ea);
	*ea += 4u;
	if (type != NULL) {
		*type = (uint8_t)(val & DMP_DESC_TYPE_MSK);
		if ((uint32_t)(*type & ~DMP_DESC_ADDRSIZE_GT32) == DMP_DESC_ADDRESS) {
			*type = (uint8_t)DMP_DESC_ADDRESS;
		}
	}
	return val;
}

/* obtain the (slave) regbase + wrapper base for the current component. Mirrors
 * brcmf_chip_dmp_get_regaddr. */
static int diag_dmpGetRegaddr(volatile uint8_t *sdhci, uint32_t *ea,
	uint32_t *regbase, uint32_t *wrapbase)
{
	uint8_t desc, stype, sztype, wraptype;
	uint32_t val, szdesc;

	*regbase = 0u;
	*wrapbase = 0u;

	val = diag_dmpGetDesc(sdhci, ea, &desc);
	if (desc == (uint8_t)DMP_DESC_MASTER_PORT) {
		wraptype = (uint8_t)DMP_SLAVE_TYPE_MWRAP;
	}
	else if (desc == (uint8_t)DMP_DESC_ADDRESS) {
		*ea -= 4u; /* revert */
		wraptype = (uint8_t)DMP_SLAVE_TYPE_SWRAP;
	}
	else {
		*ea -= 4u;
		return -1;
	}

	do {
		do {
			val = diag_dmpGetDesc(sdhci, ea, &desc);
			if (desc == (uint8_t)DMP_DESC_EOT) {
				*ea -= 4u;
				return -2;
			}
		} while (desc != (uint8_t)DMP_DESC_ADDRESS &&
			desc != (uint8_t)DMP_DESC_COMPONENT);

		if (desc == (uint8_t)DMP_DESC_COMPONENT) {
			*ea -= 4u;
			return 0;
		}

		if (val & DMP_DESC_ADDRSIZE_GT32) {
			(void)diag_dmpGetDesc(sdhci, ea, NULL);
		}

		sztype = (uint8_t)((val & DMP_SLAVE_SIZE_TYPE) >> DMP_SLAVE_SIZE_TYPE_S);
		if (sztype == (uint8_t)DMP_SLAVE_SIZE_DESC) {
			szdesc = diag_dmpGetDesc(sdhci, ea, NULL);
			if (szdesc & DMP_DESC_ADDRSIZE_GT32) {
				(void)diag_dmpGetDesc(sdhci, ea, NULL);
			}
		}

		if (sztype != (uint8_t)DMP_SLAVE_SIZE_4K &&
			sztype != (uint8_t)DMP_SLAVE_SIZE_8K) {
			continue;
		}

		stype = (uint8_t)((val & DMP_SLAVE_TYPE) >> DMP_SLAVE_TYPE_S);
		if (*regbase == 0u && stype == (uint8_t)DMP_SLAVE_TYPE_SLAVE) {
			*regbase = val & DMP_SLAVE_ADDR_BASE;
		}
		if (*wrapbase == 0u && stype == wraptype) {
			*wrapbase = val & DMP_SLAVE_ADDR_BASE;
		}
	} while (*regbase == 0u || *wrapbase == 0u);

	return 0;
}

/* Walk the EROM, filling g_erom_*. Returns core count (>=0) or <0 on error. */
static int diag_eromWalk(volatile uint8_t *sdhci)
{
	uint32_t eromaddr, val;
	uint8_t desc_type = 0u;
	uint16_t id;
	uint8_t nmw, nsw, rev;
	uint32_t base, wrap;
	int n = 0;
	int guard = 0;

	g_erom_ptr = diag_bpRead32(sdhci, SI_ENUM_BASE_43455 + CC_EROMPTR_OFF);
	eromaddr = g_erom_ptr;
	if (eromaddr == 0u || eromaddr == 0xFFFFFFFFu) {
		return -1;
	}

	while (desc_type != (uint8_t)DMP_DESC_EOT && n < EROM_MAX_CORES && guard < 4096) {
		guard++;
		val = diag_dmpGetDesc(sdhci, &eromaddr, &desc_type);
		if (!(val & DMP_DESC_VALID)) {
			continue;
		}
		if (desc_type == (uint8_t)DMP_DESC_EMPTY) {
			continue;
		}
		if (desc_type != (uint8_t)DMP_DESC_COMPONENT) {
			continue;
		}

		id = (uint16_t)((val & DMP_COMP_PARTNUM) >> DMP_COMP_PARTNUM_S);

		val = diag_dmpGetDesc(sdhci, &eromaddr, &desc_type);
		if ((val & DMP_DESC_TYPE_MSK) != DMP_DESC_COMPONENT) {
			return (n > 0) ? n : -2; /* malformed */
		}

		nmw = (uint8_t)((val & DMP_COMP_NUM_MWRAP) >> DMP_COMP_NUM_MWRAP_S);
		nsw = (uint8_t)((val & DMP_COMP_NUM_SWRAP) >> DMP_COMP_NUM_SWRAP_S);
		rev = (uint8_t)((val & DMP_COMP_REVISION) >> DMP_COMP_REVISION_S);

		if ((nmw + nsw) == 0 && id != BCMA_ID_PMU && id != BCMA_ID_GCI) {
			continue;
		}

		if (diag_dmpGetRegaddr(sdhci, &eromaddr, &base, &wrap) != 0) {
			continue;
		}

		g_erom_id[n] = id;
		g_erom_rev[n] = rev;
		g_erom_base[n] = base;
		g_erom_wrap[n] = wrap;
		n++;
	}

	return n;
}

/* Look up a core base (or wrap) by id from the walk results; 0 if not found. */
static uint32_t diag_eromCoreBase(uint16_t id)
{
	int i;
	for (i = 0; i < g_erom_ncores; ++i) {
		if (g_erom_id[i] == id) {
			return g_erom_base[i];
		}
	}
	return 0u;
}

static uint32_t diag_eromCoreWrap(uint16_t id)
{
	int i;
	for (i = 0; i < g_erom_ncores; ++i) {
		if (g_erom_id[i] == id) {
			return g_erom_wrap[i];
		}
	}
	return 0u;
}

/* ---- #91 sdpcm_shared + firmware console ---------------------------------
 * Port of brcmf_sdio_readshared (sdio.c): the fw, once booted, overwrites the
 * word at ram_top-4 (where the NVRAM length-magic token was) with a pointer to
 * its sdpcm_shared struct. From there console_addr -> rte_console gives the fw
 * console ring buffer -- letting us SEE what the firmware prints instead of
 * poking blind. On-dongle (32-bit) offsets: sdpcm_shared { flags@0, trap@4,
 * assert_exp@8, assert_file@12, assert_line@16, console_addr@20 }; rte_console
 * { ... log_le@8 { buf@0, buf_size@4, idx@8 } } => log_buf@console+8,
 * buf_size@console+12, idx@console+16. */
#define FWCON_MAX 1536
static int g_shared_valid = -1; /* -1 not attempted, 0 invalid, 1 valid */
static uint32_t g_sh_word = 0u, g_sh_addr = 0u, g_sh_flags = 0u, g_trap_addr = 0u;
static uint32_t g_console_addr = 0u, g_log_buf = 0u, g_log_bufsize = 0u, g_log_idx = 0u;
static char g_console[FWCON_MAX];
static int g_console_len = 0;

static void diag_readShared(volatile uint8_t *sdhci, uint32_t ram_size)
{
	uint32_t shaddr, a, n, i;

	g_shared_valid = 0;
	g_console_len = 0;
	if (ram_size == 0u) {
		return;
	}
	shaddr = 0x198000u + ram_size - 4u;
	a = diag_bpRead32(sdhci, shaddr);
	g_sh_word = a;
	/* brcmf_sdio_valid_shared_address: the NVRAM-token pattern (~x<<16)|x is
	 * INVALID -> means the fw never overwrote it -> not booted. */
	if (a == 0u || (((~a >> 16) & 0xffffu) == (a & 0xffffu))) {
		return;
	}
	g_shared_valid = 1;
	g_sh_addr = a;
	g_sh_flags = diag_bpRead32(sdhci, a + 0u);
	g_trap_addr = diag_bpRead32(sdhci, a + 4u);
	g_console_addr = diag_bpRead32(sdhci, a + 20u);
	if (g_console_addr != 0u && g_console_addr != 0xffffffffu) {
		g_log_buf = diag_bpRead32(sdhci, g_console_addr + 8u);
		g_log_bufsize = diag_bpRead32(sdhci, g_console_addr + 12u);
		g_log_idx = diag_bpRead32(sdhci, g_console_addr + 16u);
		if (g_log_buf != 0u && g_log_buf != 0xffffffffu) {
			n = g_log_idx;
			if (n > (uint32_t)(FWCON_MAX - 1)) {
				n = (uint32_t)(FWCON_MAX - 1);
			}
			if (g_log_bufsize != 0u && n > g_log_bufsize) {
				n = g_log_bufsize;
			}
			for (i = 0u; i < n; ++i) {
				g_console[i] = (char)diag_bpRead8(sdhci, g_log_buf + i);
			}
			g_console_len = (int)n;
		}
	}
}

/* ---- #91 BCDC control ioctl round-trip over F2 ---------------------------
 * First real driver protocol: send one BCDC GET (WLC_GET_VERSION=1) wrapped in
 * an SDPCM control frame over SDIO function 2, poll the SDIO-core intstatus for
 * I_HMB_FRAME_IND, read the reply back from the F2 FIFO, strip SDPCM+BCDC, and
 * report the returned u32 version. Spec derived byte-for-byte from brcmfmac
 * (bcdc.c/sdio.c/bcmsdh.c). F2 frame addressing: backplane window 0x18000000,
 * CMD53 addr 0x8000; write=incrementing, read=fixed FIFO. Small frame padded to
 * a 64-byte block (F2 blocksize set to 64 via CCCR FBR to reuse block-mode). */
#define IOCTL_F2_ADDR 0x8000u
#define WLC_GET_VERSION 1u
static int g_ioctl_mode = 0;
static int g_ioctl_ran = 0;
static int g_ioctl_send_rc = -100, g_ioctl_read_rc = -100;
static uint32_t g_ioctl_is_pre = 0u, g_ioctl_is_post = 0u;
static int g_ioctl_is_iters = -1;
static uint8_t g_ioctl_reply[64];
static uint8_t g_ioctl_pending[64];
static int g_ioctl_pending_rc = -100;
static int g_ioctl_reply_valid = 0;
static uint16_t g_ioctl_hwlen = 0u, g_ioctl_hwchk = 0u;
static uint8_t g_ioctl_doff = 0u, g_ioctl_channel = 0xffu;
static uint32_t g_ioctl_bcdc_flags = 0u, g_ioctl_bcdc_status = 0u, g_ioctl_version = 0u;

static uint32_t diag_le32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
		((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void diag_setWindow18(volatile uint8_t *sdhci)
{
	(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Au, 0x00u, NULL);
	(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Bu, 0x00u, NULL);
	(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Cu, 0x18u, NULL);
}

static void diag_bcdcGetVersion(volatile uint8_t *sdhci, uint32_t sdio_core)
{
	uint8_t frame[64];
	uint16_t flen = 32u; /* 12 SDPCM + 16 BCDC + 4 payload */
	int i;

	g_ioctl_ran = 1;
	for (i = 0; i < 64; ++i) {
		frame[i] = 0u;
		g_ioctl_reply[i] = 0u;
	}
	/* SDPCM HW header: len + ~len */
	frame[0] = (uint8_t)(flen & 0xffu);
	frame[1] = (uint8_t)((flen >> 8) & 0xffu);
	frame[2] = (uint8_t)((~flen) & 0xffu);
	frame[3] = (uint8_t)(((~flen) >> 8) & 0xffu);
	/* SDPCM SW header: seq=0, channel=0 (control), nextlen=0, data_offset=12 */
	frame[4] = 0x00u;
	frame[5] = 0x00u;
	frame[6] = 0x00u;
	frame[7] = 12u;
	/* word1 [8..11] = 0 */
	/* BCDC @12: cmd=WLC_GET_VERSION(1) */
	frame[12] = (uint8_t)WLC_GET_VERSION;
	/* len @16 = 4 (output buflen) */
	frame[16] = 0x04u;
	/* flags @20 = 0x00010000 (reqid=1, ifidx=0, GET: SET bit clear) */
	frame[22] = 0x01u;
	/* status @24 = 0; payload @28 = 0 (4-byte scratch) */

	for (i = 0; i < 64; ++i) {
		g_ioctl_pending[i] = 0u;
	}

	g_ioctl_is_pre = diag_bpRead32(sdhci, sdio_core + 0x20u);

	/* A frame is already pending at boot (I_HMB_FRAME_IND set). Read it from
	 * the F2 FIFO first (byte mode) -- proves F2 RX independent of our TX. */
	diag_setWindow18(sdhci);
	g_ioctl_pending_rc = diag_sdioCmd53ReadByteMode(sdhci, 2, /*incr=*/0,
		IOCTL_F2_ADDR, 64u, g_ioctl_pending);

	/* Send the control frame over F2 (window 0x18000000, addr 0x8000, incr,
	 * byte mode -- a small control frame is sub-block). */
	diag_setWindow18(sdhci);
	g_ioctl_send_rc = diag_sdioCmd53WriteByteMode(sdhci, 2, /*incr=*/1,
		IOCTL_F2_ADDR, 32u, frame);

	/* Poll SDIO-core intstatus for I_HMB_FRAME_IND (0x40). */
	for (i = 0; i < 250; ++i) {
		g_ioctl_is_post = diag_bpRead32(sdhci, sdio_core + 0x20u);
		if ((g_ioctl_is_post & 0x40u) != 0u) {
			g_ioctl_is_iters = i;
			break;
		}
		usleep(2000);
	}
	/* Clear intstatus (write value back). */
	if (g_ioctl_is_post != 0u && g_ioctl_is_post != 0xffffffffu) {
		diag_bpWrite32(sdhci, sdio_core + 0x20u, g_ioctl_is_post);
	}

	/* Read the reply from the F2 FIFO (fixed address, byte mode). */
	diag_setWindow18(sdhci);
	g_ioctl_read_rc = diag_sdioCmd53ReadByteMode(sdhci, 2, /*incr=*/0,
		IOCTL_F2_ADDR, 64u, g_ioctl_reply);

	/* Parse SDPCM HW header + SW data_offset, then BCDC. */
	g_ioctl_hwlen = (uint16_t)(g_ioctl_reply[0] | (g_ioctl_reply[1] << 8));
	g_ioctl_hwchk = (uint16_t)(g_ioctl_reply[2] | (g_ioctl_reply[3] << 8));
	if (g_ioctl_hwlen != 0u &&
		(uint16_t)(~(g_ioctl_hwlen ^ g_ioctl_hwchk)) == 0u &&
		g_ioctl_hwlen >= 12u) {
		g_ioctl_reply_valid = 1;
		g_ioctl_doff = g_ioctl_reply[7];
		g_ioctl_channel = (uint8_t)(g_ioctl_reply[5] & 0x0fu);
		if ((int)g_ioctl_doff + 20 <= 64) {
			g_ioctl_bcdc_flags = diag_le32(g_ioctl_reply + g_ioctl_doff + 8);
			g_ioctl_bcdc_status = diag_le32(g_ioctl_reply + g_ioctl_doff + 12);
			g_ioctl_version = diag_le32(g_ioctl_reply + g_ioctl_doff + 16);
		}
	}
}

/* #91 "trivial-program test" mode. When set (argv "trivial"), the 643 KB
 * production firmware is replaced by cr4tiny_blob (a ~20-byte Thumb-2 counter
 * whose reset vector is the REAL fw's verbatim B.W into rambase+0x80). The
 * identical CR4 release runs, then we read back the counter at
 * CR4TINY_COUNTER_ADDR. Increment => the release path executes CR4 code (chase
 * fw preconditions: NVRAM ram-top, clocks); dead => the release path itself is
 * broken (wrong core / reset semantics). Baseline path is byte-identical when
 * this is 0. */
static int g_trivial_mode = 0;

/* WiFi P3 final: full-firmware load + release ARM-CR4 + look for fw boot.
 *
 * Load pipeline: enum (CMD0/5/3/7) -> F1 enable -> KSO -> HS-mode ->
 * ALP-only backplane clock -> walk 643 KB firmware into SOCRAM at
 * chip-internal 0x198000 -> load NVRAM at 0x238000-len, then:
 *
 *   1. Write the firmware reset vector (first word) to chip-internal 0.
 *   2. Re-window to ARM-CR4 wrapper (0x18100000) and do the brcmfmac AXI
 *      resetcore toggle to release the CR4 (BCMA_IOCTL/RESET_CTL pokes).
 *   3. Enable Function 2 (SDPCM data channel) and wait for F2-ready.
 *   4. Sleep, then read back SOCRAM head + several scan points, HT_AVAIL
 *      (CHIPCLKCSR), SDHCI CARD_INTR, the SOCRAM NVRAM trailer, and the
 *      SDIOD tohostmailboxdata HMB_DATA_FWREADY word.
 *
 * "fw_alive" = HT_AVAIL asserted OR CARD_INTR asserted. See the inline
 * comments (kept verbatim) for the brcmfmac references behind each step. */
static int diag_format_sdio_fwrelease(char *buf, size_t cap)
{
	static uint8_t pre_buf[64];
	static uint8_t post_buf[64];
	int off = 0, r;
	void *gpio_page, *sdhci_page;
	uint32_t ocr_resp[4] = {0}, claim_resp[4] = {0};
	uint32_t rca_resp[4] = {0}, sel_resp[4] = {0};
	uint32_t ioen_pre_resp[4] = {0}, iordy_resp[4] = {0};
	uint32_t rc_pre_resp[4] = {0}, rc_post_resp[4] = {0};
	int rc_ocr = -1, rc_claim = -1, rc_sel = -1, rc_iordy = -1;
	int rc_hs = -100;
	int ready_iters = 0, rdy_iters = 0;
	uint16_t rca = 0;
	int rc_w, rc_r_pre = -100, rc_r_post = -100;
	int rc_nvram_w = -100;
	int rc_tail = -100;
	uint8_t chipclk_samples[8] = {0};
	uint8_t socram_tail[16] = {0};
	uint8_t scan_buf[64];
	int scan_rc[6] = {0};
	int scan_diff[6] = {0};
	int scan_changed_pts = -1;
	uint8_t ht_clk_csr = 0u;
	uint8_t f2_ready = 0u;
	int f2_ready_iters = -1;
	uint8_t rstvec_rb[4] = {0};
	uint32_t hmb_data = 0u;
	unsigned card_intr = 0u;
	int worst_rc_w = 0;
	int i, pre_match, post_match, diff_count;
	uint32_t bytes_written = 0u;
	int window_idx = 0;
	size_t fw_offset = 0u;
	size_t fw_target_bytes;
	const uint32_t window_bytes = 32u * 1024u;
	const uint32_t blk_size = 64u;
	const uint32_t blk_count = 64u;
	/* #91 trivial-program test extras (baseline path ignores these). */
	const uint8_t *fw_img = g_trivial_mode ? cr4tiny_blob : wifi_fw_43455;
	const size_t fw_img_len = g_trivial_mode ? (size_t)cr4tiny_blob_len : (size_t)wifi_fw_43455_len;
	uint8_t cnt_pre[4] = { 0 }, cnt_post[4] = { 0 }, cnt_post2[4] = { 0 };
	int rc_cnt_pre = -100, rc_cnt_post = -100, rc_cnt_post2 = -100;
	uint32_t ioctl_w2 = 0u, ioctl_w3 = 0u; /* dual ARM-wrapper CR4-identity cross-check */
	uint32_t cr4_core = 0u, sdio_core = 0x18004000u, ram_size = 0u; /* EROM-derived bases */

	for (i = 0; i < (int)sizeof(pre_buf); ++i) {
		pre_buf[i] = 0;
		post_buf[i] = 0;
	}

	r = snprintf(buf + off, cap - off, "PHX-DIAG/1 sdio-fwrelease\n");
	if (r < 0 || (size_t)r >= cap - off) {
		return -1;
	}
	off += r;

	if (fw_img_len == 0u) {
		r = snprintf(buf + off, cap - off,
			"error: firmware blob not staged\n.\n");
		return off + (r > 0 ? r : 0);
	}
	fw_target_bytes = (fw_img_len / blk_size) * blk_size;

	gpio_page = mmap(NULL, _PAGE_SIZE, PROT_READ | PROT_WRITE,
		MAP_DEVICE | MAP_UNCACHED | MAP_PHYSMEM | MAP_ANONYMOUS,
		-1, BCM2711_GPIO_BASE);
	sdhci_page = mmap(NULL, _PAGE_SIZE, PROT_READ | PROT_WRITE,
		MAP_DEVICE | MAP_UNCACHED | MAP_PHYSMEM | MAP_ANONYMOUS,
		-1, 0xfe300000u);

	if (gpio_page == MAP_FAILED || sdhci_page == MAP_FAILED) {
		r = snprintf(buf + off, cap - off, "error: mmap failed\n.\n");
		if (gpio_page != MAP_FAILED) {
			munmap(gpio_page, _PAGE_SIZE);
		}
		if (sdhci_page != MAP_FAILED) {
			munmap(sdhci_page, _PAGE_SIZE);
		}
		return off + (r > 0 ? r : 0);
	}

	{
		volatile uint8_t *gpio = (volatile uint8_t *)gpio_page;
		volatile uint8_t *sdhci = (volatile uint8_t *)sdhci_page;

		for (i = 34; i <= 39; ++i) {
			diag_gpioSetFsel(gpio, (unsigned)i, 7u);
		}
		diag_wifiPowerCycle();
		(void)diag_sdhciSetClockKHz(sdhci, 400u);
		(void)diag_sdhciResetCmdDat(sdhci);

		(void)diag_sdhciCmd(sdhci, 0u, 0u, SDHCI_RESP_R0, NULL);
		usleep(1000);
		rc_ocr = diag_sdhciCmd(sdhci, 5u, 0u, SDHCI_RESP_R4, ocr_resp);
		for (ready_iters = 0; ready_iters < 50; ++ready_iters) {
			rc_claim = diag_sdhciCmd(sdhci, 5u, ocr_resp[0] & 0x00ffffffu,
				SDHCI_RESP_R4, claim_resp);
			if (rc_claim != 0) {
				break;
			}
			if ((claim_resp[0] & 0x80000000u) != 0u) {
				ready_iters++;
				break;
			}
			usleep(1000);
		}
		(void)diag_sdhciCmd(sdhci, 3u, 0u, SDHCI_RESP_R6, rca_resp);
		rca = (uint16_t)((rca_resp[0] >> 16) & 0xFFFFu);
		rc_sel = diag_sdhciCmd(sdhci, 7u, (uint32_t)rca << 16, SDHCI_RESP_R1, sel_resp);

		(void)diag_sdioCmd52(sdhci, 0, 0, 0x02u, 0u, ioen_pre_resp);
		(void)diag_sdioCmd52(sdhci, 1, 0, 0x02u,
			(uint8_t)((ioen_pre_resp[0] | 0x02u) & 0xffu), NULL);
		for (rdy_iters = 0; rdy_iters < 50; ++rdy_iters) {
			rc_iordy = diag_sdioCmd52(sdhci, 0, 0, 0x03u, 0u, iordy_resp);
			if (rc_iordy != 0) {
				break;
			}
			if ((iordy_resp[0] & 0x02u) != 0u) {
				rdy_iters++;
				break;
			}
			usleep(1000);
		}

		/* KSO (Keep-SDIO-On) enable. SDIO core rev >= 12 (43455 qualifies)
		 * gates the backplane clock on KSO; without it the device can
		 * drop the clock and HT_AVAIL never latches. SLEEPCSR (F1
		 * 0x1001F) bit 0 = KSO_EN. RMW. */
		{
			uint32_t kso[4] = {0};
			(void)diag_sdioCmd52(sdhci, 0, 1, 0x1001Fu, 0u, kso);
			(void)diag_sdioCmd52(sdhci, 1, 1, 0x1001Fu,
				(uint8_t)((kso[0] | 0x01u) & 0xffu), NULL);
		}

		rc_hs = diag_sdioGoHighSpeed(sdhci);

		/* Backplane clock bring-up before CR4 release: ALP ONLY.
		 * Per brcmfmac brcmf_sdio_load_firmware(), the host sets
		 * alp_only=true for the whole firmware-download + CR4-release
		 * window and brings the backplane up on ALP only
		 * (SBSDIO_ALP_AVAIL_REQ 0x08; wait SBSDIO_ALP_AVAIL 0x40). The
		 * firmware running on the CR4 brings HT up itself once executing;
		 * forcing HT here cannot work (the CR4 has no HT clock until fw
		 * requests it). HT_AVAIL is polled AFTER release below as the
		 * firmware-alive tell. */
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Eu, 0x08u, NULL);
		for (i = 0; i < 250; ++i) {
			uint32_t cc[4] = {0};
			(void)diag_sdioCmd52(sdhci, 0, 1, 0x1000Eu, 0u, cc);
			ht_clk_csr = (uint8_t)(cc[0] & 0xffu);
			if ((ht_clk_csr & 0x40u) != 0u) {
				break;
			}
			usleep(2000);
		}

		(void)diag_sdioCmd52(sdhci, 1, 0, 0x110u, 0x40u, NULL);
		(void)diag_sdioCmd52(sdhci, 1, 0, 0x111u, 0x00u, NULL);

		/* #91: enumerate cores over the backplane (read-only) now that the
		 * ALP clock is up, so the report can replace the hardcoded core-
		 * address hypotheses with the chip's own EROM answers. Done before
		 * the fw download; it only sets/reads SBADDR windows, which the
		 * download loop re-sets on its first iteration. */
		g_erom_ncores = diag_eromWalk(sdhci);
		cr4_core = diag_eromCoreBase(BCMA_ID_ARM_CR4);
		if (cr4_core == 0u) {
			cr4_core = 0x18002000u; /* EROM-confirmed fallback */
		}
		{
			uint32_t s = diag_eromCoreBase(BCMA_ID_SDIO_DEV);
			if (s != 0u) {
				sdio_core = s;
			}
		}
		/* True TCM ramsize from CR4 bankinfo (fw is halted here — safe). */
		ram_size = diag_cr4RamSize(sdhci, cr4_core);

		while (fw_offset < fw_target_bytes && rc_hs == 0) {
			uint32_t addr = 0x00198000u + (uint32_t)window_idx * 0x8000u;
			uint8_t  lo  = (uint8_t)(((addr >> 15) & 1u) ? 0x80u : 0x00u);
			uint8_t  mid = (uint8_t)((addr >> 16) & 0xffu);
			uint8_t  hi  = (uint8_t)((addr >> 24) & 0xffu);
			size_t   remaining = fw_target_bytes - fw_offset;
			size_t   this_window = (remaining > window_bytes) ? window_bytes : remaining;
			uint32_t bytes_per_cmd = blk_count * blk_size;
			uint32_t chunks = (uint32_t)(this_window / bytes_per_cmd);
			uint32_t leftover_blocks = (uint32_t)((this_window % bytes_per_cmd) / blk_size);
			uint32_t ci;

			(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Au, lo,  NULL);
			(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Bu, mid, NULL);
			(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Cu, hi,  NULL);

			for (ci = 0; ci < chunks; ++ci) {
				rc_w = diag_sdioCmd53Write(sdhci, 1, /*incr=*/1,
					/*reg_addr=*/ci * bytes_per_cmd,
					/*block_count=*/blk_count,
					/*block_size=*/blk_size,
					fw_img + fw_offset + ci * bytes_per_cmd);
				if (rc_w != 0) {
					if (worst_rc_w == 0) worst_rc_w = rc_w;
					break;
				}
				bytes_written += bytes_per_cmd;
			}
			if (rc_w != 0) break;

			if (leftover_blocks > 0) {
				rc_w = diag_sdioCmd53Write(sdhci, 1, /*incr=*/1,
					/*reg_addr=*/chunks * bytes_per_cmd,
					/*block_count=*/leftover_blocks,
					/*block_size=*/blk_size,
					fw_img + fw_offset + chunks * bytes_per_cmd);
				if (rc_w != 0) {
					if (worst_rc_w == 0) worst_rc_w = rc_w;
					break;
				}
				bytes_written += leftover_blocks * blk_size;
			}

			fw_offset += this_window;
			window_idx++;
		}

		/* NVRAM load: chip-ready blob goes at chip-internal
		 * (rambase + ramsize - wifi_nvram_43455_len) = 0x238000 - len,
		 * inside SBADDR window 19, padded to a 64-byte boundary so it
		 * lands as a single CMD53 multi-block write. Skipped in the
		 * trivial-program test: the counter needs no NVRAM, and skipping
		 * it removes NVRAM as a variable from a dead-counter result. */
		if (!g_trivial_mode) {
			/* Place NVRAM at the TRUE ram-top from CR4 bankinfo, not the old
			 * hardcoded 0x238000. The bootloader reads the length-magic token
			 * at ram_top-4; a wrong ram-top => fw never finds NVRAM. */
			uint32_t nv_ramtop = (ram_size != 0u) ? (0x198000u + ram_size) : 0x238000u;
			uint32_t nv_start = nv_ramtop - (uint32_t)wifi_nvram_43455_len;
			uint8_t  nv_lo  = (uint8_t)(((nv_start >> 15) & 1u) ? 0x80u : 0x00u);
			uint8_t  nv_mid = (uint8_t)((nv_start >> 16) & 0xffu);
			uint8_t  nv_hi  = (uint8_t)((nv_start >> 24) & 0xffu);
			uint32_t nv_f1_offset = nv_start & 0x7FFFu;
			uint32_t nv_blocks = (uint32_t)(wifi_nvram_43455_len / 64u);

			(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Au, nv_lo,  NULL);
			(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Bu, nv_mid, NULL);
			(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Cu, nv_hi,  NULL);

			rc_nvram_w = diag_sdioCmd53Write(sdhci, 1, /*incr=*/1,
				/*reg_addr=*/nv_f1_offset,
				/*block_count=*/nv_blocks,
				/*block_size=*/64u, wifi_nvram_43455);
		}

		/* Snapshot SOCRAM[0..63] BEFORE release — should match source
		 * firmware byte-identically. */
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Au, 0x80u, NULL);
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Bu, 0x19u, NULL);
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Cu, 0x00u, NULL);
		rc_r_pre = diag_sdioCmd53Read(sdhci, 1, /*incr=*/1,
			/*reg_addr=*/0u, /*block_count=*/1u, /*block_size=*/64u, pre_buf);

		/* #91 trivial test: counter pre-state at CR4TINY_COUNTER_ADDR
		 * (0x199000 = blob offset 0x1000, which is 0 => expect 0). Same
		 * 0x198000 window as the SOCRAM snapshot; F1 offset 0x1000. */
		{
			uint32_t c0[4] = {0}, c1[4] = {0}, c2[4] = {0}, c3[4] = {0};
			(void)diag_sdioCmd52(sdhci, 0, 1, 0x1000u, 0u, c0);
			(void)diag_sdioCmd52(sdhci, 0, 1, 0x1001u, 0u, c1);
			(void)diag_sdioCmd52(sdhci, 0, 1, 0x1002u, 0u, c2);
			(void)diag_sdioCmd52(sdhci, 0, 1, 0x1003u, 0u, c3);
			cnt_pre[0] = (uint8_t)(c0[0] & 0xffu);
			cnt_pre[1] = (uint8_t)(c1[0] & 0xffu);
			cnt_pre[2] = (uint8_t)(c2[0] & 0xffu);
			cnt_pre[3] = (uint8_t)(c3[0] & 0xffu);
			rc_cnt_pre = 0;
		}

		/* brcmf_sdio_buscore_activate step 0 (was MISSING — suspect 3b):
		 * clear the SDIO-DEV core intstatus (write 0xFFFFFFFF) BEFORE the
		 * reset vector, exactly as brcmfmac does. Uses the EROM SDIO_DEV
		 * base (0x18004000) + intstatus@0x20, NOT the old 0x18005000 guess. */
		diag_bpWrite32(sdhci, sdio_core + 0x20u, 0xFFFFFFFFu);

		/* brcmfmac CR4 activation, step 1: write the firmware reset
		 * vector (first word of the blob) to chip-internal address 0.
		 * The low 32 bytes of address 0 are a writable vector-table
		 * overlay; the CR4 fetches its reset vector from here when it
		 * leaves reset. */
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Au, 0x00u, NULL);
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Bu, 0x00u, NULL);
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Cu, 0x00u, NULL);
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x0u, fw_img[0], NULL);
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x1u, fw_img[1], NULL);
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x2u, fw_img[2], NULL);
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x3u, fw_img[3], NULL);

		/* Read addr 0 back to VERIFY the rstvec landed at TRUE backplane
		 * address 0. A mismatch means the addr-0 write is landing in
		 * TCM/0x198000 (SBADDR window / address-mask bug) and the CR4
		 * fetches a garbage reset vector. */
		{
			uint32_t v0[4] = {0}, v1[4] = {0}, v2[4] = {0}, v3[4] = {0};
			(void)diag_sdioCmd52(sdhci, 0, 1, 0x0u, 0u, v0);
			(void)diag_sdioCmd52(sdhci, 0, 1, 0x1u, 0u, v1);
			(void)diag_sdioCmd52(sdhci, 0, 1, 0x2u, 0u, v2);
			(void)diag_sdioCmd52(sdhci, 0, 1, 0x3u, 0u, v3);
			rstvec_rb[0] = (uint8_t)(v0[0] & 0xffu);
			rstvec_rb[1] = (uint8_t)(v1[0] & 0xffu);
			rstvec_rb[2] = (uint8_t)(v2[0] & 0xffu);
			rstvec_rb[3] = (uint8_t)(v3[0] & 0xffu);
		}

		/* Re-window to ARM-CR4 wrapper window 0x18100000:
		 *   F1 0x2408 = chip-internal 0x18102408 = BCMA_IOCTL
		 *   F1 0x2800 = chip-internal 0x18102800 = BCMA_RESET_CTL */
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Au, 0x00u, NULL);
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Bu, 0x10u, NULL);
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Cu, 0x18u, NULL);

		/* Read IOCTL pre (POR observed 0x21 = CPUHALT|CLK). */
		(void)diag_sdioCmd52(sdhci, 0, 1, 0x2408u, 0u, rc_pre_resp);

		/* CR4-identity cross-check: read IOCTL at BOTH candidate ARM-wrapper
		 * windows (0x18102408 = the one we release, 0x18103408 = the other)
		 * so a dead-counter result can be attributed to the right half of
		 * the tree. The true CR4 exposes the CPUHALT bit (0x20). */
		{
			uint32_t w2[4] = {0}, w3[4] = {0};
			(void)diag_sdioCmd52(sdhci, 0, 1, 0x2408u, 0u, w2);
			(void)diag_sdioCmd52(sdhci, 0, 1, 0x3408u, 0u, w3);
			ioctl_w2 = w2[0] & 0xffu;
			ioctl_w3 = w3[0] & 0xffu;
		}

		/* brcmfmac CR4 activation, step 2: full AXI resetcore toggle,
		 * resetcore(core, prereset=CPUHALT(0x20), reset=0, postreset=0):
		 *   coredisable: IOCTL=0x23; RESET_CTL=0x01; IOCTL=0x03
		 *   deassert:    RESET_CTL=0 (poll until clear)
		 *   finalize:    IOCTL=0x01 (CLK only, CPU runs) */
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x2408u, 0x23u, NULL);   /* IOCTL CPUHALT|FGC|CLK */
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x2800u, 0x01u, NULL);   /* RESET_CTL assert */
		(void)diag_sdioCmd52(sdhci, 0, 1, 0x2800u, 0u, NULL);      /* readback settle */
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x2408u, 0x03u, NULL);   /* IOCTL FGC|CLK (reset=0) */
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x2800u, 0x00u, NULL);   /* RESET_CTL deassert */
		for (i = 0; i < 50; ++i) {
			uint32_t rcv[4] = {0};
			(void)diag_sdioCmd52(sdhci, 0, 1, 0x2800u, 0u, rcv);
			if ((rcv[0] & 0x01u) == 0u) {
				break;
			}
			usleep(1000);
		}
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x2408u, 0x01u, NULL);   /* IOCTL CLK (CPU runs) */

		/* Post-release SDIO handshake (brcmf_sdio_bus_init): once the CR4
		 * is running, enable Function 2 (SDPCM data channel) via CCCR
		 * IOEN bit 2 (0x04) and wait for F2-ready in CCCR IOR bit 2. */
		{
			uint32_t ioen_resp[4] = {0};
			(void)diag_sdioCmd52(sdhci, 0, 0, 0x02u, 0u, ioen_resp);
			(void)diag_sdioCmd52(sdhci, 1, 0, 0x02u,
				(uint8_t)((ioen_resp[0] | 0x04u) & 0xffu), NULL);  /* IOEN F2 */
			for (i = 0; i < 500; ++i) {
				uint32_t ior_resp[4] = {0};
				(void)diag_sdioCmd52(sdhci, 0, 0, 0x03u, 0u, ior_resp);
				f2_ready = (uint8_t)(ior_resp[0] & 0xffu);
				if ((f2_ready & 0x04u) != 0u) {
					f2_ready_iters = i;
					break;
				}
				usleep(2000);
			}
		}

		usleep(300 * 1000);  /* firmware init: NVRAM parse + chip-self-test */

		/* Read IOCTL post (expect 0x01 = CLK only, CPU running). */
		(void)diag_sdioCmd52(sdhci, 0, 1, 0x2408u, 0u, rc_post_resp);

		/* Re-window to SOCRAM and capture post-release snapshot. */
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Au, 0x80u, NULL);
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Bu, 0x19u, NULL);
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Cu, 0x00u, NULL);
		rc_r_post = diag_sdioCmd53Read(sdhci, 1, /*incr=*/1,
			/*reg_addr=*/0u, /*block_count=*/1u, /*block_size=*/64u, post_buf);

		/* #91 trivial test: counter POST-release at CR4TINY_COUNTER_ADDR.
		 * Same 0x198000 window; F1 offset 0x1000. The counter free-runs at
		 * ~MHz, so we do NOT expect the exact seed magic back -- we expect a
		 * value that (a) differs from the known-zero pre-state and (b) keeps
		 * CLIMBING between two reads a short delay apart. read2 >> read1 is
		 * unambiguous live execution (kills any static-artifact hypothesis in
		 * one boot). The 0xC0/0xC1 top byte corroborates our seed. */
		{
			uint32_t c0[4] = {0}, c1[4] = {0}, c2[4] = {0}, c3[4] = {0};
			(void)diag_sdioCmd52(sdhci, 0, 1, 0x1000u, 0u, c0);
			(void)diag_sdioCmd52(sdhci, 0, 1, 0x1001u, 0u, c1);
			(void)diag_sdioCmd52(sdhci, 0, 1, 0x1002u, 0u, c2);
			(void)diag_sdioCmd52(sdhci, 0, 1, 0x1003u, 0u, c3);
			cnt_post[0] = (uint8_t)(c0[0] & 0xffu);
			cnt_post[1] = (uint8_t)(c1[0] & 0xffu);
			cnt_post[2] = (uint8_t)(c2[0] & 0xffu);
			cnt_post[3] = (uint8_t)(c3[0] & 0xffu);
			rc_cnt_post = 0;

			usleep(50 * 1000); /* let the free-running counter advance */

			(void)diag_sdioCmd52(sdhci, 0, 1, 0x1000u, 0u, c0);
			(void)diag_sdioCmd52(sdhci, 0, 1, 0x1001u, 0u, c1);
			(void)diag_sdioCmd52(sdhci, 0, 1, 0x1002u, 0u, c2);
			(void)diag_sdioCmd52(sdhci, 0, 1, 0x1003u, 0u, c3);
			cnt_post2[0] = (uint8_t)(c0[0] & 0xffu);
			cnt_post2[1] = (uint8_t)(c1[0] & 0xffu);
			cnt_post2[2] = (uint8_t)(c2[0] & 0xffu);
			cnt_post2[3] = (uint8_t)(c3[0] & 0xffu);
			rc_cnt_post2 = 0;
		}

		/* fw-execution disambiguation (#91): SOCRAM[0..63] is entry/vector
		 * code a running fw need not modify, so it is a weak "alive" tell.
		 * Scan several points spread across the loaded image and compare
		 * the post-release on-chip bytes to the source blob. ANY changed
		 * point => the CR4 IS executing; zero change everywhere => fw
		 * genuinely not running. Skipped in trivial mode: the scan offsets
		 * exceed the small trivial blob (the counter readback is the tell). */
		if (!g_trivial_mode) {
			static const uint32_t scan_off[6] = {
				0x02000u, 0x10000u, 0x30000u, 0x60000u, 0x90000u, 0x9C000u
			};
			unsigned s;
			int k;
			scan_changed_pts = 0;
			for (s = 0u; s < 6u; ++s) {
				uint32_t a = 0x198000u + scan_off[s];
				uint8_t lo = (uint8_t)(((a >> 15) & 1u) ? 0x80u : 0x00u);
				uint8_t mid = (uint8_t)((a >> 16) & 0xffu);
				uint8_t hi = (uint8_t)((a >> 24) & 0xffu);
				uint32_t f1 = a & 0x7FFFu;
				int d = 0;
				(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Au, lo, NULL);
				(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Bu, mid, NULL);
				(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Cu, hi, NULL);
				scan_rc[s] = diag_sdioCmd53Read(sdhci, 1, /*incr=*/1,
					/*reg_addr=*/f1, /*block_count=*/1u, /*block_size=*/64u,
					scan_buf);
				if (scan_rc[s] == 0) {
					for (k = 0; k < 64; ++k) {
						if (scan_buf[k] != wifi_fw_43455[scan_off[s] + (uint32_t)k]) {
							++d;
						}
					}
					scan_diff[s] = d;
					if (d > 0) {
						++scan_changed_pts;
					}
				}
				else {
					scan_diff[s] = -1;
				}
			}
		}

		/* Firmware-running probes:
		 * 1. CHIPCLKCSR (F1 0x1000E): HT_AVAIL (bit 7, 0x80) goes high
		 *    once the booted firmware requests the HT backplane clock.
		 * 2. SDHCI CARD_INTR (INT_STATUS bit 8): the chip asserts its SDIO
		 *    interrupt line when firmware has a mailbox message.
		 * 3. SOCRAM trailer at chip-internal 0x237FFC (the NVRAM
		 *    length-magic word): firmware overwrites this after parsing
		 *    NVRAM. */
		for (i = 0; i < 8; ++i) {
			uint32_t ccsr[4] = {0};
			(void)diag_sdioCmd52(sdhci, 0, 1, 0x1000Eu, 0u, ccsr);
			chipclk_samples[i] = (uint8_t)(ccsr[0] & 0xffu);
			usleep(30 * 1000);
		}

		card_intr = (*(volatile uint32_t *)(sdhci + SDHCI_INT_STATUS)
			>> 8) & 1u;

		/* SOCRAM tail trailer: window 19 (0x230000), F1 offset 0x7FF0
		 * = chip-internal 0x237FF0. Read 16 bytes ending at 0x237FFF. */
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Au, 0x00u, NULL);
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Bu, 0x23u, NULL);
		(void)diag_sdioCmd52(sdhci, 1, 1, 0x1000Cu, 0x00u, NULL);
		rc_tail = diag_sdioCmd53Read(sdhci, 1, /*incr=*/1,
			/*reg_addr=*/0x7FF0u, /*block_count=*/1u, /*block_size=*/16u,
			socram_tail);

		/* DEFINITIVE fw-ready probe: read the SDIO-DEV core's
		 * tohostmailboxdata (core base + 0x4C). brcmfmac/WHD treat
		 * HMB_DATA_FWREADY (0x0008) here as THE "firmware booted" signal.
		 * FIXED: use the EROM-enumerated SDIO_DEV base (0x18004000), not the
		 * old 0x18005000 guess (off by 0x1000 -> was reading 0x1800504C). */
		hmb_data = diag_bpRead32(sdhci, sdio_core + 0x4Cu);

		/* #91: read sdpcm_shared @ ram_top-4 (fw overwrites the NVRAM token
		 * with it once booted) -> the fw console ring buffer. Real fw only. */
		if (!g_trivial_mode) {
			diag_readShared(sdhci, ram_size);
		}

		/* #91: BCDC control-ioctl round-trip over F2 (real fw + argv ioctl). */
		if (!g_trivial_mode && g_ioctl_mode) {
			diag_bcdcGetVersion(sdhci, sdio_core);
		}
	}

	munmap(sdhci_page, _PAGE_SIZE);
	munmap(gpio_page, _PAGE_SIZE);

	r = snprintf(buf + off, cap - off,
		"enum: CMD5=%d/%d C=%d RCA=0x%04x CMD7=%d IORDY=0x%02x rdy=%d\n",
		rc_ocr, rc_claim,
		(int)((claim_resp[0] >> 31) & 1u),
		(unsigned)rca, rc_sel,
		(unsigned)(iordy_resp[0] & 0xff), rdy_iters);
	if (r > 0 && (size_t)r < cap - off) {
		off += r;
	}
	(void)rc_iordy;

	r = snprintf(buf + off, cap - off,
		"fw_load: staged %u bytes across %d windows  HS=%d  worst rc_w=%d\n",
		bytes_written, window_idx, rc_hs, worst_rc_w);
	if (r > 0 && (size_t)r < cap - off) {
		off += r;
	}

	r = snprintf(buf + off, cap - off,
		"nvram: %zu bytes -> chip 0x%06x (ram-top 0x%06x from bankinfo)  rc_nvram_w=%d  HT_clk_csr=0x%02x (HT_AVAIL=0x80)\n",
		wifi_nvram_43455_len,
		(unsigned)(((ram_size != 0u) ? (0x198000u + ram_size) : 0x238000u) - (uint32_t)wifi_nvram_43455_len),
		(unsigned)((ram_size != 0u) ? (0x198000u + ram_size) : 0x238000u),
		rc_nvram_w, (unsigned)ht_clk_csr);
	if (r > 0 && (size_t)r < cap - off) {
		off += r;
	}

	r = snprintf(buf + off, cap - off,
		"ARMCR4 IoCtrl pre=0x%02x  post=0x%02x  (expect pre=0x21 CPUHALT+clk, post=0x01 clk-only)\n",
		(unsigned)(rc_pre_resp[0] & 0xff),
		(unsigned)(rc_post_resp[0] & 0xff));
	if (r > 0 && (size_t)r < cap - off) {
		off += r;
	}

	r = snprintf(buf + off, cap - off,
		"rstvec@addr0 readback: %02x %02x %02x %02x  vs fw[0..3]: %02x %02x %02x %02x  -> %s\n",
		rstvec_rb[0], rstvec_rb[1], rstvec_rb[2], rstvec_rb[3],
		fw_img[0], fw_img[1], fw_img[2], fw_img[3],
		(rstvec_rb[0] == fw_img[0] && rstvec_rb[1] == fw_img[1] &&
			rstvec_rb[2] == fw_img[2] && rstvec_rb[3] == fw_img[3])
			? "MATCH (vector placed at true backplane 0)"
			: "MISMATCH (addr-0 write landed elsewhere -- CR4 fetches garbage!)");
	if (r > 0 && (size_t)r < cap - off) {
		off += r;
	}

	if (rc_r_pre == 0 && rc_r_post == 0) {
		pre_match = 0;
		post_match = 0;
		diff_count = 0;
		for (i = 0; i < (int)sizeof(pre_buf); ++i) {
			if (pre_buf[i] == fw_img[i]) ++pre_match;
			if (post_buf[i] == fw_img[i]) ++post_match;
			if (pre_buf[i] != post_buf[i]) ++diff_count;
		}
		r = snprintf(buf + off, cap - off,
			"SOCRAM[0..63] pre vs fw: %d/64 match (load check)\n",
			pre_match);
		if (r > 0 && (size_t)r < cap - off) {
			off += r;
		}
		r = snprintf(buf + off, cap - off,
			"SOCRAM[0..63] post vs fw: %d/64 match  pre-vs-post diff: %d/64 bytes\n",
			post_match, diff_count);
		if (r > 0 && (size_t)r < cap - off) {
			off += r;
		}
		r = snprintf(buf + off, cap - off,
			"  fw[0..7]   %02x %02x %02x %02x %02x %02x %02x %02x\n"
			"  pre[0..7]  %02x %02x %02x %02x %02x %02x %02x %02x\n"
			"  post[0..7] %02x %02x %02x %02x %02x %02x %02x %02x\n",
			fw_img[0], fw_img[1], fw_img[2], fw_img[3],
			fw_img[4], fw_img[5], fw_img[6], fw_img[7],
			pre_buf[0], pre_buf[1], pre_buf[2], pre_buf[3],
			pre_buf[4], pre_buf[5], pre_buf[6], pre_buf[7],
			post_buf[0], post_buf[1], post_buf[2], post_buf[3],
			post_buf[4], post_buf[5], post_buf[6], post_buf[7]);
		if (r > 0 && (size_t)r < cap - off) {
			off += r;
		}
		if (diff_count > 0) {
			r = snprintf(buf + off, cap - off,
				"  -> SOCRAM CHANGED after release: firmware appears to be running\n");
			if (r > 0 && (size_t)r < cap - off) {
				off += r;
			}
		}
		else {
			r = snprintf(buf + off, cap - off,
				"  -> SOCRAM unchanged: firmware may not have started (need NVRAM?)\n");
			if (r > 0 && (size_t)r < cap - off) {
				off += r;
			}
		}
	}

	if (scan_changed_pts >= 0) {
		r = snprintf(buf + off, cap - off,
			"image-scan post vs fw (changed bytes/64 @ +off): "
			"+0x02000=%d +0x10000=%d +0x30000=%d +0x60000=%d +0x90000=%d +0x9C000=%d\n",
			scan_diff[0], scan_diff[1], scan_diff[2], scan_diff[3], scan_diff[4], scan_diff[5]);
		if (r > 0 && (size_t)r < cap - off) {
			off += r;
		}
		r = snprintf(buf + off, cap - off,
			"  -> %d/6 points changed => %s\n",
			scan_changed_pts,
			(scan_changed_pts > 0)
				? "CR4 IS EXECUTING (writing memory) -- gate is observability/early-stall"
				: "no memory writes anywhere -- fw genuinely not running (chase rstvec/activate)");
		if (r > 0 && (size_t)r < cap - off) {
			off += r;
		}
	}

	r = snprintf(buf + off, cap - off,
		"F2 enable: IOR=0x%02x ready=%s @iter=%d (F2_RDY=bit2 0x04)\n",
		f2_ready, ((f2_ready & 0x04u) != 0u) ? "YES" : "no", f2_ready_iters);
	if (r > 0 && (size_t)r < cap - off) {
		off += r;
	}

	r = snprintf(buf + off, cap - off,
		"SDIOD tohostmailboxdata@0x%08x=0x%08x -> %s (HMB_DATA_FWREADY=0x0008; SDIOD base from EROM)\n",
		(unsigned)(sdio_core + 0x4Cu), hmb_data,
		((hmb_data & 0x0008u) != 0u) ? "FWREADY set -- FIRMWARE BOOTED!"
			: ((hmb_data == 0xffffffffu || hmb_data == 0u) ? "0/0xff (no fw signal, or wrong SDIOD base)"
				: "nonzero but no FWREADY bit"));
	if (r > 0 && (size_t)r < cap - off) {
		off += r;
	}

	r = snprintf(buf + off, cap - off,
		"CHIPCLKCSR poll: %02x %02x %02x %02x %02x %02x %02x %02x (HT_AVAIL=bit7 0x80)\n",
		chipclk_samples[0], chipclk_samples[1], chipclk_samples[2],
		chipclk_samples[3], chipclk_samples[4], chipclk_samples[5],
		chipclk_samples[6], chipclk_samples[7]);
	if (r > 0 && (size_t)r < cap - off) {
		off += r;
	}

	r = snprintf(buf + off, cap - off,
		"SDHCI CARD_INTR=%u  SOCRAM-tail rc=%d  trailer[12..15]=%02x %02x %02x %02x (blob trailer=%02x %02x %02x %02x)\n",
		card_intr, rc_tail,
		socram_tail[12], socram_tail[13], socram_tail[14], socram_tail[15],
		wifi_nvram_43455[wifi_nvram_43455_len - 4], wifi_nvram_43455[wifi_nvram_43455_len - 3],
		wifi_nvram_43455[wifi_nvram_43455_len - 2], wifi_nvram_43455[wifi_nvram_43455_len - 1]);
	if (r > 0 && (size_t)r < cap - off) {
		off += r;
	}

	{
		int fw_alive = 0;
		for (i = 0; i < 8; ++i) {
			if ((chipclk_samples[i] & 0x80u) != 0u) {
				fw_alive = 1;
			}
		}
		if (card_intr != 0u) {
			fw_alive = 1;
		}
		r = snprintf(buf + off, cap - off,
			"  -> fw_alive=%d %s\n", fw_alive,
			fw_alive ? "(HT_AVAIL or CARD_INTR asserted -- firmware booted!)"
				: "(no HT_AVAIL / no CARD_INTR -- firmware not confirmed running)");
		if (r > 0 && (size_t)r < cap - off) {
			off += r;
		}
	}

	/* #91 EROM core enumeration: the chip's own answer for every core base /
	 * wrapper, replacing the hardcoded hypotheses. */
	if (g_erom_ncores > 0) {
		uint32_t cr4b = diag_eromCoreBase(BCMA_ID_ARM_CR4);
		uint32_t cr4w = diag_eromCoreWrap(BCMA_ID_ARM_CR4);
		uint32_t sdiob = diag_eromCoreBase(BCMA_ID_SDIO_DEV);
		uint32_t socb = diag_eromCoreBase(BCMA_ID_INTERNAL_MEM);
		int ci;
		r = snprintf(buf + off, cap - off,
			"EROM: eromptr=0x%08x  cores=%d\n"
			"  ARM_CR4(0x83E): core=0x%08x wrap=0x%08x (release-wrap hyp was 0x18102000 -> %s)\n"
			"  SDIO_DEV(0x829): core=0x%08x (mailbox hyp was 0x18005000 -> %s)\n"
			"  INTERNAL_MEM/SOCRAM(0x80E): core=0x%08x (0=absent: 43455 RAM is CR4 TCM)\n"
			"  CR4 TCM ramsize=0x%08x -> ram-top=0x%08x (hardcoded NVRAM top was 0x238000 -> %s)\n",
			(unsigned)g_erom_ptr, g_erom_ncores,
			(unsigned)cr4b, (unsigned)cr4w,
			(cr4w == 0x18102000u) ? "MATCH" : "DIFFERS",
			(unsigned)sdiob,
			(sdiob == 0x18005000u) ? "MATCH" : "DIFFERS(fixed)",
			(unsigned)socb,
			(unsigned)ram_size, (unsigned)(0x198000u + ram_size),
			((0x198000u + ram_size) == 0x238000u) ? "MATCH" : "DIFFERS");
		if (r > 0 && (size_t)r < cap - off) {
			off += r;
		}
		for (ci = 0; ci < g_erom_ncores; ++ci) {
			r = snprintf(buf + off, cap - off,
				"  core[%d] id=0x%03x rev=%u base=0x%08x wrap=0x%08x\n",
				ci, (unsigned)g_erom_id[ci], (unsigned)g_erom_rev[ci],
				(unsigned)g_erom_base[ci], (unsigned)g_erom_wrap[ci]);
			if (r > 0 && (size_t)r < cap - off) {
				off += r;
			}
		}
	}
	else {
		r = snprintf(buf + off, cap - off,
			"EROM: walk failed/skipped (ncores=%d, eromptr=0x%08x)\n",
			g_erom_ncores, (unsigned)g_erom_ptr);
		if (r > 0 && (size_t)r < cap - off) {
			off += r;
		}
	}

	/* #91 CR4-identity cross-check + (when active) the trivial-program test. */
	r = snprintf(buf + off, cap - off,
		"CR4-identity: IOCTL@0x18102408=0x%02x IOCTL@0x18103408=0x%02x "
		"(CPUHALT=0x20; we release 0x18102000)\n",
		(unsigned)ioctl_w2, (unsigned)ioctl_w3);
	if (r > 0 && (size_t)r < cap - off) {
		off += r;
	}

	if (g_trivial_mode) {
		uint32_t cp = (uint32_t)cnt_pre[0] | ((uint32_t)cnt_pre[1] << 8) |
			((uint32_t)cnt_pre[2] << 16) | ((uint32_t)cnt_pre[3] << 24);
		uint32_t cq = (uint32_t)cnt_post[0] | ((uint32_t)cnt_post[1] << 8) |
			((uint32_t)cnt_post[2] << 16) | ((uint32_t)cnt_post[3] << 24);
		uint32_t cq2 = (uint32_t)cnt_post2[0] | ((uint32_t)cnt_post2[1] << 8) |
			((uint32_t)cnt_post2[2] << 16) | ((uint32_t)cnt_post2[3] << 24);
		/* Correct predicate: a known-zero cell that changed => the CR4
		 * executed released code. A second read that CLIMBED => it is still
		 * live (not a static artifact). The seed's top byte (0xC0/0xC1)
		 * corroborates but is NOT required (the counter laps past 0xC0DExxxx
		 * within milliseconds at MHz). */
		int changed = (cq != cp);
		int climbing = (cq2 != cq);
		int seed_corrob = (((cq >> 24) == 0xC0u) || ((cq >> 24) == 0xC1u));
		r = snprintf(buf + off, cap - off,
			"TRIVIAL-PROGRAM TEST (counter @0x%08x, seed 0x%08x):\n"
			"  pre=0x%08x (rc=%d, expect 0)\n"
			"  post1=0x%08x (rc=%d)  post2=0x%08x (rc=%d, +50ms)  delta=%u\n"
			"  changed=%d climbing=%d seed_top_byte_corrob=%d\n"
			"  -> %s\n",
			(unsigned)CR4TINY_COUNTER_ADDR, (unsigned)CR4TINY_COUNTER_MAGIC,
			(unsigned)cp, rc_cnt_pre,
			(unsigned)cq, rc_cnt_post, (unsigned)cq2, rc_cnt_post2,
			(unsigned)(cq2 - cq), changed, climbing, seed_corrob,
			(changed && climbing)
				? "CR4 IS EXECUTING released code (counter live) -- RELEASE PATH WORKS; gate is fw preconditions (NVRAM ram-top/clocks)"
			: changed
				? "counter CHANGED from zero (executed) but 2nd read did not climb -- likely executed then stopped; confirm"
				: "counter unchanged (0) -- CR4 did NOT execute (release path / reset semantics)");
		if (r > 0 && (size_t)r < cap - off) {
			off += r;
		}
	}

	/* #91 sdpcm_shared + firmware console (real fw only). */
	if (!g_trivial_mode && g_shared_valid >= 0) {
		if (g_shared_valid == 1) {
			r = snprintf(buf + off, cap - off,
				"sdpcm_shared @0x%08x VALID (word@ram_top-4=0x%08x, fw booted+overwrote NVRAM token)\n"
				"  flags=0x%08x (ver=%u trap=%s assert_built=%s assert=%s) trap_addr=0x%08x\n"
				"  console_addr=0x%08x log_buf=0x%08x bufsize=%u idx=%u  (console %d bytes below)\n",
				(unsigned)g_sh_addr, (unsigned)g_sh_word,
				(unsigned)g_sh_flags, (unsigned)(g_sh_flags & 0xffu),
				(g_sh_flags & 0x0400u) ? "YES" : "no",
				(g_sh_flags & 0x0100u) ? "yes" : "no",
				(g_sh_flags & 0x0200u) ? "FIRED" : "no",
				(unsigned)g_trap_addr,
				(unsigned)g_console_addr, (unsigned)g_log_buf,
				(unsigned)g_log_bufsize, (unsigned)g_log_idx, g_console_len);
			if (r > 0 && (size_t)r < cap - off) {
				off += r;
			}
			if (g_console_len > 0) {
				int ci;
				r = snprintf(buf + off, cap - off, "----- FW CONSOLE -----\n");
				if (r > 0 && (size_t)r < cap - off) {
					off += r;
				}
				for (ci = 0; ci < g_console_len && (size_t)(off + 2) < cap; ++ci) {
					char c = g_console[ci];
					if (c == '\n' || (c >= 0x20 && c < 0x7f)) {
						buf[off++] = c;
					}
					else if (c != '\0') {
						buf[off++] = '.';
					}
				}
				if ((size_t)(off + 24) < cap) {
					r = snprintf(buf + off, cap - off, "\n----- END CONSOLE -----\n");
					if (r > 0 && (size_t)r < cap - off) {
						off += r;
					}
				}
			}
		}
		else {
			r = snprintf(buf + off, cap - off,
				"sdpcm_shared: word@ram_top-4=0x%08x INVALID (NVRAM-token pattern => fw not booted / no shared)\n",
				(unsigned)g_sh_word);
			if (r > 0 && (size_t)r < cap - off) {
				off += r;
			}
		}
	}

	/* #91 BCDC ioctl round-trip report. */
	if (g_ioctl_ran) {
		int bi;
		r = snprintf(buf + off, cap - off,
			"BCDC ioctl WLC_GET_VERSION: send_rc=%d read_rc=%d  intstatus pre=0x%08x post=0x%08x (I_HMB_FRAME_IND=0x40 @iter=%d)\n"
			"  reply HW len=%u chk=0x%04x valid=%d  doff=%u channel=%u\n"
			"  BCDC flags=0x%08x (id=%u err=%d) status=0x%08x  VERSION=%u\n",
			g_ioctl_send_rc, g_ioctl_read_rc,
			(unsigned)g_ioctl_is_pre, (unsigned)g_ioctl_is_post, g_ioctl_is_iters,
			(unsigned)g_ioctl_hwlen, (unsigned)g_ioctl_hwchk, g_ioctl_reply_valid,
			(unsigned)g_ioctl_doff, (unsigned)g_ioctl_channel,
			(unsigned)g_ioctl_bcdc_flags, (unsigned)(g_ioctl_bcdc_flags >> 16),
			(int)(g_ioctl_bcdc_flags & 0x1u), (unsigned)g_ioctl_bcdc_status,
			(unsigned)g_ioctl_version);
		if (r > 0 && (size_t)r < cap - off) {
			off += r;
		}
		{
			uint16_t plen = (uint16_t)(g_ioctl_pending[0] | (g_ioctl_pending[1] << 8));
			uint16_t pchk = (uint16_t)(g_ioctl_pending[2] | (g_ioctl_pending[3] << 8));
			r = snprintf(buf + off, cap - off,
				"  boot-pending F2 frame: rc=%d HWlen=%u chk=0x%04x valid=%d  bytes:",
				g_ioctl_pending_rc, (unsigned)plen, (unsigned)pchk,
				(plen != 0u && (uint16_t)(~(plen ^ pchk)) == 0u) ? 1 : 0);
			if (r > 0 && (size_t)r < cap - off) {
				off += r;
			}
			for (bi = 0; bi < 24 && (size_t)(off + 4) < cap; ++bi) {
				r = snprintf(buf + off, cap - off, " %02x", g_ioctl_pending[bi]);
				if (r > 0 && (size_t)r < cap - off) {
					off += r;
				}
			}
			r = snprintf(buf + off, cap - off, "\n");
			if (r > 0 && (size_t)r < cap - off) {
				off += r;
			}
		}
		r = snprintf(buf + off, cap - off, "  reply[0..31]:");
		if (r > 0 && (size_t)r < cap - off) {
			off += r;
		}
		for (bi = 0; bi < 32 && (size_t)(off + 4) < cap; ++bi) {
			r = snprintf(buf + off, cap - off, " %02x", g_ioctl_reply[bi]);
			if (r > 0 && (size_t)r < cap - off) {
				off += r;
			}
		}
		r = snprintf(buf + off, cap - off, "\n");
		if (r > 0 && (size_t)r < cap - off) {
			off += r;
		}
	}

	r = snprintf(buf + off, cap - off, ".\n");
	if (r > 0 && (size_t)r < cap - off) {
		off += r;
	}
	return off;
}


int main(int argc, char **argv)
{
	enum { REPORT_CAP = 16u * 1024u };
	char *report;
	int n, ai;

	for (ai = 1; ai < argc; ++ai) {
		if (strcmp(argv[ai], "trivial") == 0) {
			g_trivial_mode = 1;
		}
		else if (strcmp(argv[ai], "ioctl") == 0) {
			g_ioctl_mode = 1;
		}
	}

	report = malloc(REPORT_CAP);
	if (report == NULL) {
		fprintf(stderr, "wifi-probe: out of memory\n");
		return 1;
	}

	n = diag_format_sdio_fwrelease(report, REPORT_CAP);
	if (n < 0) {
		fprintf(stderr, "wifi-probe: report formatting failed (%d)\n", n);
		free(report);
		return 1;
	}

	fwrite(report, 1, (size_t)n, stdout);
	fflush(stdout);

	free(report);
	return 0;
}
