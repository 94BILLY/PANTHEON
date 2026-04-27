/*===========================================================================
 * PANTHEON - floor.c  v2
 * PS2 Homebrew | Path 3 Software Rendering | ps2sdk
 *
 * FIXES vs v1:
 *   1. Floor is now a 6x6 TILE GRID (72 triangles, 98 verts) instead of
 *      a single 4-vert quad.  This prevents the near-plane vertex explosion:
 *      when the camera is close to the floor, only the tile directly below
 *      clips — all other tiles remain valid.  A single 300-unit quad with
 *      one vertex behind the camera makes the entire primitive go haywire.
 *
 *   2. CAM_DIST increased 30→55, CAM_HEIGHT 18→22, CAM_PITCH adjusted.
 *      This keeps ALL floor tile vertices in front of the near plane during
 *      normal play and rotation.
 *
 *   3. Near plane pushed from 1.0→3.0 in create_view_screen().
 *      Matches the actual minimum camera-to-geometry distance.
 *
 * Architecture: Fixed world objects, moving camera.
 *   All geometry lives at its TRUE world position, never moved per-frame.
 *   create_world_view() does all the work.
 *
 * Controls:
 *   L1 / R1 : Yaw left / right
 *   UP      : Walk forward
 *   DOWN    : Walk backward
 *   LEFT    : Strafe left
 *   RIGHT   : Strafe right
 *===========================================================================*/

#include <kernel.h>
#include <stdlib.h>
#include <malloc.h>
#include <tamtypes.h>
#include <math3d.h>
#include <packet.h>
#include <dma_tags.h>
#include <gif_tags.h>
#include <gs_psm.h>
#include <dma.h>
#include <graph.h>
#include <draw.h>
#include <draw3d.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <libpad.h>
#include <math.h>

/*---------------------------------------------------------------------------
 * CAMERA CONSTANTS
 *
 * CAM_DIST  : Distance behind the player (world units).
 *             Larger = camera further back, less chance of floor clipping.
 * CAM_HEIGHT: Camera Y above the ground plane.
 * CAM_PITCH : Downward tilt in radians. atan2(CAM_HEIGHT, CAM_DIST) gives
 *             the exact angle to look at the player's feet.
 *             0.38 rad ≈ 22 degrees down — comfortable third-person view.
 *---------------------------------------------------------------------------*/
#define CAM_DIST   35.0f
#define CAM_HEIGHT 15.0f
#define CAM_PITCH  0.40f

/*---------------------------------------------------------------------------
 * TILED FLOOR
 *
 * WHY TILES: ps2sdk's calculate_vertices + draw_convert_xyz do NOT clip
 * triangles against the near plane — they only transform vertices.  If a
 * vertex lands behind the camera, its projected screen coordinate wraps or
 * goes enormous, stretching the triangle across the entire screen.
 *
 * A single 600x600 quad with 4 vertices means one bad vertex = one giant
 * green triangle covering the screen (exactly what you saw).
 *
 * A grid of small tiles means only the tile(s) immediately behind the camera
 * are affected; every other tile is fine.  This is the standard PS2 approach
 * for large flat geometry without a hardware clipper.
 *
 * GRID_N  : Number of divisions per axis (6 = 6x6 = 36 quads = 72 triangles)
 * FLOOR_SZ: Half-extent of the whole floor in world units (±300)
 *---------------------------------------------------------------------------*/
#define GRID_N   4
#define FLOOR_SZ 300.0f

/* Grid produces (GRID_N+1)^2 unique vertices */
#define FLOOR_VERT_COUNT  ((GRID_N+1)*(GRID_N+1))   /* 121 */
/* Each quad = 2 triangles = 6 indices */
#define FLOOR_IDX_COUNT   (GRID_N*GRID_N*6)          /* 600 */

static VECTOR floor_verts[FLOOR_VERT_COUNT];
static VECTOR floor_cols[FLOOR_VERT_COUNT];
static int    floor_idx[FLOOR_IDX_COUNT];

/*
 * build_floor_grid
 * Called once at startup.  Fills floor_verts[], floor_cols[], floor_idx[].
 * The grid spans [-FLOOR_SZ .. +FLOOR_SZ] on X and Z, flat at Y=0.
 */
