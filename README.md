# Keyboard Battle Heroes (KBH)

## Team Members

- **Vũ Nguyên Hạo** - 20226037
- **Võ Thành Vinh** - 20226074  
- **Bùi Hoàng Việt** - 20226073

## Project Overview

Keyboard Battle Heroes is a real-time multiplayer typing game where players compete to type paragraphs as fast and accurately as possible. The game features:

- **Training Mode**: Solo practice with performance tracking
- **Arena Mode**: Real-time multiplayer competition (up to 8 players)
- **Leaderboard**: Weekly rankings based on best WPM scores
- **User Authentication**: Account system with password management
- **Statistics Tracking**: WPM, accuracy, and performance history

### Objectives

1. Build a robust client-server architecture using TCP sockets
2. Implement real-time game synchronization across multiple clients
3. Create an engaging user interface with SDL2
4. Manage persistent user data and game statistics with PostgreSQL

## Technology Stack

### Backend
- **Language**: C++17
- **Database**: PostgreSQL with libpqxx
- **Libraries**: 
  - jsoncpp (JSON parsing)
  - pqxx (PostgreSQL C++ client)
- **Protocol**: NDJSON (Newline-Delimited JSON) over TCP

### Frontend
- **Language**: C++17
- **Graphics**: SDL2, SDL2_image, SDL2_ttf
- **Libraries**:
  - jsoncpp (JSON parsing)
  - pthread (multi-threading)

## Prerequisites

### System Requirements
- Ubuntu 20.04+ or compatible Linux distribution
- GCC/G++ with C++17 support
- PostgreSQL 12+

### Install Dependencies (Ubuntu)

```bash
# Update package list
sudo apt update

# Install build tools
sudo apt install -y build-essential g++ make

# Install PostgreSQL and development files
sudo apt install -y postgresql postgresql-contrib libpq-dev

# Install libpqxx (PostgreSQL C++ library)
sudo apt install -y libpqxx-dev

# Install jsoncpp
sudo apt install -y libjsoncpp-dev

# Install SDL2 and related libraries
sudo apt install -y libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev

# Verify installations
g++ --version          # Should show GCC 9.0+
psql --version         # Should show PostgreSQL 12+
pkg-config --modversion sdl2  # Should show SDL2 version
```

## Server Setup

### 1. Database Configuration

Start PostgreSQL service:
```bash
sudo service postgresql start
```

Create database and user:
```bash
# Switch to postgres user
sudo -u postgres psql

# In PostgreSQL prompt:
CREATE DATABASE kbh_db;
CREATE USER kbh_user WITH PASSWORD 'your_password';
GRANT ALL PRIVILEGES ON DATABASE kbh_db TO kbh_user;
\q
```

Initialize database schema:
```bash
cd KBH-IT4062E/database
psql -U kbh_user -d kbh_db -f New_DB.sql
psql -U kbh_user -d kbh_db -f paragraph_seed.sql
```

### 2. Configure Server

Edit `KBH-IT4062E/config/server_config.json`:
```json
{
    "server_ip": "127.0.0.1",
    "server_port": 5500,
    "db_host": "localhost",
    "db_port": 5432,
    "db_name": "kbh_db",
    "db_user": "kbh_user",
    "db_password": "your_password"
}
```

### 3. Build and Run Server

```bash
cd KBH-IT4062E/backend
make clean
make

# Run server
./kbh_server
```

Expected output:
```
[CONFIG] Loaded server config: 127.0.0.1:5500
[DATABASE] Connected to PostgreSQL
[SERVER] Listening on 127.0.0.1:5500
[SERVER] Waiting for connections...
```

## Client Setup

### 1. Configure Client

Edit `KBH-IT4062E/config/server_config.json` (same file as server):
```json
{
    "server_ip": "127.0.0.1",
    "server_port": 5500
}
```

### 2. Prepare Assets

Ensure the `res/` directory contains required assets:
```
KBH-IT4062E/frontend/res/
├── fonts/
│   └── Montserrat-Bold.ttf
└── images/
    ├── title_bg.png
    ├── logo_fin.png
    └── exit_button.png
```

### 3. Build and Run Client

