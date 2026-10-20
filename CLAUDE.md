# micronav ESP32

Firmware ESP32-C3 (board ESP32-2424S012: display GC9A01 240x240 + touch CST816S) per l'avviso autovelox. UI su LVGL con backend LovyanGFX. Il progetto e' gestito con OpenSpec (`openspec/`); note di contesto in `../NOTE_IMPLEMENTAZIONE.md`.

## Recap a inizio sessione

Il recap dello stato del progetto (change OpenSpec con avanzamento, ultimo commit, modifiche non committate) lo mostra l'hook `SessionStart` (`.claude/hooks/session-recap.sh`) all'utente, **prima del primo prompt**, tramite `systemMessage`; lo stesso stato arriva anche nel tuo contesto.

Quindi, al primo messaggio dell'utente, non ripetere lo stato del progetto. Se la conversazione ha un task in corso (sessione ripresa), apri con 1-3 righe in italiano su cosa si stava facendo, a che punto e qual e' il prossimo passo concreto; se non c'e' un task specifico, non fare recap e rispondi direttamente. Nessuna azione: e' solo un punto della situazione.

## Convenzioni

- Commit solo su richiesta esplicita; `git add` mai con `-A` se il tree contiene lavoro non tuo (altre change OpenSpec in parallelo).
- Verifica su hardware prima di archiviare una change OpenSpec.
- Build/upload: `./build.sh`, `./upload.sh` (richiede Rosetta su Mac arm64; chiudere `monitor.sh` prima dell'upload).
