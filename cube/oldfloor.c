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

/* --- TILED FLOOR SETTINGS --- */
// Increased grid density so tiles are small (15x15 units). 
// This prevents large chunks of the floor from vanishing under the camera.
#define GRID_N   40
#define FLOOR_SZ 300.0f
#define FLOOR_VERT_COUNT  ((GRID_N+1)*(GRID_N+1))  
#define FLOOR_IDX_COUNT   (GRID_N*GRID_N*6)        

static VECTOR floor_verts[FLOOR_VERT_COUNT];
static VECTOR floor_cols[FLOOR_VERT_COUNT];
static int    floor_idx[FLOOR_IDX_COUNT];

static void build_floor_grid(void) {
    int x, z, v = 0, i = 0;
    float step = (FLOOR_SZ * 2.0f) / (float)GRID_N;

    // Generate Vertices
    for (z = 0; z <= GRID_N; z++) {
        for (x = 0; x <= GRID_N; x++) {
            floor_verts[v][0] = -FLOOR_SZ + x * step;
            floor_verts[v][1] = 0.0f;
            floor_verts[v][2] = -FLOOR_SZ + z * step;
            floor_verts[v][3] = 1.0f;

            // Checkerboard color variation
            floor_cols[v][0] = 0.12f + (x & 1) * 0.04f;
            floor_cols[v][1] = 0.45f + (z & 1) * 0.08f;
            floor_cols[v][2] = 0.10f + (x & 1) * 0.02f;
            floor_cols[v][3] = 1.0f;
            v++;
        }
    }

    // Generate Indices
    for (z = 0; z < GRID_N; z++) {
        for (x = 0; x < GRID_N; x++) {
            int tl = z * (GRID_N+1) + x;
            int tr = tl + 1;                   
            int bl = tl + (GRID_N+1);          
            int br = bl + 1;
            
            floor_idx[i++] = tl; floor_idx[i++] = tr; floor_idx[i++] = bl;
            floor_idx[i++] = tr; floor_idx[i++] = br; floor_idx[i++] = bl;
        }
    }
}

/* --- PLAYER CUBE --- */
int cube_points_count = 36;
int cube_points[36] = {
    0,1,2,1,2,3, 4,5,6,5,6,7, 8,9,10,9,10,11,
    12,13,14,13,14,15, 16,17,18,17,18,19, 20,21,22,21,22,23
};
int cube_vertex_count = 24;
VECTOR cube_vertices[24] = {
    {3,6,3,1},{3,6,-3,1},{3,0,3,1},{3,0,-3,1},
    {-3,6,3,1},{-3,6,-3,1},{-3,0,3,1},{-3,0,-3,1},
    {-3,6,3,1},{3,6,3,1},{-3,6,-3,1},{3,6,-3,1},
    {-3,0,3,1},{3,0,3,1},{-3,0,-3,1},{3,0,-3,1},
    {-3,6,3,1},{3,6,3,1},{-3,0,3,1},{3,0,3,1},
    {-3,6,-3,1},{3,6,-3,1},{-3,0,-3,1},{3,0,-3,1},
};
VECTOR cube_colours[24] = {
    {1,0.2f,0.2f,1},{1,0.2f,0.2f,1},{1,0.2f,0.2f,1},{1,0.2f,0.2f,1},
    {1,0.2f,0.2f,1},{1,0.2f,0.2f,1},{1,0.2f,0.2f,1},{1,0.2f,0.2f,1},
    {0.2f,1,0.2f,1},{0.2f,1,0.2f,1},{0.2f,1,0.2f,1},{0.2f,1,0.2f,1},
    {0.2f,1,0.2f,1},{0.2f,1,0.2f,1},{0.2f,1,0.2f,1},{0.2f,1,0.2f,1},
    {0.2f,0.2f,1,1},{0.2f,0.2f,1,1},{0.2f,0.2f,1,1},{0.2f,0.2f,1,1},
    {0.2f,0.2f,1,1},{0.2f,0.2f,1,1},{0.2f,0.2f,1,1},{0.2f,0.2f,1,1},
};

