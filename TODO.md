# dumpfloppy — continue here (session handoff)

**HEAD:** (see `git log -1`) · **version 0.28** · GitHub `EdgeOfAssembly/dumpfloppy`  
**Fast tree:** `/tmp/dumpfloppy` (tmpfs — gone after power-off)  
**Durable:** `/mnt/dumpfloppy` + `/mnt/dumpfloppy.git` + origin  
**Mailbox / reviews:** `/mnt/grok/worktrees/dumpfloppy-xreview/mailbox/`  
**Project memory:** `~/.grok/memory/projects/dumpfloppy.md` · pmem `project.dumpfloppy.v015`

After reboot, clone or `rsync -a /mnt/dumpfloppy/ /tmp/dumpfloppy/` (or work in `/mnt/dumpfloppy`). Do **not** treat `/tmp` as the only copy.

## Do not push to GitHub

Game images, TOSEC dumps, extracted `*.EXE` / `*.PRG`. `.gitignore` covers common floppy extensions. Fixtures live **locally** under `/mnt/dumpfloppy-fixtures/` and `/mnt/PC_games/` (zips).

## Done in 0.13–0.28 (do not redo)

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
- Format catalog split: payload / container / filesystem TUs (`generated_{payload,container,filesystem}.h`)
- `analyze.hpp` includes fs views only (`cbm_view.hpp`, `amiga_view.hpp`, `fat_view.hpp`, `flux_view.hpp`)
- Whole-image XXH64 catalog names copy-protection (Paranoid, EA half-track 34.5,
  Ocean track 36, Origin HLS, HLS/Commando CRC, plus cracked/unprotected hashes)
- CBM/Amiga Filesystem line names the FS only (`CBMFS (Commodore 1541 G64)`), no `; not FAT`
- ZX Spectrum TR-DOS TRD listing + extract (`NAME.C`); 160K IBM is not TRD; `-u` refused
- IPF/WOZ/STX/2IMG skip FAT; SPS INFO + SOTB Copylock catalog; extract/update refused
- Apple DOS 3.3 / ProDOS catalog + extract (raw 140K or 2IMG); `-u` refused
- STX Pasti: assemble standard 512-byte sectors → GEMDOS/FAT12 listing + extract; `-u` refused
- WOZ 5.25 6-and-2 GCR → DOS-order 140K, then DOS 3.3 / ProDOS catalog + extract; `-u` refused
- IPF standard AmigaDOS (`4489`) sectors → DD ADF OFS/FFS listing + extract; `-u` refused

**Verify last green:** `make -s test` 209 cases / 4850 assertions (7 skipped /tmp PC copies); `make -s verify` CBMC SUCCESS (FAT12 + Commodore GCR + Apple 6-and-2).

## Next (pick one slice)

1. **Decode next:** G71 if a fixture appears. IPF copy-protected tracks stay metadata-only.

## Local fixtures (never GitHub)

| Title | Durable unpacked | Zip on `/mnt/PC_games` |
|-------|------------------|-------------------------|
| Elvira 720K | `/mnt/dumpfloppy-fixtures/Elvira (1990) (Accolade, Inc.) (720K) [!]/` | yes |
| Batman 360K MFM/86F | `/mnt/dumpfloppy-fixtures/Batman - The Caped Crusader (1989) (Data East USA, Inc.) (360K) [cp] [!]/` | yes |
| Commando 180K booter | `/mnt/dumpfloppy-fixtures/Commando (Booter) (1986) (Data East USA, Inc.) (180K) [cp] [!]/` | yes |
| Populous 360K IMA | `/mnt/dumpfloppy-fixtures/Populous (1989) (Electronic Arts, Inc.) (360K) [!]/` | yes |
| Star Control 720K | `/mnt/dumpfloppy-fixtures/Star Control (1990) (Accolade, Inc.) (720K) [!]/` | yes |
| 2400 A.D. 360K | `/mnt/dumpfloppy-fixtures/2400 A.D. (1988) (ORIGIN Systems, Inc.) (360K) [cp cr] [!]/` | yes |
| Karateka D64 | `/mnt/dumpfloppy-fixtures/c64/Karateka_Jordan_Mechner_Copy_1985-05-02.d64` | — |
| Last Ninja D64 (Paranoid) | `/mnt/dumpfloppy-fixtures/c64/Last_Ninja_The_1987_System_3_Side_A.d64` | — |
| Archon G64 (EA) | `/mnt/dumpfloppy-fixtures/c64/Archon (102402)(Electronic Arts, Inc.)(1983) [E1DAD185].g64` | IA |
| Batman C64 G64 (Ocean) | `/mnt/dumpfloppy-fixtures/c64/Batman The Caped Crusader (406-0171-00)(Ocean Software, Ltd.)(1988) [33C91B36].g64` | IA |
| OMEGAQ D81 | `/mnt/dumpfloppy-fixtures/c64/OMEGAQ.D81` | IA |
| Beast Sonix ADF | `/mnt/dumpfloppy-fixtures/amiga/Beast_Sonix_1990_Scoopex.adf` | — |
| Lemmings demo ADF | `/mnt/dumpfloppy-fixtures/amiga/lemmingdemo.adf` | IA |

## Quality bar

C++23 gnu++23, Allman, Doxygen, gcc. CLI: no-args usage, `-h`/`-v`, `-v` never verbose, `--no-*`. `make -s test` then `make -s verify`. FEATURE/FIXUP commits; `require-durable.sh --push --sync`.
