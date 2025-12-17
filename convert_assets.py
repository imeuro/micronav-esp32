#!/usr/bin/env python3
"""
Script per convertire asset dal progetto Raspberry Pi a formato compatibile ESP32
- Boot logo: JPG -> BMP o array C
- Font: TTF -> bitmap array C (opzionale)
- Icone: PNG -> bitmap array C (opzionale)
"""

import os
import sys
from PIL import Image
import json

# Path relativi
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
# PROJECT_ROOT è la directory padre di micronav-esp32 (root del progetto)
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))
PI_ASSETS_DIR = os.path.join(PROJECT_ROOT, "micronav-pi", "micronav-assets")
ESP32_DATA_DIR = os.path.join(SCRIPT_DIR, "data")

def convert_boot_logo():
    """Converte boot logo da PNG/JPG a array C RGB565 (per boot veloce)
    Supporta PNG con trasparenza: centra su sfondo nero mantenendo proporzioni
    """
    print("🖼️  Conversione boot logo...")
    
    # Cerca prima PNG con trasparenza, poi JPG
    boot_logo_src = None
    boot_logo_png = os.path.join(ESP32_DATA_DIR, "boot_logo.png")
    boot_logo_jpg = os.path.join(ESP32_DATA_DIR, "boot_logo.jpg")
    
    if os.path.exists(boot_logo_png):
        boot_logo_src = boot_logo_png
        print(f"   📄 Trovato PNG: {boot_logo_src}")
    elif os.path.exists(boot_logo_jpg):
        boot_logo_src = boot_logo_jpg
        print(f"   📄 Trovato JPG: {boot_logo_src}")
    else:
        print(f"   ⚠️  File sorgente non trovato!")
        print(f"   Cercato in: {boot_logo_png}")
        print(f"   Cercato in: {boot_logo_jpg}")
        return False
    
    boot_logo_array = os.path.join(SCRIPT_DIR, "src", "boot_logo.h")
    
    try:
        # Carica immagine
        img = Image.open(boot_logo_src)
        print(f"   📐 Dimensione originale: {img.size}, Mode: {img.mode}")
        
        # Gestisci trasparenza: converte RGBA/LA in RGB con sfondo nero
        original_mode = img.mode
        if img.mode in ('RGBA', 'LA', 'P'):
            # Crea sfondo nero
            img_rgb = Image.new('RGB', img.size, (0, 0, 0))
            if img.mode == 'P':
                # Palette mode: converte prima in RGBA
                img = img.convert('RGBA')
            # Pasta l'immagine con alpha channel su sfondo nero
            if img.mode in ('RGBA', 'LA'):
                img_rgb.paste(img, mask=img.split()[-1])  # Usa alpha channel come mask
            else:
                img_rgb.paste(img)
            img = img_rgb
            print(f"   ✅ Convertito da {original_mode} a RGB con sfondo nero")
        elif img.mode != 'RGB':
            img = img.convert('RGB')
        
        # Mantieni dimensioni originali (NON ridimensionare)
        target_size = (240, 240)
        original_width, original_height = img.size
        print(f"   📏 Mantenute dimensioni originali: {original_width}x{original_height}")
        
        # Verifica che l'immagine non sia più grande dello schermo
        if original_width > target_size[0] or original_height > target_size[1]:
            print(f"   ⚠️  ATTENZIONE: Immagine più grande dello schermo ({original_width}x{original_height} > {target_size[0]}x{target_size[1]})")
            print(f"   L'immagine verrà ritagliata se necessario")
        
        # Calcola offset per centrare (orizzontale e verticale)
        offset_x = (target_size[0] - original_width) // 2
        offset_y = (target_size[1] - original_height) // 2
        print(f"   📍 Posizione centrata: offset ({offset_x}, {offset_y})")
        
        # Salva solo l'area con contenuto (senza sfondo nero) in un file temporaneo
        # Questo riduce drasticamente i pixel da renderizzare
        import tempfile
        with tempfile.NamedTemporaryFile(suffix='.png', delete=False) as tmp:
            temp_img_path = tmp.name
        img.save(temp_img_path, 'PNG')
        
        original_pixels = target_size[0] * target_size[1]
        cropped_pixels = original_width * original_height
        reduction_percent = 100 * (1 - (cropped_pixels / original_pixels))
        print(f"   📉 Riduzione: {original_pixels} -> {cropped_pixels} pixel ({reduction_percent:.1f}% in meno)")
        
        # Genera array C RGB565 solo dell'area con contenuto (molto più veloce!)
        if create_image_array_c(temp_img_path, boot_logo_array, "boot_logo_data", 
                                offset_x=offset_x, offset_y=offset_y):
            # Rimuovi file temporaneo
            os.unlink(temp_img_path)
            print(f"   ✅ Boot logo array C generato: {boot_logo_array}")
            print(f"   📐 Dimensione area: {original_width}x{original_height} (centrata a {offset_x},{offset_y})")
            print(f"   💡 Renderizza solo l'area con contenuto (veloce, no pixel neri)")
            return True
        else:
            return False
        
    except Exception as e:
        print(f"   ❌ Errore conversione boot logo: {e}")
        return False

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

