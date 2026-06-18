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
