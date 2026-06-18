# Upstream HAL Source

The files in this directory are a vendor copy of the dsPIC33AK GPIO HAL.

Upstream repository:

- Repository: https://github.com/sulaolab/dspic33ak-gpio-hal
- Branch: main
- Source directory: src/

Synchronized into this repository under:

- Destination directory: src/hal_gpio/

## Current Synchronized Revision

- Upstream commit: 287ad2537944de6178b9f8674e9d73b92e0fb3c0

This first import is synchronized from the upstream
`gpio-event-cmsis-wrapper-migration` branch while the GPIO event layer is being
reviewed. After that branch is merged, the intended steady-state upstream
branch is `main`.

## Update Policy

The HAL-only repository is the upstream source of truth.

Please apply HAL fixes and HAL feature changes to the upstream HAL repository first, then synchronize this vendor copy.

CMSIS-Driver wrapper changes belong in this repository.
