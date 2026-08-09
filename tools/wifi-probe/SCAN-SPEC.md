# BCM43455 WiFi scan via raw BCDC ioctls — byte-level spec

Verified against brcmfmac (external/linux .../brcm80211/brcmfmac). Command payloads
are **little-endian**; event-frame `brcmf_event_msg_be` fields are **big-endian**.

## iovar mechanism
An iovar = a BCDC dcmd with `cmd = SET_VAR(263)` / `GET_VAR(262)`, payload =
`"name\0"` + data. GET reply overwrites the buffer from offset 0 (name consumed).
Command ids (fwil.h): UP=2, GET_VERSION=1, SET_VAR=263, GET_VAR=262, SCAN=50, SCAN_RESULTS=51.

## Minimal prelude (in order)
1. **event_msgs** (REQUIRED): SET_VAR `"event_msgs\0"` + byte-mask; set bit 69
   (WLC_E_ESCAN_RESULT): `mask[69/8=8] |= 1<<(69%8=5) = 0x20`. Robust: GET first to
   learn fw mask_len, OR the fw returns its mask on GET. Hardcode 16-byte mask:
   `00×8, 0x20, 00×7` works (covers 0..127).
2. **WLC_UP** (REQUIRED): SET cmd=2, 4-byte le32 (value ignored).
3. **mpc=0** (REQUIRED on SDIO/43455): SET_VAR `"mpc\0"` + le32 0 (keep radio awake).

## escan (V1 for 7.45.x fw — predates the `scan_ver` iovar that gates V2)
SET_VAR `"escan\0"` + 108-byte `brcmf_escan_params_le` (broadcast ACTIVE, all channels):
```
off  field              bytes            value
 0   version   le32      01 00 00 00      BRCMF_ESCAN_REQ_VERSION=1
 4   action    le16      01 00            WL_ESCAN_ACTION_START=1
 6   sync_id   le16      34 12            0x1234
 8   ssid_len  le32      00 00 00 00
12   ssid[32]            all 00
44   bssid[6]            ff ff ff ff ff ff  broadcast
50   bss_type  u8        02               DOT11_BSSTYPE_ANY
51   scan_type u8        00               ACTIVE (MUST be 0)
52   nprobes   le32      ff ff ff ff      -1 default
56   active_t  le32      ff ff ff ff
60   passive_t le32      ff ff ff ff
64   home_t    le32      ff ff ff ff
68   channel_num le32    00 00 01 00      n_channels=0(all), n_ssids=1
72   ssid_le[0]          36×00            ONE wildcard ssid (len 0) => active broadcast
```
NUANCE: active broadcast = exactly ONE zero-length ssid (n_ssids=1), NOT n_ssids=0
(that forces passive). channel_num low16=chan count (0=all), high16=ssid count.

## Results: WLC_E_ESCAN_RESULT events (type 69) on SDPCM channel 1
Frame stack (offsets from SDPCM payload start = frame + sdpcm_data_offset[byte7], normally 12):
```
+0   BDC header 4B: flags/prio/flags2/data_offset. Skip 4 + 4*data_offset (data_offset normally 0 for events).
+4   ethhdr 14B: h_proto @+12 (be16) must == 0x886C (ETH_P_LINK_CTL)
+18  brcm_ethhdr 10B: oui @+ (bytes) must == 00 10 18
+28  brcmf_event_msg_be 48B (BIG-ENDIAN):
       event_type be32 @+28-from-ethhdr  (==69 for ESCAN_RESULT)
       status     be32 @+32-from-ethhdr
       datalen    be32 @+44-from-ethhdr
+72(from ethhdr) escan payload = brcmf_escan_result_le (LE): buflen le32@0, version@4,
       sync_id le16@8, bss_count le16@10 (MUST==1), bss_info_le @12
```
Simplest absolute offsets from the SDPCM frame start, assuming sdpcm doff=12, bdc doff=0:
ethhdr @16, event_type @16+28=44 (be32), status @16+32=48 (be32),
escan payload @16+72=88, bss_info_le @88+12=100.

**brcmf_bss_info_le** (LE, size 128), offsets from bss_info start (=frame+100):
```
 0  version le32
 4  length  le32  (walk to next record)
 8  BSSID[6]
16  capability le16
18  SSID_len u8
19  SSID[32]
72  chanspec le16  channel = chanspec & 0xFF
78  RSSI     le16  SIGNED int16 dBm
88  ctl_ch   u8    plain control channel (alt to chanspec)
```
Extract per AP: BSSID@+8, SSID@+19 (len @+18), RSSI=(int16)le16@+78, channel=le16@+72 & 0xFF.

## Completion
status field (big-endian, @ethhdr+32): PARTIAL=8 (one AP, keep reading),
SUCCESS=0 (done), ABORT=4 (failed). STOP when status != PARTIAL(8).

## End-to-end recipe
1. GET_VAR "event_msgs" (learn mask_len)  2. SET_VAR "event_msgs" (bit8|=0x20)
3. SET UP (cmd 2)  4. SET_VAR "mpc" 0  5. SET_VAR "escan" (108B V1 params)
6. read chan-1 frames, filter h_proto=0x886C + oui 00:10:18 + event_type=69,
   on status==8 parse bss_info -> AP; stop on status!=8.  7. (opt) SET_VAR "mpc" 1.

43455 notes: escan V1; skip event_msgs_ext (43012-only); mpc=0 matters; BDC header
present on event channel; one AP per ESCAN_RESULT event.
