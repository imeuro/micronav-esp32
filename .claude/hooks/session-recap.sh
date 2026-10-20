#!/bin/bash
# All'avvio/ripresa di una sessione emette un JSON con:
#  - systemMessage: recap mostrato all'utente subito, prima del primo prompt
#  - additionalContext: stesso stato nel contesto dell'agente (vedi CLAUDE.md)
cd "$(dirname "$0")/../.." || exit 0

changes=$(openspec list 2>/dev/null || echo "(openspec non disponibile)")
commits=$(git log --oneline -5 2>/dev/null)
dirty=$(git status --short 2>/dev/null | head -25)
dirty_n=$(git status --short 2>/dev/null | wc -l | tr -d ' ')

CHANGES="$changes" COMMITS="$commits" DIRTY="$dirty" DIRTY_N="$dirty_n" python3 -I - <<'PYEOF'
import json, os

changes = os.environ["CHANGES"]
commits = os.environ["COMMITS"]
dirty = os.environ["DIRTY"]
dirty_n = os.environ["DIRTY_N"]

last_commit = commits.splitlines()[0] if commits else "(nessun commit)"
dirty_line = "working tree pulito" if dirty_n == "0" else f"{dirty_n} file con modifiche non committate"

banner = (
    "== micronav: stato progetto ==\n"
    f"Change OpenSpec attive:\n{changes}\n"
    f"Ultimo commit: {last_commit}\n"
    f"{dirty_line}"
)

context = (
    "== Stato progetto micronav (generato all'avvio sessione) ==\n\n"
    f"-- Change OpenSpec attive --\n{changes}\n\n"
    f"-- Ultimi commit --\n{commits}\n\n"
    f"-- Modifiche non committate --\n{dirty}"
)

print(json.dumps({
    "systemMessage": banner,
    "hookSpecificOutput": {
        "hookEventName": "SessionStart",
        "additionalContext": context,
    },
}))
PYEOF
