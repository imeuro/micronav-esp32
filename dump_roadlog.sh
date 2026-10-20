#!/bin/bash
# Scarica il log eventi (/roadlog*.txt su LittleFS) dalla board via seriale.
# Uso: ./dump_roadlog.sh [file_output]   (chiudere prima monitor.sh)
#      ./dump_roadlog.sh --clear         (cancella il log sulla board)
# Nota: ./upload_littlefs.sh sovrascrive la partizione e cancella il log: scaricarlo prima.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORT=$(ls /dev/cu.* 2>/dev/null | grep -i "usb\|usbserial\|usbmodem" | head -1)
if [ -z "$PORT" ]; then
    echo "Errore: nessuna porta seriale trovata. Collega la board."
    exit 1
fi

PY="$SCRIPT_DIR/venv/bin/python"
[ -x "$PY" ] || PY=python3

if [ "$1" == "--clear" ]; then
    CMD="C"; OUT=""
else
    CMD="L"; OUT="${1:-$SCRIPT_DIR/roadlog_$(date +%Y%m%d_%H%M%S).txt}"
fi

"$PY" -I - "$PORT" "$CMD" "$OUT" <<'PYEOF'
import sys, time, serial

port, cmd, out = sys.argv[1], sys.argv[2], sys.argv[3]
# DTR/RTS a False prima dell'apertura: evita il reset della board (USB CDC dell'ESP32-C3)
ser = serial.Serial()
ser.port, ser.baudrate, ser.timeout = port, 115200, 1
ser.dtr = False
ser.rts = False
ser.open()
time.sleep(0.3)
ser.reset_input_buffer()
ser.write(cmd.encode())

if cmd == "C":
    time.sleep(1)
    print("Comando di cancellazione inviato")
    sys.exit(0)

lines, started, deadline = [], False, time.time() + 60
while time.time() < deadline:
    raw = ser.readline().decode("utf-8", "replace").rstrip("\r\n")
    if raw == "---ROADLOG-BEGIN---":
        started = True
        continue
    if raw == "---ROADLOG-END---":
        break
    if started:
        lines.append(raw)
else:
    print("Timeout: nessuna risposta (firmware senza RoadLog?)")
    sys.exit(1)

with open(out, "w") as f:
    f.write("\n".join(lines) + ("\n" if lines else ""))
print(f"{len(lines)} righe salvate in {out}")
PYEOF
