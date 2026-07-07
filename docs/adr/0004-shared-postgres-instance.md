# ADR 0004: Share the existing Postgres instance with a dedicated database and role

## Status
Accepted

## Context
My home server already runs a Postgres instance in Docker, used by other
services managed through a separate Docker Compose stack. This project needs
persistence for timer sessions and settings. The options were: run a second,
dedicated Postgres container for this project, or reuse the existing instance.
If reusing, the data must still be isolated from the other services.

## Decision
Reuse the existing Postgres instance, but create a dedicated database
(`pomodoro`) and a dedicated role scoped with least privilege to only that
database. This project lives in its own repository with its own
`docker-compose.yml`, which connects to the existing Postgres over a shared
external Docker network rather than declaring its own Postgres service. Schema
changes are managed with version-controlled migrations (`golang-migrate` or
`goose`(?)).

## Consequences
- Avoids running a redundant database engine on the same host while keeping the
  project's data logically isolated at the database and role level, which is the
  production-shaped choice.
- The project remains self-contained and cloneable: its Compose file joins the
  existing Postgres via an external network, and does not entangle with the other
  stack's Compose configuration.
- Sharing an instance makes disciplined, version-controlled migrations
  mandatory rather than optional, since ad-hoc schema changes on a shared server
  are risky.
- Trade-off: a noisy or resource-heavy neighbor on the shared instance could
  affect this service, and the two stacks now share a Postgres lifecycle
  (upgrades, restarts). Acceptable for a single-user home-server project.
- Postgres stays internal to the Docker network and is never exposed publicly;
  only the backend reaches it.