
#ifndef __LEVELS_H__
#define __LEVELS_H__

#include <stdlib.h>

enum level_object_type {
    LEVEL_OBJECT_TYPE_END = 0,
    LEVEL_OBJECT_TYPE_SOLID,
    LEVEL_OBJECT_TYPE_BOX,
    LEVEL_OBJECT_TYPE_TARGET
};

struct level_object {
    enum level_object_type type;
    size_t col;
    size_t row;
};

struct level {
    int pixels;
    const struct level_object *objects;
};

extern const struct level levels[];
extern const size_t level_count;

#endif /* __LEVELS_H__ */
