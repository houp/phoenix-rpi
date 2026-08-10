# Quake 1 multiplayer #68 — diagnosis plan

**KNOWN-ISSUES #68:** "Quake multiplayer hangs at the LOADING screen" (open;
single-player + demos work). Owner's #1-listed continue task. quakespasm port.

## Infra built (2026-08-10)

A headless host dedicated NetQuake server: `scripts/quake-mp-server.sh` builds
`external/quakespasm/Quake/quakespasm` (same codebase as the Phoenix client →
matching NetQuake protocol) and runs it `-dedicated` with the SDL dummy drivers,
binding `0.0.0.0:26000`. Verified running + bound; reachable from the Pi over the
netboot network at the host IP (10.42.0.1). The Pi client connects with
`connect 10.42.0.1`.

## Net-path analysis (what #68 is NOT, and where it is)

Read the client UDP path in `external/quakespasm/Quake/net_udp.c`:
- **The client UDP socket is NON-BLOCKING** (`UDP_OpenSocket` → `ioctlsocket(FIONBIO)`;
  the port confirms FIONBIO is implemented on Phoenix, task #26).
- **`UDP_Read` busy-polls** `recvfrom` every frame; `NET_EWOULDBLOCK → return 0`.
- So the client net loop does **not** use `poll()`/`select()`.

**→ #68 is NOT the Phoenix `poll()`-readiness stall** ([[project_nfs_poll_stall_fix]]
is a red herring here — that path never runs for quakespasm's UDP). The client
spins the LOADING screen busy-polling `recvfrom`; it hangs because the **signon /
precache message exchange never completes**, i.e. one of:
1. **recv:** the server's signon packets never come back from `recvfrom` on Phoenix
   (UDP delivery / source-addr / bind-vs-connect / large-datagram issue), or
2. **send/ack:** the client's reliable-message ACK send fails, so the server never
   advances to the next signon block, or
3. **large datagram / fragmentation:** signon precache blocks are large (~near the
   NET_MAXMESSAGE / MTU limit); if Phoenix lwIP drops/mis-handles a large or
   fragmented UDP datagram, the signon payload is lost.

Single-player works because it uses the loopback landriver (`net_loop.c`), no real
UDP — consistent with the fault being in the real-UDP signon exchange.

## Next-step test (a fresh heartbeat)

1. Build a Pi quakespasm client that, instead of the hardcoded boot map, runs
   `connect 10.42.0.1` at startup (mirror the port's boot-command hook), with net
   logging: in `net_dgrm.c` (`Datagram_Connect` / the signon-driving
   `_Datagram_CheckMessage` / `NET_GetMessage`) and `UDP_Read`, log each read
   result + byte count + the signon state (`cls.signon`, `msg` reliable seq/ack).
2. Run `./scripts/quake-mp-server.sh start` on the host; netboot the Pi client;
   capture over UART where it stalls:
   - Does `UDP_Read` ever return the server's connection-accept + signon packets?
     (recv side works vs not.)
   - Does the client's ACK/reliable send succeed? (send side.)
   - What byte size are the signon blocks that arrive vs stall? (large-datagram.)
3. Compare vs Linux-on-Pi4 (owner directive): if a Linux client joins the same
   server fine and the Phoenix client doesn't, it's a Phoenix UDP/lwIP bug → fix
   at the socket/lwip level (e.g. large-datagram RX, or the connected-socket recv
   filter). If Linux also struggles → protocol/config.

The fix likely lands in the lwIP UDP path or a small net_udp/net_dgrm adaptation,
depending on which of (1)/(2)/(3) the capture shows.
