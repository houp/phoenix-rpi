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

## ★ 2026-08-10 RESULT — #68 LOCALIZED to the NET_Connect silent-slist loop

Built a Pi client that auto-connects (via `id1/phoenix-connect.cfg` → `connect
<ip>`, added to pl_phoenix_main.c) + net-trace logging, ran it against the host
server. UART trace:
```
PHXNET68: boot connect -> 10.42.0.1
Playing demo from demo1.dem.
PHXNET68: NET_Connect host=10.42.0.1
PHXNET68: NET_Connect -> silent slist (SearchForHosts broadcast)
   <hangs here — no "slist done", no JustDoIt, no _Datagram_Connect>
```
**The connect reaches `NET_Connect` (net_main.c:415) and hangs in its silent
server-list phase:** `slistSilent = true; NET_Slist_f(); while (slistInProgress)
NET_Poll();` — the `while (slistInProgress) NET_Poll()` loop never terminates on
Phoenix (`slistInProgress` never clears), so the client spins there forever =
"hangs at LOADING." It never reaches `_Datagram_Connect` (the actual TCP-less
handshake) or signon. FitzQuake/quakespasm does a broadcast `SearchForHosts`
slist before *every* connect (to resolve the host into the cache).

**So #68 is NOT the signon exchange** (as first hypothesized) — it's earlier, in
the pre-connect host-discovery slist. Two candidate roots:
1. **UDP broadcast** send/recv broken on Phoenix lwIP (SearchForHosts broadcasts
   to the subnet; if the broadcast never goes out or no response, but the slist
   completion still hinges on it), or
2. the **slist timeout** never fires (`slistInProgress` is cleared on a
   `SetNetTime()`-based deadline in `_Datagram_SearchForHosts`/`NET_Poll`; if
   net_time doesn't advance or the deadline logic stalls, the loop is infinite).

Also confirmed: the connect competes with the demo loop (startdemos runs first),
but the connect DID run — the hang is the slist, not the demo.

## Next-step test/fix (a fresh heartbeat)

1. Add logging inside `NET_Slist_f` / `_Datagram_SearchForHosts` / `NET_Poll`:
   log `slistInProgress`, the deadline vs `net_time`, and whether the broadcast
   send + any response happen. One netboot cycle → is it (1) broadcast or (2)
   timeout?
2. Compare vs Linux-on-Pi4 (owner directive): a Linux quakespasm client slists
   fine → confirms a Phoenix UDP-broadcast/lwIP bug → fix it (lwip broadcast RX/TX
   or SO_BROADCAST).
3. Pragmatic parallel fix: for a **direct IP** `connect`, the slist is
   unnecessary — skip `NET_Slist_f` and go straight to `JustDoIt`/`_Datagram_Connect`
   with the given address (a small net_main.c change gated on "host is a literal
   addr"). That both fixes #68 for direct connects and sidesteps the broadcast
   dependency; the slist bug is then fixed separately for LAN discovery.

--- original plan below (signon hypothesis, now superseded by the slist finding) ---
## Next-step test (original)

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
