# CMSIS GPIO Official API Gap Summary

This repository's local GPIO wrapper was ported from the `dspic33ak-hal-starter`
`cmsis-driver-gpio-official-api-alignment` branch. It aligns the local GPIO
wrapper with official CMSIS Driver_GPIO API names and signatures as far as is
useful for the current Phase 1 validation.

## Validation Result

- `ARM_GPIO_Pin_t` is `uint32_t`.
- `SetOutput()` uses the official void-return signature.
- `Driver_GPIO0` was validated on the `hal_starter` hardware before being moved
  into this wrapper repository.
- SW3 is active-low: falling edge = press = LED5 ON.
- SW3 is active-low: rising edge = release = LED5 OFF.
- SW1/SW2 polling behavior remains unchanged.
- Heartbeat and existing demos continue running.

## Accepted Phase 1 Gaps

- The wrapper still uses local `Driver_GPIO_dsPIC33AK.h`.
- It does not include the official CMSIS `Driver_GPIO.h` yet.
- `Setup()` only registers the callback for the packed pin slot.
- `Setup()` does not force default GPIO configuration.
- `Setup()` does not validate package-level pin availability.
- `Setup()` does not force hardware register access.
- Package-level pin availability is not checked.
- `GetInput()` returns `0` for invalid pins because there is no error return
  path.
- `SetOutput()` ignores invalid-pin failures because the official-style
  signature has no return path.

## HAL Impact

- The `dspic33ak_gpio` core sources are vendored under `src/hal_gpio/`.
- The `dspic33ak_gpio_event` sources are vendored under `src/hal_gpio/`.
- The GPIO core and event layer remain synchronized from `dspic33ak-gpio-hal`,
  which is the upstream source of truth.
- PPS remains a board/application responsibility.
