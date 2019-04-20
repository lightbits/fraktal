// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

#pragma once
#include <log.h>
#include <GLFW/glfw3.h>

// Required to link with vc2010 GLFW
#if defined(_MSC_VER) && (_MSC_VER >= 1900)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

struct fContext
{
    GLFWwindow *window;
};

static void fraktal_glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Fraktal GLFW error %d: %s\n", error, description);
}

static fContext *fraktal_default_context = NULL;
static int fraktal_num_user_created_contexts = 0;

fContext *fraktal_create_context()
{
    glfwSetErrorCallback(fraktal_glfw_error_callback);
    if (!glfwInit())
        return NULL;

    #if __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);            // Required on Mac
    #else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    #endif
    glfwWindowHint(GLFW_VISIBLE, false);

    GLFWwindow* window = glfwCreateWindow(32, 32, "fraktal_context", NULL, NULL);
    if (window == NULL)
    {
        fprintf(stderr, "Error creating context: failed to create GLFW window.\n");
        return NULL;
    }
    glfwMakeContextCurrent(window);

    if (gl3wInit() != 0)
    {
        fprintf(stderr, "Error creating context: failed to load OpenGL backend.\n");
        glfwDestroyWindow(window);
        return NULL;
    }

    fContext *ctx = (fContext*)calloc(1, sizeof(fContext));
    ctx->window = window;
    fraktal_num_user_created_contexts++;
    return ctx;
}

void fraktal_share_context()
{
    fraktal_assert(!fraktal_default_context && "fraktal_share_context must be called before any other fraktal function.");
    fContext *ctx = (fContext*)calloc(1, sizeof(fContext));
    ctx->window = NULL;
    fraktal_default_context = ctx;
}

void fraktal_destroy_context(fContext *ctx)
{
    if (ctx)
    {
        if (ctx->window)
            glfwDestroyWindow(ctx->window);
        free(ctx);
        fraktal_num_user_created_contexts--;
    }
}

void fraktal_make_context_current(fContext *ctx)
{
    if (ctx && ctx->window)
        glfwMakeContextCurrent(ctx->window);
    else if (!ctx)
        glfwMakeContextCurrent(NULL);
}

static void fraktal_ensure_context()
{
    if (!fraktal_default_context)
    {
        if (fraktal_num_user_created_contexts > 0)
        {
            // the user is responsible for ensuring that their
            // created context is current on the thread.
        }
        else
        {
            // the user has not created any contexts, and has
            // not specified to share contexts. so we create a
            // default context.
            fraktal_default_context = fraktal_create_context();
            fraktal_assert(fraktal_default_context && "Failed to create default GPU context (OpenGL)");
            fraktal_num_user_created_contexts--; // that one didn't count...
        }
    }
    else
    {
        if (fraktal_default_context->window)
        {
            fraktal_assert(fraktal_num_user_created_contexts == 0 && "You must call fraktal_create_context before any other fraktal functions.");
            // we have a default context and will force current
            // context back to fraktal.
            glfwMakeContextCurrent(fraktal_default_context->window);
        }
        else
        {
            fraktal_assert(fraktal_num_user_created_contexts == 0 && "You do not call fraktal_create_context together with fraktal_share_context.");
            // user has specified to share contexts. it is their
            // responsibility to manage the current context.
        }
    }
}