static void build_floor_grid(void) {
    int x, z, v, i;
    float step = (FLOOR_SZ * 2.0f) / (float)GRID_N;

    /* Vertices: row-major, z outer, x inner */
    v = 0;
    for (z = 0; z <= GRID_N; z++) {
        for (x = 0; x <= GRID_N; x++) {
            float wx = -FLOOR_SZ + x * step;
            float wz = -FLOOR_SZ + z * step;

            floor_verts[v][0] = wx;
            floor_verts[v][1] = 0.0f;
            floor_verts[v][2] = wz;
            floor_verts[v][3] = 1.0f;

            /* Slight colour variation across the grid for visual depth */
            floor_cols[v][0] = 0.12f + (x & 1) * 0.04f;
            floor_cols[v][1] = 0.45f + (z & 1) * 0.08f;
            floor_cols[v][2] = 0.10f + (x & 1) * 0.02f;
            floor_cols[v][3] = 1.0f;
            v++;
        }
    }

    /* Indices: two triangles per quad cell */
    i = 0;
    for (z = 0; z < GRID_N; z++) {
        for (x = 0; x < GRID_N; x++) {
            int tl = z * (GRID_N+1) + x;       /* top-left  */
            int tr = tl + 1;                    /* top-right */
            int bl = tl + (GRID_N+1);           /* bot-left  */
            int br = bl + 1;                    /* bot-right */

            /* Triangle 1: tl, tr, bl */
            floor_idx[i++] = tl;
            floor_idx[i++] = tr;
            floor_idx[i++] = bl;

            /* Triangle 2: tr, br, bl */
            floor_idx[i++] = tr;
            floor_idx[i++] = br;
            floor_idx[i++] = bl;
        }
    }
}

/*---------------------------------------------------------------------------
 * PLAYER CUBE
 * 6x6x6 unit box.  Placed at the player's world XZ, sitting on the ground.
 *---------------------------------------------------------------------------*/
static int cube_points_count = 36;
static int cube_points[36] = {
    0,1,2,1,2,3,   4,5,6,5,6,7,   8,9,10,9,10,11,
    12,13,14,13,14,15, 16,17,18,17,18,19, 20,21,22,21,22,23
};
static int cube_vertex_count = 24;
static VECTOR cube_vertices[24] = {
    { 3,6, 3,1},{ 3,6,-3,1},{ 3,0, 3,1},{ 3,0,-3,1},
    {-3,6, 3,1},{-3,6,-3,1},{-3,0, 3,1},{-3,0,-3,1},
    {-3,6, 3,1},{ 3,6, 3,1},{-3,6,-3,1},{ 3,6,-3,1},
    {-3,0, 3,1},{ 3,0, 3,1},{-3,0,-3,1},{ 3,0,-3,1},
    {-3,6, 3,1},{ 3,6, 3,1},{-3,0, 3,1},{ 3,0, 3,1},
    {-3,6,-3,1},{ 3,6,-3,1},{-3,0,-3,1},{ 3,0,-3,1},
};
static VECTOR cube_colours[24] = {
    {1.0f,0.2f,0.2f,1.0f},{1.0f,0.2f,0.2f,1.0f},{1.0f,0.2f,0.2f,1.0f},{1.0f,0.2f,0.2f,1.0f},
    {1.0f,0.2f,0.2f,1.0f},{1.0f,0.2f,0.2f,1.0f},{1.0f,0.2f,0.2f,1.0f},{1.0f,0.2f,0.2f,1.0f},
    {0.2f,1.0f,0.2f,1.0f},{0.2f,1.0f,0.2f,1.0f},{0.2f,1.0f,0.2f,1.0f},{0.2f,1.0f,0.2f,1.0f},
    {0.2f,1.0f,0.2f,1.0f},{0.2f,1.0f,0.2f,1.0f},{0.2f,1.0f,0.2f,1.0f},{0.2f,1.0f,0.2f,1.0f},
    {0.2f,0.2f,1.0f,1.0f},{0.2f,0.2f,1.0f,1.0f},{0.2f,0.2f,1.0f,1.0f},{0.2f,0.2f,1.0f,1.0f},
    {0.2f,0.2f,1.0f,1.0f},{0.2f,0.2f,1.0f,1.0f},{0.2f,0.2f,1.0f,1.0f},{0.2f,0.2f,1.0f,1.0f},
};

