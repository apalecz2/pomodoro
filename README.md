# Pomodoro Timer

A physical pomodoro timer with a 7-segment display, synced live over the internet to a web dashboard.

<!-- TODO: demo gif -->

## What it does

- Physical device (ESP8266 + 4-digit 7-segment display) shows the live countdown
- Buttons on the device toggle the timer and reset work/break sessions
- A Go backend is the single source of truth for timer state and persists sessions to Postgres
- A React web app mirrors the device in real time and shows history and daily progress

## Architecture at a glance

Three units (firmware, backend, web app) all stay in sync because the backend owns the countdown and broadcasts `{ status, end_time, session_id }` over WebSocket; every client derives remaining time from `end_time - now` rather than trusting a pushed number. See [docs/architecture-system-design.md](docs/architecture-system-design.md) for the full design.

## Tech stack

Go · PostgreSQL · WebSocket · React + TypeScript · ESP8266 · Cloudflare Tunnels · Docker

## Status

Early development

## Repo layout

```
firmware/   ESP8266 device code (PlatformIO)
backend/    Go WebSocket server + Postgres persistence
web/        React + TS control plane / dashboard
docs/       Architecture notes and ADRs
```
