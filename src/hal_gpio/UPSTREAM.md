# Upstream HAL Source

The files in this directory are a vendor copy of the NORA GPIO HAL.

Upstream repository:

- Repository: https://github.com/sulaolab/nora-hal-dspic33ak-gpio
- Branch: refactor/nora-hal
- Source directory: src/

Synchronized into this repository under:

- Destination directory: src/hal_gpio/

The upstream repository was previously named `dspic33ak-hal-gpio` and its public API
was `dspic33ak_gpio_*`. Both changed together: the API is now `nora_gpio_*`, and the
`_dspic33ak` tag survives only on backend implementation files
(`nora_gpio_dspic33ak.c`, `nora_gpio_event_dspic33ak.c`,
`nora_gpio_dspic33ak_reg.h`). There are no compatibility aliases — the old names are
gone, not deprecated.

Upstream also ships `nora_pps.{h,c}`. This repository does not vendor them: the
wrapper is a GPIO-only validation layer and does not own peripheral pin select or
interrupt routing. The exclusion is recorded in
`tools/sync_hal_from_upstream.py`, which fails rather than silently skipping any
upstream file that is neither vendored nor listed there.

## Current Synchronized Revision

- Upstream commit: a1de0fd695bd63ee922e2083df613213d2eb4fda

This revision was taken from the upstream `refactor/nora-hal` branch, which is the
NORA rename waiting to land. Until the fleet-wide landing puts those bytes on
upstream `main`, syncing without an explicit branch will pull the older
`dspic33ak_*` API and break this repository:

```sh
python tools/sync_hal_from_upstream.py --branch refactor/nora-hal
```

After the landing, plain `python tools/sync_hal_from_upstream.py` is correct again
and this note can go.

## Update Policy

The HAL-only repository is the upstream source of truth for this directory: apply
HAL fixes and HAL feature changes there, then synchronize this vendor copy. Do not
edit `src/hal_gpio/` in place — every file here is byte-identical to upstream, and
`tools/sync_hal_from_upstream.py` overwrites local edits without warning.

Note that the HAL-only repository is itself a published snapshot rather than the
place the code is written. HAL changes are made and hardware-validated in the
integration projects (the audio-DSP validation project and the HAL starter), then
published to the standalone HAL repository, and only then vendored here. A HAL fix
therefore has two hops to travel before it reaches this repository.

CMSIS-Driver wrapper changes belong in this repository.
