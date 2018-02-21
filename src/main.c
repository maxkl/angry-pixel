
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#define __BSD_VISIBLE // enable math constants
#include <math.h>

#include <driverlib/sysctl.h>
#include <driverlib/interrupt.h>
#include <driverlib/timer.h>
#include <driverlib/gpio.h>
#include <inc/hw_types.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_gpio.h>

#include "display.h"
#include "canvas.h"

#include "levels.h"

#include "bitmaps/digits.c"
#include "bitmaps/lvl.c"
#include "bitmaps/cleared.c"
#include "bitmaps/failed.c"
#include "bitmaps/retry.c"
#include "bitmaps/next.c"

#define BUTTONS_PORT_PERIPH SYSCTL_PERIPH_GPIOA
#define BUTTONS_PORT_BASE GPIO_PORTA_BASE
#define BUTTON_PIN_A_DOWN GPIO_PIN_2
#define BUTTON_PIN_A_UP GPIO_PIN_3
#define BUTTON_PIN_P_DOWN GPIO_PIN_4
#define BUTTON_PIN_P_UP GPIO_PIN_5
#define BUTTON_PIN_THROW GPIO_PIN_6
#define BUTTON_PINS (BUTTON_PIN_A_DOWN | BUTTON_PIN_A_UP | BUTTON_PIN_P_DOWN | BUTTON_PIN_P_UP | BUTTON_PIN_THROW)

#define WIDTH CANVAS_WIDTH
#define HEIGHT CANVAS_HEIGHT

#define REFRESH_RATE 30
// Physics updates per frame
#define PHYSICS_STEPS 2

#define GRAVITY -0.01f
#define FRICTION 0.95f
#define BOUNCE_FRICTION_X 0.8f
#define BOUNCE_FRICTION_Y 0.0f

// Where the pixel starts
#define START_X 6.0f
#define START_Y 5.0f
// How fast angle and power change when pressing the buttons
#define ANGLE_INPUT_SPEED 0.05f
#define POWER_INPUT_SPEED 0.1f
// Factor between power value and initial speed of the pixel
#define AIM_POWER_FACTOR 0.15f

// 'Kill' pixel when its speed is below 0.01 for 60 physics updates
#define NOT_MOVING_THRESHOLD 0.01f
#define NOT_MOVING_TIMEOUT 60

// Accept input after 10 frames, to avoid accidentally throwing the pixel
#define INPUT_START_TIMEOUT 10

enum game_state {
    GAME_STATE_AIM,
    GAME_STATE_THROW,
    GAME_STATE_UPDATE_WORLD,
    GAME_STATE_LOST,
    GAME_STATE_WON
};

enum grid_cell_type {
    GRID_CELL_EMPTY = 0,
    GRID_CELL_SOLID,
    GRID_CELL_BOX,
    GRID_CELL_TARGET
};

struct grid_cell {
    enum grid_cell_type type;
};

struct angry_pixel {
    bool alive;
    float x, y;
    float vx, vy;
};

static void load_level();
static void update_world();
static void update_physics();
static void render();

static enum game_state game_state;

static int current_level;

// The world is divided into grid cells, each can hold a box/wall/target
static struct grid_cell grid[5][10];
static int target_count;
// Level failed when all available pixels were thrown
static int pixels_available;
static int pixels_used;

static struct angry_pixel angry_pixel;

static float aim_angle;
static float aim_power;
// Where the pixel is on the display
static float aim_x, aim_y;

// Counts the frames that the pixel is not moving
static int not_moving_count;

// Counts the frames until input is accepted
static int input_start_timeout;

int main() {
    // Configure system clock to 80 MHz
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    // Configure Timer 0 A to interrupt periodically for updating the game
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet() / REFRESH_RATE);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntEnable(INT_TIMER0A);

    // Configure the GPIO pins of the buttons
    SysCtlPeripheralEnable(BUTTONS_PORT_PERIPH);
    GPIOPinTypeGPIOInput(BUTTONS_PORT_BASE, BUTTON_PINS);
    // Enable internal pull-ups
    GPIOPadConfigSet(BUTTONS_PORT_BASE, BUTTON_PINS, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);

    display_init();

    canvas_set_buffer(display_get_buffer());

    aim_angle = M_PI_4;
    aim_power = 4.0f;

    load_level(0);

    IntMasterEnable();

    TimerEnable(TIMER0_BASE, TIMER_A);

    while (1) {
        display_refresh();
    }
}

