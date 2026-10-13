#!/bin/bash
# Script completo: compila, carica il firmware e apre il monitor seriale
# in un unico comando. Utile durante lo sviluppo/test iterativo.

# Colori per output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Flash & Monitor MicroNav ESP32-C3${NC}"
echo -e "${BLUE}========================================${NC}\n"

# Step 1+2: Compila e carica (build_and_upload.sh fa gia' entrambi)
"$SCRIPT_DIR/build_and_upload.sh"

if [ $? -ne 0 ]; then
    echo -e "${RED}Build/upload falliti. Monitor non avviato.${NC}"
    exit 1
fi

echo ""
echo -e "${YELLOW}Avvio monitor seriale tra 2 secondi (Ctrl+C per uscire)...${NC}"
sleep 2

# Step 3: Monitor seriale
"$SCRIPT_DIR/monitor.sh"
