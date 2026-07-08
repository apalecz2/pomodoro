# Backend

Go service that owns timer state and serves it over WebSocket. See
[docs/websocket-protocol.md](../docs/websocket-protocol.md) for the message
contract and [docs/architecture.md](../docs/architecture.md) for how this
fits with the device and web app.

## Run

```bash
go run .
```

Listens on `:8080`.

- `GET /healthz` - liveness check
- `GET /ws` - WebSocket endpoint, currently broadcasts a hardcoded state
  message every 2 seconds

## Develop

```bash
go build ./...
go vet ./...
go test ./...
```