static enum grid_cell_type level_object_type_to_grid_cell_type(enum level_object_type object_type) {
    switch (object_type) {
        case LEVEL_OBJECT_TYPE_SOLID: return GRID_CELL_SOLID;
        case LEVEL_OBJECT_TYPE_BOX: return GRID_CELL_BOX;
        case LEVEL_OBJECT_TYPE_TARGET: return GRID_CELL_TARGET;
        default: return GRID_CELL_EMPTY;
    }
}

static void load_level(int index) {
    if (index >= level_count) {
        return;
    }

    current_level = index;

    target_count = 0;

    const struct level *level = &levels[index];
    const struct level_object *objects = level->objects;

    // Reset world grid
    memset(grid, 0x00, sizeof(grid));

    // Fill world grid with objects from the level definition
    size_t i = 0;
    while (objects[i].type != LEVEL_OBJECT_TYPE_END) {
        const struct level_object *object = &objects[i];

        grid[object->row][object->col].type = level_object_type_to_grid_cell_type(object->type);

        // Count how many targets the level contains
        if (object->type == LEVEL_OBJECT_TYPE_TARGET) {
            target_count++;
        }

        i++;
    }

    pixels_available = level->pixels;
    pixels_used = 0;

    angry_pixel.alive = false;

    game_state = GAME_STATE_AIM;

    input_start_timeout = INPUT_START_TIMEOUT;
}

