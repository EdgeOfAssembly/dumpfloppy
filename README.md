# dumpfloppy

C++23 CLI that rips secrets out of IBM PC floppy images (`.img` / `.ima`),
HxC bitstreams (`.mfm`), 86Box flux dumps (`.86f`), Commodore 1541/1571/1581
`.d64` / `.d71` / `.d81` images and 1541 `.g64` GCR containers (CBMFS listing
and extract), Amiga `.adf` images (OFS/FFS listing and extract), ZX
Spectrum TR-DOS `.trd` images (directory listing and extract),
`.ipf` / `.woz` / `.stx` / `.2mg` flux containers (metadata; no FAT),
and Apple DOS 3.3 / ProDOS (`.dsk` / `.po` / 2IMG).

`.ima` is WinImage’s raw dump; the sector layout is the same as `.img`.

## What it prints

- Image size, SHA-256, size-inferred CHS (160K … 2.88M, DMF, 81-track)
- Boot jump, OEM, `55 AA`, **booter vs MS-DOS data/system disk**
- Full FAT12/16 BIOS Parameter Block
- **Volume serial** (`XXXX-XXXX` from DOS 4+ EBPB 0x28/0x29)
- **Volume label** from EBPB *and* the root directory (they can differ)
- FAT copies, free/bad/orphan clusters
- Directory tree including **deleted** 8.3 names (`0xE5` → `?`)
- Commodore **D64 / D71 / D81 / G64 CBMFS**: disk name, ID, DOS type, PRG/SEQ/…
  listing (G64 is GCR-decoded to a 35-track D64 map; deleted rows use the same
  light-red + bold white as FAT)
- Amiga **ADF / OFS or FFS**: volume name, DOS type, directory (files + DIR), XXH64
- ZX Spectrum **TRD / TR-DOS**: disk label, geometry, BASIC/CODE/DATA/PRINT listing
  (160K IBM is not TRD — disk-info sector 8 is required)
- **IPF / WOZ / STX / 2IMG**: container metadata (SPS id, tracks, platform); FAT is
  not invented from flux bytes. Extract/update refused until a decoder exists.
- Apple **DOS 3.3 / ProDOS**: catalog listing and extract from raw 140K or 2IMG
  payload (VTOC T17/S0 vs ProDOS volume header in block 2)
- Type column (DATA until a catalog format matches; FAT12/ADF/AIFF/…)
- **XXH64** of each recovered file (16 hex)
- Whole-image **XXH64 catalog**: known dumps print a **CATALOG** section with
  title and copy-protection scheme (Paranoid, HLS, EA half-track, Ocean track 36, …)
- Deleted rows: light-red background, white **bold** text (`tui/ansi.h`)
- Boot-sector hex dump and printable strings
- Format catalog: `include/dumpfloppy/formats/` (Archiveteam floppy images + Shikadi file formats).
Commodore image notes (Schepers): `docs/cbm/` (`G64.TXT`, `D64.TXT`, …).

## Build

```bash
make -s V=0 -j"$(nproc)"          # debug + ASan/UBSan
make -s test                      # Catch2 + CLI contracts
make -s verify                    # tests, then CBMC on FAT12 and GCR codecs
make -s release
```

Needs **g++** (GNU C++23), **libcrypto**, Catch2 (`pkg-config catch2-with-main`),
and `/usr/local/include/libsf/tui/ansi.h`.

```bash
make install   # PREFIX=/usr/local
```

## Usage

```text
dumpfloppy [options] [images…]
```

No arguments (and `-h` / `--help`) print usage. `-v` / `--version` prints
`dumpfloppy 0.25`. Options and paths may be interleaved. A directory argument
expands to `*.img` / `*.ima` / `*.mfm` / `*.86f` / `*.d64` / `*.d71` / `*.d81` /
`*.adf` / `*.g64` / `*.trd` / `*.ipf` / `*.woz` / `*.stx` / `*.2mg` / `*.dsk` /
`*.po`.

```bash
dumpfloppy disk.ima
dumpfloppy game.d64
dumpfloppy game.d64 -x
dumpfloppy disk.d71
dumpfloppy disk.d81 -x
dumpfloppy work.adf -x
dumpfloppy game.g64
dumpfloppy game.g64 -x
dumpfloppy game.trd
dumpfloppy game.trd -x
dumpfloppy disk.2mg
dumpfloppy disk.dsk -x
dumpfloppy --no-color --no-hex disk.ima -o report.txt
dumpfloppy ./floppies -o ./reports/
dumpfloppy disk.ima -u HELLO.TXT
dumpfloppy disk.mfm -u PENGUIN.EXE
dumpfloppy disk.mfm -uPENGUIN.EXE
dumpfloppy game.d64 -u HELLO.prg
dumpfloppy work.adf -u README
```

`-x` on D64/D71/D81/G64 writes PETSCII names plus `.prg` / `.seq` / `.usr` /
`.rel` / `.del` (deleted files included). `-x` on ADF writes OFS/FFS files
(directories skipped; `/` in Amiga paths becomes `_`). `-x` on TRD writes
`NAME.C` (deleted `?AME.C`). `-u` on D64/D71/D81 and ADF is same-size in-place
replace; G64 GCR, REL, and TRD are refused.

| Default | Switch |
|---------|--------|
| ANSI colour on | `--no-color` |
| Boot hex dump on | `--no-hex` |
| Deleted entries shown in the listing | `--no-deleted` (listing only; `-x` still extracts deleted) |
| Extract files | `-x` / `--extract` (optional glob) |
| Update a named file | `-u` / `--update FILE` (repeatable; silent; glued `-uFILE` ok) |

## Example (Elvira 720K Disk 2)

That image is MS-DOS 3.3 FAT12, media `F9`, 80×2×9. There is **no** EBPB
serial (byte 0x26 is boot code, not `0x29`). Disk 2 still has a deleted
`?91.PKD` entry, which dumpfloppy lists in light-red with bold white.

## Layout

| Path | Role |
|------|------|
| `include/dumpfloppy/` | Public headers (Doxygen) |
| `src/` | Implementation; `fat12_codec.c` is C23 for CBMC |
| `tests/` | Catch2 |
| `formal/harness_fat12.c` | FAT12 even/odd round-trip |
| `man/dumpfloppy.1` | Manual page |
