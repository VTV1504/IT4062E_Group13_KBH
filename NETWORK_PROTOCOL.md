# Network Protocol Specification

## Overview

Keyboard Battle Heroes uses **NDJSON (Newline-Delimited JSON)** over **TCP** for all client-server communication. Each message is a complete JSON object terminated by a newline character (`\n`).

### Protocol Characteristics
- **Transport**: TCP (reliable, ordered delivery)
- **Format**: JSON (human-readable, easy to debug)
- **Delimiter**: Newline (`\n`)
- **Port**: 5500 (default, configurable)
- **Encoding**: UTF-8

---

## Message Structure

All messages follow this base structure:

```json
{
    "type": "message_type",
    // Additional fields specific to message type
}
```

---

## Client → Server Messages

### 1. Time Sync Request

**Purpose**: Synchronize client and server clocks for accurate game timing.

**Message**:
```json
{
    "type": "time_sync",
    "client_time_ms": 1234567890
}
```

**Fields**:
- `client_time_ms` (integer): Client's current timestamp in milliseconds

**Response**: [Time Sync Response](#1-time-sync-response)

---

### 2. Set Username (Guest Mode)

**Purpose**: Set display name for guest users before joining rooms.

**Message**:
```json
{
    "type": "set_username",
    "username": "Player123"
}
```

**Fields**:
- `username` (string): Desired display name (1-20 characters)

**Response**: No explicit response. Username is set immediately.

---

### 3. Sign In

**Purpose**: Authenticate user with existing credentials.

**Message**:
```json
{
    "type": "sign_in",
    "username": "hao_vu",
    "password": "mypassword123"
}
```

**Fields**:
- `username` (string): Account username
- `password` (string): Account password

**Response**: [Sign In Response](#2-sign-in-response)

---

### 4. Create Account

**Purpose**: Register a new user account.

**Message**:
```json
{
    "type": "create_account",
    "username": "new_player",
    "password": "securepass456"
}
```

**Fields**:
- `username` (string): Desired username (must be unique, 3-20 chars)
- `password` (string): Password (minimum 6 characters)

**Response**: [Create Account Response](#3-create-account-response)

---

### 5. Change Password

**Purpose**: Update password for authenticated user.

**Message**:
```json
{
    "type": "change_password",
    "username": "hao_vu",
    "old_password": "oldpass",
    "new_password": "newpass123"
}
```

**Fields**:
- `username` (string): Current username
- `old_password` (string): Current password for verification
- `new_password` (string): New password (minimum 6 characters)

**Response**: [Change Password Response](#4-change-password-response)

---

### 6. Sign Out

**Purpose**: Log out from current account.

**Message**:
```json
{
    "type": "sign_out"
}
```

**Response**: [Sign Out Response](#5-sign-out-response)

---

### 7. Create Room

**Purpose**: Create a new multiplayer room.

**Message**:
```json
{
    "type": "create_room"
}
```

**Response**: [Room State](#6-room-state)

---

### 8. Join Room

**Purpose**: Join an existing room by ID.

**Message**:
```json
{
    "type": "join_room",
    "room_id": "ROOM_ABC123"
}
```

**Fields**:
- `room_id` (string): Target room identifier

**Response**: [Room State](#6-room-state) or [Error](#10-error)

---

### 9. Join Random Room

**Purpose**: Join any available public room.

**Message**:
```json
{
    "type": "join_random"
}
```

**Response**: [Room State](#6-room-state) or [Error](#10-error) if no rooms available

---

### 10. Exit Room

**Purpose**: Leave current room.

**Message**:
```json
{
    "type": "exit_room"
}
```

**Response**: No explicit response. Server removes player from room.

---

### 11. Ready

**Purpose**: Mark player as ready to start game.

**Message**:
```json
{
    "type": "ready"
}
```

**Response**: [Room State](#6-room-state) (broadcast to all players)

---

### 12. Unready

**Purpose**: Mark player as not ready.

**Message**:
```json
{
    "type": "unready"
}
```

**Response**: [Room State](#6-room-state) (broadcast to all players)

---

### 13. Set Room Privacy

**Purpose**: Host changes room to public/private.

**Message**:
```json
{
    "type": "set_private",
    "is_private": true
}
```

**Fields**:
- `is_private` (boolean): `true` for private, `false` for public

**Response**: [Room State](#6-room-state) (broadcast to all players)

**Notes**: Only host can change privacy. Private rooms cannot be joined via `join_random`.

---

### 14. Start Game

**Purpose**: Host starts the game (Arena mode).

**Message**:
```json
{
    "type": "start_game",
    "duration_ms": 50000
}
```

**Fields**:
- `duration_ms` (integer): Game duration in milliseconds (typically 50000)

**Response**: [Game Init](#7-game-init) (broadcast to all players)

**Requirements**:
- Sender must be host
- All players must be ready
- At least 2 players in room

---

### 15. Start Training

**Purpose**: Start solo training mode.

**Message**:
```json
{
    "type": "start_training"
}
```

**Response**: [Game Init](#7-game-init) with `room_id` = "training"

---

### 16. Save Training Result

**Purpose**: Save training result after guest signs in.

**Message**:
```json
{
    "type": "save_training_result",
    "paragraph": "the full paragraph text...",
    "wpm": 85.5,
    "accuracy": 94.2,
    "duration_ms": 48000,
    "word_idx": 42
}
```

**Fields**:
- `paragraph` (string): The paragraph that was typed
- `wpm` (float): Words per minute achieved
- `accuracy` (float): Accuracy percentage (0-100)
- `duration_ms` (integer): Time taken in milliseconds
- `word_idx` (integer): Number of words completed

**Response**: No explicit response. Result saved to database.

**Notes**: Only for authenticated users who completed training as guest.

---

### 17. Player Input

**Purpose**: Send typing progress during game.

**Message**:
```json
{
    "type": "input",
    "room_id": "ROOM_ABC123",
    "word_idx": 10,
    "char_events": [
        {"time_ms": 1000, "char": "h", "correct": true},
        {"time_ms": 1150, "char": "e", "correct": true},
        {"time_ms": 1300, "char": "l", "correct": true},
        {"time_ms": 1450, "char": "o", "correct": false}
    ]
}
```

**Fields**:
- `room_id` (string): Current room ID
- `word_idx` (integer): Current word index (0-based)
- `char_events` (array): Array of character typing events
  - `time_ms` (integer): Relative timestamp since game start
  - `char` (string): Character typed
  - `correct` (boolean): Whether character was correct

**Response**: Server updates game state, broadcasts [Game State](#8-game-state) to all players

**Notes**: 
- Sent periodically during gameplay (e.g., every word completion)
- Server calculates WPM and accuracy from char_events

---

### 18. Request Leaderboard

**Purpose**: Get top players from last 7 days.

**Message**:
```json
{
    "type": "leaderboard"
}
```

**Response**: [Leaderboard Response](#11-leaderboard-response)

---

## Server → Client Messages

### 1. Time Sync Response

**Purpose**: Reply to time sync request with server time.

**Message**:
```json
{
    "type": "time_sync",
    "client_id": 12345,
    "server_time_ms": 1234567950,
    "client_time_ms": 1234567890
}
```

**Fields**:
- `client_id` (integer): Unique client identifier assigned by server
- `server_time_ms` (integer): Server's current timestamp
- `client_time_ms` (integer): Echo of client's timestamp

**Client Action**: Calculate offset: `server_time_ms - client_time_ms`

---

### 2. Sign In Response

**Purpose**: Result of sign in attempt.

**Message (Success)**:
```json
{
    "type": "sign_in_response",
    "success": true,
    "user_id": 42,
    "username": "hao_vu",
    "error": ""
}
```

**Message (Failure)**:
```json
{
    "type": "sign_in_response",
    "success": false,
    "user_id": -1,
    "username": "",
    "error": "Invalid username or password"
}
```

**Fields**:
- `success` (boolean): Whether sign in succeeded
- `user_id` (integer): Database user ID (if successful)
- `username` (string): Confirmed username (if successful)
- `error` (string): Error message (if failed)

---

### 3. Create Account Response

**Purpose**: Result of account creation attempt.

**Message (Success)**:
```json
{
    "type": "create_account_response",
    "success": true,
    "user_id": 43,
    "username": "new_player",
    "error": ""
}
```

**Message (Failure)**:
```json
{
    "type": "create_account_response",
    "success": false,
    "user_id": -1,
    "username": "",
    "error": "Username already exists"
}
```

**Fields**: Same as Sign In Response

**Notes**: User must still sign in after successful account creation.

---

### 4. Change Password Response

**Purpose**: Result of password change attempt.

**Message**:
```json
{
    "type": "change_password_response",
    "success": true,
    "error": ""
}
```

**Fields**:
- `success` (boolean): Whether password was changed
- `error` (string): Error message if failed (e.g., "Invalid old password")

---

### 5. Sign Out Response

**Purpose**: Confirmation of sign out.

**Message**:
```json
{
    "type": "sign_out_response",
    "success": true
}
```

**Fields**:
- `success` (boolean): Always true

---

### 6. Room State

**Purpose**: Complete state of a room (sent on join, ready changes, etc.)

**Message**:
```json
{
    "type": "room_state",
    "room_id": "ROOM_ABC123",
    "is_private": false,
    "max_players": 8,
    "self_client_id": 12345,
    "host_slot_idx": 0,
    "all_ready": false,
    "can_start": true,
    "slots": [
        {
            "slot_idx": 0,
            "occupied": true,
            "client_id": 12345,
            "display_name": "hao_vu",
            "is_host": true,
            "is_ready": true,
            "knight_idx": 0
        },
        {
            "slot_idx": 1,
            "occupied": true,
            "client_id": 12346,
            "display_name": "vinh_vo",
            "is_host": false,
            "is_ready": false,
            "knight_idx": 1
        },
        {
            "slot_idx": 2,
            "occupied": false,
            "client_id": 0,
            "display_name": "",
            "is_host": false,
            "is_ready": false,
            "knight_idx": 0
        }
        // ... slots 3-7
    ]
}
```

**Fields**:
- `room_id` (string): Room identifier
- `is_private` (boolean): Whether room is private
- `max_players` (integer): Maximum slots (always 8)
- `self_client_id` (integer): Receiving client's ID
- `host_slot_idx` (integer): Slot index of current host
- `all_ready` (boolean): Whether all players are ready
- `can_start` (boolean): Whether game can start (>= 2 players, all ready)
- `slots` (array[8]): Array of 8 slot objects
  - `slot_idx` (integer): Slot number (0-7)
  - `occupied` (boolean): Whether slot has a player
  - `client_id` (integer): Player's client ID
  - `display_name` (string): Player's name
  - `is_host` (boolean): Whether this player is host
  - `is_ready` (boolean): Whether player is ready
  - `knight_idx` (integer): Character avatar index

**Notes**: 
- Broadcast to all players in room whenever state changes
- Host is automatically the lowest occupied slot
- Empty slots have `occupied: false`

---

### 7. Game Init

**Purpose**: Game is starting, provides paragraph and player list.

**Message**:
```json
{
    "type": "game_init",
    "room_id": "ROOM_ABC123",
    "server_start_ms": 1234568000,
    "duration_ms": 50000,
    "paragraph": "silent rivers carry small dreams across open fields where tired travelers rest beside warm stones...",
    "total_words": 85,
    "players": [
        {
            "slot_idx": 0,
            "client_id": 12345,
            "display_name": "hao_vu"
        },
        {
            "slot_idx": 1,
            "client_id": 12346,
            "display_name": "vinh_vo"
        }
    ]
}
```

**Fields**:
- `room_id` (string): Room ID ("training" for training mode)
- `server_start_ms` (integer): Server timestamp when game started
- `duration_ms` (integer): Game duration in milliseconds
- `paragraph` (string): Full text to type
- `total_words` (integer): Number of words in paragraph
- `players` (array): List of participating players
  - `slot_idx` (integer): Player's slot index
  - `client_id` (integer): Player's client ID
  - `display_name` (string): Player's display name

**Client Action**: 
- Navigate to game screen
- Display paragraph
- Start countdown timer
- Begin typing

---

### 8. Game State

**Purpose**: Real-time update of all players' progress (broadcast periodically).

**Message**:
```json
{
    "type": "game_state",
    "room_id": "ROOM_ABC123",
    "server_now_ms": 1234568500,
    "duration_ms": 50000,
    "ended": false,
    "players": [
        {
            "slot_idx": 0,
            "occupied": true,
            "word_idx": 15,
            "latest_time_ms": 1234568450,
            "progress": 0.176,
            "wpm": 72.5,
            "accuracy": 96.8
        },
        {
            "slot_idx": 1,
            "occupied": true,
            "word_idx": 12,
            "latest_time_ms": 1234568400,
            "progress": 0.141,
            "wpm": 68.2,
            "accuracy": 94.1
        }
        // ... up to 8 players
    ]
}
```

**Fields**:
- `room_id` (string): Room identifier
- `server_now_ms` (integer): Current server timestamp
- `duration_ms` (integer): Game duration
- `ended` (boolean): Whether game has ended
- `players` (array[8]): Array of player states
  - `slot_idx` (integer): Slot index (0-7)
  - `occupied` (boolean): Whether slot has a player
  - `word_idx` (integer): Current word index (0-based)
  - `latest_time_ms` (integer): Timestamp of last input
  - `progress` (float): Progress ratio (0.0-1.0)
  - `wpm` (float): Current words per minute
  - `accuracy` (float): Current accuracy percentage (0-100)

**Notes**: 
- Sent every ~50ms during active game
- Client uses this to render other players' progress bars and metrics

---

### 9. Game End

**Purpose**: Game finished, provides final rankings.

**Message**:
```json
{
    "type": "game_end",
    "room_id": "ROOM_ABC123",
    "reason": "timeout",
    "rankings": [
        {
            "rank": 1,
            "slot_idx": 0,
            "client_id": 12345,
            "display_name": "hao_vu",
            "word_idx": 85,
            "latest_time_ms": 1234568000,
            "wpm": 102.5,
            "accuracy": 98.3
        },
        {
            "rank": 2,
            "slot_idx": 1,
            "client_id": 12346,
            "display_name": "vinh_vo",
            "word_idx": 82,
            "latest_time_ms": 1234568000,
            "wpm": 98.1,
            "accuracy": 96.7
        }
    ]
}
```

**Fields**:
- `room_id` (string): Room identifier
- `reason` (string): End reason ("timeout", "all_finished")
- `rankings` (array): Sorted list of players by performance
  - `rank` (integer): Final rank (1-based)
  - `slot_idx` (integer): Player's slot index
  - `client_id` (integer): Player's client ID
  - `display_name` (string): Player's display name
  - `word_idx` (integer): Words completed
  - `latest_time_ms` (integer): Time of last input
  - `wpm` (float): Final words per minute
  - `accuracy` (float): Final accuracy (0-100)

**Ranking Rules**:
1. Primary: Higher `word_idx` (more words typed)
2. Secondary: Lower `latest_time_ms` (finished faster)

**Client Action**:
- Display result screen with rankings
- For training mode: Show Try Again button
- For arena mode: Return to lobby

---

### 10. Error

**Purpose**: Report errors to client.

**Message**:
```json
{
    "type": "error",
    "code": "ROOM_FULL",
    "message": "Room is full (8/8 players)"
}
```

**Fields**:
- `code` (string): Error code for programmatic handling
- `message` (string): Human-readable error description

**Common Error Codes**:
- `UNKNOWN_TYPE`: Invalid message type
- `ROOM_NOT_FOUND`: Room ID doesn't exist
- `ROOM_FULL`: Room has maximum players
- `NOT_HOST`: Player tried host-only action
- `NOT_AUTHENTICATED`: Action requires login
- `INVALID_CREDENTIALS`: Wrong username/password
- `USERNAME_EXISTS`: Username already taken
- `MISSING_FIELDS`: Required message fields missing

---

### 11. Leaderboard Response

**Purpose**: Weekly top players and user's rank.

**Message**:
```json
{
    "type": "leaderboard_response",
    "top8": [
        {
            "rank": 1,
            "username": "speed_demon",
            "wpm": 125.8
        },
        {
            "rank": 2,
            "username": "hao_vu",
            "wpm": 118.2
        },
        {
            "rank": 3,
            "username": "vinh_vo",
            "wpm": 112.5
        }
        // ... up to 8 entries
    ],
    "self_rank": {
        "rank": 15,
        "username": "current_user",
        "wpm": 95.3
    }
}
```

**Fields**:
- `top8` (array): Top 8 players from last 7 days
  - `rank` (integer): Global rank
  - `username` (string): Player's username
  - `wpm` (float): Best WPM in last 7 days
- `self_rank` (object or null): Current user's rank
  - `rank` (integer): User's global rank
  - `username` (string): User's username
  - `wpm` (float): User's best WPM
  - **null if**: User is guest OR no results in last 7 days

**Notes**: 
- Ranking based on best single WPM result in last 7 days
- Only shows authenticated users with recorded results

---

## Connection Flow

### 1. Initial Connection

```
Client                                  Server
  |                                       |
  |--- TCP connect to port 5500 -------->|
  |<------ TCP connection accepted -------|
  |                                       |
  |<------ hello ------------------------|
  |        {                              |
  |          "type": "hello",             |
  |          "client_id": 12345,          |
  |          "server_time_ms": 1234567890 |
  |        }                              |
```

### 2. Arena Mode Game Flow

```
Client                                  Server
  |                                       |
  |--- create_room ---------------------->|
  |<------ room_state --------------------|
  |                                       |
[Other clients join via join_room]
  |                                       |
  |--- ready --------------------------->|
  |<------ room_state (all players) ------|
  |                                       |
[Host starts game]
  |--- start_game ----------------------->|
  |<------ game_init ---------------------|
  |                                       |
[Typing phase - 50 seconds]
  |--- input ---------------------------->|
  |<------ game_state (periodic) ---------|
  |--- input ---------------------------->|
  |<------ game_state --------------------|
  |                                       |
[Game ends]
  |<------ game_end ----------------------|
```

### 3. Training Mode Flow

```
Client                                  Server
  |                                       |
  |--- start_training ------------------->|
  |<------ game_init ---------------------|
  |        (room_id = "training")         |
  |                                       |
[Solo typing]
  |--- input ---------------------------->|
  |                                       |
[Complete or timeout]
  |<------ game_end ----------------------|
```

---

## Protocol Design Principles

### 1. Message Ordering
- TCP guarantees ordered delivery
- Messages processed sequentially in order received
- No need for sequence numbers

### 2. Error Handling
- Server validates all messages
- Invalid messages trigger `error` response
- Malformed JSON closes connection

### 3. Scalability
- Stateless message format
- Server maintains client state internally
- Each client has independent connection

### 4. Performance
- NDJSON allows streaming parsing
- No message size limits (practically limited by TCP buffer)
- Game state updates at 20Hz (50ms intervals)

### 5. Extensibility
- New message types easily added
- Optional fields can be added without breaking compatibility
- Version negotiation possible via `hello` message

---

## Message Examples

### Complete Training Session

**1. Client connects and starts training**
```json
{"type":"start_training"}
```

**2. Server sends game init**
```json
{
    "type":"game_init",
    "room_id":"training",
    "server_start_ms":1705320000000,
    "duration_ms":50000,
    "paragraph":"the quick brown fox jumps over the lazy dog",
    "total_words":9,
    "players":[{"slot_idx":0,"client_id":12345,"display_name":"Guest_12345"}]
}
```

**3. Client sends typing progress**
```json
{
    "type":"input",
    "room_id":"training",
    "word_idx":4,
    "char_events":[
        {"time_ms":1000,"char":"t","correct":true},
        {"time_ms":1100,"char":"h","correct":true},
        {"time_ms":1200,"char":"e","correct":true}
    ]
}
```

**4. Server sends game end**
```json
{
    "type":"game_end",
    "room_id":"training",
    "reason":"timeout",
    "rankings":[{
        "rank":1,
        "slot_idx":0,
        "client_id":12345,
        "display_name":"Guest_12345",
        "word_idx":9,
        "latest_time_ms":1705320048000,
        "wpm":90.5,
        "accuracy":95.2
    }]
}
```

---

## Security Considerations

1. **Passwords**: Hashed with bcrypt (gen_salt('bf')) before storage
2. **Input Validation**: All user inputs sanitized and validated
3. **SQL Injection**: Prevented using parameterized queries (libpqxx)
4. **Resource Limits**: 
   - Maximum message size: 1MB
   - Connection timeout: 30 seconds
   - Max concurrent connections: 1000
5. **Authentication**: Username/password sent in plaintext (use TLS in production)

---

## Future Enhancements

1. **Message Compression**: Gzip for large messages (paragraphs)
2. **Binary Protocol**: Protocol Buffers for better performance
3. **WebSocket Support**: Enable web client
4. **Encryption**: TLS for secure communication
5. **Reconnection**: Handle network interruptions gracefully
6. **Spectator Mode**: Allow observing games without playing

---

## References

- NDJSON Specification: http://ndjson.org/
- JsonCpp Documentation: https://github.com/open-source-parsers/jsoncpp
- TCP Socket Programming: POSIX sockets API
