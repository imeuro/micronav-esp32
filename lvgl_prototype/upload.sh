#!/bin/bash
# Script per caricare il firmware LVGL Hello World su ESP32-C3 con arduino-cli

# Colori per output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Percorsi
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Verifica che arduino-cli sia installato
if ! command -v arduino-cli &> /dev/null; then
    echo -e "${RED}Errore: arduino-cli non trovato!${NC}"
    echo "Esegui prima: ./setup.sh"
    exit 1
fi

# Trova la porta seriale
PORT=$(ls /dev/cu.* 2>/dev/null | grep -i "usb\|usbserial\|usbmodem" | head -1)

if [ -z "$PORT" ]; then
    echo -e "${RED}Errore: Nessuna porta seriale trovata!${NC}"
    echo "Collega l'ESP32-C3 e riprova."
    echo ""
    echo "Porte disponibili:"
    ls /dev/cu.* 2>/dev/null | head -5
    exit 1
fi

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Upload Firmware LVGL Hello World${NC}"
echo -e "${BLUE}========================================${NC}\n"

echo -e "${GREEN}Porta seriale: $PORT${NC}"

# Verifica che la porta non sia occupata
if lsof "$PORT" >/dev/null 2>&1; then
    echo -e "${YELLOW}Avviso: La porta $PORT è occupata da un altro processo.${NC}"
    echo "Chiudi Serial Monitor o altri programmi che usano la porta seriale."
    echo ""
    echo "Processi che usano la porta:"
    lsof "$PORT" 2>/dev/null | head -5
    echo ""
    read -p "Vuoi continuare comunque? (s/n): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Ss]$ ]]; then
        echo "Operazione annullata."
        exit 1
    fi
fi

# FQBN con tutte le opzioni (stesse di build.sh)
FQBN="esp32:esp32:esp32c3:UploadSpeed=921600,CPUFreq=160,FlashFreq=80,FlashMode=qio,FlashSize=4M,DebugLevel=info,CDCOnBoot=cdc"

echo -e "${YELLOW}Upload in corso...${NC}"
echo "Premi il pulsante BOOT se necessario durante l'upload."
echo ""

# Carica firmware
arduino-cli upload \
    -p "$PORT" \
    --fqbn "$FQBN" \
    "$SCRIPT_DIR"

if [ $? -eq 0 ]; then
    echo -e "\n${GREEN}========================================${NC}"
    echo -e "${GREEN}✓ Upload completato con successo!${NC}"
    echo -e "${GREEN}========================================${NC}\n"
    echo "Ora puoi aprire il monitor seriale per vedere i log:"
    echo "  arduino-cli monitor -p $PORT -c baudrate=115200"
    exit 0
else
    echo -e "\n${RED}========================================${NC}"
    echo -e "${RED}✗ Errore durante l'upload!${NC}"
    echo -e "${RED}========================================${NC}\n"
    echo "Possibili cause:"
    echo "  - La porta seriale è occupata"
    echo "  - L'ESP32-C3 non è collegato correttamente"
    echo "  - Prova a premere il pulsante BOOT durante l'upload"
    exit 1
fi

