# Making the Pi 4 xHCI driver ready for more than one device class

Owner directive, 2026-09-20: the port **will** grow USB audio, USB video capture, USB
disks, USB ethernet and USB WLAN. Do the general work now rather than per class.

## What the driver is today, stated plainly

`xhci_transferEnqueue()` is not a dispatcher, it is a **chain of `if` statements grown
one scenario at a time** — root-hub emulation, control on an addressed device, control
behind a non-root hub, interrupt-IN, and (since today) bulk. Each arm re-tests an
overlapping set of conditions on `pipe->dev->hub`, `pipe->dev->hub->hub`,
`pipe->dev->address`, `setup->bmRequestType` and `t->type`. Anything not matching an arm
falls off the end as `-ENOSYS`.

That structure is exactly why USB mass storage failed the way it did: bulk was not
*refused*, it was **unrepresented**. `usb_open()` returned a valid pipe because the
framework allocates pipes and only the HCD knows the endpoint was never configured in
hardware, so the first symptom was twenty failed transfers on a healthy device. The next
class to be added will hit the same shape.

## The four things worth generalizing, in dependency order

### 1. Endpoint types: finish the table (small, mechanical)
xHCI defines 8 endpoint types. The driver knows three:

| EP type | value | status |
|---|---|---|
| Isoch OUT | 1 | ❌ absent — **USB audio + video capture need this** |
| Bulk OUT | 2 | ✅ added 2026-09-20 |
| Interrupt OUT | 3 | ❌ absent |
| Control | 4 | ✅ |
| Isoch IN | 5 | ❌ absent — **USB audio capture + UVC need this** |
| Bulk IN | 6 | ✅ added 2026-09-20 |
| Interrupt IN | 7 | ✅ |

Interrupt-OUT is a one-line addition to the type selection. Isochronous needs more than a
type value (Isoch TRBs carry a Frame ID, and Mult / Max Burst / Max ESIT Payload actually
matter), but the *endpoint-context* half generalizes with the rest.

### 2. Replace the if-chain with a real dispatch (the load-bearing change)
Decide once, at the top: root hub → emulation; `pipe->num == 0` → the control path;
otherwise → the generic pipe path, selected by endpoint type. A new class then needs a
*transfer-type* implementation, not a new arm in a 200-line conditional, and an
unsupported combination fails at `xhci_initPipe` with a named reason instead of silently
reaching the bottom of the function.

### 3. Completion latency — this one also blocks the throughput measurement
Completions are reaped by `xhci_roothubStatusThread`, which sleeps **1 ms** whenever any
transfer is outstanding (100 ms otherwise). Every synchronous transfer therefore pays up
to 1 ms *after the controller is already done*.

For mass storage that is not a detail. One SCSI command is three transfers (CBW out, data,
CSW in) and umass moves at most 4 KiB per command, so the poll alone caps the device at
roughly **1.3 MB/s** — against ~35 MB/s for the bus. Any throughput number measured before
this is fixed measures the poll loop, not the stick.

Two ways out, in increasing order of value:
- Have the submitting thread await its own completion inline. `xhci_eventAwait()` is
  already locked and stash-aware precisely so more than one consumer can use it, so this
  is a modest change and removes the hand-off entirely.
- Drive the interrupter properly (MSI/legacy IRQ) instead of polling the event ring. The
  correct end state, and what every other class will want too — an isochronous audio
  stream cannot be serviced from a 1 ms poll.

### 4. Queue depth per endpoint
`xhci_pipePriv_t` holds a single `pendingTransfer` and returns `-EBUSY` for a second. One
transfer in flight per endpoint is survivable for a keyboard and costly for a disk or a
NIC, where the point is to keep several TDs on the ring at once. The ring and the producer
index already support it; what is missing is a small in-flight list keyed by the TRB
address the event will report.

## Sequencing, deliberately

Bulk goes to hardware **first**, unchanged, and only then does the refactor happen. The
if-chain is ugly but it encodes real, hard-won knowledge about this controller — the
comments on each arm record specific failures (Context State Error after a retry, the
addressed-vs-slot-id distinction, why ERDP must not be touched per-submit). Rewriting it
without a working bulk path to test against would risk trading a known-good control and
HID path for a cleaner-looking one that regresses the keyboard.
