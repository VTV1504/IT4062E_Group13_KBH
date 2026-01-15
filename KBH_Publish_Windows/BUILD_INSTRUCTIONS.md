# Keyboard Battle Heroes - Windows Build Instructions

## Yêu cầu

### 1. Cài đặt MSYS2
Download và cài đặt MSYS2 từ: https://www.msys2.org/

### 2. Cài đặt các dependencies trong MSYS2 terminal

Mở **MSYS2 MINGW64** terminal và chạy:

```bash
# Update package database
pacman -Syu

# Install build tools and libraries
pacman -S --needed mingw-w64-x86_64-gcc \
    mingw-w64-x86_64-make \
    mingw-w64-x86_64-SDL2 \
    mingw-w64-x86_64-SDL2_image \
    mingw-w64-x86_64-SDL2_ttf \
    mingw-w64-x86_64-jsoncpp \
    mingw-w64-x86_64-postgresql
```

## Build Steps

### 1. Copy source code

Copy toàn bộ folder `KBH-IT4062E/frontend` vào máy Windows

### 2. Build trong MSYS2 MINGW64

```bash
cd /path/to/KBH-IT4062E/frontend
make clean
make
```

### 3. Collect DLL files

Sau khi build thành công, copy các DLL cần thiết vào cùng thư mục với `kbh_client.exe`:

```bash
# Copy SDL2 DLLs
cp /mingw64/bin/SDL2.dll .
cp /mingw64/bin/SDL2_image.dll .
cp /mingw64/bin/SDL2_ttf.dll .

# Copy jsoncpp
cp /mingw64/bin/libjsoncpp-*.dll .

# Copy image format libraries
cp /mingw64/bin/libpng16-*.dll .
cp /mingw64/bin/libjpeg-*.dll .
cp /mingw64/bin/libtiff-*.dll .
cp /mingw64/bin/libwebp-*.dll .

# Copy freetype
cp /mingw64/bin/libfreetype-*.dll .

# Copy standard libraries
cp /mingw64/bin/libgcc_s_seh-1.dll .
cp /mingw64/bin/libstdc++-6.dll .
cp /mingw64/bin/libwinpthread-1.dll .

# Copy zlib and other dependencies
cp /mingw64/bin/zlib1.dll .
cp /mingw64/bin/libbz2-*.dll .
cp /mingw64/bin/libzstd.dll .
cp /mingw64/bin/liblzma-*.dll .
```

### 4. Create portable package

Tạo folder structure:

```
KBH_Windows/
├── kbh_client.exe
├── *.dll (all DLLs from step 3)
├── config/
│   └── client_config.json
└── res/
    ├── fonts/
    └── images/
```

### 5. Test

Double-click `kbh_client.exe` để chạy game!

## Automatic DLL Collection Script

Tạo file `collect_dlls.sh` trong MSYS2:

```bash
#!/bin/bash
# Run this in MSYS2 MINGW64 after successful build

OUTPUT_DIR="KBH_Windows"
mkdir -p "$OUTPUT_DIR"

# Copy executable
cp kbh_client.exe "$OUTPUT_DIR/"

# Copy resources
cp -r res "$OUTPUT_DIR/"
mkdir -p "$OUTPUT_DIR/config"
cp client_config.json "$OUTPUT_DIR/config/"

# Function to copy DLL and its dependencies
copy_deps() {
    local exe="$1"
    local dest="$2"
    
    # Get list of DLLs
    ldd "$exe" | grep mingw64 | awk '{print $3}' | while read dll; do
        if [ -f "$dll" ]; then
            cp "$dll" "$dest/"
            echo "Copied: $(basename $dll)"
        fi
    done
}

# Copy all dependencies
echo "Collecting DLL dependencies..."
copy_deps "./kbh_client.exe" "$OUTPUT_DIR"

echo ""
echo "Done! Package created in: $OUTPUT_DIR"
echo "You can now copy this folder to any Windows machine."
```

Chạy script:
```bash
chmod +x collect_dlls.sh
./collect_dlls.sh
```

## Troubleshooting

### Missing DLL errors

Nếu khi chạy báo thiếu DLL, dùng Dependency Walker hoặc trong MSYS2:

```bash
ldd kbh_client.exe
```

Copy các DLL còn thiếu từ `/mingw64/bin/`

### Application crashes on startup

1. Đảm bảo tất cả DLLs cùng architecture (x86_64)
2. Check file `config/client_config.json` có đúng format
3. Check folder `res/fonts` và `res/images` có đầy đủ files

### Cannot connect to server

Edit `config/client_config.json`:
```json
{
    "server_ip": "YOUR_SERVER_IP",
    "server_port": 5500
}
```

## Distribution

Để distribute game:

1. **Zip the folder**: Compress `KBH_Windows` thành `.zip`
2. **Installer** (optional): Dùng NSIS hoặc Inno Setup để tạo installer
3. **README.txt**: Thêm hướng dẫn ngắn cho end users

## End User Requirements

- **OS**: Windows 10/11 64-bit
- **RAM**: 512MB
- **Network**: Internet connection để kết nối server
- **No installation needed**: Chỉ cần extract và double-click!
