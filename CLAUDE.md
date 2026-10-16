# micronav ESP32

Firmware ESP32-C3 (board ESP32-2424S012: display GC9A01 240x240 + touch CST816S) per l'avviso autovelox. UI su LVGL con backend LovyanGFX. Il progetto e' gestito con OpenSpec (`openspec/`); note di contesto in `../NOTE_IMPLEMENTAZIONE.md`.

## Recap a inizio sessione

Quando una sessione parte o viene ripresa (anche in una finestra agent riaperta), prima di rispondere al primo messaggio dell'utente fai un recap brevissimo, in italiano, di 3-5 righe:

- Se la conversazione ha un task in corso: cosa si stava facendo, a che punto e qual e' il prossimo passo concreto.
- Se non c'e' un task specifico: stato del progetto, cioe' change OpenSpec attive (con avanzamento task), ultimi commit e modifiche non committate. L'hook `SessionStart` (`.claude/hooks/session-recap.sh`) ne fornisce i dati nel contesto.

Niente liste lunghe e nessuna azione: e' solo un punto della situazione. Se il primo messaggio dell'utente e' gia' una richiesta precisa, dai il recap in una riga e poi rispondi.

## Convenzioni

- Commit solo su richiesta esplicita; `git add` mai con `-A` se il tree contiene lavoro non tuo (altre change OpenSpec in parallelo).
- Verifica su hardware prima di archiviare una change OpenSpec.
- Build/upload: `./build.sh`, `./upload.sh` (richiede Rosetta su Mac arm64; chiudere `monitor.sh` prima dell'upload).
