#!/usr/bin/env python3
# Regenerate tools/wifi-probe/clm-43455.h from the (gitignored, EULA'd) CLM blob.
import sys
b = open(".firmware/brcmfmac43455-sdio.clm_blob","rb").read()
out = "tools/wifi-probe/clm-43455.h"
with open(out,"w") as f:
    f.write("/* AUTO-GENERATED from .firmware/brcmfmac43455-sdio.clm_blob (Cypress EULA — gitignored).\n")
    f.write(" * Regenerate: python3 tools/wifi-probe/gen-clm.py  */\n")
    f.write("#ifndef CLM_43455_H\n#define CLM_43455_H\n#include <stdint.h>\n\n")
    f.write("static const uint32_t clm_43455_len = %du;\n" % len(b))
    f.write("static const uint8_t clm_43455[%d] = {\n" % len(b))
    for i in range(0, len(b), 16):
        f.write("\t" + ", ".join("0x%02x" % x for x in b[i:i+16]) + ",\n")
    f.write("};\n\n#endif\n")
print("wrote", out, len(b), "bytes")
