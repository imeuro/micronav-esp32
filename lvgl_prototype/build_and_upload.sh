#!/bin/bash
# Script master: compila e carica il firmware in un unico comando

# Colori per output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Build & Upload LVGL Hello World${NC}"
echo -e "${BLUE}========================================${NC}\n"

# Step 1: Compila
echo -e "${YELLOW}[1/2] Compilazione...${NC}\n"
"$SCRIPT_DIR/build.sh"

if [ $? -ne 0 ]; then
    echo -e "${RED}Compilazione fallita. Upload annullato.${NC}"
    exit 1
fi

echo ""

# Step 2: Carica
echo -e "${YELLOW}[2/2] Upload...${NC}\n"
"$SCRIPT_DIR/upload.sh"

if [ $? -eq 0 ]; then
    echo -e "\n${GREEN}========================================${NC}"
    echo -e "${GREEN}✓ Build & Upload completati!${NC}"
    echo -e "${GREEN}========================================${NC}\n"
    echo "Ora puoi aprire il monitor seriale per vedere i log:"
    echo "  arduino-cli monitor -p /dev/cu.usbserial-* -c baudrate=115200"
fi

