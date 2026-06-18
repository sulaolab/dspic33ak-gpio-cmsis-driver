# dspic33ak-gpio-cmsis-driver

Experimental CMSIS-Driver GPIO-style wrapper for the dsPIC33AK GPIO HAL.

This repository provides a thin Driver_GPIO-style wrapper on top of
[dspic33ak-gpio-hal](https://github.com/sulaolab/dspic33ak-gpio-hal).
It is intended for evaluation, FAE demos, and early software architecture
experiments.

## Status

This repository is in an early experimental stage.

The initial GPIO wrapper design has been validated in the
[dspic33ak-hal-starter](https://github.com/sulaolab/dspic33ak-hal-starter)
hardware validation project before being moved here.

## Scope

This repository is intended to contain:

* A local CMSIS-Driver GPIO-style wrapper for dsPIC33AK
* `Driver_GPIO0` access structure
* GPIO direction, output mode, pull resistor, input, and output APIs
* GPIO CN event bridge support through the dsPIC33AK GPIO HAL event layer
* Documentation and small integration examples or snippets

## Dependencies

This repository depends on
[dspic33ak-gpio-hal](https://github.com/sulaolab/dspic33ak-gpio-hal).

The GPIO HAL repository provides:

* `dspic33ak_gpio.h`
* `dspic33ak_gpio_event.h`
* the GPIO core source
* the GPIO CN event layer source

Those HAL sources are intentionally not copied into this repository.

## Files

```text
src/
  Driver_GPIO_dsPIC33AK.h
  Driver_GPIO_dsPIC33AK.c
docs/
  cmsis_driver_gpio_wrapper_design.md
  cmsis_gpio_official_gap_summary.md
```

## Consumer Integration

Consumer projects should:

* compile `src/Driver_GPIO_dsPIC33AK.c`
* compile/link the required `dspic33ak-gpio-hal` sources separately
* add this repository's `src/` to the C include path
* add `dspic33ak-gpio-hal/src/` to the C include path
* define board-level packed pin names using the GPIO HAL pin representation
* configure ANSEL/TRIS/pull policy before attaching events as needed
* own CN interrupt vectors in the application
* call `dspic33ak_gpio_event_process_isr()` from the app-owned CN vector

PPS remains a board/application responsibility and is not moved into this
wrapper.

## Non-Goals

This repository does not intend to:

* Replace `dspic33ak-gpio-hal`
* Move PPS configuration into the GPIO driver
* Own interrupt vectors inside the driver
* Provide production-certified code
* Import the official CMSIS `Driver_GPIO.h` header in the first phase

## Related Repositories

* [dspic33ak-gpio-hal](https://github.com/sulaolab/dspic33ak-gpio-hal)
* [dspic33ak-hal-starter](https://github.com/sulaolab/dspic33ak-hal-starter)

## License

MIT-0
