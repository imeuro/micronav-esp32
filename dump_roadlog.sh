#!/bin/bash
# Scarica il log eventi (/roadlog*.txt su LittleFS) dalla board via seriale.
# Uso: ./dump_roadlog.sh [file_output]   (chiudere prima monitor.sh)
#      ./dump_roadlog.sh --clear         (cancella il log sulla board)
# Nota: aprire la porta puo' resettare la board (avvio ~15 s): lo script attende.
#       ./upload_littlefs.sh sovrascrive la partizione e cancella il log: scaricarlo prima.

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
ser = serial.Serial()
ser.port, ser.baudrate, ser.timeout = port, 115200, 0.5
ser.dtr = False
ser.rts = False
ser.open()

# Il comando viene reinviato ogni 2 s finche' la board non risponde: se l'apertura
# della porta l'ha resettata, ignora i comandi durante l'avvio
lines, started, finished = [], False, False
deadline = time.time() + 60
last_send = 0.0
buf = b""

if cmd == "C":
    while time.time() < deadline and not finished:
        if time.time() - last_send > 2:
            ser.write(b"C")
            last_send = time.time()
        buf += ser.read(4096)
        if b"Log cancellato" in buf:
            finished = True
    print("Log cancellato" if finished else "Nessuna conferma dalla board")
    sys.exit(0 if finished else 1)

while time.time() < deadline and not finished:
    if not started and time.time() - last_send > 2:
        ser.write(b"L")
        last_send = time.time()
    buf += ser.read(4096)
    while b"\n" in buf:
        raw, buf = buf.split(b"\n", 1)
        raw = raw.decode("utf-8", "replace").rstrip("\r")
        if "---ROADLOG-BEGIN---" in raw:
            started, lines = True, []
            continue
        if raw == "---ROADLOG-END---":
            finished = True
            break
        if started:
            lines.append(raw)

if not started:
    print("Nessuna risposta dalla board (firmware senza RoadLog?)")
    sys.exit(1)
if not finished:
    print(f"ATTENZIONE: dump incompleto ({len(lines)} righe), manca il marcatore di fine")

with open(out, "w") as f:
    f.write("\n".join(lines) + ("\n" if lines else ""))
print(f"{len(lines)} righe salvate in {out}")
PYEOF
