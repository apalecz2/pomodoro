# WebSocket Protocol

The contract for the single WebSocket hub the backend exposes. The device and
the web app are both clients of it: they send **commands**, the backend is the
only party that decides what the timer is doing, and it pushes the result back
to everyone as **state**. See [architecture.md](architecture.md) and
[ADR 0003](adr/0003-backend-as-source-of-truth.md) for the reasoning behind
this split; this document only pins down the wire format.

## Connection & auth

Single endpoint, one message type per socket (no separate device/web
protocols):

```
wss://<host>/ws
```

- **Device** authenticates with a long-lived service token, sent as an
  `Authorization: Bearer <token>` header (or `?token=` query param, since some
  ESP8266 WebSocket clients can't set headers) on the upgrade request.
- **Web app** sits behind Cloudflare Access; the browser's Access session
  cookie authenticates the upgrade request, no separate app-level login.
- A connection that fails auth is rejected at the HTTP upgrade (never reaches
  the message layer below).

## Message envelope

Every message, in both directions, is a single JSON object with a `type`
discriminator:

```json
{ "type": "<message_type>", ... }
```

Timestamps are RFC3339 UTC strings (`"2026-07-08T14:32:00Z"`). Durations sent
over the wire are always milliseconds (`remaining_ms`) or whole seconds when
configuring a session (`duration_s`), never a live countdown.

## Commands (frontend -> backend, device -> backend)

Both clients send the same four command types; the backend doesn't care which
kind of client a command came from, only whether it's valid for the current
state. This is the point of [ADR 0003](adr/0003-backend-as-source-of-truth.md):
a button press and a web click follow one path.

| type      | fields                                          | who sends it |
|-----------|--------------------------------------------------|--------------|
| `start`   | `session_type: "work" \| "break"`, `duration_s?: int` | web app (device's toggle button sends `start` or `resume`, see below) |
| `pause`   | *(none)*                                          | web app, device (toggle button while running) |
| `resume`  | *(none)*                                          | web app, device (toggle button while paused) |
| `reset`   | `session_type: "work" \| "break"`                 | web app, device (reset-work / reset-break buttons) |

```json
{ "type": "start", "session_type": "work", "duration_s": 1500 }
{ "type": "pause" }
{ "type": "resume" }
{ "type": "reset", "session_type": "break" }
```

Notes:

- `duration_s` is optional on `start`. Omit it to use the configured default
  for `session_type`; the web app sends it explicitly to support custom
  durations (requirement 3).
- The device has one physical **toggle** button, not separate start/pause
  buttons. Firmware decides which command to send by checking the `status` in
  the last `state` message it received: send `start` if `idle`, `pause` if
  `running`, `resume` if `paused`. The backend is still the authority: if the
  device's cache is stale and the command doesn't fit the current state, the
  backend rejects it (see Errors) rather than trusting the client's guess.
- The device's two reset buttons map directly to `reset` with a fixed
  `session_type` (work button always sends `"work"`, break button always
  sends `"break"`).
- `reset` stops the timer (`status` -> `idle`) and sets the display to the
  full duration for `session_type`; it does not start it running.
- Invalid transitions (e.g. `pause` while `idle`, `resume` while `running`)
  get an error reply to the sender and no state change:
  `{ "type": "error", "message": "..." }`.

## Broadcast state (backend -> all clients)

The only message the backend ever pushes proactively. Sent to **every**
connected client whenever state changes (a command was applied) and once to a
client immediately after it connects (see Reconnect handshake below).

```json
{
  "type": "state",
  "status": "idle" | "running" | "paused",
  "end_time": "2026-07-08T14:57:00Z" | null,
  "remaining_ms": 1500000 | null,
  "session_id": "a1b2c3" | null
}
```

Field rules by `status`, so exactly one of `end_time` / `remaining_ms` is
meaningful at a time, never both:

| status    | `end_time`               | `remaining_ms`                          | `session_id`            |
|-----------|---------------------------|------------------------------------------|--------------------------|
| `idle`    | `null`                     | full duration for the next session, static | `null` (no session yet) |
| `running` | future timestamp           | `null`                                    | current session's id     |
| `paused`  | `null`                     | frozen remaining time, static             | current session's id     |

## Remaining time is always derived

`remaining_ms` is never a live, ticking value pushed over the socket. A client
computes what to display as:

- `running`: `remaining = end_time - now` (recomputed locally every tick,
  against the client's own clock).
- `paused` or `idle`: `remaining = remaining_ms` as given, unchanging until
  the next `state` message.

This is why the device must keep its clock NTP-synced (at boot and
periodically): its derived countdown only agrees with the backend's if both
sides agree on "now". The backend itself finalizes a session past `end_time`
on its own clock, independent of whether any client is connected.

## Reconnect handshake

The device (or web app) can drop and reconnect at any time: WiFi drop,
tunnel reset, browser refresh. On every new connection:

1. Client completes the WebSocket upgrade (auth as above).
2. Client sends a `sync` message, its first message on the new connection:
   ```json
   { "type": "sync" }
   ```
3. Backend replies to that client with the current `state` message
   (same shape as any other broadcast).
4. Client discards whatever it had cached locally and adopts this state as
   truth, per [ADR 0003](adr/0003-backend-as-source-of-truth.md).

The device does not need to persist state across a reconnect for correctness.
Worst case, the display briefly shows the last thing it rendered before the
`sync` reply arrives, then snaps to the authoritative state.