# CMSIS GPIO Wrapper Validation Design

This repository contains a small CMSIS-Driver GPIO-like wrapper together with a
vendor copy of the `dspic33ak-gpio-hal` GPIO core and CN event layer. It is
deliberately experimental: the goal is readable FAE/evaluation code, not a
complete production GPIO driver.

The wrapper design and behavior were first validated in the
`dspic33ak-hal-starter` `cmsis-driver-gpio-official-api-alignment` branch before
being moved into this standalone wrapper repository.

## Scope

Implemented files:

- `cmsis_driver/Driver_GPIO_dsPIC33AK.h`
- `cmsis_driver/Driver_GPIO_dsPIC33AK.c`
- `docs/cmsis_driver_gpio_wrapper_design.md`

Vendored HAL headers under `src/hal_gpio/`:

- `dspic33ak_gpio.h`
- `dspic33ak_gpio_event.h`

The wrapper exports one instance:

```c
extern ARM_DRIVER_GPIO Driver_GPIO0;
```

## Layering

The wrapper sits above the existing layers:

```text
Consumer app
  -> Driver_GPIO0                    CMSIS-Driver-like API
    -> dspic33ak_gpio                ANSEL/TRIS/LAT/PORT/ODC/CNPU/CNPD
    -> dspic33ak_gpio_event          CN edge dispatch
      -> app-owned CN interrupt vector
```

The core GPIO HAL and GPIO event layer are vendored from `dspic33ak-gpio-hal`
under `src/hal_gpio/`. PPS remains in the board/application layer and is not
moved into the wrapper.

## Types

CMSIS-like GPIO types are isolated in this repository's `cmsis_driver/` wrapper
files.

`ARM_GPIO_Pin_t` is a local CMSIS-compatible `uint32_t` typedef:

```c
typedef uint32_t ARM_GPIO_Pin_t;
```

The value layout remains compatible with the existing packed HAL pin value from
`DSPIC33AK_GPIO_PIN(port, bit)`. The wrapper casts to `dspic33ak_gpio_pin_t`
internally only after validating the packed port/bit slot.

This keeps the wrapper thin and avoids introducing a second pin database for
Phase 1 validation.

## Official CMSIS API Alignment

The local validation header uses official CMSIS Driver_GPIO API names and
signatures where practical, without importing the official CMSIS Pack header
yet.

Local enum values now use official-style names:

- `ARM_GPIO_INPUT` / `ARM_GPIO_OUTPUT`
- `ARM_GPIO_PUSH_PULL` / `ARM_GPIO_OPEN_DRAIN`
- `ARM_GPIO_TRIGGER_NONE`
- `ARM_GPIO_TRIGGER_RISING_EDGE`
- `ARM_GPIO_TRIGGER_FALLING_EDGE`
- `ARM_GPIO_TRIGGER_EITHER_EDGE`

The event bits include:

- `ARM_GPIO_EVENT_RISING_EDGE`
- `ARM_GPIO_EVENT_FALLING_EDGE`
- `ARM_GPIO_EVENT_EITHER_EDGE`

`SetOutput(pin, val)` now follows the official void-return signature. It still
calls `dspic33ak_gpio_write()` internally, but invalid-pin failures are ignored
because the official-style API has no return path.

Remaining gap: this project still provides a local
`cmsis_driver/Driver_GPIO_dsPIC33AK.h` header and does not include the official
CMSIS `Driver_GPIO.h` header. This keeps the validation wrapper self-contained
while preserving the intended API shape.

## API Mapping

The `ARM_DRIVER_GPIO` function table contains:

- `Setup(pin, cb_event)`
- `SetDirection(pin, direction)`
- `SetOutputMode(pin, mode)`
- `SetPullResistor(pin, resistor)`
- `SetEventTrigger(pin, trigger)`
- `SetOutput(pin, val)`
- `GetInput(pin)`

Mapping to existing HAL calls:

| Wrapper API | HAL mapping |
| --- | --- |
| `SetDirection(ARM_GPIO_INPUT/ARM_GPIO_OUTPUT)` | `dspic33ak_gpio_set_direction()` |
| `SetOutputMode(ARM_GPIO_PUSH_PULL/ARM_GPIO_OPEN_DRAIN)` | `dspic33ak_gpio_set_open_drain()` |
| `SetPullResistor(ARM_GPIO_PULL_NONE/UP/DOWN)` | `dspic33ak_gpio_set_pull()` |
| `SetOutput()` | `dspic33ak_gpio_write()` |
| `GetInput()` | `dspic33ak_gpio_read()` |
| `SetEventTrigger()` | `dspic33ak_gpio_event_attach()` / `detach()` |

## `Setup()` Validation Policy

Phase 1 adopts option C for `Driver_GPIO_Setup()` behavior.

`Setup(pin, cb_event)` only registers the callback for the wrapper's packed pin
slot. It does not validate package-level pin availability and does not force any
hardware register access.

Actual pin/register validity is checked later when wrapper APIs call the
underlying HAL or event layer:

- `SetDirection()`
- `SetOutputMode()`
- `SetPullResistor()`
- `SetEventTrigger()`