```bash
cd KBH-IT4062E/frontend
make clean
make

# Run client
./kbh_client.exe
```

Expected output:
```
=== Keyboard Hero Client Starting ===
[Main] Calling app.init()...
[App] Initializing SDL...
[App] Creating window...
[App] Connecting to server...
[NetClient] Connected to 127.0.0.1:5500
[App] Initialization complete!
```

## Game Controls

### General
- **ESC**: Back/Close overlay
- **F11 / Alt+Enter**: Toggle fullscreen
- **Mouse**: Navigate menus and interact

### In-Game (Training/Arena)
- **Type**: Start typing the displayed paragraph
- **Backspace**: Delete last character
- Game automatically tracks WPM and accuracy

### Menu Navigation
- Click on menu items or use Tab to cycle through options

## Project Structure

```
IT4062E_Group13_KBH/
├── README.md
├── NETWORK_PROTOCOL.md          # Detailed protocol specification
├── KBH-IT4062E/
│   ├── backend/                 # Server code
│   │   ├── server/              # Network server
│   │   ├── room/                # Room management
│   │   ├── database/            # PostgreSQL integration
│   │   ├── typing_engine/       # Typing metrics calculation
│   │   ├── gamemode/            # Game mode implementations
│   │   └── Makefile
│   ├── frontend/                # Client code
│   │   ├── src/
│   │   │   ├── app/             # Main application
│   │   │   ├── screens/         # Game screens
│   │   │   ├── overlays/        # UI overlays
│   │   │   ├── net/             # Network client
│   │   │   ├── typing/          # Typing engine
│   │   │   └── ui/              # UI utilities
│   │   ├── res/                 # Assets (fonts, images)
│   │   └── Makefile
│   ├── config/                  # Configuration files
│   └── database/                # SQL schema and seeds
└── Prompt.txt                   # Development notes
```

## Features

### Implemented
- ✅ User authentication (sign up, sign in, password change)
- ✅ Training mode with result tracking
- ✅ Arena mode (up to 8 players)
- ✅ Real-time WPM and accuracy calculation
- ✅ Room system (create, join, ready/unready)
- ✅ Leaderboard (weekly top 8)
- ✅ Guest mode with optional account creation
- ✅ Game result persistence
- ✅ Profile screen with statistics

### Game Modes

#### Training Mode
- Solo practice with random paragraphs
- Auto-save results for logged-in users
- Try Again button for continuous practice
- Guest users can sign up after completion

#### Arena Mode
- Up to 8 players in real-time
- Host-controlled game start
- Ready/unready system
- Public/private rooms
- Live progress tracking
- Final rankings display

## Troubleshooting

### Server won't start
- Check if port 5500 is already in use: `netstat -tuln | grep 5500`
- Verify PostgreSQL is running: `sudo service postgresql status`
- Check database credentials in `server_config.json`

### Client can't connect
- Ensure server is running first
- Verify IP and port in configuration match
- Check firewall settings: `sudo ufw status`

### Database errors
- Ensure database exists: `psql -U kbh_user -d kbh_db -c '\l'`
- Re-run schema: `psql -U kbh_user -d kbh_db -f database/New_DB.sql`

### Build errors
- Verify all dependencies are installed: `pkg-config --list-all | grep -E 'sdl2|pqxx|jsoncpp'`
- Clean and rebuild: `make clean && make`
- Check GCC version: `g++ --version` (requires 9.0+)

## Network Protocol

For detailed message protocol specification, see [NETWORK_PROTOCOL.md](NETWORK_PROTOCOL.md).

The game uses NDJSON (Newline-Delimited JSON) over TCP for all client-server communication. Each message is a complete JSON object terminated by a newline character.

## Performance Notes

- Server handles concurrent connections using threading
- Client uses separate network thread for non-blocking I/O
- Game state updates sent at 20Hz (50ms intervals)
- Database queries optimized with indexes on frequently accessed columns

## License

This project is created for educational purposes as part of IT4062E Network Programming course at HUST.

## Acknowledgments

- SDL2 Development Team
- PostgreSQL Development Team
- JsonCpp Contributors
- Course instructors and teaching assistants
