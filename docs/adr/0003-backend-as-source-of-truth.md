# ADR 0003: Backend as the source of truth for timer state

## Status
Accepted

## Context
Both the physical device and the web app can start, pause, reconfigure, and reset
the timer, so the system needs a single authority to avoid drift and conflicting
state. Two models were considered: the device owns the running countdown (it is
physically doing the counting), or the backend owns all state and the device is a
display-and-executor client. The connection can also drop (WiFi flaps, tunnel
resets, device reboots), so the model must survive the device losing contact.

## Decision
The backend is authoritative for all state, including the running countdown
(Option A). Running state is fully described by `{ status, end_time,
remaining_ms (if paused), session_id }`. Remaining time is always derived as
`end_time - now`, never sent or stored as a live decrementing number. Commands
from either the device or the web app flow to the backend, which updates the
truth and broadcasts the new state to all clients. The device keeps a local copy
only as a display cache and defers to the backend on reconnect.

## Consequences
- A physical button press and a web-app action follow the same path
  (command -> backend -> broadcast), so there is no special-casing by origin and
  the reconciliation logic stays simple.
- Because both device and web app derive remaining time from the same
  `end_time` timestamp, they stay in agreement by construction even without
  communicating during a brief drop.
- Requires the device to keep a clock synchronized via NTP (at boot and
  periodically); otherwise its derived countdown diverges from the backend's.
- The backend can finalize a session on its own clock when `now > end_time`,
  so completions are recorded even if the device is offline at the moment the
  timer ends.
- Pause cannot be represented by `end_time` alone; it must freeze `remaining_ms`,
  and resume recomputes `end_time = now + remaining_ms`.
- Race conditions can be present and need to be accounted for on the backend