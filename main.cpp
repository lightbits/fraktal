/*
Pathtracing on the GPU:
    http://is.muni.cz/th/396530/fi_b/Bachelor.pdf
    Describes relevant notation and provides GLSL snippets.

http://blog.hvidtfeldts.net/index.php/2012/10/image-based-lighting/
http://people.cs.kuleuven.be/~philip.dutre/GI/TotalCompendium.pdf
http://blog.hvidtfeldts.net/index.php/2015/01/path-tracing-3d-fractals/
*/

#include "config.h"
#include "matrix.h"
#include "util.h"
#include <stdint.h>
typedef uint32_t    uint;
typedef uint64_t    uint64;
typedef uint32_t    uint32;
typedef uint16_t    uint16;
typedef uint8_t     uint8;
typedef int32_t     int32;
typedef int16_t     int16;
typedef int8_t      int8;

#include <stdio.h>
#include "util.cpp"
#include "matrix.cpp"

struct Shader
{
    GLuint program;
    GLint position;
    GLint sampler0;
    GLint view;
    GLint aspect;
    GLint tan_fov_h;
    GLint iteration;
    GLint pixel_size;
    GLint time;
};

struct Frame
{
    GLuint buffer;
    GLuint texture;
    int width;
    int height;
};

struct App
{
    SDL_Window *window;
    SDL_GLContext gl_context;
    int window_width;
    int window_height;
    bool running;
    mat4 view;
    float aspect;
    float tan_fov_h;
    int iteration;
    float elapsed_time;

    GLuint tex_sky;
};

static App app;

void 
save_screenshot()
{
    int w = app.window_width;
    int h = app.window_height;
    uint8 *pixels = new uint8[w * h * 3];
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    // Flip y
    for (int y = 0; y < h / 2; y++)
    for (int x = 0; x < w * 3; x++)
    {
        int from = (h - 1 - y) * w * 3 + x;
        int to = y * w * 3 + x;
        uint8 temp = pixels[from];
        pixels[from] = pixels[to];
        pixels[to] = temp;
    }

    save_rgb_to_png("screenshot.png", w, h, (void*)pixels);
    delete[] pixels;
}

Frame
gen_frame(int width, int height)
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 
                 width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLuint buffer;
    glGenFramebuffers(1, &buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, buffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                           GL_TEXTURE_2D, texture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    Frame result;
    result.texture = texture;
    result.buffer = buffer;
    result.width = width;
    result.height = height;
    return result;
}

void
draw_triangles(Shader &shader, GLuint buffer, GLsizei count)
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(shader.position);
    glVertexAttribPointer(shader.position, 2, GL_FLOAT, GL_FALSE, 
                          sizeof(GLfloat) * 2, 0);
    glDrawArrays(GL_TRIANGLES, 0, count);
    glDisableVertexAttribArray(shader.position);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void
clear_frame(Frame &frame)
{
    glBindFramebuffer(GL_FRAMEBUFFER, frame.buffer);
    glViewport(0, 0, frame.width, frame.height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void 
render_scene(Frame &frame, Shader &shader, GLuint quad)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glBlendEquation(GL_FUNC_ADD);
    glBindFramebuffer(GL_FRAMEBUFFER, frame.buffer);
    glViewport(0, 0, frame.width, frame.height);
    glUseProgram(shader.program);
    glBindTexture(GL_TEXTURE_2D, app.tex_sky);
    glUniformMatrix4fv(shader.view, 1, GL_FALSE, app.view.data);
    glUniform2f(shader.pixel_size, 
                1.0f / (float)frame.width, 
                1.0f / (float)frame.height);
    glUniform1f(shader.time, app.elapsed_time);
    glUniform1f(shader.aspect, app.aspect);
    glUniform1f(shader.tan_fov_h, app.tan_fov_h);
    glUniform1i(shader.sampler0, 0);
    draw_triangles(shader, quad, 6);
    glDisable(GL_BLEND);
}

void
render_blit(Frame &frame_to_blit, Shader &shader, GLuint quad)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, app.window_width, app.window_height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shader.program);
    glBindTexture(GL_TEXTURE_2D, frame_to_blit.texture);
    glUniform1i(shader.iteration, app.iteration);
    glUniform1i(shader.sampler0, 0);
    draw_triangles(shader, quad, 6);
}

double 
get_elapsed_time(uint64 begin, uint64 end)
{
    uint64 frequency = SDL_GetPerformanceFrequency();
    return (double)(end - begin) / (double)frequency;
}

