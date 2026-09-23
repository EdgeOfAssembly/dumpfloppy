# Commodore emulator file-format notes

Peter Schepers’ collection (INTRO last updated 2008). Copied intact from
`/tmp/RetroCodeMess/docs` (TXT only; C64 schematics/PDFs stay there).

dumpfloppy uses these as the on-disk spec for D64/D71/D81/G64 and CBMFS:

| File | Use |
|------|-----|
| `G64.TXT` | GCR-1541 container, SYNC, header/data GCR |
| `ZIP_SIX.TXT` | 4-to-5 GCR nibble table (F = 10101) |
| `D64.TXT` / `D71.TXT` / `D81.TXT` | Sector images and CBMFS |
| `DISK.TXT` | 1541/1571/1581 file layout |

Do not edit the Schepers `.TXT` files; quote them from dumpfloppy sources.