/* --- BUILDINGS --- */
int bld_points_count = 36;
int bld_points[36] = {
    0,1,2,1,2,3, 4,5,6,5,6,7, 8,9,10,9,10,11,
    12,13,14,13,14,15, 16,17,18,17,18,19, 20,21,22,21,22,23
};
VECTOR bld_vertices[24] = {
    {4,20,4,1},{4,20,-4,1},{4,0,4,1},{4,0,-4,1},
    {-4,20,4,1},{-4,20,-4,1},{-4,0,4,1},{-4,0,-4,1},
    {-4,20,4,1},{4,20,4,1},{-4,20,-4,1},{4,20,-4,1},
    {-4,0,4,1},{4,0,4,1},{-4,0,-4,1},{4,0,-4,1},
    {-4,20,4,1},{4,20,4,1},{-4,0,4,1},{4,0,4,1},
    {-4,20,-4,1},{4,20,-4,1},{-4,0,-4,1},{4,0,-4,1},
};
VECTOR bld_colours[24] = {
    {0.6f,0.6f,0.7f,1},{0.6f,0.6f,0.7f,1},{0.6f,0.6f,0.7f,1},{0.6f,0.6f,0.7f,1},
    {0.6f,0.6f,0.7f,1},{0.6f,0.6f,0.7f,1},{0.6f,0.6f,0.7f,1},{0.6f,0.6f,0.7f,1},
    {0.5f,0.5f,0.6f,1},{0.5f,0.5f,0.6f,1},{0.5f,0.5f,0.6f,1},{0.5f,0.5f,0.6f,1},
    {0.4f,0.4f,0.5f,1},{0.4f,0.4f,0.5f,1},{0.4f,0.4f,0.5f,1},{0.4f,0.4f,0.5f,1},
    {0.55f,0.55f,0.65f,1},{0.55f,0.55f,0.65f,1},{0.55f,0.55f,0.65f,1},{0.55f,0.55f,0.65f,1},
    {0.55f,0.55f,0.65f,1},{0.55f,0.55f,0.65f,1},{0.55f,0.55f,0.65f,1},{0.55f,0.55f,0.65f,1},
};
float bld_wx[4] = { 30.0f, -30.0f,  30.0f, -30.0f };
float bld_wz[4] = { 30.0f,  30.0f, -30.0f, -30.0f };

/* --- PLAYER STATE --- */
float player_x   = 0.0f;
float player_z   = 0.0f;
float player_yaw = 0.0f;

VECTOR camera_position = { 0.00f, 15.00f, 60.00f, 1.00f };
VECTOR camera_rotation = { 0.20f,  0.00f,  0.00f, 1.00f };

static char padBuf[256] __attribute__((aligned(64)));

void load_modules(void) {
    SifInitRpc(0);
    SifLoadModule("rom0:SIO2MAN",0,NULL);
    SifLoadModule("rom0:PADMAN", 0,NULL);
}

void read_pad(void) {
    struct padButtonStatus buttons;
    u32 paddata;
    float speed = 0.8f;
    float s, c;
    if(padRead(0,0,&buttons)==0) return;
    paddata = 0xffff ^ buttons.btns;

    if(paddata & PAD_L1) player_yaw -= 0.04f;
    if(paddata & PAD_R1) player_yaw += 0.04f;

    s = sinf(player_yaw);
    c = cosf(player_yaw);

    if(paddata & PAD_UP)    { player_x += s*speed; player_z += c*speed; }
    if(paddata & PAD_DOWN)  { player_x -= s*speed; player_z -= c*speed; }
    if(paddata & PAD_LEFT)  { player_x -= c*speed; player_z += s*speed; }
    if(paddata & PAD_RIGHT) { player_x += c*speed; player_z -= s*speed; }
}

void init_gs(framebuffer_t *frame, zbuffer_t *z) {
    frame->width=640;frame->height=512;frame->mask=0;frame->psm=GS_PSM_32;
    frame->address=graph_vram_allocate(frame->width,frame->height,frame->psm,GRAPH_ALIGN_PAGE);
    z->enable=DRAW_ENABLE;z->mask=0;z->method=ZTEST_METHOD_GREATER_EQUAL;z->zsm=GS_ZBUF_32;
    z->address=graph_vram_allocate(frame->width,frame->height,z->zsm,GRAPH_ALIGN_PAGE);
    graph_initialize(frame->address,frame->width,frame->height,frame->psm,0,0);
}

void init_drawing_environment(framebuffer_t *frame, zbuffer_t *z) {
    packet_t *packet=packet_init(16,PACKET_NORMAL);
    qword_t *q=packet->data;
    q=draw_setup_environment(q,0,frame,z);
    q=draw_primitive_xyoffset(q,0,(2048-320),(2048-256));
    q=draw_finish(q);
    dma_channel_send_normal(DMA_CHANNEL_GIF,packet->data,q-packet->data,0,0);
    dma_wait_fast();
    packet_free(packet);
}

