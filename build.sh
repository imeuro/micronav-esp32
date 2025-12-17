#!/bin/bash
# Script per compilare il progetto MicroNav ESP32-C3 con arduino-cli
# Usa le stesse impostazioni di Arduino IDE 1.8.19

# Colori per output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Percorsi
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SKETCH_FILE="$SCRIPT_DIR/micronav_esp32.ino"
PARTITIONS_FILE="$SCRIPT_DIR/partitions.csv"

# Verifica che arduino-cli sia installato
if ! command -v arduino-cli &> /dev/null; then
    echo -e "${RED}Errore: arduino-cli non trovato!${NC}"
    echo "Esegui prima: ./setup_arduino_cli.sh"
    exit 1
fi

# Verifica che lo sketch esista
if [ ! -f "$SKETCH_FILE" ]; then
    echo -e "${RED}Errore: Sketch non trovato: $SKETCH_FILE${NC}"
    exit 1
fi

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Compilazione MicroNav ESP32-C3${NC}"
echo -e "${BLUE}========================================${NC}\n"

# FQBN con tutte le opzioni come in Arduino IDE
# Le opzioni corrispondono alle impostazioni in ARDUINO_IDE_SETUP.md:
# - Upload Speed: 921600
# - CPU Frequency: 160MHz (WiFi)
# - Flash Frequency: 80MHz
# - Flash Mode: QIO
# - Flash Size: 4M (32Mb) - Nota: usa "4M" non "4MB"
# - Partition Scheme: Custom (usiamo partitions.csv)
# - Core Debug Level: Info
# - USB CDC On Boot: Enabled (IMPORTANTE per Serial!)
# Nota: ESP32-C3 non ha PSRAM, quindi non c'è opzione PSRAM
FQBN="esp32:esp32:esp32c3:UploadSpeed=921600,CPUFreq=160,FlashFreq=80,FlashMode=qio,FlashSize=4M,PartitionScheme=custom,DebugLevel=info,CDCOnBoot=cdc"

echo -e "${YELLOW}Configurazione:${NC}"
echo "  Board: ESP32C3 Dev Module"
echo "  Upload Speed: 921600"
echo "  CPU Frequency: 160MHz"
echo "  Flash Frequency: 80MHz"
echo "  Flash Mode: QIO"
echo "  Flash Size: 4MB"
echo "  Partition Scheme: Custom (partitions.csv)"
echo "  Debug Level: Info"
echo -e "  USB CDC On Boot: ${GREEN}Enabled${NC} (per Serial output)"
echo ""

# Verifica che partitions.csv esista e prepara build properties
if [ ! -f "$PARTITIONS_FILE" ]; then
    echo -e "${YELLOW}Avviso: partitions.csv non trovato, uso partition scheme di default${NC}"
    # Usa default se partitions.csv non esiste
    FQBN="esp32:esp32:esp32c3:UploadSpeed=921600,CPUFreq=160,FlashFreq=80,FlashMode=qio,FlashSize=4M,PartitionScheme=default,DebugLevel=info,CDCOnBoot=cdc"
    BUILD_PROPERTIES=(
        --build-property "compiler.cpp.extra_flags=-DCORE_DEBUG_LEVEL=3"
    )
else
    BUILD_PROPERTIES=(
        --build-property "build.partitions=partitions"
        --build-property "build.partitions_file=partitions.csv"
        --build-property "compiler.cpp.extra_flags=-DCORE_DEBUG_LEVEL=3"
    )
fi

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

