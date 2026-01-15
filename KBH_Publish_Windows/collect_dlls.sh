#!/bin/bash
# Automatic DLL collection script for Windows build
# Run this in MSYS2 MINGW64 terminal after successful compilation

OUTPUT_DIR="KBH_Windows"

echo "========================================="
echo "KBH Windows Package Creator"
echo "========================================="
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Check if executable exists
if [ ! -f "kbh_client.exe" ]; then
    echo "ERROR: kbh_client.exe not found!"
    echo "Please run 'make' first to build the project."
    exit 1
fi

echo "[1/4] Copying executable..."
cp kbh_client.exe "$OUTPUT_DIR/"
echo "✓ kbh_client.exe copied"

echo ""
echo "[2/4] Copying resources..."
cp -r res "$OUTPUT_DIR/" 2>/dev/null || echo "Warning: res/ folder not found"
mkdir -p "$OUTPUT_DIR/config"
cp client_config.json "$OUTPUT_DIR/config/" 2>/dev/null || echo "Warning: client_config.json not found"
echo "✓ Resources copied"

echo ""
echo "[3/4] Collecting DLL dependencies..."

# Get all mingw64 DLLs
ldd kbh_client.exe | grep mingw64 | awk '{print $3}' | sort -u | while read dll; do
    if [ -f "$dll" ]; then
        dll_name=$(basename "$dll")
        cp "$dll" "$OUTPUT_DIR/"
        echo "  ✓ $dll_name"
    fi
done

echo ""
echo "[4/4] Creating launcher..."

# Create a simple batch file launcher
cat > "$OUTPUT_DIR/Start_KBH.bat" << 'EOBAT'
@echo off
title Keyboard Battle Heroes
start "" kbh_client.exe
EOBAT

echo "✓ Start_KBH.bat created"

echo ""
echo "========================================="
echo "✓ Package created successfully!"
echo "========================================="
echo ""
echo "Output directory: $OUTPUT_DIR"
echo ""
echo "Contents:"
ls -lh "$OUTPUT_DIR" | tail -n +2 | awk '{print "  - " $9 " (" $5 ")"}'
echo ""
echo "Total DLLs: $(ls -1 "$OUTPUT_DIR"/*.dll 2>/dev/null | wc -l)"
echo "Total size: $(du -sh "$OUTPUT_DIR" | cut -f1)"
echo ""
echo "Next steps:"
echo "1. Copy the entire '$OUTPUT_DIR' folder to Windows"
echo "2. Double-click 'kbh_client.exe' or 'Start_KBH.bat' to run"
echo "3. Edit 'config/client_config.json' to set server IP"
echo ""
