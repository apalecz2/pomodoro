# ADR 0001: WebSocket for device–backend real-time transport (over MQTT)

## Status
Accepted

## Context
The physical timer (ESP8266) and the backend need low-latency, bidirectional
communication: the device must receive commands (start, pause, reset, custom
duration) and the backend must push authoritative state to the device and web
app the moment it changes. Plain HTTP polling introduces visible latency and is
the weakest option to demonstrate. The two credible real-time transports are
MQTT (the IoT choice, pub/sub via a broker) and WebSocket (a persistent
connection the device holds directly to the backend).

## Decision
Use WebSocket as the transport between the device, backend, and web app. The
device holds a persistent WebSocket connection to the backend; the backend
broadcasts state to all connected clients over the same protocol.

## Consequences
- One fewer moving part than MQTT: no broker to run, secure, or bridge into the
  web backend. The backend is the single hub for all clients.
- WebSocket is a broadly recognized web technology, so the design reads clearly
  to a general software audience and demonstrates web-relevant skills, which
  matches the project's goal better.
- The connection lifecycle is more interesting and I can talk about it during interviews. 
  Heartbeat (ping/pong), reconnect-with-backoff, and state reconciliation on reconnect,
  which must be handled explicitly rather than delegated to a broker's QoS.
- Trade-off: MQTT would have been the more "correct" answer for a device-heavy
  or embedded-targeted system, and its pub/sub decoupling is lost. For a
  single-device project with a general SWE audience, that decoupling is not
  needed and the simpler topology wins.