void draw_building(int b, float px, float pz, float yaw,
                   MATRIX world_view, MATRIX view_screen,
                   VECTOR *tmp, xyz_t *xyz, color_t *col,
                   prim_t *prim, color_t *color, qword_t **qp) {
    MATRIX lw, ls;
    int i;
    float dx = bld_wx[b] - px;
    float dz = bld_wz[b] - pz;
    float rx = dx*cosf(-yaw) - dz*sinf(-yaw);
    float rz = dx*sinf(-yaw) + dz*cosf(-yaw);
    VECTOR pos = {rx, 0.0f, rz, 1.0f};
    VECTOR rot = {0.0f, 0.0f, 0.0f, 1.0f};
    
    create_local_world(lw, pos, rot);
    create_local_screen(ls, lw, world_view, view_screen);
    calculate_vertices(tmp, 24, bld_vertices, ls);
    draw_convert_xyz(xyz, 2048, 2048, 32, 24, (vertex_f_t*)tmp);
    draw_convert_rgbq(col, 24, (vertex_f_t*)tmp, (color_f_t*)bld_colours, 0x80);
    
    *qp = draw_prim_start(*qp, 0, prim, color);
    for(i=0; i<bld_points_count; i+=3){
        // Strict Near-Plane check (1.0f) to prevent coordinate wrapping
        if(tmp[bld_points[i]][3] <= 1.0f || tmp[bld_points[i+1]][3] <= 1.0f || tmp[bld_points[i+2]][3] <= 1.0f) continue;
        
        (*qp)->dw[0]=col[bld_points[i]].rgbaq;   (*qp)->dw[1]=xyz[bld_points[i]].xyz;   (*qp)++;
        (*qp)->dw[0]=col[bld_points[i+1]].rgbaq; (*qp)->dw[1]=xyz[bld_points[i+1]].xyz; (*qp)++;
        (*qp)->dw[0]=col[bld_points[i+2]].rgbaq; (*qp)->dw[1]=xyz[bld_points[i+2]].xyz; (*qp)++;
    }
    *qp = draw_prim_end(*qp, 2, DRAW_RGBAQ_REGLIST);
}