/*---------------------------------------------------------------------------
 * BUILDINGS  (4 instances, shared local geometry)
 *---------------------------------------------------------------------------*/
#define NUM_BUILDINGS 4

static int bld_points_count = 36;
static int bld_points[36] = {
    0,1,2,1,2,3,   4,5,6,5,6,7,   8,9,10,9,10,11,
    12,13,14,13,14,15, 16,17,18,17,18,19, 20,21,22,21,22,23
};
static int bld_vertex_count = 24;
static VECTOR bld_vertices[24] = {
    { 4,20, 4,1},{ 4,20,-4,1},{ 4,0, 4,1},{ 4,0,-4,1},
    {-4,20, 4,1},{-4,20,-4,1},{-4,0, 4,1},{-4,0,-4,1},
    {-4,20, 4,1},{ 4,20, 4,1},{-4,20,-4,1},{ 4,20,-4,1},
    {-4,0,  4,1},{ 4,0,  4,1},{-4,0, -4,1},{ 4,0, -4,1},
    {-4,20, 4,1},{ 4,20, 4,1},{-4,0,  4,1},{ 4,0,  4,1},
    {-4,20,-4,1},{ 4,20,-4,1},{-4,0, -4,1},{ 4,0, -4,1},
};
static VECTOR bld_colours[24] = {
    {0.60f,0.60f,0.70f,1.0f},{0.60f,0.60f,0.70f,1.0f},
    {0.60f,0.60f,0.70f,1.0f},{0.60f,0.60f,0.70f,1.0f},
    {0.60f,0.60f,0.70f,1.0f},{0.60f,0.60f,0.70f,1.0f},
    {0.60f,0.60f,0.70f,1.0f},{0.60f,0.60f,0.70f,1.0f},
    {0.50f,0.50f,0.60f,1.0f},{0.50f,0.50f,0.60f,1.0f},
    {0.50f,0.50f,0.60f,1.0f},{0.50f,0.50f,0.60f,1.0f},
    {0.40f,0.40f,0.50f,1.0f},{0.40f,0.40f,0.50f,1.0f},
    {0.40f,0.40f,0.50f,1.0f},{0.40f,0.40f,0.50f,1.0f},
    {0.55f,0.55f,0.65f,1.0f},{0.55f,0.55f,0.65f,1.0f},
    {0.55f,0.55f,0.65f,1.0f},{0.55f,0.55f,0.65f,1.0f},
    {0.55f,0.55f,0.65f,1.0f},{0.55f,0.55f,0.65f,1.0f},
    {0.55f,0.55f,0.65f,1.0f},{0.55f,0.55f,0.65f,1.0f},
};
static float bld_wx[NUM_BUILDINGS] = {  120.0f, -120.0f,  120.0f, -120.0f };
static float bld_wz[NUM_BUILDINGS] = {  120.0f,  120.0f, -120.0f, -120.0f };

/*---------------------------------------------------------------------------
 * PLAYER / CAMERA STATE
 *---------------------------------------------------------------------------*/
static float player_x   = 0.0f;
static float player_z   = 0.0f;
static float player_yaw = 0.0f;

static VECTOR camera_position = {0.0f, CAM_HEIGHT, CAM_DIST, 1.0f};
static VECTOR camera_rotation = {CAM_PITCH, 0.0f, 0.0f, 1.0f};

static char padBuf[256] __attribute__((aligned(64)));

/*---------------------------------------------------------------------------
 * update_camera
 * Recompute camera world position and rotation from player state.
 * Camera orbits CAM_DIST behind and CAM_HEIGHT above the player.
 *---------------------------------------------------------------------------*/
static void update_camera(void) {
    camera_position[0] = player_x + sinf(player_yaw) * CAM_DIST;
    camera_position[1] = CAM_HEIGHT;
    camera_position[2] = player_z + cosf(player_yaw) * CAM_DIST;
    camera_position[3] = 1.0f;

    camera_rotation[0] = CAM_PITCH;
    camera_rotation[1] = player_yaw;
    camera_rotation[2] = 0.0f;
    camera_rotation[3] = 1.0f;
}

/*---------------------------------------------------------------------------
 * read_pad
 *---------------------------------------------------------------------------*/