def create_image_array_c(image_path_or_img, output_path, var_name="image_data", offset_x=0, offset_y=0):
    """Converte immagine in array C (RGB565)
    Accetta path string o oggetto PIL Image
    offset_x, offset_y: coordinate per centrare l'immagine ritagliata
    """
    try:
        # Se è un path, carica l'immagine
        if isinstance(image_path_or_img, str):
            img = Image.open(image_path_or_img)
            image_name = os.path.basename(image_path_or_img)
        else:
            # È già un oggetto PIL Image
            img = image_path_or_img
            image_name = "resized_image"
        
        # Converti a RGB se necessario
        if img.mode != 'RGB':
            img = img.convert('RGB')
        
        width, height = img.size
        
        # IMPORTANTE: Per oggetti PIL Image ridimensionati, dobbiamo forzare il caricamento
        # Accediamo a tutti i pixel per forzare il caricamento completo prima di getdata()
        if not isinstance(image_path_or_img, str):
            # Forza il caricamento completo accedendo a tutti i pixel
            pixels_temp = img.load()
            # Accedi a tutti i pixel per forzare il caricamento
            for y in range(height):
                for x in range(width):
                    _ = pixels_temp[x, y]
        
        # Ottieni tutti i pixel come lista di tuple (R,G,B)
        # Ora dovrebbe funzionare correttamente perché tutti i pixel sono stati caricati
        pixel_data = list(img.getdata())
        
        # Genera array C
        output = f"// Immagine: {image_name}\n"
        output += f"// Dimensione: {width}x{height}\n"
        output += f"// Generato automaticamente - non modificare manualmente\n"
        output += f"#ifndef BOOT_LOGO_H\n"
        output += f"#define BOOT_LOGO_H\n\n"
        output += f"// Definisci BOOT_LOGO_DATA_AVAILABLE all'inizio per permettere il controllo prima dell'uso\n"
        output += f"#define BOOT_LOGO_DATA_AVAILABLE\n\n"
        output += f"const uint16_t {var_name}[] PROGMEM = {{\n"
        
        # Itera sui pixel usando getdata() invece di pixels[x,y]
        # getdata() restituisce i pixel in ordine row-major (per riga)
        for y in range(height):
            output += "  "
            for x in range(width):
                # Calcola l'indice lineare: y * width + x
                idx = y * width + x
                r, g, b = pixel_data[idx]
                # Converti RGB888 a RGB565
                rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                output += f"0x{rgb565:04X}, "
            output += "\n"
        
        output += "};\n"
        output += f"const uint16_t {var_name}_width = {width};\n"
        output += f"const uint16_t {var_name}_height = {height};\n"
        output += f"const int16_t {var_name}_offset_x = {offset_x};\n"
        output += f"const int16_t {var_name}_offset_y = {offset_y};\n\n"
        output += f"#endif // BOOT_LOGO_H\n"
        
        with open(output_path, 'w') as f:
            f.write(output)
        
        return True
        
    except Exception as e:
        print(f"   ❌ Errore conversione immagine: {e}")
        return False

def main():
    print("=" * 60)
    print("🔄 Conversione Asset MicroNav ESP32")
    print("=" * 60)
    print()
    
    # Crea directory data se non esiste
    os.makedirs(ESP32_DATA_DIR, exist_ok=True)
    
    success = True
    
    # 1. Converti boot logo
    if not convert_boot_logo():
        success = False
    
    print()
    
    # 2. Copia database speedcam
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
