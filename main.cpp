/*
Pathtracing on the GPU:
    http://is.muni.cz/th/396530/fi_b/Bachelor.pdf
    Describes relevant notation and provides GLSL snippets.
*/

#include "GL/glew.h"
#include "SDL.h"
#include "SDL_opengl.h"
#include "config.h"
#include "matrix.h"
#include "util.h"
#include <stdio.h>

#include "util.cpp"
#include "matrix.cpp"
#define EXIT_FAILURE -1
#define EXIT_SUCCESS 0

struct Attribs
{
    GLint position;
    GLint texel;
    GLint extra1;
    GLint extra2;
};

struct Uniforms
{

};

struct App
{
    SDL_Window *window;
    SDL_GLContext gl_context;
    GLuint shader_test;

    bool running;
};

static App app;

void 
progressive_render()
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    SDL_GL_SwapWindow(app.window);
}

void
handle_event(SDL_Event &event)
{
    switch (event.type)
    {
    case SDL_QUIT:
        app.running = false;
        break;
    case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_SPACE)
            progressive_render();
        break;
    case SDL_KEYUP:
        if (event.key.keysym.sym == SDLK_ESCAPE)
            app.running = false;
        break;
    }
}

int 
wmain(int argc, wchar_t **argv)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        const char *error = SDL_GetError();
        printf("Failed to initialize SDL: %s", error);
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR, "An error occurred", error, 0);
        return EXIT_FAILURE;
    }

    app.window = SDL_CreateWindow(
        WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
    app.gl_context = SDL_GL_CreateContext(app.window);
    
    if (!app.window)
    {
        const char *error = SDL_GetError();
        printf("Failed to create window: %s", error);
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR, "An error occurred", error, 0);
        return EXIT_FAILURE;
    }

    GLboolean glew_experimental = true;
    GLenum glew_error = glewInit();
    if (glew_error != GLEW_OK)
    {
        const char *error = (const char *)glewGetErrorString(glew_error);
        printf("Failed to load OpenGL functions: %s\n", error);
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR, "An error occurred", error, 0);
        return EXIT_FAILURE;
    }

    app.shader_test = load_program("./shaders/test.vs", "./shaders/test.fs");
    if (!app.shader_test)
    {
        return EXIT_FAILURE;
    }

    app.running = true;
    SDL_Event event = {};
    while (app.running)
    {
        SDL_PollEvent(&event);
        handle_event(event);
    }

    SDL_GL_DeleteContext(app.gl_context);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
    return EXIT_SUCCESS;
}