int render(framebuffer_t *frame, zbuffer_t *z) {
    int i, b, context=0, frame_count=0;
    MATRIX local_world, world_view, view_screen, local_screen;
    qword_t *dmatag;

    VECTOR  *tmp_f = memalign(128,sizeof(VECTOR)*FLOOR_VERT_COUNT);
    xyz_t   *xyz_f = memalign(128,sizeof(xyz_t)*FLOOR_VERT_COUNT);
    color_t *col_f = memalign(128,sizeof(color_t)*FLOOR_VERT_COUNT);
    
    VECTOR  *tmp_c = memalign(128,sizeof(VECTOR)*cube_vertex_count);
    xyz_t   *xyz_c = memalign(128,sizeof(xyz_t)*cube_vertex_count);
    color_t *col_c = memalign(128,sizeof(color_t)*cube_vertex_count);
    
    VECTOR  *tmp_b = memalign(128,sizeof(VECTOR)*24);
    xyz_t   *xyz_b = memalign(128,sizeof(xyz_t)*24);
    color_t *col_b = memalign(128,sizeof(color_t)*24);

    prim_t prim;
    color_t color;
    packet_t *packets[2];
    
    // Increased packet size significantly to hold the denser floor grid 
    packets[0]=packet_init(20000,PACKET_NORMAL);
    packets[1]=packet_init(20000,PACKET_NORMAL);

    prim.type=PRIM_TRIANGLE;prim.shading=PRIM_SHADE_GOURAUD;
    prim.mapping=DRAW_DISABLE;prim.fogging=DRAW_DISABLE;
    prim.blending=DRAW_DISABLE;prim.antialiasing=DRAW_ENABLE;
    prim.mapping_type=PRIM_MAP_ST;prim.colorfix=PRIM_UNFIXED;
    color.r=0x80;color.g=0x80;color.b=0x80;color.a=0x80;color.q=1.0f;

    create_view_screen(view_screen,graph_aspect_ratio(),-3,3,-3,3,1,2000);
    dma_wait_fast();

    for(;;) {
        qword_t *q;
        packet_t *current=packets[context];

        frame_count++;
        if(frame_count==1){padInit(0);padPortOpen(0,0,padBuf);}

        read_pad();

        create_world_view(world_view, camera_position, camera_rotation);

        // --- DRAW FLOOR GRID ---
        VECTOR floor_pos = {-player_x, 0.0f, -player_z, 1.0f};
        VECTOR floor_rot = {0.0f, -player_yaw, 0.0f, 1.0f};
        create_local_world(local_world, floor_pos, floor_rot);
        create_local_screen(local_screen, local_world, world_view, view_screen);
        calculate_vertices(tmp_f, FLOOR_VERT_COUNT, floor_verts, local_screen);
        draw_convert_xyz(xyz_f,2048,2048,32,FLOOR_VERT_COUNT,(vertex_f_t*)tmp_f);
        draw_convert_rgbq(col_f,FLOOR_VERT_COUNT,(vertex_f_t*)tmp_f,(color_f_t*)floor_cols,0x80);

        // --- DRAW PLAYER CUBE ---
        VECTOR cube_pos = {0.0f, 0.0f, 0.0f, 1.0f};
        VECTOR cube_rot = {0.0f, 0.0f, 0.0f, 1.0f};
        create_local_world(local_world, cube_pos, cube_rot);
        create_local_screen(local_screen, local_world, world_view, view_screen);
        calculate_vertices(tmp_c, cube_vertex_count, cube_vertices, local_screen);
        draw_convert_xyz(xyz_c,2048,2048,32,cube_vertex_count,(vertex_f_t*)tmp_c);
        draw_convert_rgbq(col_c,cube_vertex_count,(vertex_f_t*)tmp_c,(color_f_t*)cube_colours,0x80);

        dmatag=current->data; q=dmatag; q++;

        q=draw_disable_tests(q,0,z);
        q=draw_clear(q,0,2048.0f-320.0f,2048.0f-256.0f,frame->width,frame->height,0x44,0x88,0xFF);
        q=draw_enable_tests(q,0,z);

        // Process Floor Triangles with STRICT W-Culling (<= 1.0f)
        q=draw_prim_start(q,0,&prim,&color);
        for(i=0; i<FLOOR_IDX_COUNT; i+=3){
            if(tmp_f[floor_idx[i]][3] <= 1.0f || tmp_f[floor_idx[i+1]][3] <= 1.0f || tmp_f[floor_idx[i+2]][3] <= 1.0f) continue;
            
            q->dw[0]=col_f[floor_idx[i]].rgbaq;   q->dw[1]=xyz_f[floor_idx[i]].xyz;   q++;
            q->dw[0]=col_f[floor_idx[i+1]].rgbaq; q->dw[1]=xyz_f[floor_idx[i+1]].xyz; q++;
            q->dw[0]=col_f[floor_idx[i+2]].rgbaq; q->dw[1]=xyz_f[floor_idx[i+2]].xyz; q++;
        }
        q=draw_prim_end(q,2,DRAW_RGBAQ_REGLIST);

        // Process Player Cube
        q=draw_prim_start(q,0,&prim,&color);
        for(i=0; i<cube_points_count; i+=3){
            if(tmp_c[cube_points[i]][3] <= 1.0f || tmp_c[cube_points[i+1]][3] <= 1.0f || tmp_c[cube_points[i+2]][3] <= 1.0f) continue;
            
            q->dw[0]=col_c[cube_points[i]].rgbaq;   q->dw[1]=xyz_c[cube_points[i]].xyz;   q++;
            q->dw[0]=col_c[cube_points[i+1]].rgbaq; q->dw[1]=xyz_c[cube_points[i+1]].xyz; q++;
            q->dw[0]=col_c[cube_points[i+2]].rgbaq; q->dw[1]=xyz_c[cube_points[i+2]].xyz; q++;
        }
        q=draw_prim_end(q,2,DRAW_RGBAQ_REGLIST);

        // Process 4 Buildings
        for(b=0;b<4;b++) {
            draw_building(b, player_x, player_z, player_yaw,
                          world_view, view_screen,
                          tmp_b, xyz_b, col_b,
                          &prim, &color, &q);
        }

        q=draw_finish(q);
        DMATAG_END(dmatag,(q-current->data)-1,0,0,0);
        dma_wait_fast();
        dma_channel_send_chain(DMA_CHANNEL_GIF,current->data,q-current->data,0,0);
        context^=1;
        draw_wait_finish();
        graph_wait_vsync();
    }

    packet_free(packets[0]);
    packet_free(packets[1]);
    free(tmp_f); free(xyz_f); free(col_f);
    free(tmp_c); free(xyz_c); free(col_c);
    free(tmp_b); free(xyz_b); free(col_b);
    return 0;
}

int main(int argc,char *argv[]) {
    framebuffer_t frame; zbuffer_t z;
    
    build_floor_grid(); // Initialize the floor geometry before booting
    
    load_modules();
    padInit(0);padPortOpen(0,0,padBuf);
    while(padGetState(0,0)!=PAD_STATE_STABLE);
    dma_channel_initialize(DMA_CHANNEL_GIF,NULL,0);
    dma_channel_fast_waits(DMA_CHANNEL_GIF);
    init_gs(&frame,&z);
    init_drawing_environment(&frame,&z);
    render(&frame,&z);
    SleepThread();
    return 0;
}