static void update_world() {
    // Keep track of whether we changed anything
    bool changed = false;

    for (size_t row = 0; row < 5; row++) {
        for (size_t col = 0; col < 10; col++) {
            struct grid_cell *grid_cell = &grid[row][col];

            switch (grid_cell->type) {
                case GRID_CELL_BOX:
                case GRID_CELL_TARGET: {
                    // Objects fall down if nothing is below them
                    if (row > 0 && grid[row - 1][col].type == GRID_CELL_EMPTY) {
                        struct grid_cell *new_grid_cell = &grid[row - 1][col];

                        new_grid_cell->type = grid_cell->type;

                        grid_cell->type = GRID_CELL_EMPTY;

                        changed = true;
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    // If nothing changed, we can proceed
    if (!changed) {
        if (pixels_used < pixels_available) {
            // The player still has pixels remaining
            game_state = GAME_STATE_AIM;
        } else {
            // No more pixels :(
            game_state = GAME_STATE_LOST;
        }
    }
}

static void update_physics() {
    // Update the pixels position (if 'alive')
    if (angry_pixel.alive) {
        angry_pixel.vy += GRAVITY;

        angry_pixel.x += angry_pixel.vx;
        angry_pixel.y += angry_pixel.vy;

        // Bounce off the walls
        if (angry_pixel.x < 0.0f) {
            angry_pixel.x = 0.0f;
            angry_pixel.vx = -angry_pixel.vx * BOUNCE_FRICTION_X;
            angry_pixel.vy *= FRICTION;
        } else if (angry_pixel.x > 63.0f) {
            angry_pixel.x = 63.0f;
            angry_pixel.vx = -angry_pixel.vx * BOUNCE_FRICTION_X;
            angry_pixel.vy *= FRICTION;
        }

        // Bounce off the ground
        if (angry_pixel.y < 0.0f) {
            angry_pixel.y = 0.0f;
            angry_pixel.vy = -angry_pixel.vy * BOUNCE_FRICTION_Y;
            angry_pixel.vx *= FRICTION;
        }

        // Check collisions with objects
        if (angry_pixel.x >= 32.0f && angry_pixel.y <= 15.0f) {
            // Infer grid cell the pixel is in from its position
            int row = (int) angry_pixel.y / 3;
            int col = ((int) angry_pixel.x - 32) / 3;
            struct grid_cell *grid_cell = &grid[row][col];

            if (grid_cell->type != GRID_CELL_EMPTY) {
                if (grid_cell->type == GRID_CELL_SOLID) {
                    // Bounce off a solid grid cell

                    // Distance from the grid cells center
                    float dx = angry_pixel.x - (32.0f + (float) col * 3.0f + 1.5f);
                    float dy = angry_pixel.y - ((float) row * 3.0f + 1.5f);

                    if (fabsf(dx) > fabsf(dy)) {
                        // Collided with left or right edge
                        angry_pixel.x = (dx < 0) ? (32.0f + (float) col * 3.0f - 0.1f) : (32.0f + (float) (col + 1) * 3.0f);
                        angry_pixel.vx = -angry_pixel.vx * BOUNCE_FRICTION_X;
                        angry_pixel.vy *= FRICTION;
                    } else {
                        // Collided with top or bottom edge
                        angry_pixel.y = (dy < 0) ? ((float) row * 3.0f - 0.1f) : ((float) (row + 1) * 3.0f);
                        angry_pixel.vy = -angry_pixel.vy * BOUNCE_FRICTION_Y;
                        angry_pixel.vx *= FRICTION;
                    }
                } else {
                    // Grid cell isn't solid

                    if (grid_cell->type == GRID_CELL_TARGET) {
                        // The pixel hit a target
                        target_count--;
                        if (target_count <= 0) {
                            // The player has cleared the level if there are no more targets left
                            game_state = GAME_STATE_WON;

                            return;
                        }
                    }

                    // Clear the grid cell
                    grid_cell->type = GRID_CELL_EMPTY;

                    // R.I.P.
                    angry_pixel.alive = false;

                    // Proceed with updating the world
                    game_state = GAME_STATE_UPDATE_WORLD;

                    // No need to do anything else here, the pixel is no more
                    return;
                }
            }
        }

        // Check whether the pixel stopped moving
        if (fabsf(angry_pixel.vx) < NOT_MOVING_THRESHOLD && fabsf(angry_pixel.vy) < NOT_MOVING_THRESHOLD) {
            // Speed is below the threshold -> didn't move

            // Count the frames
            not_moving_count++;
            if (not_moving_count >= NOT_MOVING_TIMEOUT) {
                // The pixel dies when not moving for some time

                angry_pixel.alive = false;

                game_state = GAME_STATE_UPDATE_WORLD;
            }
        } else {
            // It did move, reset the counter
            not_moving_count = 0;
        }
    }
}

static int draw_number(int start_x, int y, unsigned number) {
    int number_ = number;

    // Count digits
    int digit_count = 0;
    while (number_ > 0) {
        digit_count++;
        number_ /= 10;
    }
    // 0 gives digit_count = 0, but we'd like to print '0', which has one digit
    if (digit_count == 0) {
        digit_count = 1;
    }

    // All digits have the same width, plus a space between the digits
    int w = digit_count * digit_bitmap_width + (digit_count - 1);

    number_ = number;

    int x = start_x + w;
    for (int i = 0; i < digit_count; i++) {
        uint8_t digit = number_ % 10;

        // The bitmap is drawn left-aligned
        x -= digit_bitmap_width;

        // Draw the digits bitmap
        canvas_bitmap(x, y, digit_bitmaps[digit], digit_bitmap_width, digit_bitmap_height);

        // Add a space
        x -= 1;

        number_ /= 10;
    }

    // Return where the last digit ends, to ease layout calculation for the caller
    return start_x + w;
}

static void render() {
    // Clear the canvas every frame
    canvas_clear();

    if (game_state == GAME_STATE_WON) {
        /*********************
         * LVL X CLEARED!    *
         *  · X           -> *
         *********************/

        // "LVL"
        canvas_bitmap(2, 2, bitmap_lvl, bitmap_lvl_width, bitmap_lvl_height);

        // Level number
        int end_x = draw_number(15, 2, current_level);

        // "CLEARED!"
        canvas_bitmap(end_x + 2, 2, bitmap_cleared, bitmap_cleared_width, bitmap_cleared_height);

        // The pixel 'icon'
        canvas_pixel_set(3, 11);
        // Number of pixels thrown
        draw_number(6, 9, pixels_used);

        // A 'next' arrow if there is another level
        if (current_level < level_count - 1) {
            canvas_bitmap(55, 7, bitmap_next, bitmap_next_width, bitmap_next_height);
        }
    } else if (game_state == GAME_STATE_LOST) {
        /*********************
         * FAILED!           *
         *                <- *
         *********************/

        // "FAILED!"
        canvas_bitmap(2, 2, bitmap_failed, bitmap_failed_width, bitmap_failed_height);

        // A 'retry' arrow
        canvas_bitmap(55, 7, bitmap_retry, bitmap_retry_width, bitmap_retry_height);
    } else {
        // Draw the world grid
        for (size_t row = 0; row < 5; row++) {
            for (size_t col = 0; col < 10; col++) {
                struct grid_cell *grid_cell = &grid[row][col];

                switch (grid_cell->type) {
                    case GRID_CELL_SOLID: {
                        float x = 32.0f + 3.0f * col;
                        float y = 3.0f * row;
                        canvas_rect_fill(x, 15.0f - y, x + 2.0f, 15.0f - (y + 2.0f));
                        break;
                    }
                    case GRID_CELL_BOX: {
                        float x = 32.0f + 3.0f * col;
                        float y = 3.0f * row;
                        canvas_rect_stroke(x, 15.0f - y, x + 2.0f, 15.0f - (y + 2.0f));
                        break;
                    }
                    case GRID_CELL_TARGET: {
                        int x = 32 + 3 * (int) col + 1;
                        int y = 3 * (int) row + 1;
                        // A circle (well, sort of)
                        canvas_pixel_set(x - 1, 15 - y);
                        canvas_pixel_set(x, 15 - (y + 1));
                        canvas_pixel_set(x + 1, 15 - y);
                        canvas_pixel_set(x, 15 - (y - 1));
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        if (angry_pixel.alive) {
            // Draw the angry pixel
            canvas_pixel_set((int) angry_pixel.x, 15 - (int) angry_pixel.y);
        }

        if (game_state == GAME_STATE_AIM) {
            // Draw the angry pixel in the slingshot
            canvas_pixel_set((int) aim_x, 15 - (int) aim_y);
        }

        // The slingshot stand
        canvas_vline(START_X, 15.0f - START_Y, 15.0f);
    }
}

void Timer0AIntHandler() {
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    if (input_start_timeout > 0) {
        // Wait for some time after starting a level before accepting input
        input_start_timeout--;
    } else {
        uint32_t input = GPIOPinRead(BUTTONS_PORT_BASE, BUTTON_PINS);

        if (game_state == GAME_STATE_AIM) {
            // Adjust angle
            if (input & BUTTON_PIN_A_DOWN) {
                aim_angle -= ANGLE_INPUT_SPEED;
            } else if (input & BUTTON_PIN_A_UP) {
                aim_angle += ANGLE_INPUT_SPEED;
            }

            // Adjust power
            if (input & BUTTON_PIN_P_DOWN) {
                aim_power -= POWER_INPUT_SPEED;
            } else if (input & BUTTON_PIN_P_UP) {
                aim_power += POWER_INPUT_SPEED;
            }

            // Calculate the pixels position
            aim_x = START_X - cosf(aim_angle) * aim_power;
            aim_y = START_Y - sinf(aim_angle) * aim_power;

            // Throw it
            if (input & BUTTON_PIN_THROW) {
                angry_pixel.x = START_X;
                angry_pixel.y = START_Y;
                angry_pixel.vx = cosf(aim_angle) * (aim_power * AIM_POWER_FACTOR);
                angry_pixel.vy = sinf(aim_angle) * (aim_power * AIM_POWER_FACTOR);
                angry_pixel.alive = true;

                game_state = GAME_STATE_THROW;
                pixels_used++;
            }
        } else if (game_state == GAME_STATE_WON) {
            // Advance to the next level, if there is one
            if (input & BUTTON_PIN_THROW) {
                if (current_level < level_count - 1) {
                    load_level(current_level + 1);
                }
            }
        } else if (game_state == GAME_STATE_LOST) {
            // Retry the current level
            if (input & BUTTON_PIN_THROW) {
                load_level(current_level);
            }
        }
    }

    // Only simulate physics when we're in the 'THROW' state
    if (game_state == GAME_STATE_THROW) {
        for (size_t i = 0; i < PHYSICS_STEPS; i++) {
            update_physics();
        }
    }

    // Only update the world when we're in the 'UPDATE_WORLD' state
    if (game_state == GAME_STATE_UPDATE_WORLD) {
        update_world();
    }

    render();
}
