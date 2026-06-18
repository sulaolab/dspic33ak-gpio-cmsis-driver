#ifndef DRIVER_GPIO_DSPIC33AK_H
#define DRIVER_GPIO_DSPIC33AK_H

/*
 * Driver_GPIO_dsPIC33AK.h
 * -----------------------
 * Small CMSIS-Driver GPIO-like wrapper for dsPIC33AK validation.
 *
 * This is intentionally isolated from the core HAL. ARM_GPIO_Pin_t is a
 * 32-bit local CMSIS-compatible handle; values remain compatible with the
 * existing packed dspic33ak_gpio_pin_t: (port << 4) | bit.
 */

#include <stdint.h>

#include "dspic33ak_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ARM_DRIVER_OK
#define ARM_DRIVER_OK                 (0)
#endif
#ifndef ARM_DRIVER_ERROR
#define ARM_DRIVER_ERROR              (-1)
#endif
#ifndef ARM_DRIVER_ERROR_UNSUPPORTED
#define ARM_DRIVER_ERROR_UNSUPPORTED  (-4)
#endif
#ifndef ARM_DRIVER_ERROR_PARAMETER
#define ARM_DRIVER_ERROR_PARAMETER    (-5)
#endif

typedef uint32_t ARM_GPIO_Pin_t;

typedef enum
{
    ARM_GPIO_INPUT = 0,
    ARM_GPIO_OUTPUT
} ARM_GPIO_DIRECTION;

typedef enum
{
    ARM_GPIO_PUSH_PULL = 0,
    ARM_GPIO_OPEN_DRAIN
} ARM_GPIO_OUTPUT_MODE;

typedef enum
{
    ARM_GPIO_PULL_NONE = 0,
    ARM_GPIO_PULL_UP,
    ARM_GPIO_PULL_DOWN
} ARM_GPIO_PULL_RESISTOR;

typedef enum
{
    ARM_GPIO_TRIGGER_NONE = 0,
    ARM_GPIO_TRIGGER_RISING_EDGE,
    ARM_GPIO_TRIGGER_FALLING_EDGE,
    ARM_GPIO_TRIGGER_EITHER_EDGE
} ARM_GPIO_EVENT_TRIGGER;

#define ARM_GPIO_EVENT_RISING_EDGE   (1UL << 0)
#define ARM_GPIO_EVENT_FALLING_EDGE  (1UL << 1)
#define ARM_GPIO_EVENT_EITHER_EDGE   (1UL << 2)

typedef void (*ARM_GPIO_SignalEvent_t)(ARM_GPIO_Pin_t pin, uint32_t event);

typedef struct
{
    int32_t  (*Setup)(ARM_GPIO_Pin_t pin, ARM_GPIO_SignalEvent_t cb_event);
    int32_t  (*SetDirection)(ARM_GPIO_Pin_t pin, ARM_GPIO_DIRECTION direction);
    int32_t  (*SetOutputMode)(ARM_GPIO_Pin_t pin, ARM_GPIO_OUTPUT_MODE mode);
    int32_t  (*SetPullResistor)(ARM_GPIO_Pin_t pin, ARM_GPIO_PULL_RESISTOR resistor);
    int32_t  (*SetEventTrigger)(ARM_GPIO_Pin_t pin, ARM_GPIO_EVENT_TRIGGER trigger);
    void     (*SetOutput)(ARM_GPIO_Pin_t pin, uint32_t val);
    uint32_t (*GetInput)(ARM_GPIO_Pin_t pin);
} ARM_DRIVER_GPIO;

extern ARM_DRIVER_GPIO Driver_GPIO0;

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_GPIO_DSPIC33AK_H */
