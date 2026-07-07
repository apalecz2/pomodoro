# ADR 0002: Go for the backend service

## Status
Accepted

## Context
The backend is the source of truth: it manages timer state, serves a WebSocket
hub, persists sessions to Postgres, and runs behind a Cloudflare Tunnel in a
Docker container. The frontend was already decided to be React/TypeScript, so staying in
TypeScript everywhere was the low-risk option. The competing goal is that this
is a portfolio project intended to build experience with new, backend-credible
technology for new-grad SWE roles.

## Decision
Implement the backend in Go rather than extending the TypeScript stack to the
server.

## Consequences
- Introduces a substantial new technology to the project. Go is
  well-regarded for backend work, compiles to a single static binary that
  containerizes cleanly, and its concurrency model (goroutines/channels) is a
  natural fit for a WebSocket hub broadcasting to multiple clients.
- Keeps learning focused: Go is the one new thing, so novelty is not
  spread thin across many unfamiliar tools.
- Trade-off: loses end-to-end type sharing with the React/TS frontend; the
  WebSocket message contract must be kept in sync manually via the protocol
  document rather than shared types.
- The concurrency and single-binary deployment story is itself a talking point,
  and a small, idiomatic, standard-library-heavy Go service reads better to
  reviewers than a larger framework-driven one.