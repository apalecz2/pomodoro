package main

import (
	"log"
	"sync"

	"github.com/gorilla/websocket"
)

// Hub tracks connected WebSocket clients and fans a state message out to all
// of them. It has no notion of session/command state yet -- that arrives
// once the backend owns real timer logic.
type Hub struct {
	mu      sync.Mutex
	clients map[*websocket.Conn]struct{}
}

func newHub() *Hub {
	return &Hub{clients: make(map[*websocket.Conn]struct{})}
}

func (h *Hub) register(conn *websocket.Conn) {
	h.mu.Lock()
	defer h.mu.Unlock()
	h.clients[conn] = struct{}{}
}

func (h *Hub) unregister(conn *websocket.Conn) {
	h.mu.Lock()
	defer h.mu.Unlock()
	delete(h.clients, conn)
	conn.Close()
}

// broadcast sends msg to every connected client, dropping any client that
// fails to write (its read pump will notice the closed connection and
// unregister it).
func (h *Hub) broadcast(msg []byte) {
	h.mu.Lock()
	defer h.mu.Unlock()
	for conn := range h.clients {
		if err := conn.WriteMessage(websocket.TextMessage, msg); err != nil {
			log.Printf("ws write error, dropping client: %v", err)
			go h.unregister(conn)
		}
	}
}

// readPump does nothing with incoming messages yet; it exists to read control
// frames (ping/pong/close) so the connection notices when the peer goes away.
func (h *Hub) readPump(conn *websocket.Conn) {
	defer h.unregister(conn)
	for {
		if _, _, err := conn.ReadMessage(); err != nil {
			return
		}
	}
}