int 
wmain(int argc, wchar_t **argv)
{
    int init_status = SDL_Init(SDL_INIT_EVERYTHING);
    assert(init_status == 0);

    app.window_width = 1000;
    app.window_height = 500;

    app.window = SDL_CreateWindow(
        WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        app.window_width, app.window_height, SDL_WINDOW_OPENGL);
    app.gl_context = SDL_GL_CreateContext(app.window);
    assert(app.window);

    GLboolean glew_experimental = true;
    GLenum glew_error = glewInit();
    assert(glew_error == GLEW_OK);

    app.view = mat_identity();
    app.aspect = app.window_width / (float)app.window_height;
    app.tan_fov_h = tan(PI / 10.0f);

    Shader shader_test = {};
    GLuint program = 
        load_program("./shaders/test.vs", "./shaders/test.fs");    
    shader_test.position   = glGetAttribLocation(program, "position");
    shader_test.view       = glGetUniformLocation(program, "view");
    shader_test.aspect     = glGetUniformLocation(program, "aspect");
    shader_test.pixel_size = glGetUniformLocation(program, "pixel_size");
    shader_test.tan_fov_h  = glGetUniformLocation(program, "tan_fov_h");
    shader_test.sampler0   = glGetUniformLocation(program, "sampler0");
    shader_test.iteration  = glGetUniformLocation(program, "iteration");
    shader_test.time       = glGetUniformLocation(program, "time");
    shader_test.program    = program;
    assert(program);

    Shader shader_blit = {};
    program = 
        load_program("./shaders/blit.vs", "./shaders/blit.fs");    
    shader_blit.position  = glGetAttribLocation(program, "position");
    shader_blit.iteration = glGetUniformLocation(program, "iteration");
    shader_blit.sampler0  = glGetUniformLocation(program, "sampler0");
    shader_blit.program   = program;
    assert(program);

    app.tex_sky = load_texture("./data/aosky001_lite.jpg",
                               GL_LINEAR, GL_LINEAR,
                               GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    assert(app.tex_sky);

    float quad_data[] = {
        -1.0f, -1.0f,
        +1.0f, -1.0f,
        +1.0f, +1.0f,
        +1.0f, +1.0f,
        -1.0f, +1.0f,
        -1.0f, -1.0f
    };
    GLuint quad = gen_buffer(quad_data, sizeof(quad_data));

    Frame frame = gen_frame(app.window_width / 2, app.window_height / 2);
    clear_frame(frame);

    struct Camera
    {
        vec3 position;
        vec3 rotation;
    };
    Camera camera = {};
    camera.position = Vec3(0.0f, -0.3f, -1.8f);
    camera.rotation = Vec3(0.2f, -0.9f, 0.0f);

    app.running = true;
    app.iteration = 1;
    uint64 app_begin = SDL_GetPerformanceCounter();
    while (app.running)
    {
        SDL_Event event = {};
        SDL_PollEvent(&event);
        switch (event.type)
        {
        case SDL_QUIT:
        {
            app.running = false;  
        } break;

        case SDL_KEYDOWN:
        {
            if (event.key.keysym.sym == SDLK_ESCAPE)
                app.running = false;

            float move_step = 0.1f;
            float rotate_step = 0.1f;
            if (event.key.keysym.sym == SDLK_SPACE)
                camera.position.y -= move_step;
            if (event.key.keysym.sym == SDLK_LCTRL)
                camera.position.y += move_step;

            if (event.key.keysym.sym == SDLK_a)
                camera.position.x += move_step;
            if (event.key.keysym.sym == SDLK_d)
                camera.position.x -= move_step;
            if (event.key.keysym.sym == SDLK_w)
                camera.position.z += move_step;
            if (event.key.keysym.sym == SDLK_s)
                camera.position.z -= move_step;

            if (event.key.keysym.sym == SDLK_LEFT)
                camera.rotation.y -= rotate_step;
            if (event.key.keysym.sym == SDLK_RIGHT)
                camera.rotation.y += rotate_step;

            if (event.key.keysym.sym == SDLK_UP)
                camera.rotation.x += rotate_step;
            if (event.key.keysym.sym == SDLK_DOWN)
                camera.rotation.x -= rotate_step;

            if (event.key.keysym.sym == SDLK_PRINTSCREEN)
                save_screenshot();

            if (event.key.keysym.sym != SDLK_r)
            {
                clear_frame(frame);
                app.iteration = 1;
            }

            uint64 scene_begin = SDL_GetPerformanceCounter();

            app.view = mat_translate(camera.position) *
                       mat_rotate_x(camera.rotation.x) * 
                       mat_rotate_y(camera.rotation.y);
            render_scene(frame, shader_test, quad);
            render_blit(frame, shader_blit, quad);
            glFinish();
            SDL_GL_SwapWindow(app.window);

            uint64 scene_end = SDL_GetPerformanceCounter();
            double scene_time = get_elapsed_time(scene_begin, scene_end);

            // Should probably use a framerate independent timer for this
            // Since we do motion blur, and things must move at a consistent
            // speed from frame to frame.
            double time_step = 0.03;
            app.elapsed_time += time_step;

            // Otherwise, do this I guess...
            // app.elapsed_time = get_elapsed_time(app_begin, scene_end);

            app.iteration++;
            char title[256];
            sprintf(title, "Iteration %d (%.2f ms)", app.iteration, 1000.0 * scene_time);
            SDL_SetWindowTitle(app.window, title);
            
        } break;
        }
    }

    SDL_GL_DeleteContext(app.gl_context);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
    return EXIT_SUCCESS;
}