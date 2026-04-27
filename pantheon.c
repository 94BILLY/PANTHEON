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

int points_count = 6;
int points[6] = { 0, 1, 2, 1, 2, 3 };
int vertex_count = 4;
VECTOR vertices[4] = {
    {-100.0f, 0.0f,-100.0f,1.0f},
    { 100.0f, 0.0f,-100.0f,1.0f},
    {-100.0f, 0.0f, 100.0f,1.0f},
    { 100.0f, 0.0f, 100.0f,1.0f},
};
VECTOR colours[4] = {
    {0.20f,0.60f,0.15f,1.0f},
    {0.25f,0.65f,0.10f,1.0f},
    {0.18f,0.55f,0.20f,1.0f},
    {0.22f,0.58f,0.12f,1.0f},
};
VECTOR object_position = {0.00f,  0.00f,  0.00f,1.00f};
VECTOR object_rotation = {0.00f,  0.00f,  0.00f,1.00f};
VECTOR camera_position = {0.00f, 150.00f, 0.00f,1.00f};
VECTOR camera_rotation = {1.57f,  0.00f,  0.00f,1.00f};

void init_gs(framebuffer_t *frame, zbuffer_t *z) {
    frame->width=640; frame->height=512; frame->mask=0; frame->psm=GS_PSM_32;
    frame->address=graph_vram_allocate(frame->width,frame->height,frame->psm,GRAPH_ALIGN_PAGE);
    z->enable=DRAW_ENABLE; z->mask=0; z->method=ZTEST_METHOD_GREATER_EQUAL; z->zsm=GS_ZBUF_32;
    z->address=graph_vram_allocate(frame->width,frame->height,z->zsm,GRAPH_ALIGN_PAGE);
    graph_initialize(frame->address,frame->width,frame->height,frame->psm,0,0);
}

void init_drawing_environment(framebuffer_t *frame, zbuffer_t *z) {
    packet_t *packet=packet_init(16,PACKET_NORMAL);
    qword_t *q=packet->data;
    q=draw_setup_environment(q,0,frame,z);
    q=draw_primitive_xyoffset(q,0,(2048-frame->width/2),(2048-frame->height/2));
    q=draw_finish(q);
    dma_channel_send_normal(DMA_CHANNEL_GIF,packet->data,q-packet->data,0,0);
    dma_wait_fast();
    packet_free(packet);
}

int render(framebuffer_t *frame, zbuffer_t *z) {
    int i;
    static color_t  c_out[4];
    static xyz_t    v_out[4];
    static VECTOR   t_vert[4];
    MATRIX local_world,world_view,view_screen,local_screen;

    create_local_world(local_world,object_position,object_rotation);
    create_world_view(world_view,camera_position,camera_rotation);
    create_view_screen(view_screen,graph_aspect_ratio(),-3.0f,3.0f,-256.0f,256.0f,1.0f,1000.0f);
    create_local_screen(local_screen,local_world,world_view,view_screen);
    calculate_vertices(t_vert,vertex_count,vertices,local_screen);
    draw_convert_rgbq(c_out,vertex_count,(vertex_f_t*)t_vert,(color_f_t*)colours,0x80);
    draw_convert_xyz(v_out,2048,2048,32,vertex_count,(vertex_f_t*)t_vert);

    packet_t *packet=packet_init(100,PACKET_NORMAL);
    qword_t *q=packet->data;
    q=draw_disable_tests(q,0,z);
    q=draw_clear(q,0,2048.0f-frame->width/2,2048.0f-frame->height/2,frame->width,frame->height,30,30,50);
    q=draw_enable_tests(q,0,z);

    prim_t prim;
    prim.type=PRIM_TRIANGLE; prim.shading=PRIM_SHADE_GOURAUD;
    prim.mapping=DRAW_DISABLE; prim.fogging=DRAW_DISABLE;
    prim.blending=DRAW_DISABLE; prim.antialiasing=DRAW_ENABLE;
    prim.mapping_type=PRIM_MAP_ST; prim.colorfix=PRIM_UNFIXED;

    color_t col; col.r=0x80; col.g=0x80; col.b=0x80; col.a=0x80; col.q=1.0f;
    q=draw_prim_start(q,0,&prim,&col);
    for(i=0;i<points_count;i++) {
        q->dw[0]=c_out[points[i]].rgbaq;
        q->dw[1]=v_out[points[i]].xyz;
        q++;
    }
    q=draw_prim_end(q,2,DRAW_RGBAQ_REGLIST);
    q=draw_finish(q);

    dma_channel_send_normal(DMA_CHANNEL_GIF,packet->data,q-packet->data,0,0);
    draw_wait_finish();
    graph_wait_vsync();
    packet_free(packet);
    return 0;
}

int main(void) {
    framebuffer_t frame;
    zbuffer_t z;
    dma_channel_initialize(DMA_CHANNEL_GIF,NULL,0);
    dma_channel_fast_waits(DMA_CHANNEL_GIF);
    init_gs(&frame,&z);
    init_drawing_environment(&frame,&z);
    while(1) {
        object_rotation[1]+=0.008f;
        render(&frame,&z);
    }
    return 0;
}
