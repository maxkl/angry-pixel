
#ifndef __CANVAS_H__
#define __CANVAS_H__

#include <stdint.h>

#define CANVAS_WIDTH 64
#define CANVAS_HEIGHT 16

void canvas_set_buffer(uint8_t *buffer);
void canvas_clear();
void canvas_pixel_set(int x, int y);
void canvas_pixel_clear(int x, int y);
void canvas_hline(float x1, float x2, float y);
void canvas_vline(float x, float y1, float y2);
void canvas_line(float x1, float y1, float x2, float y2);
void canvas_rect_fill(float x1, float y1, float x2, float y2);
void canvas_rect_stroke(float x1, float y1, float x2, float y2);
void canvas_circle_fill(float x, float y, float r);
void canvas_circle_stroke(float x, float y, float r);
void canvas_bitmap(int offset_x, int offset_y, const uint8_t *bitmap, int w, int h);

#endif /* __CANVAS_H__ */
