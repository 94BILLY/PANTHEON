#ifndef PANTHEON_TIMECYCLE_H
#define PANTHEON_TIMECYCLE_H

/*
 * Small San Andreas-style atmosphere table: discrete weather/time slots with
 * runtime interpolation. Values are intentionally PS2-friendly 0..255 colors.
 */
typedef enum PantheonWeather {
    PANTHEON_WEATHER_CLEAR = 0,
    PANTHEON_WEATHER_SUNSET,
    PANTHEON_WEATHER_OVERCAST,
    PANTHEON_WEATHER_RAIN,
    PANTHEON_WEATHER_COUNT
} PantheonWeather;

typedef struct PantheonColor {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} PantheonColor;

typedef struct PantheonTimecycleSlot {
    PantheonColor sky_top;
    PantheonColor sky_horizon;
    PantheonColor cloud;
    PantheonColor post_rgb1;
    PantheonColor post_rgb2;
    unsigned char cloud_alpha;
    unsigned char post_alpha;
    float far_clip;
    float fog_start;
} PantheonTimecycleSlot;

typedef struct PantheonAtmosphere {
    PantheonColor sky_top;
    PantheonColor sky_horizon;
    PantheonColor cloud;
    PantheonColor post_rgb1;
    PantheonColor post_rgb2;
    unsigned char cloud_alpha;
    unsigned char post_alpha;
    float far_clip;
    float fog_start;
} PantheonAtmosphere;

static const PantheonTimecycleSlot pantheon_timecycle[PANTHEON_WEATHER_COUNT][8] = {
    /* clear */
    {
        {{  7,  16,  42}, { 28,  54,  94}, { 28,  42,  68}, { 64,  72,  88}, {  8,  12,  22},  20,  36, 1800.0f, 1100.0f},
        {{ 34,  82, 170}, {170, 206, 244}, {218, 230, 238}, {78,  92, 112}, { 18,  26,  42},  64,  42, 1900.0f, 1350.0f},
        {{ 54, 112, 218}, {190, 224, 250}, {240, 240, 232}, {88, 104, 120}, { 18,  26,  44},  54,  36, 2000.0f, 1450.0f},
        {{ 34,  92, 198}, {180, 218, 248}, {238, 240, 232}, {88, 104, 120}, { 16,  24,  42},  42,  32, 2000.0f, 1500.0f},
        {{ 36, 110, 214}, {184, 220, 248}, {238, 240, 234}, {92, 108, 122}, { 12,  20,  36},  36,  28, 2000.0f, 1500.0f},
        {{ 72,  74, 168}, {246, 124,  58}, {248, 176,  92}, {104,  82,  72}, { 84,  34,  18},  58,  64, 1750.0f, 950.0f},
        {{ 20,  28,  68}, {124,  52,  72}, { 92,  66,  88}, { 58,  48,  62}, { 34,  16,  24},  48,  44, 1400.0f, 650.0f},
        {{  6,  10,  26}, { 26,  30,  48}, { 22,  24,  34}, { 42,  44,  54}, {  8,   8,  14},  16,  24, 1200.0f, 500.0f}
    },
    /* sunset showcase */
    {
        {{  8,  12,  34}, { 42,  28,  52}, { 30,  26,  44}, { 54,  42,  58}, { 20,  10,  18},  22,  30, 1350.0f, 600.0f},
        {{ 26,  64, 132}, {208, 154, 104}, {222, 190, 148}, {86,  70,  72}, { 52,  24,  18},  74,  58, 1650.0f, 850.0f},
        {{ 40,  90, 182}, {236, 184, 102}, {244, 206, 132}, {98,  78,  70}, { 68,  28,  16},  78,  68, 1750.0f, 900.0f},
        {{ 50,  82, 170}, {250, 140,  54}, {248, 168,  84}, {106,  76,  62}, { 88,  28,  14},  70,  74, 1650.0f, 800.0f},
        {{ 58,  70, 154}, {254, 118,  44}, {248, 154,  76}, {108,  72,  58}, { 96,  26,  12},  66,  78, 1500.0f, 700.0f},
        {{ 64,  50, 118}, {244,  88,  46}, {224, 116,  72}, {92,  56,  56}, { 78,  20,  18},  58,  66, 1250.0f, 520.0f},
        {{ 22,  22,  62}, {112,  42,  72}, { 82,  54,  80}, {56,  42,  56}, { 34,  12,  22},  40,  40, 1000.0f, 420.0f},
        {{  6,   8,  24}, { 24,  18,  38}, { 20,  18,  28}, {38,  34,  44}, {  8,   6,  12},  16,  22, 900.0f, 350.0f}
    },
    /* overcast */
    {
        {{ 18,  24,  38}, { 52,  58,  70}, { 68,  70,  74}, {56,  58,  62}, { 16,  18,  22},  92,  28, 900.0f, 300.0f},
        {{ 52,  62,  78}, {134, 146, 154}, {162, 166, 166}, {70,  72,  74}, { 24,  26,  28}, 128,  34, 1150.0f, 480.0f},
        {{ 78,  92, 110}, {154, 166, 174}, {178, 180, 178}, {76,  78,  78}, { 24,  26,  28}, 148,  32, 1300.0f, 600.0f},
        {{ 86, 100, 118}, {164, 174, 180}, {184, 184, 180}, {78,  80,  80}, { 24,  26,  28}, 156,  30, 1350.0f, 650.0f},
        {{ 80,  94, 112}, {158, 168, 176}, {182, 182, 178}, {76,  78,  78}, { 24,  26,  28}, 158,  30, 1300.0f, 620.0f},
        {{ 62,  68,  86}, {136, 122, 116}, {154, 142, 136}, {68,  62,  62}, { 28,  20,  20}, 142,  38, 1050.0f, 400.0f},
        {{ 24,  28,  42}, { 66,  58,  66}, { 72,  66,  74}, {46,  44,  50}, { 14,  12,  16}, 108,  28, 850.0f, 260.0f},
        {{  8,  10,  20}, { 26,  28,  34}, { 28,  30,  34}, {34,  34,  38}, {  8,   8,  10},  72,  20, 750.0f, 220.0f}
    },
    /* rain */
    {
        {{ 10,  14,  24}, { 30,  34,  42}, { 44,  46,  50}, {42,  44,  50}, { 10,  12,  16}, 140,  32, 650.0f, 150.0f},
        {{ 34,  42,  58}, { 78,  88,  94}, {108, 112, 112}, {56,  60,  64}, { 18,  20,  22}, 184,  42, 800.0f, 220.0f},
        {{ 48,  58,  74}, { 98, 110, 116}, {132, 136, 134}, {62,  66,  68}, { 18,  20,  22}, 204,  44, 900.0f, 280.0f},
        {{ 54,  64,  82}, {106, 118, 124}, {142, 144, 140}, {64,  68,  70}, { 18,  20,  22}, 214,  42, 950.0f, 300.0f},
        {{ 50,  60,  78}, {102, 112, 120}, {138, 140, 138}, {62,  66,  68}, { 18,  20,  22}, 214,  42, 900.0f, 280.0f},
        {{ 38,  42,  62}, { 76,  70,  76}, {102,  98, 100}, {52,  50,  56}, { 16,  14,  18}, 190,  44, 700.0f, 180.0f},
        {{ 18,  18,  34}, { 44,  34,  44}, { 54,  50,  58}, {38,  36,  44}, { 10,   8,  14}, 158,  32, 580.0f, 130.0f},
        {{  5,   7,  16}, { 18,  20,  26}, { 24,  26,  30}, {30,  30,  36}, {  6,   6,  10}, 112,  24, 520.0f, 100.0f}
    }
};

