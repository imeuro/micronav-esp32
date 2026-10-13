#!/usr/bin/env python3
"""
Script per convertire font TTF in formato Adafruit GFX bitmap font
Usa fontconvert ufficiale di Adafruit per la conversione
"""

import os
import sys
import argparse
import subprocess
import re

# Path a fontconvert (cerca in vari posti comuni)
FONTCONVERT_PATHS = [
    "/Users/meuro/Documents/Arduino/libraries/Adafruit_GFX_Library/fontconvert/fontconvert",
    os.path.expanduser("~/Documents/Arduino/libraries/Adafruit_GFX_Library/fontconvert/fontconvert"),
    os.path.expanduser("~/Arduino/libraries/Adafruit_GFX_Library/fontconvert/fontconvert"),
    "fontconvert",  # Se è nel PATH
]

def find_fontconvert():
    """Trova fontconvert in uno dei path comuni"""
    for path in FONTCONVERT_PATHS:
        if os.path.exists(path) and os.access(path, os.X_OK):
            return path
    return None

def convert_ttf_to_gfxfont(ttf_path, output_path, size=16, first_char=32, last_char=126, letter_spacing_reduction=0):
    """
    Converte font TTF in formato GFXfont per Adafruit GFX usando fontconvert
    
    Args:
        ttf_path: Path al file TTF
        output_path: Path al file header C da generare
        size: Dimensione font in pixel (altezza)
        first_char: Primo carattere ASCII da includere (default 32 = spazio)
        last_char: Ultimo carattere ASCII da includere (default 126 = ~)
        letter_spacing_reduction: Riduzione spaziatura in pixel (default 0 = nessuna riduzione)
                                  Valore positivo riduce lo spazio tra caratteri
    """
    print(f"🔤 Conversione font: {os.path.basename(ttf_path)}")
    print(f"   📏 Dimensione: {size}px")
    print(f"   📝 Caratteri: {first_char} ({chr(first_char)}) - {last_char} ({chr(last_char)})")
    if letter_spacing_reduction > 0:
        print(f"   📐 Riduzione spaziatura: -{letter_spacing_reduction}px")
    
    if not os.path.exists(ttf_path):
        print(f"   ❌ File TTF non trovato: {ttf_path}")
        return False
    
    # Trova fontconvert
    fontconvert_path = find_fontconvert()
    if not fontconvert_path:
        print("   ❌ fontconvert non trovato!")
        print("   Cerca in:")
        for path in FONTCONVERT_PATHS:
            print(f"     - {path}")
        print()
        print("   Per compilare fontconvert:")
        print("     cd ~/Documents/Arduino/libraries/Adafruit_GFX_Library/fontconvert")
        print("     make")
        return False
    
    print(f"   🔧 Usando: {fontconvert_path}")
    
    try:
        # Crea file temporaneo per l'output di fontconvert
        import tempfile
        with tempfile.NamedTemporaryFile(mode='w', suffix='.h', delete=False) as tmp_file:
            tmp_output = tmp_file.name
        
        # Esegui fontconvert
        # fontconvert accetta: fontfile size [first] [last]
        cmd = [fontconvert_path, ttf_path, str(size)]
        if first_char != 32 or last_char != 126:
            cmd.extend([str(first_char), str(last_char)])
        
        print(f"   ⚙️  Esecuzione: {' '.join(cmd)}")
        
        with open(tmp_output, 'w') as out_file:
            result = subprocess.run(
                cmd,
                stdout=out_file,
                stderr=subprocess.PIPE,
                text=True
            )
        
        if result.returncode != 0:
            print(f"   ❌ Errore fontconvert:")
            print(result.stderr)
            os.unlink(tmp_output)
            return False
        
        # Leggi il file generato da fontconvert
        with open(tmp_output, 'r') as f:
            font_content = f.read()
        
        # Estrai nome font dal file di output
        output_basename = os.path.basename(output_path)
        font_name = output_basename.replace('_font.h', '').replace('.h', '').replace('-', '_').replace(' ', '_')
        font_name_upper = font_name.upper()
        
        # fontconvert genera nomi variabili basati sul nome del file TTF
        # Dobbiamo sostituirli con i nomi desiderati
        # Pattern: FontNameSizept7b -> font_name
        
        # Trova i nomi delle variabili generate da fontconvert
        bitmap_match = re.search(r'const uint8_t (\w+Bitmaps)\[\]', font_content)
        glyphs_match = re.search(r'const GFXglyph (\w+Glyphs)\[\]', font_content)
        font_match = re.search(r'const GFXfont (\w+) PROGMEM', font_content)
        
        if not (bitmap_match and glyphs_match and font_match):
            print("   ❌ Impossibile trovare variabili nel font generato")
            os.unlink(tmp_output)
            return False
        
        old_bitmap_name = bitmap_match.group(1)
        old_glyphs_name = glyphs_match.group(1)
        old_font_name = font_match.group(1)
        
        new_bitmap_name = f"{font_name}_bitmap"
        new_glyphs_name = f"{font_name}_glyphs"
        new_font_name = f"{font_name}_font"
        
        # Sostituisci i nomi delle variabili
        font_content = font_content.replace(old_bitmap_name, new_bitmap_name)
        font_content = font_content.replace(old_glyphs_name, new_glyphs_name)
        font_content = font_content.replace(old_font_name, new_font_name)
        
        # Aggiorna anche i riferimenti nella struttura font
        font_content = re.sub(
            rf'\(uint8_t\s+\*\){re.escape(old_bitmap_name)}',
            f'(uint8_t*){new_bitmap_name}',
            font_content
        )
        font_content = re.sub(
            rf'\(GFXglyph\s+\*\){re.escape(old_glyphs_name)}',
            f'(GFXglyph*){new_glyphs_name}',
            font_content
        )
        
        # Aggiorna header guard se necessario
        # fontconvert non genera header guard, aggiungiamolo se manca
        if f"#ifndef {font_name_upper}_FONT_H" not in font_content:
            # Trova la prima riga e aggiungi header guard
            lines = font_content.split('\n')
            new_lines = [
                f"#ifndef {font_name_upper}_FONT_H",
                f"#define {font_name_upper}_FONT_H",
                ""
            ] + lines + [
                "",
                f"#endif // {font_name_upper}_FONT_H"
            ]
            font_content = '\n'.join(new_lines)
        else:
            # Aggiorna header guard esistente
            font_content = re.sub(
                r'#ifndef \w+',
                f'#ifndef {font_name_upper}_FONT_H',
                font_content,
                count=1
            )
            font_content = re.sub(
                r'#define \w+',
                f'#define {font_name_upper}_FONT_H',
                font_content,
                count=1
            )
            # Aggiorna anche #endif se presente
            font_content = re.sub(
                r'#endif\s*$',
                f'#endif // {font_name_upper}_FONT_H',
                font_content,
                flags=re.MULTILINE
            )
        
        # Riduci letter spacing se richiesto
        # Modifica xAdvance (terzo valore) in ogni glyph: {offset, width, height, xAdvance, d_x, d_y}
        if letter_spacing_reduction > 0:
            def reduce_xadvance(match):
                # Estrai tutti i valori del glyph
                values = match.group(1).strip()
                # Parsing: { offset, width, height, xAdvance, d_x, d_y }
                parts = [p.strip() for p in values.split(',')]
                if len(parts) >= 4:
                    try:
                        xadvance = int(parts[3])
                        # Riduci xAdvance, ma non scendere sotto 1
                        new_xadvance = max(1, xadvance - letter_spacing_reduction)
                        parts[3] = str(new_xadvance)
                        return '{ ' + ', '.join(parts) + ' }'
                    except ValueError:
                        pass
                return match.group(0)
            
            # Pattern per trovare le strutture glyph: { valori }
            # Cerca pattern come: { 1234, 8, 12, 10, 0, -12 }
            font_content = re.sub(
                r'\{\s*(\d+\s*,\s*\d+\s*,\s*\d+\s*,\s*\d+\s*,\s*-?\d+\s*,\s*-?\d+)\s*\}',
                reduce_xadvance,
                font_content
            )
            print(f"   ✂️  Spaziatura ridotta di {letter_spacing_reduction}px per tutti i caratteri")
        
        # Scrivi il file finale
        with open(output_path, 'w') as f:
            f.write(font_content)
        
        # Rimuovi file temporaneo
        os.unlink(tmp_output)
        
        # Conta caratteri generati
        char_count = last_char - first_char + 1
        
        print(f"   ✅ Font convertito: {output_path}")
        print(f"   📊 Caratteri generati: {char_count}")
        print(f"   🔤 Variabili: {new_font_name}, {new_bitmap_name}, {new_glyphs_name}")
        
        return True
        
    except subprocess.CalledProcessError as e:
        print(f"   ❌ Errore esecuzione fontconvert: {e}")
        return False
    except Exception as e:
        print(f"   ❌ Errore conversione font: {e}")
        import traceback
        traceback.print_exc()
        return False

