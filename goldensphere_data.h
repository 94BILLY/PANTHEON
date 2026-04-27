/* PANTHEON ASSET PIPELINE | Auto-generated from goldensphere.hrc */
#ifndef GOLDENSPHERE_DATA_H
#define GOLDENSPHERE_DATA_H

#include <tamtypes.h>

#ifndef PANTHEON_STRUCTS
#define PANTHEON_STRUCTS
typedef struct {
    float x, y, z, w;
    float r, g, b, a;
} __attribute__((aligned(16))) PantheonVertex;

typedef struct {
    u32 a, b, c, pad;
} __attribute__((aligned(16))) PantheonTriangle;
#endif

#define GOLDENSPHERE_VERT_COUNT 0
#define GOLDENSPHERE_TRI_COUNT 0

PantheonVertex goldensphere_vertices[GOLDENSPHERE_VERT_COUNT] = {
};

PantheonTriangle goldensphere_indices[GOLDENSPHERE_TRI_COUNT] = {
};

#endif // GOLDENSPHERE_DATA_H
