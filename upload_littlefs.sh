#!/bin/bash
# Script per caricare file su LittleFS per ESP32-C3

# Colori per output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Percorsi
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DATA_DIR="$SCRIPT_DIR/data"
VENV_DIR="$SCRIPT_DIR/venv"

# Cerca mklittlefs (Arduino IDE o arduino-cli)
MKLTFS=""
# Prova prima Arduino IDE (macOS)
if [ -f "$HOME/Library/Arduino15/packages/esp32/tools/mklittlefs/4.0.2-db0513a/mklittlefs" ]; then
    MKLTFS="$HOME/Library/Arduino15/packages/esp32/tools/mklittlefs/4.0.2-db0513a/mklittlefs"
# Prova arduino-cli (cross-platform)
elif [ -f "$HOME/.arduino15/packages/esp32/tools/mklittlefs/"*"/mklittlefs" ]; then
    MKLTFS=$(find "$HOME/.arduino15/packages/esp32/tools/mklittlefs" -name "mklittlefs" -type f 2>/dev/null | head -1)
fi

# Attiva virtual environment se esiste
if [ -d "$VENV_DIR" ]; then
    source "$VENV_DIR/bin/activate"
    echo -e "${GREEN}Virtual environment attivato${NC}"
    PYTHON_CMD="python3"
else
    PYTHON_CMD="python3"
fi

# Verifica che la cartella data esista
if [ ! -d "$DATA_DIR" ]; then
    echo -e "${RED}Errore: Cartella data/ non trovata!${NC}"
    exit 1
fi

# Verifica che speedcams.json esista
if [ ! -f "$DATA_DIR/speedcams.json" ]; then
    echo -e "${RED}Errore: speedcams.json non trovato in data/!${NC}"
    echo "Esegui prima: python3 convert_assets.py"
    exit 1
fi

# Trova la porta seriale
PORT=$(ls /dev/cu.* | grep -i "usb\|usbserial\|usbmodem" | head -1)

if [ -z "$PORT" ]; then
    echo -e "${RED}Errore: Nessuna porta seriale trovata!${NC}"
    echo "Collega l'ESP32-C3 e riprova."
    exit 1
fi

echo -e "${GREEN}Porta seriale trovata: $PORT${NC}"

# Verifica che mklittlefs esista
if [ -z "$MKLTFS" ] || [ ! -f "$MKLTFS" ]; then
    echo -e "${RED}Errore: mklittlefs non trovato!${NC}"
    echo ""
    echo "mklittlefs è necessario per creare l'immagine LittleFS."
    echo "Viene installato automaticamente quando installi il core ESP32."
    echo ""
    echo "Percorsi cercati:"
    echo "  - ~/Library/Arduino15/packages/esp32/tools/mklittlefs/ (Arduino IDE)"
    echo "  - ~/.arduino15/packages/esp32/tools/mklittlefs/ (arduino-cli)"
    echo ""
    echo "Installa il core ESP32 con:"
    echo "  arduino-cli core install esp32:esp32"
    exit 1
fi

echo -e "${GREEN}mklittlefs trovato: $MKLTFS${NC}"

# Verifica che esptool sia disponibile
ESPTOOL_CMD=""
if [ -d "$VENV_DIR" ]; then
    # Prova a usare esptool dal venv
    if $PYTHON_CMD -m esptool --help >/dev/null 2>&1; then
        ESPTOOL_CMD="$PYTHON_CMD -m esptool"
        echo -e "${GREEN}Usando esptool dal virtual environment${NC}"
    elif command -v esptool.py >/dev/null 2>&1; then
        ESPTOOL_CMD="esptool.py"
        echo -e "${GREEN}Usando esptool.py dal sistema${NC}"
    fi
else
    # Cerca esptool nel sistema
    ESPTOOL_CMD=$(which esptool.py 2>/dev/null || which esptool 2>/dev/null)
fi

# Se non trovato, cerca nel percorso Arduino
if [ -z "$ESPTOOL_CMD" ]; then
    ESPTOOL=$(find ~/Library/Arduino15/packages/esp32 -name "esptool.py" 2>/dev/null | head -1)
    if [ -n "$ESPTOOL" ]; then
        ESPTOOL_CMD="$PYTHON_CMD $ESPTOOL"
        echo -e "${GREEN}Usando esptool da Arduino packages${NC}"
    fi
