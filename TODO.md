# dumpfloppy — continue here (session handoff)

**HEAD:** worktree 0.37 (no git in this tree) · **version 0.37** · GitHub `EdgeOfAssembly/dumpfloppy`  
**Fast tree:** `/tmp/dumpfloppy` (tmpfs — gone after power-off)  
**Durable:** `/mnt/dumpfloppy` + `/mnt/dumpfloppy.git` + origin  
**Mailbox / reviews:** `/mnt/grok/worktrees/dumpfloppy-xreview/mailbox/`  
**Project memory:** `~/.grok/memory/projects/dumpfloppy.md` · pmem `project.dumpfloppy.v015`

After reboot, clone or `rsync -a /mnt/dumpfloppy/ /tmp/dumpfloppy/` (or work in `/mnt/dumpfloppy`). Do **not** treat `/tmp` as the only copy.

## Do not push to GitHub

Game images, TOSEC dumps, extracted `*.EXE` / `*.PRG`. `.gitignore` covers common floppy extensions. Fixtures live **locally** under `/mnt/dumpfloppy-fixtures/` and `/mnt/PC_games/` (zips).

## Done in 0.13–0.32 (do not redo)

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
- G71 GCR-1571: decode to a 70-track D71 map (168 half-tracks or 84 whole tracks), CBMFS listing + `-x`; `-u` refused
- Real G71 fixtures (transnet_c64): blank 168-slot CBMFS, Super Fast File Copy listing, Clone Machine 84-slot header-only; Tetris SPS #736 IPF standard OFS; Awesome demo IPF metadata-only
- FAT unused-cluster recovery: leftover free/bad clusters → `unused_cNNNN.{c,map,txt,bin}`; listing + `-x`; `--no-unused` listing-only. SQ2 Disk 1 is the example (AGI C + AGI.EXE map in 402K “free” space).
- Unused text split on DOS Ctrl-Z (`0x1A`) + `/* NAME` banners → `unused_SHOWOBJ.c` (generic; SQ2 yields dozens of AGI .c files).

**Verify last green (this tree, 0.36):** `make test` 274 cases, 267 passed, 7 skipped, 27856 assertions; `make verify` CBMC SUCCESS (FAT12 + Commodore GCR + Apple 6-and-2 + fat_slack).

**Verify last green (this tree, 0.37):** `make -s test` 276 cases, 269 passed, 7 skipped, 27880 assertions (exit 0); `make -s verify` exit 0, CBMC 6.10 SUCCESS — fat12 0 of 67 failed, gcr 0 of 35 failed, apple_gcr 0 of 178 failed, fat_slack 0 of 7 failed.

## Still open (2026-10-05)

Shipped in 0.35 and not part of this list: symlink-safe extract, directory-walk caps, `--slack`, `--leaked`, `--carve`.

Shipped in 0.36 (this tree): local depth and entry caps, file chains to `max_cluster`, cap warnings (`dumpfloppy:` plus image and depth/entries/chain), escaped `--slack` paths, merged `--carve` ranges, `-x` continues after a host-path error, symlinked `-o` is the destination, slack past logical EOF, non-FAT forensics still lists, `/` and `\` inside one FAT name become `_`, bare `make` links `dumpfloppy`.

Shipped in 0.37: `k_w_size` is 10 so a FAT directory line prints `4294967295` (CBM, Amiga, TRD, and Apple share that width). Live extract collisions use `stem.dup.ext` then `stem.dup.N.ext` (`FOO.dup.TXT`, `FOO.dup.2.TXT`). A deleted 8.3 name such as `?ACTICS.PKG` stays that name.

- [x] **Size column is 7 characters.** Done in 0.37. `k_w_size` is 10. `4294967295` is no longer clipped to `4294967`.
- [x] **Live name collisions are called deleted.** Done in 0.37. Two live files that share a host path write `stem.dup.ext`, then `stem.dup.N.ext`, not `.deleted`.
- [ ] **Strings / source-pattern search.** `--slack`, `--leaked`, and `--carve` do not search for source text. Still wanted: offsets of patterns such as `#include`, `proc near`, `org 100h`, `uses crt`, and BASIC line numbers, over file slack, free clusters, and bytes past the end of the filesystem.
- [ ] **JSON or TSV report.** Listing output is text only. A machine-readable report for batch runs is not implemented. No `--json` yet. Do not add both `--json` and `--no-json`.

## Next (pick one slice)

1. **Decode next:** IPF copy-protected tracks stay metadata-only. Other flux/container formats as fixtures appear.
2. One of the still-open items above.

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
| Blank 1571 G71 | `/mnt/dumpfloppy-fixtures/c64/blankdisk.g71` | IA transnet_c64 |
| Clone Machine 1571 G71 | `/mnt/dumpfloppy-fixtures/c64/Clone_Machine_1571_Original_Disk_Side1.g71` | IA transnet_c64 |
| Super Fast File Copy G71 | `/mnt/dumpfloppy-fixtures/c64/VG_Datashack_Super_Fast_File_Copy.TN.JBC.g71` | IA transnet_c64 |
| Tetris SPS IPF | `/mnt/dumpfloppy-fixtures/amiga/Tetris.ipf` | IA SPS #736 |
| Awesome demo SPS IPF | `/mnt/dumpfloppy-fixtures/amiga/AwesomeDemo.ipf` | IA SPS #1450 |
| SQ2 Disk 1 720K IMA | `/mnt/dumpfloppy-fixtures/pc/sq2-disk1.ima` | leftover AGI C + map in unused FAT |

## Quality bar

C++23 gnu++23, Allman, Doxygen, gcc. CLI: no-args usage, `-h`/`-v`, `-v` never verbose, `--no-*`. `make -s test` then `make -s verify`. FEATURE/FIXUP commits; `require-durable.sh --push --sync`.
