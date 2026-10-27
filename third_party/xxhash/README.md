# xxHash (bundled)

`xxhash.h` is the unmodified single header from xxHash **v0.8.3**
(<https://github.com/Cyan4973/xxHash>, tag `v0.8.3`), by Yann Collet.
It is licensed under the BSD 2-Clause License; see `LICENSE` in this
directory (copied from the same tag). The header also carries the
license text.

dumpfloppy compiles it header-only (`XXH_INLINE_ALL` in `src/util.cpp`),
so a fresh clone builds without a system xxhash package or `-lxxhash`.
The directory is passed with `-isystem`, so the project's strict warning
flags do not apply to the vendored code.

SHA-256 of `xxhash.h`:
`17973c0dc49d9854ca26caa191f0e12f7a424b68858d9a78de3860d959d85e4b`
