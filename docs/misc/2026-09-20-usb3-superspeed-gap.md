# The Pi 4 already has a SuperSpeed link we are not using

Owner, 2026-09-20: *"the stick is in the blue USB3 port... it SHOULD support full USB 3.0
speeds which should be more than 30MB/s. Please set a new goal to try maxing the speed."*

Correct, and the hardware agrees. **The SuperSpeed link is already established.**

## The evidence, from the board

```
xhci: protocol 'USB ' 2.0 ports 1..1
xhci: protocol 'USB ' 3.0 ports 2..5
xhci: port 1 PORTSC=0x400202e1 ccs=1 ped=0 speed=0 pls=7
xhci: port 2 PORTSC=0x00281203 ccs=1 ped=1 speed=4 pls=0
xhci: port 3..5                ccs=0
```

Port 2 belongs to the **USB 3.0** protocol range, is **connected** (CCS=1), **enabled**
(PED=1), running at **speed 4 = SuperSpeed Gen1**, in link state **U0**. A SuperSpeed port
trains itself; the controller had done it before our driver looked.

And yet the device we talk to is the USB 2 one:

```
usb: New device: 2109:3431 Generic, USB2.0 Hub (2, 00000011)
usb: New device: 0781:5581 SanDisk, SanDisk Ultra (3, 00000111)
xhci: bulk-OUT pipe ready slot=2 ep=4 maxpkt=512
```

`maxpkt=512` is High-Speed bulk; SuperSpeed bulk is **1024**. A USB 3 device in a USB 3
port has both personalities physically present — SS pins to the SS root port, USB 2 pins to
the VL805's internal hub. We enumerate the USB 2 one and never look at port 2.

**So the 29.3 MB/s ceiling is USB 2.0 line rate, and it is self-inflicted.** Nothing about
the stick or the port imposes it.

## Why the driver misses it

Two structural reasons, both in `usb/xhci/xhci.c`:

1. **No notion of SuperSpeed at all.** It defines `XHCI_PORT_SPEED_{FULL,LOW,HIGH}` — 1, 2,
   3 — and nothing for 4. Wherever a speed is mapped, SuperSpeed falls through.
2. **It never read the Supported Protocol capability** until this diagnostic, so it had no
   way to know ports 2..5 are USB 3 and port 1 is USB 2. Every root port was treated as one
   flat USB 2 set.

## What the work looks like

- **Do not reset an already-enabled SS port.** USB 2 enumeration is "reset to enable"; a
  SuperSpeed port arrives already `PED=1` in `U0`, and a USB2-style reset would knock a
  working link down. The enumeration path must branch on the protocol the port belongs to.
- **Speed in the Slot Context** must carry 4, and the **route string** must address the SS
  root port rather than a path through the USB 2 hub.
- **Bulk endpoints at SuperSpeed** use `wMaxPacketSize` **1024** and gain a **SuperSpeed
  Endpoint Companion descriptor** carrying `bMaxBurst` (and `bmAttributes` MaxStreams). Max
  Burst must be programmed into the endpoint context — that is where most of the throughput
  comes from: a burst of N moves N packets per handshake.
- **Prefer the SS personality.** Once port 2 enumerates, the same physical stick must not be
  driven through the USB 2 hub as well.

## What to expect

A SanDisk Ultra is typically ~100 MB/s read / ~30 MB/s write. USB 3.0 Gen1 is 5 Gbps
(~400 MB/s usable), so the device becomes the limit, not the bus. Realistically a **3x or
better read improvement** over the current 29.3 MB/s raw, with writes bounded by the stick's
own NAND.

⚠ Grade it by `dd`'s own reported rate on the raw device, as every other number here, and
confirm the lane by `maxpkt=1024` in the pipe-ready line before believing any of it.
