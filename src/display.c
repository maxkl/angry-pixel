
#include "display.h"

#include <stdint.h>

#include <driverlib/sysctl.h>
#include <inc/hw_types.h>

// We use this macro for really fast GPIO access
#define fast_GPIOPinWrite(base, pins, data) (HWREG((base) | (pins) << 2) = (data))

static uint8_t display_buffer[DISPLAY_HEIGHT][DISPLAY_WIDTH / 8];

void display_init() {
    // Configure GPIO pins
    SysCtlPeripheralEnable(DISPLAY_PORT_PERIPH);
    // Access the GPIO registers over the AHB, for increased performance
    SysCtlGPIOAHBEnable(DISPLAY_PORT_PERIPH);
    GPIOPinTypeGPIOOutput(DISPLAY_PORT_BASE, DISPLAY_PINS);
    fast_GPIOPinWrite(DISPLAY_PORT_BASE, DISPLAY_PINS, 0x00);
}

uint8_t *display_get_buffer() {
    // Make the underlying buffer accessible to the outside world (see canvas.c)
    return display_buffer;
}

void display_refresh() {
    // Push the buffer out

    // The display is essentially a 64-bit-wide buffered shift register
    // One of the 16 lines at a time displays the contents of that shift register
    // A pulse on 'SHIFT' shifts the date on 'DATA' in from the left
    // A pulse on 'LATCH' transfers the data to the shift registers output buffer
    // The signals 'LINE_A' to 'LINE_D' select the active line

    // Iterate over all 16 lines
    for (uint8_t line = 0; line < DISPLAY_HEIGHT; line++) {
        // Shift the lines data out
        for (uint8_t byte_index = 0; byte_index < DISPLAY_WIDTH / 8; byte_index++) {
            uint8_t data = display_buffer[line][byte_index];

            for (uint8_t i = 0; i < 8; i++) {
                uint8_t data_bit = !(data & 0x1) ? DISPLAY_PIN_DATA : 0;

                // Apply the data
                fast_GPIOPinWrite(DISPLAY_PORT_BASE, DISPLAY_PIN_DATA, data_bit);

                // Shift a single bit by pulsing 'SHIFT'
                fast_GPIOPinWrite(DISPLAY_PORT_BASE, DISPLAY_PIN_SHIFT, DISPLAY_PIN_SHIFT);
                fast_GPIOPinWrite(DISPLAY_PORT_BASE, DISPLAY_PIN_SHIFT, 0);

                // Next bit
                data >>= 1;
            }
        }

        // Update the selected line
        fast_GPIOPinWrite(DISPLAY_PORT_BASE, DISPLAY_LINE_PINS, line << 4);

        // Latch the new data
        fast_GPIOPinWrite(DISPLAY_PORT_BASE, DISPLAY_PIN_LATCH, DISPLAY_PIN_LATCH);
        fast_GPIOPinWrite(DISPLAY_PORT_BASE, DISPLAY_PIN_LATCH, 0);
    }
}
