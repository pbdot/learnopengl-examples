//------------------------------------------------------------------------------
//  Camera (1)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_fetch.h"
#include "sokol_time.h"
#include "hmm/HandmadeMath.h"
#include "stb/stb_image.h"
#include "shaders.glsl.h"
#include "ui/ui.h"

/* application state */
static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    uint8_t file_buffer[256 * 1024];
    hmm_vec3 cube_positions[10];
} state;

static void fetch_callback(const sfetch_response_t*);

static void init(void) {
    sg_setup(&(sg_desc){
        .gl_force_gles2 = true,
        .mtl_device = sapp_metal_get_device(),
        .mtl_renderpass_descriptor_cb = sapp_metal_get_renderpass_descriptor,
        .mtl_drawable_cb = sapp_metal_get_drawable,
        .d3d11_device = sapp_d3d11_get_device(),
        .d3d11_device_context = sapp_d3d11_get_device_context(),
        .d3d11_render_target_view_cb = sapp_d3d11_get_render_target_view,
        .d3d11_depth_stencil_view_cb = sapp_d3d11_get_depth_stencil_view
    });
    ui_setup(1);

     /* setup sokol-fetch
        The 1 channel and 1 lane configuration essentially serializes
        IO requests. Which is just fine for this example. */
    sfetch_setup(&(sfetch_desc_t){
        .max_requests = 2,
        .num_channels = 1,
        .num_lanes = 1
    });

    /* initialize sokol_time */
    stm_setup();

    /* Allocate an image handle, but don't actually initialize the image yet,
       this happens later when the asynchronous file load has finished.
       Any draw calls containing such an "incomplete" image handle
       will be silently dropped.
    */
    state.bind.fs_images[SLOT_texture1] = sg_alloc_image();
    state.bind.fs_images[SLOT_texture2] = sg_alloc_image();

    /* flip images vertically after loading */
    stbi_set_flip_vertically_on_load(true);  

    state.cube_positions[0] = HMM_Vec3( 0.0f,  0.0f,  0.0f);
    state.cube_positions[1] = HMM_Vec3( 2.0f,  5.0f, -15.0f);
    state.cube_positions[2] = HMM_Vec3(-1.5f, -2.2f, -2.5f);
    state.cube_positions[3] = HMM_Vec3(-3.8f, -2.0f, -12.3f);
    state.cube_positions[4] = HMM_Vec3( 2.4f, -0.4f, -3.5f);
    state.cube_positions[5] = HMM_Vec3(-1.7f,  3.0f, -7.5f);
    state.cube_positions[6] = HMM_Vec3( 1.3f, -2.0f, -2.5f);
    state.cube_positions[7] = HMM_Vec3( 1.5f,  2.0f, -2.5f);
    state.cube_positions[8] = HMM_Vec3( 1.5f,  0.2f, -1.5f);
    state.cube_positions[9] = HMM_Vec3(-1.3f,  1.0f, -1.5f);

    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
        0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };
    
    state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(vertices),
        .content = vertices,
        .label = "cube-vertices"
    });

    /* create shader from code-generated sg_shader_desc */
    sg_shader shd = sg_make_shader(simple_shader_desc());

    /* create a pipeline object (default render states are fine for triangle) */
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
        .layout = {
            .attrs = {
                [ATTR_vs_aPos].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_aTexCoord].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .depth_stencil = {
            .depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL,
            .depth_write_enabled = true,
        },
        .label = "triangle-pipeline"
    });
    
    /* a pass action to clear framebuffer */
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .action=SG_ACTION_CLEAR, .val={0.2f, 0.3f, 0.3f, 1.0f} }
    };

    uint32_t slot1 = SLOT_texture1;

    /* start loading the JPG file */
    sfetch_send(&(sfetch_request_t){
        .path = "container.jpg",
        .callback = fetch_callback,
        .buffer_ptr = state.file_buffer,
        .buffer_size = sizeof(state.file_buffer),
        .user_data_ptr = &slot1,
        .user_data_size = sizeof(slot1)
    });

    uint32_t slot2 = SLOT_texture2;

    /* start loading the PNG file
       we can use the same buffer because we are serializing the request (see sfetch_setup) */
    sfetch_send(&(sfetch_request_t){
        .path = "awesomeface.png",
        .callback = fetch_callback,
        .buffer_ptr = state.file_buffer,
        .buffer_size = sizeof(state.file_buffer),
        .user_data_ptr = &slot2,
        .user_data_size = sizeof(slot2)
    });
}

/* The fetch-callback is called by sokol_fetch.h when the data is loaded,
   or when an error has occurred.
*/
static void fetch_callback(const sfetch_response_t* response) {
    if (response->fetched) {
        /* the file data has been fetched, since we provided a big-enough
           buffer we can be sure that all data has been loaded here
        */
        int img_width, img_height, num_channels;
        const int desired_channels = 4;
        stbi_uc* pixels = stbi_load_from_memory(
            response->buffer_ptr,
            (int)response->fetched_size,
            &img_width, &img_height,
            &num_channels, desired_channels);
        if (pixels) {
            /* we attached the image slot value to the request's user_data */
            uint32_t image_slot = *(uint32_t*)response->user_data;
            /* initialize the sokol-gfx texture */
            sg_init_image(state.bind.fs_images[image_slot], &(sg_image_desc){
                .width = img_width,
                .height = img_height,
                /* set pixel_format to RGBA8 for WebGL */
                .pixel_format = SG_PIXELFORMAT_RGBA8,
                .wrap_u = SG_WRAP_REPEAT,
                .wrap_v = SG_WRAP_REPEAT,
                .min_filter = SG_FILTER_LINEAR,
                .mag_filter = SG_FILTER_LINEAR,
                .content.subimage[0][0] = {
                    .ptr = pixels,
                    .size = img_width * img_height * 4,
                }
            });
            stbi_image_free(pixels);
        }
    }
    else if (response->failed) {
        // if loading the file failed, set clear color to red
        state.pass_action = (sg_pass_action) {
            .colors[0] = { .action = SG_ACTION_CLEAR, .val = { 1.0f, 0.0f, 0.0f, 1.0f } }
        };
    }
}

void frame(void) {
    sfetch_dowork();

    const float radius = 10.0f;
    float camX = sinf((float)stm_sec(stm_now())) * radius;
    float camZ = cosf((float)stm_sec(stm_now())) * radius;

    hmm_mat4 view = HMM_LookAt( HMM_Vec3(camX, 0.0f, camZ), 
                                HMM_Vec3(0.0f, 0.0f, 0.0f), 
                                HMM_Vec3(0.0f, 1.0f, 0.0f));

    hmm_mat4 projection = HMM_Perspective(45.0f, 800.0f / 600.0f, 0.1f, 100.0f);

    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);

    vs_params_t vs_params = {
        .view = view,
        .projection = projection
    };

    for(size_t i = 0; i < 10; i++) {
        hmm_mat4 model = HMM_Translate(state.cube_positions[i]);
        float angle = 20.0f * i; 
        model = HMM_MultiplyMat4(model, HMM_Rotate(angle, HMM_Vec3(1.0f, 0.3f, 0.5f)));
        vs_params.model = model;
        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));

        sg_draw(0, 36, 1);
    }

    ui_draw(NULL);
    sg_end_pass();
    sg_commit();
}

void cleanup(void) {
    ui_shutdown();
    sg_shutdown();
    sfetch_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = ui_event,
        .width = 800,
        .height = 600,
        .gl_force_gles2 = true,
        .window_title = "Lookat (LearnOpenGL)",
    };
}