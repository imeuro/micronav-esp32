#!/bin/bash
# Script di setup iniziale per arduino-cli
# Configura arduino-cli per lavorare con ESP32-C3

# Colori per output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Setup Arduino CLI per MicroNav ESP32-C3${NC}"
echo -e "${BLUE}========================================${NC}\n"

# Verifica che arduino-cli sia installato
if ! command -v arduino-cli &> /dev/null; then
    echo -e "${RED}Errore: arduino-cli non trovato!${NC}"
    echo "Installa con: brew install arduino-cli"
    exit 1
fi

echo -e "${GREEN}✓ arduino-cli trovato${NC}"

# Inizializza configurazione se non esiste
if [ ! -f ~/.arduino15/arduino-cli.yaml ]; then
    echo -e "${YELLOW}Inizializzazione configurazione arduino-cli...${NC}"
    arduino-cli config init
fi

# Aggiungi ESP32 board manager URL
echo -e "${YELLOW}Aggiunta ESP32 board manager URL...${NC}"
arduino-cli config add board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

# Aggiorna index
echo -e "${YELLOW}Aggiornamento index board manager...${NC}"
arduino-cli core update-index

# Installa ESP32 core
echo -e "${YELLOW}Installazione ESP32 core...${NC}"
arduino-cli core install esp32:esp32

# Installa librerie necessarie
echo -e "${YELLOW}Installazione librerie...${NC}"

echo "  - Adafruit GFX Library..."
arduino-cli lib install "Adafruit GFX Library"

echo "  - Adafruit GC9A01A..."
arduino-cli lib install "Adafruit GC9A01A"

echo "  - ArduinoJson..."
arduino-cli lib install "ArduinoJson"

echo "  - TinyGPSPlus..."
arduino-cli lib install "TinyGPSPlus"

echo -e "\n${GREEN}========================================${NC}"
echo -e "${GREEN}Setup completato!${NC}"
echo -e "${GREEN}========================================${NC}\n"

echo "Ora puoi usare:"
echo "  ./build.sh        - Compila il progetto"
echo "  ./upload.sh       - Carica il firmware"
echo "  ./monitor.sh      - Apri monitor seriale"
echo "  ./build_and_upload.sh - Compila e carica"

