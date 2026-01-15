# Keyboard Battle Heroes - Windows Version

## Hướng dẫn dành cho người dùng cuối (End User)

### Yêu cầu hệ thống
- Windows 10/11 64-bit
- RAM: Tối thiểu 512MB
- Kết nối Internet

### Cách chạy game

1. **Extract file .zip** ra folder bất kỳ
2. **Double-click vào `kbh_client.exe`** hoặc **`Start_KBH.bat`**
3. Game sẽ tự động chạy!

### Cấu hình server

Mở file **`config/client_config.json`** bằng Notepad:

```json
{
    "server_ip": "127.0.0.1",
    "server_port": 5500
}
```

Thay đổi `server_ip` thành địa chỉ của server bạn muốn kết nối.

### Troubleshooting

**Game không chạy được:**
- Đảm bảo đã extract đầy đủ tất cả files
- Windows Defender có thể block file .exe, chọn "Run anyway"
- Cần có folder `res` (chứa fonts và images)

**Không kết nối được server:**
- Kiểm tra `config/client_config.json`
- Đảm bảo server đang chạy
- Kiểm tra firewall

---

## Hướng dẫn dành cho Developer

Nếu bạn muốn tự build từ source code trên Windows, xem file [BUILD_INSTRUCTIONS.md](BUILD_INSTRUCTIONS.md)

### Quick Build Guide

1. Cài đặt MSYS2 từ https://www.msys2.org/
2. Trong MSYS2 MINGW64 terminal:
   ```bash
   pacman -S mingw-w64-x86_64-gcc \
             mingw-w64-x86_64-SDL2 \
             mingw-w64-x86_64-SDL2_image \
             mingw-w64-x86_64-SDL2_ttf \
             mingw-w64-x86_64-jsoncpp
   ```
3. Build:
   ```bash
   cd frontend
   make
   ```
4. Collect DLLs:
   ```bash
   chmod +x collect_dlls.sh
   ./collect_dlls.sh
   ```

Xem chi tiết trong [BUILD_INSTRUCTIONS.md](BUILD_INSTRUCTIONS.md)

---

## Liên hệ

**Team**: IT4062E - Group 13 - KBH
- Vũ Nguyên Hạo (20226037)
- Võ Thành Vinh (20226074)
- Bùi Hoàng Việt (20226073)

**GitHub**: [Link to repository]