fi

# Se ancora non trovato, installa nel venv
if [ -z "$ESPTOOL_CMD" ]; then
    echo -e "${YELLOW}esptool non trovato. Installazione nel virtual environment...${NC}"
    if [ -d "$VENV_DIR" ]; then
        pip install esptool
        if $PYTHON_CMD -m esptool --help >/dev/null 2>&1; then
            ESPTOOL_CMD="$PYTHON_CMD -m esptool"
            echo -e "${GREEN}esptool installato nel virtual environment${NC}"
        else
            echo -e "${RED}Errore: Impossibile installare esptool!${NC}"
            exit 1
        fi
    else
        echo -e "${RED}Errore: esptool non trovato e venv non disponibile!${NC}"
        echo "Installa esptool: pip3 install esptool"
        exit 1
    fi
fi

echo -e "${GREEN}Comando esptool: $ESPTOOL_CMD${NC}"

# Crea directory temporanea con solo speedcams.json (esclude boot_logo.*)
TEMP_DATA_DIR="$SCRIPT_DIR/data_temp_littlefs"
rm -rf "$TEMP_DATA_DIR"
mkdir -p "$TEMP_DATA_DIR"

# Copia tutti i file JSON (include speedcams.json e fake_gps.json)
# Nota: il glob deve essere fuori dalle virgolette per essere espanso
if ls "$DATA_DIR"/*.json 1> /dev/null 2>&1; then
    cp "$DATA_DIR"/*.json "$TEMP_DATA_DIR/"
    echo -e "${GREEN}File JSON da caricare:${NC}"
    ls -lh "$TEMP_DATA_DIR"/*.json 2>/dev/null | awk '{print "  - " $9 " (" $5 ")"}'
else
    echo -e "${RED}Errore: Nessun file *.json trovato in data/!${NC}"
    exit 1
fi
echo -e "${YELLOW}Nota: boot_logo.* esclusi (boot logo è compilato nel firmware)${NC}"

echo -e "${GREEN}Creazione immagine LittleFS (solo *.json)...${NC}"

# Crea immagine LittleFS usando solo la directory temporanea (2.5MB = 0x270000 bytes)
IMAGE_FILE="$SCRIPT_DIR/littlefs.bin"
$MKLTFS -c "$TEMP_DATA_DIR" -b 4096 -p 256 -s 0x270000 "$IMAGE_FILE"

# Rimuovi directory temporanea
rm -rf "$TEMP_DATA_DIR"

if [ $? -ne 0 ]; then
    echo -e "${RED}Errore durante la creazione dell'immagine LittleFS!${NC}"
    exit 1
fi

echo -e "${GREEN}Immagine LittleFS creata: $IMAGE_FILE${NC}"

# Verifica che la porta non sia occupata
if lsof "$PORT" >/dev/null 2>&1; then
    echo -e "${YELLOW}Avviso: La porta $PORT è occupata da un altro processo.${NC}"
    echo "Chiudi Arduino IDE, Serial Monitor o altri programmi che usano la porta seriale."
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

# Carica l'immagine sulla partizione LittleFS (offset 0x190000)
echo -e "${GREEN}Caricamento immagine su ESP32-C3...${NC}"
echo "Porta: $PORT"
echo "Offset: 0x190000"

# Usa write-flash invece di write_flash (nuovo formato)
$ESPTOOL_CMD --chip esp32c3 --port "$PORT" write-flash 0x190000 "$IMAGE_FILE"

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✅ File caricati con successo su LittleFS!${NC}"
    rm -f "$IMAGE_FILE"
    echo "Immagine temporanea rimossa."
else
    echo -e "${RED}❌ Errore durante il caricamento!${NC}"
    echo ""
    echo "Possibili cause:"
    echo "  - La porta seriale è occupata (chiudi Arduino IDE/Serial Monitor)"
    echo "  - L'ESP32-C3 non è collegato correttamente"
    echo "  - Prova a premere il pulsante BOOT durante il caricamento"
    exit 1
fi

