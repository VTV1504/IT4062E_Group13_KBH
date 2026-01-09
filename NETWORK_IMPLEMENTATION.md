# Network Implementation Summary

## Backend Changes

### 1. Server (server.h, server.cpp)
- **NDJSON Protocol**: Implemented newline-delimited JSON parsing with TCP streaming buffer
- **Client Tracking**: Each client gets unique ID and display name
- **Message Handlers**: Implemented all protocol messages:
  - `time_sync` - Time synchronization with client
  - `create_room` - Create new room
  - `join_room` - Join room by ID
  - `join_random` - Join random public room
  - `ready/unready` - Player ready state
  - `set_private` - Host sets room privacy
  - `start_game` - Host starts game
  - `input` - Player typing input with char_events
- **Broadcasting**: `room_state`, `game_init`, `game_state`, `game_end`

### 2. Room System (room.h, room.cpp)
- **8 Fixed Slots**: Each room has exactly 8 slots (0-7)
- **Slot Management**: Automatic slot assignment, lowest empty slot first
- **Host System**: Host is always the lowest occupied slot index
- **Ready System**: All players must ready, host starts when can_start=true
- **Privacy**: Public/private rooms, only public rooms in join_random
- **Game Metrics**: Tracks WPM, accuracy, progress per player
- **Rankings**: Sorts by word_idx (desc), then time (asc)

### 3. RoomManager (room_manager.h, room_manager.cpp)
- **create_room()**: Generate unique room ID
- **join_room()**: Join by room_id
- **join_random()**: Find public non-full room
- **Auto cleanup**: Deletes empty rooms

## Frontend Changes

### 1. NetClient (src/net/NetClient.h, NetClient.cpp)
- **Separate Network Thread**: Non-blocking receive
- **TCP Streaming**: Handles partial/merged JSON messages
- **Thread-Safe**: Mutex-protected send and event queue
- **Send Methods**: All client→server messages
  - `send_create_room()`
  - `send_join_room(room_id)`
  - `send_ready()`
  - `send_start_game(duration_ms)`
  - `send_input(room_id, word_idx, char_events)`
  - etc.
- **Event Polling**: `poll_event()` returns typed events for UI thread

### 2. NetEvents (src/net/NetEvents.h)
- **Typed Events**: C++ structs for each message type
  - `HelloEvent` - Initial server greeting
  - `RoomStateEvent` - Complete lobby state (8 slots)
  - `GameInitEvent` - Game start with paragraph
  - `GameStateEvent` - Real-time player metrics
  - `GameEndEvent` - Final rankings
  - `ErrorEvent`, `InfoEvent` - Server responses

## Usage Example (Frontend)

```cpp
// In App::init()
NetClient netClient;
netClient.connect("127.0.0.1", 5000);

// In main loop
while (netClient.has_events()) {
    auto event = netClient.poll_event();
    
    switch (event->type) {
        case NetEventType::Hello: {
            auto* e = static_cast<HelloEvent*>(event.get());
            std::cout << "Connected! Client ID: " << e->client_id << "\n";
            break;
        }
        
        case NetEventType::RoomState: {
            auto* e = static_cast<RoomStateEvent*>(event.get());
            // Update UI with room state
            for (int i = 0; i < 8; i++) {
                if (e->slots[i].occupied) {
                    // Render player slot
                    bool is_self = (e->slots[i].client_id == e->self_client_id);
                    bool is_host = e->slots[i].is_host;
                    bool is_ready = e->slots[i].is_ready;
                    // ...
                }
            }
            break;
        }
        
        case NetEventType::GameInit: {
            auto* e = static_cast<GameInitEvent*>(event.get());
            // Start game with e->paragraph
            break;
        }
        
        // ... handle other events
    }
}

// User actions
if (user_clicked_create_room) {
    netClient.send_create_room();
}

if (user_clicked_ready) {
    netClient.send_ready();
}
```

## Protocol Compliance

✅ NDJSON format (one JSON per line)
✅ TCP streaming with buffer accumulation
✅ snake_case JSON naming
✅ 8 fixed slots per room
✅ Host auto-reassignment
✅ Ready/start conditions
✅ Time synchronization support
✅ Input with char_events array
✅ WPM/accuracy/progress calculation
✅ Ranking system

## Build Instructions

### Backend
```bash
cd KBH-IT4062E/backend
make
./kbh_server
```

### Frontend
```bash
cd KBH-IT4062E/frontend
make
./keyboard_hero_title.exe
```

## Dependencies
- Backend: libjsoncpp, libpqxx, libpq
- Frontend: SDL2, SDL2_image, SDL2_ttf, libjsoncpp, pthread