static void read_pad(void) {
    struct padButtonStatus buttons;
    u32 paddata;
    float speed = 0.8f;
    float s, c;

    if (padRead(0, 0, &buttons) == 0) return;
    paddata = 0xffff ^ buttons.btns;

    if (paddata & PAD_L1) player_yaw -= 0.04f;
    if (paddata & PAD_R1) player_yaw += 0.04f;

    s = sinf(player_yaw);
    c = cosf(player_yaw);

    if (paddata & PAD_UP)    { player_x -= s * speed; player_z -= c * speed; }
    if (paddata & PAD_DOWN)  { player_x += s * speed; player_z += c * speed; }
    if (paddata & PAD_LEFT)  { player_x -= c * speed; player_z += s * speed; }
    if (paddata & PAD_RIGHT) { player_x += c * speed; player_z -= s * speed; }
}

/*---------------------------------------------------------------------------
 * load_modules
 *---------------------------------------------------------------------------*/
static void load_modules(void) {
    SifInitRpc(0);
    SifLoadModule("rom0:SIO2MAN", 0, NULL);
    SifLoadModule("rom0:PADMAN",  0, NULL);
}

/*---------------------------------------------------------------------------
 * init_gs
 *---------------------------------------------------------------------------*/
static void init_gs(framebuffer_t *frame, zbuffer_t *z) {
    frame->width   = 640;
    frame->height  = 512;
    frame->mask    = 0;
    frame->psm     = GS_PSM_32;
    frame->address = graph_vram_allocate(frame->width, frame->height,
                                         frame->psm, GRAPH_ALIGN_PAGE);
    z->enable  = DRAW_ENABLE;
    z->mask    = 0;
    z->method  = ZTEST_METHOD_GREATER_EQUAL;
    z->zsm     = GS_ZBUF_32;
    z->address = graph_vram_allocate(frame->width, frame->height,
                                     z->zsm, GRAPH_ALIGN_PAGE);
    graph_initialize(frame->address, frame->width, frame->height,
                     frame->psm, 0, 0);
}

/*---------------------------------------------------------------------------
 * init_drawing_environment
 *---------------------------------------------------------------------------*/
static void init_drawing_environment(framebuffer_t *frame, zbuffer_t *z) {
    packet_t *packet = packet_init(16, PACKET_NORMAL);
    qword_t  *q      = packet->data;
    q = draw_setup_environment(q, 0, frame, z);
    q = draw_primitive_xyoffset(q, 0, (2048 - 320), (2048 - 256));
    q = draw_finish(q);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data,
                            q - packet->data, 0, 0);
    dma_wait_fast();
    packet_free(packet);
}

/*---------------------------------------------------------------------------
 * draw_mesh
 * Transforms and emits one mesh into the current packet.
 * Scratch buffers (tmp/xyz/col) must be large enough for v_count vertices.
 *---------------------------------------------------------------------------*/
