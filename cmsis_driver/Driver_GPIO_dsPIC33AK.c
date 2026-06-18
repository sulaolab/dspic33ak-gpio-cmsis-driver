/*
 * Driver_GPIO_dsPIC33AK.c
 * -----------------------
 * CMSIS-Driver GPIO-like wrapper implemented on top of dspic33ak_gpio and
 * dspic33ak_gpio_event. This validation layer does not own PPS or interrupt
 * vectors.
 */

#include "Driver_GPIO_dsPIC33AK.h"

#include <stdbool.h>
#include <stddef.h>

#include "dspic33ak_gpio_event.h"


//===========================================================
// Definition
//===========================================================

#define DRIVER_GPIO_PORT_COUNT  8u
#define DRIVER_GPIO_PIN_COUNT   16u

typedef struct
{
    ARM_GPIO_Pin_t          pin;
    ARM_GPIO_EVENT_TRIGGER  trigger;
    ARM_GPIO_SignalEvent_t  cb_event;
} driver_gpio_slot_t;


//===========================================================
// Function Prototype
//===========================================================

static int32_t  Driver_GPIO_Setup(ARM_GPIO_Pin_t pin, ARM_GPIO_SignalEvent_t cb_event);
static int32_t  Driver_GPIO_SetDirection(ARM_GPIO_Pin_t pin, ARM_GPIO_DIRECTION direction);
static int32_t  Driver_GPIO_SetOutputMode(ARM_GPIO_Pin_t pin, ARM_GPIO_OUTPUT_MODE mode);
static int32_t  Driver_GPIO_SetPullResistor(ARM_GPIO_Pin_t pin, ARM_GPIO_PULL_RESISTOR resistor);
static int32_t  Driver_GPIO_SetEventTrigger(ARM_GPIO_Pin_t pin, ARM_GPIO_EVENT_TRIGGER trigger);
static void     Driver_GPIO_SetOutput(ARM_GPIO_Pin_t pin, uint32_t val);
static uint32_t Driver_GPIO_GetInput(ARM_GPIO_Pin_t pin);

static bool                  driver_gpio_pin_index(ARM_GPIO_Pin_t pin,
                                                   uint8_t *port_index,
                                                   uint8_t *bit_index);
static driver_gpio_slot_t   *driver_gpio_slot_for(ARM_GPIO_Pin_t pin);
static dspic33ak_gpio_pull_t driver_gpio_pull_to_hal(ARM_GPIO_PULL_RESISTOR resistor);
static bool                  driver_gpio_trigger_matches(ARM_GPIO_EVENT_TRIGGER trigger,
                                                         uint32_t event);
static bool                  driver_gpio_to_hal_pin(ARM_GPIO_Pin_t pin,
                                                    dspic33ak_gpio_pin_t *hal_pin);
static void                  driver_gpio_hal_event(dspic33ak_gpio_pin_t pin,
                                                   dspic33ak_gpio_event_edge_t edge,
                                                   void *user_data);


//===========================================================
// Variables
//===========================================================

static driver_gpio_slot_t s_gpio_slots[DRIVER_GPIO_PORT_COUNT][DRIVER_GPIO_PIN_COUNT];

ARM_DRIVER_GPIO Driver_GPIO0 =
{
    Driver_GPIO_Setup,
    Driver_GPIO_SetDirection,
    Driver_GPIO_SetOutputMode,
    Driver_GPIO_SetPullResistor,
    Driver_GPIO_SetEventTrigger,
    Driver_GPIO_SetOutput,
    Driver_GPIO_GetInput
};


//===========================================================
// Local Function
//===========================================================

