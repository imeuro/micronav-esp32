#!/bin/bash
# Stampa lo stato del progetto all'avvio/ripresa di una sessione: l'output
# entra nel contesto dell'agente, che (vedi CLAUDE.md) ne fa un breve recap.
cd "$(dirname "$0")/../.." || exit 0

echo "== Stato progetto micronav (generato all'avvio sessione) =="
echo
echo "-- Change OpenSpec attive --"
openspec list 2>/dev/null || echo "(openspec non disponibile)"
echo
echo "-- Ultimi commit --"
git log --oneline -5 2>/dev/null
echo
echo "-- Modifiche non committate --"
git status --short 2>/dev/null | head -25
