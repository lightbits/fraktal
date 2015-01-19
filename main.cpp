#include "SDL.h"
#include "config.h"
#include <stdio.h>

#define EXIT_FAILURE -1
#define EXIT_SUCCESS 0

struct App
{
    SDL_Window *window;
    SDL_GLContext gl_context;
    unsigned int window_flags;
};

static App app;

void 
create_window()
{
    app.window = SDL_CreateWindow(
        WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL | app.window_flags);
    app.gl_context = SDL_GL_CreateContext(app.window);
    SDL_GL_SetSwapInterval(0);
}

int 
wmain(int argc, wchar_t **argv)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        const char *error = SDL_GetError();
        printf("Failed to initialize SDL: %s", error);
        return EXIT_FAILURE;
    }

    create_window();
    if (!app.window)
    {
        const char *error = SDL_GetError();
        printf("Failed to create window: %s", error);
        return EXIT_FAILURE;
    }

    glew_experimental = true;
    GLenum glew_error = glewInit();
    if (glew_error != GLEW_OK)
    {
        const char *error = glewGetErrorString(glew_error);
        printf("Failed to load OpenGL functions: %s\n", error);
        return EXIT_FAILURE;
    }

    SDL_GL_DeleteContext(app.gl_context);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
    return EXIT_SUCCESS;
}