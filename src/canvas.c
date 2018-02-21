
#include "canvas.h"

#include <stdint.h>
#include <string.h>

#define CANVAS_BOUNDS_CHECK(x, y) ((x) < 0 || (x) >= CANVAS_WIDTH || (y) < 0 || (y) >= CANVAS_HEIGHT)

// This is the buffer we draw to, every bit is a pixel/LED
static uint8_t *canvas_buffer;

// canvas.c doesn't transfer the buffer to the display, that's what display.c does

void canvas_set_buffer(uint8_t *buffer) {
    canvas_buffer = buffer;
}

void canvas_clear() {
    memset(canvas_buffer, 0x00, (CANVAS_WIDTH * CANVAS_HEIGHT) / 8);
}

void canvas_pixel_set(int x, int y) {
    if (CANVAS_BOUNDS_CHECK(x, y)) {
        return;
    }

    canvas_buffer[(y * CANVAS_WIDTH + x) / 8] |= 1 << (x % 8);
}

void canvas_pixel_clear(int x, int y) {
    if (CANVAS_BOUNDS_CHECK(x, y)) {
        return;
    }

    canvas_buffer[(y * CANVAS_WIDTH + x) / 8] &= ~(1 << (x % 8));
}

void canvas_hline(float x1, float x2, float y) {
    if (x1 > x2) {
        float tmp = x2;
        x2 = x1;
        x1 = tmp;
    }

    for (int x = (int) x1; x <= (int) x2; x++) {
        canvas_pixel_set(x, y);
    }
}

void canvas_vline(float x, float y1, float y2) {
    if (y1 > y2) {
        float tmp = y2;
        y2 = y1;
        y1 = tmp;
    }

    for (int y = (int) y1; y <= (int) y2; y++) {
        canvas_pixel_set(x, y);
    }
}

void canvas_rect_fill(float x1, float y1, float x2, float y2) {
    if (x1 > x2) {
        float tmp = x2;
        x2 = x1;
        x1 = tmp;
    }
    if (y1 > y2) {
        float tmp = y2;
        y2 = y1;
        y1 = tmp;
    }

    for (int y = (int) y1; y <= (int) y2; y++) {
        for (int x = (int) x1; x <= (int) x2; x++) {
            canvas_pixel_set((int) x, (int) y);
        }
    }
}

void canvas_rect_stroke(float x1, float y1, float x2, float y2) {
    canvas_hline(x1, x2, y1);
    canvas_vline(x2, y1, y2);
    canvas_hline(x2, x1, y2);
    canvas_vline(x1, y2, y1);
}

void canvas_bitmap(int offset_x, int offset_y, const uint8_t *bitmap, int w, int h) {
    int bytes_per_row = (w + 7) / 8;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            // The MSB is drawn left-most
            if (bitmap[y * bytes_per_row + x / 8] & (1 << (7 - (x % 8)))) {
                canvas_pixel_set(offset_x + x, offset_y + y);
            } else {
                canvas_pixel_clear(offset_x + x, offset_y + y);
            }
        }
    }
}
