# dumpfloppy — agent notes

C++23 CLI (`gnu++23`, **g++** only). Quality: `modern-c-cpp-quality`,
`cli-design`, `max-quality-testing`, `max-quality-formal`.

## Build / test

```bash
make -s V=0 -j"$(nproc)"
make -s test          # alias: make tests
make -s verify        # test + CBMC on src/fat12_codec.c and src/gcr_codec.c
```

Debug is the default (ASan+UBSan). `make release` adds `-DNDEBUG`.

## CLI house rules

- No-args → usage (exit 0), same as `-h` / `--help`
- `-v` / `--version` from **0.1** (`-v` is never verbose)
- Colour / hex / deleted listing default **on** → only `--no-*`
- Operands and options interleaved; directories expand to `.img`/`.ima`/`.mfm`/`.86f`/`.d64`/`.d71`/`.d81`/`.adf`/`.g64`/`.trd`/`.ipf`/`.woz`/`.stx`/`.2mg`

## Includes

Compile with `-I/usr/local/include/libsf` and `#include <tui/ansi.h>`.
Deleted entries: `TUI_BG_BRIGHT_RED` + `TUI_WHITE` + `TUI_BOLD` (no blink).

## Formal

`formal/harness_fat12.c` proves FAT12 even/odd packed-entry round-trip.
`formal/harness_gcr.c` proves Commodore 4-to-5 GCR nibble/byte round-trip.
Keep `fat12_entry_get` / `fat12_entry_set` and `gcr_*` in C23 (no C++ in those TUs).

## Continue tomorrow

Read **`TODO.md`** (next slices, fixture paths, do-not-push games).
Durable trees: `/mnt/dumpfloppy`, GitHub `EdgeOfAssembly/dumpfloppy`.
`/tmp` is tmpfs. Unpacked TOSEC fixtures: `/mnt/dumpfloppy-fixtures/` (local only).
