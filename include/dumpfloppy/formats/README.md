# Format catalog

Each header is one `file_format` subclass. Unknown payloads stay **DATA**.

| Tree | Source | Count |
|------|--------|------:|
| `fat12.h` | IBM PC FAT12 (core) | 1 |
| `pkd.h` | Horrorsoft Packed Archive (Elvira `.PKD`, type `HS PACK ARC`) | 1 |
| `com.h` | DOS `.COM` (extension only; no on-disk magic) | 1 |
| `archiveteam/` | http://fileformats.archiveteam.org/wiki/Floppy_disk and Disk_Image_Formats | 70 |
| `shikadi/` | https://moddingwiki.shikadi.net/wiki/Category:File_Formats | 439 |

`type()` is uppercase, at most 24 characters (directory Type column).
`generated_formats.h` is produced by `scripts/gen_format_registry.py`.
