#!/bin/bash
# Script per verificare lo stato dell'ESP32-C3

PORT="/dev/cu.usbmodem1101"

echo "=== Verifica ESP32-C3 ==="
echo ""

# Verifica che la porta esista
if [ ! -e "$PORT" ]; then
    echo "❌ Porta $PORT non trovata!"
    exit 1
fi

echo "✅ Porta $PORT trovata"
echo ""

# Verifica permessi
PERMS=$(ls -l "$PORT" | awk '{print $1}')
echo "Permessi porta: $PERMS"
echo ""

# Verifica se la porta è occupata
if lsof "$PORT" >/dev/null 2>&1; then
    echo "⚠️  Porta occupata da:"
    lsof "$PORT" | grep -v COMMAND
    echo ""
    echo "Chiudi Arduino IDE o altri programmi e riprova"
else
    echo "✅ Porta libera"
fi

echo ""
echo "=== Test con screen ==="
echo "Esegui: ./test_serial_screen.sh"
echo "Poi premi RESET sull'ESP32-C3"
echo ""
echo "=== Test con hexdump ==="
echo "Esegui: ./test_serial_hex.sh"
echo "Poi premi RESET sull'ESP32-C3"
