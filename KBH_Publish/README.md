# Keyboard Battle Heroes - Published Version

## Cấu trúc thư mục

```
KBH_Publish/
├── kbh_client.exe          # Game executable
├── run_client.sh           # Script để chạy game
├── config/
│   └── client_config.json  # Cấu hình client (server IP, port)
├── res/
│   ├── fonts/              # Font chữ
│   └── images/             # Hình ảnh, sprites
└── lib*.so.*               # Shared libraries (SDL2, jsoncpp, etc.)
```

## Yêu cầu hệ thống

- **OS**: Linux (Ubuntu 20.04+ hoặc tương đương)
- **Architecture**: x86_64
- **RAM**: Tối thiểu 512MB
- **Disk**: 100MB trống

## Hướng dẫn chạy game

### Cách 1: Sử dụng script (Khuyến nghị)

```bash
cd KBH_Publish
./run_client.sh
```

### Cách 2: Chạy trực tiếp

```bash
cd KBH_Publish
export LD_LIBRARY_PATH=$PWD:$LD_LIBRARY_PATH
./kbh_client.exe
```

## Cấu hình

Chỉnh sửa file `config/client_config.json` để thay đổi server:

```json
{
    "server_ip": "127.0.0.1",
    "server_port": 5500
}
```

## Lưu ý

1. **Shared Libraries**: Folder này chứa tất cả .so files cần thiết:
   - libSDL2-2.0.so.0
   - libSDL2_image-2.0.so.0
   - libSDL2_ttf-2.0.so.0
   - libjsoncpp.so.25
   - libpng16.so.16, libjpeg.so.8, libtiff.so.6
   - libwebp.so.7, libwebpdemux.so.2
   - libfreetype.so.6

2. **Không cần cài đặt**: Game đã self-contained, chỉ cần các system libraries cơ bản (glibc, libm, pthread)

3. **Network**: Đảm bảo có thể kết nối đến server qua TCP port 5500

## Troubleshooting

### Game không chạy được

1. Kiểm tra executable permission:
   ```bash
   chmod +x kbh_client.exe run_client.sh
   ```

2. Kiểm tra missing libraries:
   ```bash
   ldd kbh_client.exe
   ```
   
   Nếu thấy "not found", đảm bảo các .so files có trong thư mục hiện tại.

### Không kết nối được server

1. Kiểm tra server đang chạy:
   ```bash
   nc -zv <server_ip> 5500
   ```

2. Kiểm tra firewall:
   ```bash
   sudo ufw allow 5500/tcp
   ```

## Liên hệ

- **Team**: IT4062E - Group 13
- **Members**: 
  - Vũ Nguyên Hạo (20226037)
  - Võ Thành Vinh (20226074)
  - Bùi Hoàng Việt (20226073)