`SetOutput()` also calls the underlying HAL, but it has no return path after the
official API alignment and therefore ignores invalid-pin failures.

`GetInput()` still returns `0` for invalid pins because the CMSIS-like signature
has no error return path.

This behavior is accepted for the Phase 1 validation port to keep the wrapper
thin and avoid adding a new HAL pin validation API.

## Event Bridge

The GPIO event HAL currently supports `EDGE_EITHER` attach. The CMSIS wrapper
therefore always attaches the HAL event layer with `DSPIC33AK_GPIO_EVENT_EDGE_EITHER`
for rising, falling, and either-edge requests.

The wrapper stores the requested CMSIS trigger per pin and filters the HAL event
before calling the CMSIS callback:

- HAL `DSPIC33AK_GPIO_EVENT_EDGE_RISING` maps to `ARM_GPIO_EVENT_RISING_EDGE`
- HAL `DSPIC33AK_GPIO_EVENT_EDGE_FALLING` maps to `ARM_GPIO_EVENT_FALLING_EDGE`
- CMSIS `RISING`, `FALLING`, and `EITHER` are filtered in the wrapper
- CMSIS `NONE` detaches the pin from the HAL event layer

The application still owns the CN interrupt vector. A consumer app typically
forwards the vector to the event layer provided by `dspic33ak-gpio-hal`:

```c
void __attribute__((__interrupt__, __no_auto_psv__)) _CNBInterrupt(void)
{
    dspic33ak_gpio_event_process_isr();
}
```

The wrapper does not define interrupt vectors.

## Consumer Integration Snippet

Consumer projects must compile this repository's wrapper source and the vendored
GPIO core and event layer sources.

Include paths must cover both repositories:

- `cmsis_driver/`
- `src/hal_gpio/`

Minimal setup shape:

```c
#include "Driver_GPIO_dsPIC33AK.h"
#include "dspic33ak_gpio.h"
#include "dspic33ak_gpio_event.h"

static volatile uint32_t sw3_event_flags;

static void sw3_gpio_event(ARM_GPIO_Pin_t pin, uint32_t event)
{
    (void)pin;
    sw3_event_flags |= event;
}

void __attribute__((__interrupt__, __no_auto_psv__)) _CNBInterrupt(void)
{
    dspic33ak_gpio_event_process_isr();
}

void app_gpio_init(void)
{
    (void)dspic33ak_gpio_set_analog(BOARD_SW3, false);
    (void)Driver_GPIO0.SetDirection(BOARD_SW3, ARM_GPIO_INPUT);
    (void)Driver_GPIO0.SetPullResistor(BOARD_SW3, ARM_GPIO_PULL_UP);
    (void)Driver_GPIO0.Setup(BOARD_SW3, sw3_gpio_event);
    (void)Driver_GPIO0.SetEventTrigger(BOARD_SW3, ARM_GPIO_TRIGGER_EITHER_EDGE);
}
```

## SW3 Source Validation Behavior

SW1 and SW2 remain polling inputs. SW3 uses `Driver_GPIO0`:

1. The app clears ANSEL for SW3 because the event layer and wrapper do not change
   analog/digital mode.
2. The app calls `Driver_GPIO0.SetDirection(SW3, ARM_GPIO_INPUT)`.
3. The app calls `Driver_GPIO0.SetPullResistor(SW3, ARM_GPIO_PULL_UP)`.
4. The app reads initial SW3 state with `Driver_GPIO0.GetInput(SW3)`.
5. The app calls `Driver_GPIO0.Setup(SW3, callback)`.
6. The app calls `Driver_GPIO0.SetEventTrigger(SW3, ARM_GPIO_TRIGGER_EITHER_EDGE)`.
7. The app enables the CNB interrupt source.

SW3 is active-low:

- falling edge = press = LED5 ON
- rising edge = release = LED5 OFF

The callback only updates volatile state. LED5 and printf logging are updated in
the main loop.

## What The Wrapper Does Not Do

The wrapper does not change:

- ANSEL
- PPS
- interrupt vectors
- CN interrupt priority
- CN interrupt enable bits

`SetDirection()`, `SetOutputMode()`, and `SetPullResistor()` do call the
external GPIO HAL and therefore update TRIS, ODC, CNPU, and CNPD as requested.

## Current Limitations

- This uses local official-style CMSIS GPIO names, but it is not yet wired to
  the official CMSIS `Driver_GPIO.h` header.
- No version/capability query functions are implemented.
- `GetInput()` returns `0` for invalid pins because its signature has no error
  return path.
- There is no debounce.
- There is one callback slot per packed port/bit pin.
- Package-level pin availability is not checked.
- PPS ownership remains an application/board-layer concern.

## Future Work

- Decide whether a production wrapper should use board enum IDs instead of raw
  packed pins.
- Decide whether GPIO setup should force ANSEL to digital or require explicit
  analog ownership from the application.
- Add optional capability/version functions if aligning with a broader CMSIS
  driver pattern.
- Add debounce or event timestamping only if a real validation need appears.
- Promote single-edge attach into the HAL event layer only after the dsPIC33AK
  `CNEN0x`/`CNEN1x` polarity mapping is documented in the HAL.
