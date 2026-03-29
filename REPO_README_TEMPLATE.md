# SuperT

`SuperT` is an application project for the TuyaOS T5 platform.

This repository contains the application layer only, not the full TuyaOS SDK.

## Directory

- `src/`: application source code
- `include/`: public headers and generated app config
- `build/`: app build configuration files
- `docs/`: project documents
- `scripts/`: helper scripts

## How To Use

This project is intended to be placed under:

`<TuyaOS-SDK>/software/TuyaOS/apps/SuperT`

It depends on the surrounding TuyaOS SDK and vendor platform files, so it cannot be built standalone without that SDK tree.

## Suggested SDK Layout

```text
<TuyaOS-SDK>/
  software/
    TuyaOS/
      apps/
        SuperT/
```

## Not Included

The following are intentionally not tracked in this repository:

- build outputs in `output/`
- generated binaries such as `.bin`, `.elf`, `.map`
- temporary logs

## Notes

- If you want a fully reproducible build environment, keep a separate record of the TuyaOS SDK version you used.
- Current workspace base: `T5_TuyaOS-3.13.3`
