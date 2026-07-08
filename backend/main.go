package main

import (
	"encoding/json"
	"log"
	"net/http"
	"time"

	"github.com/gorilla/websocket"
)

const broadcastInterval = 2 * time.Second

// stateMessage mirrors the broadcast shape in docs/websocket-protocol.md.
type stateMessage struct {
	Type        string  `json:"type"`
	Status      string  `json:"status"`
	EndTime     *string `json:"end_time"`
	RemainingMS *int64  `json:"remaining_ms"`
	SessionID   *string `json:"session_id"`
}

var upgrader = websocket.Upgrader{
	// No auth or origin restriction yet -- fine for the walking skeleton,
	// must be locked down before this is reachable off localhost.
	CheckOrigin: func(r *http.Request) bool { return true },
}

func main() {
	hub := newHub()

	// Hardcoded state for now: a fixed end_time computed once at startup,
	// rebroadcast unchanged on every tick to prove the broadcast loop works.
	endTime := time.Now().Add(25 * time.Minute).UTC().Format(time.RFC3339)
	sessionID := "hardcoded-session"
	state := stateMessage{
		Type:      "state",
		Status:    "running",
		EndTime:   &endTime,
		SessionID: &sessionID,
	}
	payload, err := json.Marshal(state)
	if err != nil {
		log.Fatal(err)
	}

	http.HandleFunc("/healthz", func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
		_, _ = w.Write([]byte("ok"))
	})

	http.HandleFunc("/ws", func(w http.ResponseWriter, r *http.Request) {
		conn, err := upgrader.Upgrade(w, r, nil)
		if err != nil {
			log.Printf("ws upgrade error: %v", err)
			return
		}
		hub.register(conn)
		log.Printf("client connected")
		go hub.readPump(conn)
	})

	go func() {
		ticker := time.NewTicker(broadcastInterval)
		defer ticker.Stop()
		for range ticker.C {
			hub.broadcast(payload)
		}
	}()

	log.Println("listening on :8080")
	if err := http.ListenAndServe(":8080", nil); err != nil {
		log.Fatal(err)
	}
}
