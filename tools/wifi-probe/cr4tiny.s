	.syntax unified
	.thumb
	.text
	.global _start
_start:
	movw r0, #0x9000      @ counter address low  (0x199000)
	movt r0, #0x0019      @ counter address high -> r0 = 0x00199000
	movw r1, #0x0001      @ magic low
	movt r1, #0xC0DE      @ magic high -> r1 = 0xC0DE0001
1:
	str  r1, [r0]         @ *(0x199000) = r1
	adds r1, r1, #1       @ r1++
	b    1b               @ spin forever, incrementing
