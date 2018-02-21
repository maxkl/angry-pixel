
#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <stdint.h>
#include <stdbool.h>

#include <driverlib/gpio.h>
#include <inc/hw_memmap.h>

#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 16

#define DISPLAY_PORT_PERIPH SYSCTL_PERIPH_GPIOB
#define DISPLAY_PORT_BASE GPIO_PORTB_AHB_BASE

#define DISPLAY_PIN_DATA GPIO_PIN_0
#define DISPLAY_PIN_SHIFT GPIO_PIN_1
#define DISPLAY_PIN_LATCH GPIO_PIN_2
#define DISPLAY_PIN_ENABLE GPIO_PIN_3
#define DISPLAY_PIN_LINE_A GPIO_PIN_4
#define DISPLAY_PIN_LINE_B GPIO_PIN_5
#define DISPLAY_PIN_LINE_C GPIO_PIN_6
#define DISPLAY_PIN_LINE_D GPIO_PIN_7
#define DISPLAY_LINE_PINS (DISPLAY_PIN_LINE_A | DISPLAY_PIN_LINE_B | DISPLAY_PIN_LINE_C | DISPLAY_PIN_LINE_D)
#define DISPLAY_PINS (\
    DISPLAY_PIN_DATA | DISPLAY_PIN_SHIFT | DISPLAY_PIN_LATCH | DISPLAY_PIN_ENABLE | \
    DISPLAY_LINE_PINS \
)

void display_init();
uint8_t *display_get_buffer();
void display_refresh();

#endif /* __DISPLAY_H__ */
