# dspic33ak-gpio-cmsis-driver

Experimental CMSIS-Driver GPIO-style wrapper package for the dsPIC33AK GPIO HAL.

This repository provides a CMSIS-Driver GPIO-style wrapper together with a
vendor copy of the dsPIC33AK GPIO HAL. It is intended for evaluation, FAE demos,
and early software architecture experiments.

## Repository Layout

```text
src/
  hal_gpio/
    dspic33ak_gpio.c
    dspic33ak_gpio.h
    dspic33ak_gpio_event.c
    dspic33ak_gpio_event.h
    dspic33ak_gpio_reg.h
    UPSTREAM.md

tools/
  sync_hal_from_upstream.py

cmsis_driver/
  Driver_GPIO_dsPIC33AK.c
  Driver_GPIO_dsPIC33AK.h

docs/
  cmsis_driver_gpio_wrapper_design.md
  cmsis_gpio_official_gap_summary.md
```

## Current Status

This repository is in an early experimental stage.

The initial GPIO wrapper design has been validated in the
[dspic33ak-hal-starter](https://github.com/sulaolab/dspic33ak-hal-starter)
hardware validation project before being moved here.

The HAL vendor copy has been imported from:

- https://github.com/sulaolab/dspic33ak-gpio-hal

The CMSIS-Driver wrapper files are provided under `cmsis_driver/`.

## Scope

This repository is intended to contain:

* A local CMSIS-Driver GPIO-style wrapper for dsPIC33AK
* `Driver_GPIO0` access structure
* GPIO direction, output mode, pull resistor, input, and output APIs
* GPIO CN event bridge support through the dsPIC33AK GPIO HAL event layer
* Documentation and small integration snippets

## Include Path

Applications or build systems should provide include paths for:

```text
src/hal_gpio
cmsis_driver
```

## Consumer Integration

Consumer projects should:

* compile `cmsis_driver/Driver_GPIO_dsPIC33AK.c`
* compile/link the required `src/hal_gpio/` sources
* add `src/hal_gpio` to the C include path
* add `cmsis_driver` to the C include path
* define board-level packed pin names using the GPIO HAL pin representation
* configure ANSEL/TRIS/pull policy before attaching events as needed
* own CN interrupt vectors in the application
* call `dspic33ak_gpio_event_process_isr()` from the app-owned CN vector

PPS remains a board/application responsibility and is not moved into this
wrapper.

## HAL Synchronization

The HAL vendor copy under `src/hal_gpio/` can be synchronized from the upstream
HAL-only repository using:

```powershell
# Windows (Python launcher)
py -3 tools/sync_hal_from_upstream.py

# or, if python is on PATH
python tools/sync_hal_from_upstream.py
```

For temporary pre-main validation, a branch or tag can be selected explicitly:

```powershell
py -3 tools/sync_hal_from_upstream.py --branch gpio-event-cmsis-wrapper-migration
```

## Upstream HAL Policy

The HAL-only repository is the upstream source of truth:

- https://github.com/sulaolab/dspic33ak-gpio-hal

HAL fixes should be applied to the upstream HAL repository first, then
synchronized into this repository.

CMSIS-Driver wrapper changes should be made in this repository.

## Non-Goals

This repository does not intend to:

* Replace `dspic33ak-gpio-hal`
* Move PPS configuration into the GPIO driver
* Own interrupt vectors inside the driver
* Provide production-certified code
* Import the official CMSIS `Driver_GPIO.h` header in the first phase
* Vendor ARM CMSIS third-party headers in the first phase

## Related Repositories

* [dspic33ak-gpio-hal](https://github.com/sulaolab/dspic33ak-gpio-hal)
* [dspic33ak-hal-starter](https://github.com/sulaolab/dspic33ak-hal-starter)

## License

The original dsPIC33AK GPIO CMSIS-Driver wrapper code in this repository is
licensed under the MIT No Attribution License (MIT-0). See `LICENSE`.
