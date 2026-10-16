#!/usr/bin/env python3
"""
Copia il database speedcam dal progetto Raspberry Pi in data/ (da caricare su LittleFS).
Il boot logo e i font sono ora asset LVGL gia' compilati in src/ (ui_img_boot_logo_png.c, geist_black_*.c).
"""

import os
import sys
import json

# Path relativi
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
# PROJECT_ROOT è la directory padre di micronav-esp32 (root del progetto)
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))
PI_ASSETS_DIR = os.path.join(PROJECT_ROOT, "micronav-pi", "micronav-assets")
ESP32_DATA_DIR = os.path.join(SCRIPT_DIR, "data")

def copy_speedcam_json():
    """Copia JSON speedcam cleaned"""
    print("📄 Copia database speedcam...")
    
    json_src = os.path.join(PI_ASSETS_DIR, "speedcams", "json", "SCDB-Northern-Italy_cleaned.json")
    json_dst = os.path.join(ESP32_DATA_DIR, "speedcams.json")
    
    if not os.path.exists(json_src):
        print(f"   ⚠️  File sorgente non trovato: {json_src}")
        return False
    
    try:
        # Copia file
        import shutil
        shutil.copy2(json_src, json_dst)
        
        # Verifica dimensione
        size_mb = os.path.getsize(json_dst) / (1024 * 1024)
        print(f"   ✅ Database copiato: {json_dst}")
        print(f"   📊 Dimensione: {size_mb:.2f} MB")
        
        # Verifica numero speedcam
        with open(json_dst, 'r') as f:
            data = json.load(f)
            if 'result' in data:
                count = len(data['result'])
                print(f"   📍 Numero speedcam: {count}")
        
        return True
        
    except Exception as e:
        print(f"   ❌ Errore copia database: {e}")
        return False

def main():
    print("=" * 60)
    print("🔄 Conversione Asset MicroNav ESP32")
    print("=" * 60)
    print()
    
    # Crea directory data se non esiste
    os.makedirs(ESP32_DATA_DIR, exist_ok=True)
    
    success = True
    
    # Copia database speedcam
    if not copy_speedcam_json():
        success = False
    
    print()
    print("=" * 60)
    if success:
        print("✅ Conversione completata!")
        print()
        print("📝 Prossimi passi:")
        print("   1. Verifica file in data/")
        print("   2. Upload filesystem: sh ./upload_littlefs.sh")
    else:
        print("⚠️  Conversione completata con errori")
    print("=" * 60)

if __name__ == "__main__":
    main()
