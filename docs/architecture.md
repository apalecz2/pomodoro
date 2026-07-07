# Architecture & System Design


## 3 Units:

### Firmware (ESP8266) -- Executor and Display

- Drives display with shift register to multiplex and not use all GPIO
- Reads 3 buttons for toggle timer running / off, reset of work, reset of break
- Holds websocket to backend, authenticates with service token
- Local cache of end time and renders the end time - now -- time remaining to the display. Not authoritative

### Backend (Go, running on home server behind cloudflare tunnel in a docker container)

- Source of truth -- owns all state, validates commands, computes the end time, broadcasts this over websocket
- Persists session to postgres, finalizes session on its clock

### Web App (React / TS)

- Control plane / surface and dashboard
- Sends commands, renders live state from the backend to mirror the physical display
- Shows history and progress bars / pomodoro completion stats
- Behind cloudflare access (SSO)




## Relation between units:

- Backend owns countdown, the device and frontend are just clients
- Running state described by { status, end_time, remaining_ms, session_id }
- Remaining time is always derived (end_time - now), not stored or sent as live number
- Device and frontend stay in sync because they both derive from the same timestamp against network time protocol clock
- On reconnect the device adopts the backend state for whatever it is, and discards its local guess that was derived from the cache


## Stack:
- Go
- Postgress
- Websocket
- React + TS
- Cloudflare tunnels
- Docker


## Hardware:
- Microcontroller: ESP8266 -- ESP8266 WiFi Development Board (2A4RQ-ESP8266)
- Timer display: 5461AS -- 4-digit 7-segment display
- Shift register (74HC595)



## Data Model:

- Session: id, start_ts, end_ts, planned_duration, actual_duration, type (work/break), status (running/completed/aborted), goal, note, date
- Settings: default work, default break, daily target (per day of week)