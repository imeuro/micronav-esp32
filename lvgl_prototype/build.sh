#!/bin/bash
# Script per compilare il progetto LVGL Hello World con arduino-cli
# Usa le stesse impostazioni del progetto principale

# Colori per output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Percorsi
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SKETCH_FILE="$SCRIPT_DIR/lvgl_prototype.ino"

# Verifica che arduino-cli sia installato
if ! command -v arduino-cli &> /dev/null; then
    echo -e "${RED}Errore: arduino-cli non trovato!${NC}"
    echo "Esegui prima: ./setup.sh"
    exit 1
fi

# Verifica che lo sketch esista
if [ ! -f "$SKETCH_FILE" ]; then
    echo -e "${RED}Errore: Sketch non trovato: $SKETCH_FILE${NC}"
    exit 1
fi

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Compilazione LVGL Hello World ESP32-C3${NC}"
echo -e "${BLUE}========================================${NC}\n"

# FQBN con tutte le opzioni (stesse del progetto principale)
# - Upload Speed: 921600
# - CPU Frequency: 160MHz
# - Flash Frequency: 80MHz
# - Flash Mode: QIO
# - Flash Size: 4M
# - Debug Level: Info
# - USB CDC On Boot: Enabled (IMPORTANTE per Serial!)
FQBN="esp32:esp32:esp32c3:UploadSpeed=921600,CPUFreq=160,FlashFreq=80,FlashMode=qio,FlashSize=4M,DebugLevel=info,CDCOnBoot=cdc"

echo -e "${YELLOW}Configurazione:${NC}"
echo "  Board: ESP32C3 Dev Module"
echo "  Upload Speed: 921600"
echo "  CPU Frequency: 160MHz"
echo "  Flash Frequency: 80MHz"
echo "  Flash Mode: QIO"
echo "  Flash Size: 4MB"
echo "  Debug Level: Info"
echo -e "  USB CDC On Boot: ${GREEN}Enabled${NC} (per Serial output)"
echo ""

# Build properties per LVGL
# Configurazione LVGL tramite build flags
# Disabilita filesystem driver che causa errori di compilazione
BUILD_PROPERTIES=(
    --build-property "compiler.cpp.extra_flags=-DCORE_DEBUG_LEVEL=3 -DLV_COLOR_DEPTH=16 -DLV_USE_LOG=0 -DLV_USE_FILESYSTEM=0 -DLV_USE_FS_FATFS=0 -DLV_USE_FS_POSIX=0 -DLV_USE_FS_WIN32=0 -DLV_USE_FS_STDIO=0"
)

# Compila
echo -e "${YELLOW}Compilazione in corso...${NC}"
arduino-cli compile \
    --fqbn "$FQBN" \
    "${BUILD_PROPERTIES[@]}" \
    "$SCRIPT_DIR"

if [ $? -eq 0 ]; then
    echo -e "\n${GREEN}========================================${NC}"
    echo -e "${GREEN}✓ Compilazione completata con successo!${NC}"
    echo -e "${GREEN}========================================${NC}\n"
    exit 0
else
    echo -e "\n${RED}========================================${NC}"
    echo -e "${RED}✗ Errore durante la compilazione!${NC}"
    echo -e "${RED}========================================${NC}\n"
    exit 1
fi