def main():
    parser = argparse.ArgumentParser(description='Converte font TTF in formato Adafruit GFX usando fontconvert')
    parser.add_argument('ttf_path', help='Path al file TTF da convertire')
    parser.add_argument('-o', '--output', help='Path file header C output (default: src/font_name.h)')
    parser.add_argument('-s', '--size', type=int, default=16, help='Dimensione font in pixel (default: 16)')
    parser.add_argument('--first-char', type=int, default=32, help='Primo carattere ASCII (default: 32)')
    parser.add_argument('--last-char', type=int, default=126, help='Ultimo carattere ASCII (default: 126)')
    parser.add_argument('--letter-spacing', type=int, default=0, 
                       help='Riduzione spaziatura tra caratteri in pixel (default: 0, es. 1-2 per font più compatti)')
    
    args = parser.parse_args()
    
    # Estrai nome font dal file TTF (sempre necessario per i messaggi finali)
    font_name = os.path.splitext(os.path.basename(args.ttf_path))[0].replace('-', '_').replace(' ', '_')
    
    # Determina output path
    if args.output:
        output_path = args.output
    else:
        script_dir = os.path.dirname(os.path.abspath(__file__))
        output_path = os.path.join(script_dir, 'src', f'{font_name}_font.h')
    
    print("=" * 60)
    print("🔤 Conversione Font TTF -> Adafruit GFX (fontconvert)")
    print("=" * 60)
    print()
    
    success = convert_ttf_to_gfxfont(
        args.ttf_path,
        output_path,
        size=args.size,
        first_char=args.first_char,
        last_char=args.last_char,
        letter_spacing_reduction=args.letter_spacing
    )
    
    print()
    print("=" * 60)
    if success:
        # Estrai nome font dal path di output per i messaggi
        output_basename = os.path.basename(output_path)
        # Rimuovi _font.h per ottenere il nome base
        font_var_name = output_basename.replace('_font.h', '').replace('.h', '')
        
        print("✅ Conversione completata!")
        print()
        print("📝 Prossimi passi:")
        print(f"   1. Il font è già incluso automaticamente in display_controller.cpp")
        print(f"   2. Configura in config.h (se non già fatto):")
        print(f"      #define CUSTOM_FONT_ENABLED")
        print(f"      #define CUSTOM_FONT_NAME {font_var_name}")
        print(f"   3. Il font verrà usato automaticamente come font di sistema")
    else:
        print("❌ Conversione fallita!")
    print("=" * 60)

if __name__ == "__main__":
    main()
