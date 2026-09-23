# dumpfloppy — continue here (session handoff)

**HEAD:** (see `git log -1`) · **version 0.18** · GitHub `EdgeOfAssembly/dumpfloppy`  
**Fast tree:** `/tmp/dumpfloppy` (tmpfs — gone after power-off)  
**Durable:** `/mnt/dumpfloppy` + `/mnt/dumpfloppy.git` + origin  
**Mailbox / reviews:** `/mnt/grok/worktrees/dumpfloppy-xreview/mailbox/`  
**Project memory:** `~/.grok/memory/projects/dumpfloppy.md` · pmem `project.dumpfloppy.v015`

After reboot, clone or `rsync -a /mnt/dumpfloppy/ /tmp/dumpfloppy/` (or work in `/mnt/dumpfloppy`). Do **not** treat `/tmp` as the only copy.

## Do not push to GitHub

Game images, TOSEC dumps, extracted `*.EXE` / `*.PRG`. `.gitignore` covers common floppy extensions. Fixtures live **locally** under `/mnt/dumpfloppy-fixtures/` and `/mnt/PC_games/` (zips).

## Done in 0.13–0.18 (do not redo)

- FAT12 `-u` (atomic rename, reclaim-before-relocate, deleted occupancy / Star Control TACTICS)
- HxC CHS from BPB/modal SPT; HLS vs that SPT; extra-head DAM skip
- `volume.hpp` inverted (no `analyze.hpp`)
- CBMFS: D64 / D71 / D81 listing + `-x`; `-u` refused
- Amiga ADF OFS/FFS listing + `-x`; `-u` refused
- TRD Type needs sector-8 signature (160K IBM is not TRD)
- G64 GCR-1541: decode tracks 1–35 to a D64 map, CBMFS listing + `-x`; `-u` refused
- Schepers CBM format TXT notes in `docs/cbm/` (from RetroCodeMess; leave `.TXT` intact)
- D81 vs IBM 800K: size stays 800K; CBMFS needs header 40/0 plus BAM 40/1 DOS/`~DOS`
- CBM/ADF `-u` same-size in-place (D64/D71/D81 PRG/SEQ/USR, OFS/FFS); G64 and REL refused

**Verify last green:** `make -s test` 167 cases / 1884 assertions (7 skipped fixtures); `make -s verify` CBMC SUCCESS (FAT12 + GCR).

## Next (pick one slice)

1. **Split the 509-class format TU** — `all_formats()` is cached; still one generated mega-include. Payload vs container vs filesystem registries.
2. **Narrow `analyze.hpp`** — still includes FAT + CBM + Amiga + MFM. Forward-declare fs views.
3. **Other platforms:** Apple WOZ/2MG, Atari ST/STX, Spectrum TRD (parser, not size sniff), Amiga IPF. G71 (GCR-1571) if a fixture appears.

## Local fixtures (never GitHub)

| Title | Durable unpacked | Zip on `/mnt/PC_games` |
|-------|------------------|-------------------------|
| Elvira 720K | `/mnt/dumpfloppy-fixtures/Elvira (1990) (Accolade, Inc.) (720K) [!]/` | yes |
| Batman 360K MFM/86F | `/mnt/dumpfloppy-fixtures/Batman - The Caped Crusader (1989) (Data East USA, Inc.) (360K) [cp] [!]/` | yes |
| Commando 180K booter | `/mnt/dumpfloppy-fixtures/Commando (Booter) (1986) (Data East USA, Inc.) (180K) [cp] [!]/` | yes |
| Populous 360K IMA | `/mnt/dumpfloppy-fixtures/Populous (1989) (Electronic Arts, Inc.) (360K) [!]/` | yes |
| Star Control 720K | `/mnt/dumpfloppy-fixtures/Star Control (1990) (Accolade, Inc.) (720K) [!]/` | yes |
| 2400 A.D. 360K | `/mnt/dumpfloppy-fixtures/2400 A.D. (1988) (ORIGIN Systems, Inc.) (360K) [cp cr] [!]/` | yes |
| Karateka D64 | `/mnt/Lataukset/Karateka_Jordan_Mechner_Copy_1985-05-02.d64` | — |
| Last Ninja D64 | `/mnt/RetroCodeMess/c64/last_ninja.d64` | — |
| Beast Sonix ADF | `/mnt/music/_src/sotb/Beast_Sonix_1990_Scoopex.adf` | — |

## Quality bar

C++23 gnu++23, Allman, Doxygen, gcc. CLI: no-args usage, `-h`/`-v`, `-v` never verbose, `--no-*`. `make -s test` then `make -s verify`. FEATURE/FIXUP commits; `require-durable.sh --push --sync`.
