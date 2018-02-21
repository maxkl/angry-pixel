
#include "levels.h"

#define OBJECT_END { LEVEL_OBJECT_TYPE_END, 0, 0 }

static const struct level_object level0_objects[] = {
    { LEVEL_OBJECT_TYPE_BOX, 0, 0 },
    { LEVEL_OBJECT_TYPE_TARGET, 0, 1 },
    OBJECT_END
};

static const struct level_object level1_objects[] = {
    { LEVEL_OBJECT_TYPE_SOLID, 0, 0 },
    { LEVEL_OBJECT_TYPE_TARGET, 0, 1 },
    { LEVEL_OBJECT_TYPE_SOLID, 3, 0 },
    { LEVEL_OBJECT_TYPE_SOLID, 3, 1 },
    { LEVEL_OBJECT_TYPE_TARGET, 3, 2 },
    { LEVEL_OBJECT_TYPE_SOLID, 6, 2 },
    { LEVEL_OBJECT_TYPE_TARGET, 6, 3 },
    OBJECT_END
};

static const struct level_object level2_objects[] = {
    { LEVEL_OBJECT_TYPE_SOLID, 0, 0 },
    { LEVEL_OBJECT_TYPE_SOLID, 0, 1 },
    { LEVEL_OBJECT_TYPE_SOLID, 0, 2 },
    { LEVEL_OBJECT_TYPE_BOX, 0, 3 },
    { LEVEL_OBJECT_TYPE_SOLID, 0, 4 },
    { LEVEL_OBJECT_TYPE_SOLID, 1, 2 },
    { LEVEL_OBJECT_TYPE_TARGET, 1, 3 },
    OBJECT_END
};

static const struct level_object level3_objects[] = {
    { LEVEL_OBJECT_TYPE_BOX, 2, 0 },
    { LEVEL_OBJECT_TYPE_BOX, 2, 1 },
    { LEVEL_OBJECT_TYPE_BOX, 2, 2 },
    { LEVEL_OBJECT_TYPE_BOX, 3, 0 },
    { LEVEL_OBJECT_TYPE_TARGET, 3, 1 },
    { LEVEL_OBJECT_TYPE_SOLID, 4, 2 },
    { LEVEL_OBJECT_TYPE_SOLID, 4, 3 },
    { LEVEL_OBJECT_TYPE_TARGET, 4, 4 },
    { LEVEL_OBJECT_TYPE_SOLID, 8, 0 },
    { LEVEL_OBJECT_TYPE_SOLID, 8, 1 },
    { LEVEL_OBJECT_TYPE_SOLID, 8, 2 },
    { LEVEL_OBJECT_TYPE_TARGET, 9, 0 },
    OBJECT_END
};

static const struct level_object level4_objects[] = {
    { LEVEL_OBJECT_TYPE_BOX, 0, 0 },
    { LEVEL_OBJECT_TYPE_TARGET, 0, 1 },
    { LEVEL_OBJECT_TYPE_BOX, 1, 0 },
    { LEVEL_OBJECT_TYPE_BOX, 1, 1 },
    { LEVEL_OBJECT_TYPE_SOLID, 3, 0 },
    { LEVEL_OBJECT_TYPE_SOLID, 3, 1 },
    { LEVEL_OBJECT_TYPE_SOLID, 3, 3 },
    { LEVEL_OBJECT_TYPE_SOLID, 3, 4 },
    { LEVEL_OBJECT_TYPE_BOX, 7, 0 },
    { LEVEL_OBJECT_TYPE_TARGET, 7, 1 },
    { LEVEL_OBJECT_TYPE_BOX, 9, 0 },
    { LEVEL_OBJECT_TYPE_BOX, 9, 1 },
    OBJECT_END
};

const struct level levels[] = {
    {
        3,
        level0_objects
    },
    {
        8,
        level1_objects
    },
    {
        6,
        level2_objects
    },
    {
        6,
        level3_objects
    },
    {
        6,
        level4_objects
    }
};

const size_t level_count = sizeof(levels) / sizeof(*levels);