static inline float pantheon_clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static inline unsigned char pantheon_lerp_u8(unsigned char a, unsigned char b, float t) {
    float v = (float)a + ((float)b - (float)a) * t;
    v = pantheon_clampf(v, 0.0f, 255.0f);
    return (unsigned char)(v + 0.5f);
}

static inline PantheonColor pantheon_lerp_color(PantheonColor a, PantheonColor b, float t) {
    PantheonColor out;
    out.r = pantheon_lerp_u8(a.r, b.r, t);
    out.g = pantheon_lerp_u8(a.g, b.g, t);
    out.b = pantheon_lerp_u8(a.b, b.b, t);
    return out;
}

static inline PantheonAtmosphere pantheon_sample_timecycle(PantheonWeather weather, float day01) {
    if ((int)weather < 0 || weather >= PANTHEON_WEATHER_COUNT) {
        weather = PANTHEON_WEATHER_CLEAR;
    }

    float wrapped = day01 - (float)((int)day01);
    if (wrapped < 0.0f) {
        wrapped += 1.0f;
    }
    float slot_f = wrapped * 8.0f;
    int slot0 = (int)slot_f;
    if (slot0 >= 8) {
        slot0 = 7;
    }
    int slot1 = (slot0 + 1) & 7;
    float t = slot_f - (float)slot0;

    const PantheonTimecycleSlot *a = &pantheon_timecycle[weather][slot0];
    const PantheonTimecycleSlot *b = &pantheon_timecycle[weather][slot1];
    PantheonAtmosphere out;
    out.sky_top = pantheon_lerp_color(a->sky_top, b->sky_top, t);
    out.sky_horizon = pantheon_lerp_color(a->sky_horizon, b->sky_horizon, t);
    out.cloud = pantheon_lerp_color(a->cloud, b->cloud, t);
    out.post_rgb1 = pantheon_lerp_color(a->post_rgb1, b->post_rgb1, t);
    out.post_rgb2 = pantheon_lerp_color(a->post_rgb2, b->post_rgb2, t);
    out.cloud_alpha = pantheon_lerp_u8(a->cloud_alpha, b->cloud_alpha, t);
    out.post_alpha = pantheon_lerp_u8(a->post_alpha, b->post_alpha, t);
    out.far_clip = a->far_clip + (b->far_clip - a->far_clip) * t;
    out.fog_start = a->fog_start + (b->fog_start - a->fog_start) * t;
    return out;
}

#endif /* PANTHEON_TIMECYCLE_H */