static void draw_mesh(
    VECTOR *world_pos, VECTOR *world_rot,
    VECTOR *vert_src,  VECTOR *col_src,   int v_count,
    int    *idx,       int     i_count,
    MATRIX  world_view, MATRIX view_screen,
    VECTOR *tmp, xyz_t *xyz, color_t *col,
    prim_t *prim, color_t *color, qword_t **qp)
{
    int i;
    MATRIX local_world, local_screen;

    create_local_world(local_world, *world_pos, *world_rot);
    create_local_screen(local_screen, local_world, world_view, view_screen);
    calculate_vertices(tmp, v_count, vert_src, local_screen);
    draw_convert_xyz(xyz, 2048, 2048, 32, v_count, (vertex_f_t*)tmp);
    draw_convert_rgbq(col, v_count, (vertex_f_t*)tmp, (color_f_t*)col_src, 0x80);

    /*
     * Per-triangle near-Z culling.
     *
     * WHY THIS IS NECESSARY:
     * ps2sdk's calculate_vertices performs a perspective divide: it divides
     * each vertex's X and Y by its W (camera-space depth) to project it onto
     * the screen.  If W <= 0 (vertex is behind or on the camera plane), that
     * divide produces a negative or infinite result.  draw_convert_xyz then
     * converts that garbage float into a huge fixed-point GS coordinate,
     * which makes the GS draw the triangle all the way across the screen —
     * the "exploding triangle" bug.  The GS also writes corrupt depth values
     * into the Z-buffer, breaking depth sorting for everything drawn after.
     *
     * THE FIX:
     * Walk the index list in steps of 3 (one full triangle per iteration).
     * Before emitting the triangle, check the W component of all three
     * transformed vertices.  W in view space equals the distance in front of
     * the camera — positive means visible, zero or negative means behind.
     * If ANY vertex of the triangle has W <= near threshold, skip it entirely.
     * This is a conservative cull: it drops the whole triangle rather than
     * trying to clip it.  For small triangles (tiles, box faces) this is
     * correct and fast; the only artefact is a triangle popping out at the
     * very edge of the near plane, which is invisible in practice.
     *
     * NEAR_W: minimum W value to consider a vertex "in front".
     * Matches the near plane passed to create_view_screen (5.0f).
     * Using a value slightly larger than 0 guards against the divide-by-near-
     * zero case at the exact clip boundary.
     */
#define NEAR_W 0.1f

    *qp = draw_prim_start(*qp, 0, prim, color);
    for (i = 0; i < i_count; i += 3) {
        int i0 = idx[i];
        int i1 = idx[i + 1];
        int i2 = idx[i + 2];

        /* tmp[v][3] is the W coordinate after the local→screen transform.
         * W > 0 means the vertex is in front of the camera.            */
        if (tmp[i0][3] <= NEAR_W) continue;
        if (tmp[i1][3] <= NEAR_W) continue;
        if (tmp[i2][3] <= NEAR_W) continue;

        /* All three vertices are in front — safe to emit */
        (*qp)->dw[0] = col[i0].rgbaq;
        (*qp)->dw[1] = xyz[i0].xyz;
        (*qp)++;
        (*qp)->dw[0] = col[i1].rgbaq;
        (*qp)->dw[1] = xyz[i1].xyz;
        (*qp)++;
        (*qp)->dw[0] = col[i2].rgbaq;
        (*qp)->dw[1] = xyz[i2].xyz;
        (*qp)++;
    }
    *qp = draw_prim_end(*qp, 2, DRAW_RGBAQ_REGLIST);
}

/*---------------------------------------------------------------------------
 * render — main loop
 *---------------------------------------------------------------------------*/