static int32_t Driver_GPIO_Setup(ARM_GPIO_Pin_t pin, ARM_GPIO_SignalEvent_t cb_event)
{
    driver_gpio_slot_t *slot = driver_gpio_slot_for(pin);

    if (slot == 0)
    {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    slot->pin = pin;
    slot->cb_event = cb_event;
    return ARM_DRIVER_OK;
}

static int32_t Driver_GPIO_SetDirection(ARM_GPIO_Pin_t pin, ARM_GPIO_DIRECTION direction)
{
    dspic33ak_gpio_pin_t hal_pin;
    dspic33ak_gpio_dir_t hal_direction;

    if (!driver_gpio_to_hal_pin(pin, &hal_pin))
    {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    if (direction == ARM_GPIO_INPUT)
    {
        hal_direction = DSPIC33AK_GPIO_DIR_INPUT;
    }
    else if (direction == ARM_GPIO_OUTPUT)
    {
        hal_direction = DSPIC33AK_GPIO_DIR_OUTPUT;
    }
    else
    {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    return dspic33ak_gpio_set_direction(hal_pin, hal_direction) ? ARM_DRIVER_OK : ARM_DRIVER_ERROR_PARAMETER;
}

static int32_t Driver_GPIO_SetOutputMode(ARM_GPIO_Pin_t pin, ARM_GPIO_OUTPUT_MODE mode)
{
    dspic33ak_gpio_pin_t hal_pin;
    bool open_drain;

    if (!driver_gpio_to_hal_pin(pin, &hal_pin))
    {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    if (mode == ARM_GPIO_PUSH_PULL)
    {
        open_drain = false;
    }
    else if (mode == ARM_GPIO_OPEN_DRAIN)
    {
        open_drain = true;
    }
    else
    {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    return dspic33ak_gpio_set_open_drain(hal_pin, open_drain) ? ARM_DRIVER_OK : ARM_DRIVER_ERROR_PARAMETER;
}

static int32_t Driver_GPIO_SetPullResistor(ARM_GPIO_Pin_t pin, ARM_GPIO_PULL_RESISTOR resistor)
{
    dspic33ak_gpio_pin_t hal_pin;
    dspic33ak_gpio_pull_t hal_pull = driver_gpio_pull_to_hal(resistor);

    if (!driver_gpio_to_hal_pin(pin, &hal_pin))
    {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    if ((resistor != ARM_GPIO_PULL_NONE) &&
        (resistor != ARM_GPIO_PULL_UP) &&
        (resistor != ARM_GPIO_PULL_DOWN))
    {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    return dspic33ak_gpio_set_pull(hal_pin, hal_pull) ? ARM_DRIVER_OK : ARM_DRIVER_ERROR_PARAMETER;
}

static int32_t Driver_GPIO_SetEventTrigger(ARM_GPIO_Pin_t pin, ARM_GPIO_EVENT_TRIGGER trigger)
{
    driver_gpio_slot_t *slot = driver_gpio_slot_for(pin);
    dspic33ak_gpio_pin_t hal_pin;

    if ((slot == 0) || !driver_gpio_to_hal_pin(pin, &hal_pin))
    {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    if (trigger == ARM_GPIO_TRIGGER_NONE)
    {
        if (!dspic33ak_gpio_event_detach(hal_pin))
        {
            return ARM_DRIVER_ERROR_PARAMETER;
        }
        slot->trigger = trigger;
        return ARM_DRIVER_OK;
    }

    if ((trigger != ARM_GPIO_TRIGGER_RISING_EDGE) &&
        (trigger != ARM_GPIO_TRIGGER_FALLING_EDGE) &&
        (trigger != ARM_GPIO_TRIGGER_EITHER_EDGE))
    {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    if (slot->cb_event == 0)
    {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    if (!dspic33ak_gpio_event_attach(hal_pin,
                                     DSPIC33AK_GPIO_EVENT_EDGE_EITHER,
                                     driver_gpio_hal_event,
                                     slot))
    {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    slot->pin = pin;
    slot->trigger = trigger;
    return ARM_DRIVER_OK;
}

static void Driver_GPIO_SetOutput(ARM_GPIO_Pin_t pin, uint32_t val)
{
    dspic33ak_gpio_pin_t hal_pin;

    if (!driver_gpio_to_hal_pin(pin, &hal_pin))
    {
        return;
    }

    (void)dspic33ak_gpio_write(hal_pin, (val != 0u));
}

static uint32_t Driver_GPIO_GetInput(ARM_GPIO_Pin_t pin)
{
    dspic33ak_gpio_pin_t hal_pin;

    if (!driver_gpio_to_hal_pin(pin, &hal_pin))
    {
        return 0u;
    }

    return dspic33ak_gpio_read(hal_pin) ? 1u : 0u;
}

static bool driver_gpio_pin_index(ARM_GPIO_Pin_t pin, uint8_t *port_index, uint8_t *bit_index)
{
    unsigned decoded_port = (unsigned)(pin >> 4);
    uint8_t decoded_bit = (uint8_t)(pin & 0x0Fu);

    if ((port_index == 0) || (bit_index == 0) || (decoded_port >= DRIVER_GPIO_PORT_COUNT))
    {
        return false;
    }

    *port_index = (uint8_t)decoded_port;
    *bit_index = decoded_bit;
    return true;
}

static driver_gpio_slot_t *driver_gpio_slot_for(ARM_GPIO_Pin_t pin)
{
    uint8_t port_index;
    uint8_t bit_index;

    if (!driver_gpio_pin_index(pin, &port_index, &bit_index))
    {
        return 0;
    }

    return &s_gpio_slots[port_index][bit_index];
}

static dspic33ak_gpio_pull_t driver_gpio_pull_to_hal(ARM_GPIO_PULL_RESISTOR resistor)
{
    switch (resistor)
    {
    case ARM_GPIO_PULL_UP:
        return DSPIC33AK_GPIO_PULL_UP;
    case ARM_GPIO_PULL_DOWN:
        return DSPIC33AK_GPIO_PULL_DOWN;
    case ARM_GPIO_PULL_NONE:
    default:
        return DSPIC33AK_GPIO_PULL_NONE;
    }
}

static bool driver_gpio_trigger_matches(ARM_GPIO_EVENT_TRIGGER trigger, uint32_t event)
{
    if (trigger == ARM_GPIO_TRIGGER_EITHER_EDGE)
    {
        return true;
    }
    if ((trigger == ARM_GPIO_TRIGGER_RISING_EDGE) &&
        ((event & ARM_GPIO_EVENT_RISING_EDGE) != 0u))
    {
        return true;
    }
    if ((trigger == ARM_GPIO_TRIGGER_FALLING_EDGE) &&
        ((event & ARM_GPIO_EVENT_FALLING_EDGE) != 0u))
    {
        return true;
    }
    return false;
}

static bool driver_gpio_to_hal_pin(ARM_GPIO_Pin_t pin, dspic33ak_gpio_pin_t *hal_pin)
{
    uint8_t port_index;
    uint8_t bit_index;

    if ((hal_pin == 0) || !driver_gpio_pin_index(pin, &port_index, &bit_index))
    {
        return false;
    }

    (void)port_index;
    (void)bit_index;
    *hal_pin = (dspic33ak_gpio_pin_t)pin;
    return true;
}

static void driver_gpio_hal_event(dspic33ak_gpio_pin_t pin,
                                  dspic33ak_gpio_event_edge_t edge,
                                  void *user_data)
{
    driver_gpio_slot_t *slot = (driver_gpio_slot_t *)user_data;
    uint32_t event;

    if ((slot == 0) || (slot->cb_event == 0))
    {
        return;
    }

    if (edge == DSPIC33AK_GPIO_EVENT_EDGE_RISING)
    {
        event = ARM_GPIO_EVENT_RISING_EDGE;
    }
    else if (edge == DSPIC33AK_GPIO_EVENT_EDGE_FALLING)
    {
        event = ARM_GPIO_EVENT_FALLING_EDGE;
    }
    else
    {
        return;
    }

    if (driver_gpio_trigger_matches(slot->trigger, event))
    {
        slot->cb_event((ARM_GPIO_Pin_t)pin, event);
    }
}
