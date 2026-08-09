#!/usr/bin/env python3
# Generate tools/bt-probe/bt-hcd.h from the Pi4B BT patch-RAM firmware (Cypress
# EULA -> gitignored). Source: the netboot Linux rootfs's brcm/ .hcd for the
# 4-model-b (BCM4345C0). The .hcd is a raw sequence of HCI records
# [opcode_le16][plen][params]; the probe replays them for patchram.
import sys
src = "artifacts/linux-netboot/rootfs/usr/lib/firmware/brcm/BCM4345C0.raspberrypi,4-model-b.hcd"
b = open(src, "rb").read()
out = "tools/bt-probe/bt-hcd.h"
with open(out, "w") as f:
    f.write("/* AUTO-GENERATED from %s (Cypress EULA -- gitignored).\n" % src)
    f.write(" * Regenerate: python3 tools/bt-probe/gen-bt-hcd.py  */\n")
    f.write("#ifndef BT_HCD_H\n#define BT_HCD_H\n#include <stdint.h>\n\n")
    f.write("static const uint32_t bt_hcd_len = %du;\n" % len(b))
    f.write("static const uint8_t bt_hcd[%d] = {\n" % len(b))
    for i in range(0, len(b), 16):
        f.write("\t" + ", ".join("0x%02x" % x for x in b[i:i+16]) + ",\n")
    f.write("};\n\n#endif\n")
print("wrote", out, len(b), "bytes")
