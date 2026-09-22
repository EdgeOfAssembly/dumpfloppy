# dumpfloppy

C++23 CLI that rips secrets out of IBM PC floppy images (`.img` / `.ima`).

`.ima` is WinImage’s raw dump; the sector layout is the same as `.img`.

## What it prints

- Image size, SHA-256, size-inferred CHS (160K … 2.88M, DMF, 81-track)
- Boot jump, OEM, `55 AA`, **booter vs MS-DOS data/system disk**
- Full FAT12/16 BIOS Parameter Block
- **Volume serial** (`XXXX-XXXX` from DOS 4+ EBPB 0x28/0x29)
- **Volume label** from EBPB *and* the root directory (they can differ)
- FAT copies, free/bad/orphan clusters
- Directory tree including **deleted** 8.3 names (`0xE5` → `?`)
- Type column (DATA until a catalog format matches; FAT12/ADF/AIFF/…)
- **XXH64** of each recovered file (16 hex)
- Deleted rows: light-red background, white **bold blinking** text (`tui/ansi.h`)
- Boot-sector hex dump and printable strings
- Format catalog: `include/dumpfloppy/formats/` (Archiveteam floppy images + Shikadi file formats)

## Build

```bash
make -s V=0 -j"$(nproc)"          # debug + ASan/UBSan
make -s test                      # Catch2 + CLI contracts
make -s verify                    # tests, then CBMC on the FAT12 codec
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
`dumpfloppy 0.7`. Options and paths may be interleaved. A directory argument
expands to `*.img` / `*.ima`.

```bash
dumpfloppy disk.ima
dumpfloppy --no-color --no-hex disk.ima -o report.txt
dumpfloppy ./floppies -o ./reports/
```

| Default | Switch |
|---------|--------|
| ANSI colour on | `--no-color` |
| Boot hex dump on | `--no-hex` |
| Deleted entries shown | `--no-deleted` |
| Extract files | `-x` / `--extract` (optional glob) |

## Example (Elvira 720K Disk 2)

That image is MS-DOS 3.3 FAT12, media `F9`, 80×2×9. There is **no** EBPB
serial (byte 0x26 is boot code, not `0x29`). Disk 2 still has a deleted
`?91.PKD` entry, which dumpfloppy lists in blinking light-red.

## Layout

| Path | Role |
|------|------|
| `include/dumpfloppy/` | Public headers (Doxygen) |
| `src/` | Implementation; `fat12_codec.c` is C23 for CBMC |
| `tests/` | Catch2 |
| `formal/harness_fat12.c` | FAT12 even/odd round-trip |
| `man/dumpfloppy.1` | Manual page |
