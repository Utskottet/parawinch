# ebyteremote — LoRa remote v2 (experiment)

Candidate replacement for the current `remote/` (XIAO nRF52840 + Wio SX1262).
Custom PCB, Ebyte modules:

| | Part | LCSC |
|---|---|---|
| MCU | EBYTE `E73-2G4M08S1C` (nRF52840 module, 43 pins) | `C356849` |
| LoRa | EBYTE `E22-900M30S` | — |

Lives under `experiments/` because it is unproven — no board has been fabricated yet.
It moves up to `remote/` only after the exit criteria below are met.

## Status

Hardware design, pre-fab. Per `docs/Todo.md` (2026-08-12): no board-killers remain;
one schematic edit (D5 flyback diode), BOM bookkeeping, ERC cleanup, and three
ordering decisions outstanding. Layout should not start until ERC is clean.

**`docs/Todo.md` is the authoritative task list.** If any other document here
disagrees with it, Todo.md wins. `docs/FIXLIST.md` is superseded and kept only
for history.

## Layout

```
kicad/          KiCad project — open kicad/ParaWinchRemote.kicad_pro
  lib/          project symbol/footprint/3D libraries, resolved via ${KIPRJMOD}
  backups/      manual pre-change snapshots
  .history/     KiCad local history (gitignored)
docs/           Todo.md, pcb-handoff.md, REMAP-E73.md, ROUTING.md, FIXLIST.md
```

`fp-lib-table`, `sym-lib-table` and `lib/` must stay beside the `.kicad_pro` —
their URIs are `${KIPRJMOD}`-relative, which resolves to the directory holding
the project file. Moving any of them apart breaks every footprint and symbol.

## Exit criteria — what "proven" means

Before this replaces `remote/`:

- [ ] ERC and DRC clean
- [ ] Board fabricated and assembled
- [ ] Firmware brought up: BLE to phone + LoRa link to the winch
- [ ] Range/link budget at least matches the current Wio SX1262 remote, measured, not assumed
- [ ] Full flying session on one charge
- [ ] BLE relay path (phone → remote → winch config) verified end to end

On graduation: tag `remote-xiao-final`, move `remote/` to `archive/remote-xiao/`,
move this folder to `remote/`, update paths in `CLAUDE.md`.

## History

Developed outside the repo in `Documents/EbyteWinchRemote/Vertion 2` and imported
2026-08-16. That folder is the pre-import original and is not the working copy —
this is.

## Known issue

Ten of the 18 footprints in `kicad/lib/parawinch.pretty/` carry absolute 3D model
paths pointing at the old `EbyteWinchRemote/Vertion 2` location. They resolve only
on the original machine while that folder exists, and need rewriting to
`${KIPRJMOD}`-relative.
