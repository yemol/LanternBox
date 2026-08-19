# FT-02 v2.75i2 Version Sync Audit

## Root cause
`VERSION.txt` had been advanced, but the firmware's authoritative compile-time version macro in
`src/FT02_BuildInfo.h` was still `v2.75a`.

## Authoritative version
`FT02_FIRMWARE_VERSION = "v2.75i2"`

Consumers include:
- Home UI
- Knowledge UI diagnostic output
- Location recorder metadata
- System self-test log
- Main startup/version log

Historical test documents retain their original version headings intentionally.