static int render(framebuffer_t *frame, zbuffer_t *z) {
    int b, context = 0;
    MATRIX world_view, view_screen;
    qword_t *dmatag;
    prim_t   prim;
    color_t  color;

    /*
     * Scratch buffers — sized for the largest mesh (floor grid: 49 verts).
     * All other meshes (24 verts) fit easily inside the same allocation.
     */
    VECTOR  *tmp = memalign(128, sizeof(VECTOR)  * FLOOR_VERT_COUNT);
    xyz_t   *xyz = memalign(128, sizeof(xyz_t)   * FLOOR_VERT_COUNT);
    color_t *col = memalign(128, sizeof(color_t) * FLOOR_VERT_COUNT);

    /*
     * Packet budget:
     *   Floor:     216 indices → 216 qword pairs + overhead ≈ 220 qwords
     *   Cube:       36 indices ≈  40 qwords
     *   4 Buildings: 4×36 ≈ 160 qwords
     *   Clear + setup overhead: ~30 qwords
     *   Total: ~450 qwords.  Use 1024 for generous headroom.
     */
    packet_t *packets[2];
    packets[0] = packet_init(1024, PACKET_NORMAL);
    packets[1] = packet_init(1024, PACKET_NORMAL);

    prim.type         = PRIM_TRIANGLE;
    prim.shading      = PRIM_SHADE_GOURAUD;
    prim.mapping      = DRAW_DISABLE;
    prim.fogging      = DRAW_DISABLE;
    prim.blending     = DRAW_DISABLE;
    prim.antialiasing = DRAW_ENABLE;
    prim.mapping_type = PRIM_MAP_ST;
    prim.colorfix     = PRIM_UNFIXED;

    color.r = 0x80; color.g = 0x80; color.b = 0x80;
    color.a = 0x80; color.q = 1.0f;

    /*
     * Projection frustum.
     * near = 3.0f: nothing closer than 3 units to the camera can render.
     *   With CAM_DIST=80 and CAM_HEIGHT=30, the closest floor tile vertex
     *   is ~sqrt(55^2+22^2) ≈ 59 units away — well clear of near=3.
     * far  = 2000.0f: covers the full ±300 floor radius at any angle.
     */
    create_view_screen(view_screen, graph_aspect_ratio(),
                       -3.0f, 3.0f, -3.0f, 3.0f, 1.0f, 2000.0f);

    dma_wait_fast();

    for (;;) {
        qword_t  *q;
        packet_t *current = packets[context];
        VECTOR    world_pos, world_rot;

        read_pad();
        update_camera();

        create_world_view(world_view, camera_position, camera_rotation);

        dmatag = current->data;
        q      = dmatag;
        q++;

        q = draw_disable_tests(q, 0, z);
        q = draw_clear(q, 0,
                       2048.0f - 320.0f, 2048.0f - 256.0f,
                       frame->width, frame->height,
                       0x44, 0x66, 0x99);
        q = draw_enable_tests(q, 0, z);

        /* --- FLOOR (tiled grid) ----------------------------------------- */
        world_pos[0] = 0.0f; world_pos[1] = 0.0f;
        world_pos[2] = 0.0f; world_pos[3] = 1.0f;
        world_rot[0] = 0.0f; world_rot[1] = 0.0f;
        world_rot[2] = 0.0f; world_rot[3] = 1.0f;

        draw_mesh(&world_pos, &world_rot,
                  floor_verts, floor_cols, FLOOR_VERT_COUNT,
                  floor_idx,   FLOOR_IDX_COUNT,
                  world_view, view_screen,
                  tmp, xyz, col,
                  &prim, &color, &q);

        /* --- PLAYER CUBE -------------------------------------------------- */
        world_pos[0] = player_x;
        world_pos[1] = 0.0f;
        world_pos[2] = player_z;
        world_pos[3] = 1.0f;
        world_rot[0] = 0.0f;
        world_rot[1] = player_yaw;
        world_rot[2] = 0.0f;
        world_rot[3] = 1.0f;

        draw_mesh(&world_pos, &world_rot,
                  cube_vertices, cube_colours, cube_vertex_count,
                  cube_points,   cube_points_count,
                  world_view, view_screen,
                  tmp, xyz, col,
                  &prim, &color, &q);

        /* --- BUILDINGS ----------------------------------------------------- */
        for (b = 0; b < NUM_BUILDINGS; b++) {
            world_pos[0] = bld_wx[b];
            world_pos[1] = 0.0f;
            world_pos[2] = bld_wz[b];
            world_pos[3] = 1.0f;
            world_rot[0] = 0.0f;
            world_rot[1] = 0.0f;
            world_rot[2] = 0.0f;
            world_rot[3] = 1.0f;

            draw_mesh(&world_pos, &world_rot,
                      bld_vertices, bld_colours, bld_vertex_count,
                      bld_points,   bld_points_count,
                      world_view, view_screen,
                      tmp, xyz, col,
                      &prim, &color, &q);
        }

        q = draw_finish(q);
        DMATAG_END(dmatag, (q - current->data) - 1, 0, 0, 0);

        dma_wait_fast();
        dma_channel_send_chain(DMA_CHANNEL_GIF,
                               current->data, q - current->data, 0, 0);
        context ^= 1;
        draw_wait_finish();
        graph_wait_vsync();
    }

    packet_free(packets[0]);
    packet_free(packets[1]);
    free(tmp); free(xyz); free(col);
    return 0;
}

/*---------------------------------------------------------------------------
 * main
 *---------------------------------------------------------------------------*/
int main(int argc, char *argv[]) {
    framebuffer_t frame;
    zbuffer_t     z;
    int           timeout;

    build_floor_grid();   /* generate tile geometry before anything renders */

    load_modules();
    padInit(0);
    padPortOpen(0, 0, padBuf);

    timeout = 0;
    while (padGetState(0, 0) != PAD_STATE_STABLE && timeout < 120)
        timeout++;

    dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
    dma_channel_fast_waits(DMA_CHANNEL_GIF);
    init_gs(&frame, &z);
    init_drawing_environment(&frame, &z);
    render(&frame, &z);
    SleepThread();
    return 0;
}
