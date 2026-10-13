#!/bin/bash
# Script per aprire il monitor seriale con arduino-cli

# Colori per output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

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
echo -e "${BLUE}Monitor Seriale MicroNav ESP32-C3${NC}"
echo -e "${BLUE}========================================${NC}\n"

echo -e "${GREEN}Porta: $PORT${NC}"
echo -e "${GREEN}Baudrate: 115200${NC}"
echo ""
echo -e "${YELLOW}Premi Ctrl+C per uscire${NC}\n"

# Apri monitor seriale
arduino-cli monitor \
    -p "$PORT" \
    -c baudrate=115200

