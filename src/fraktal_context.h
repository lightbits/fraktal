// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

#pragma once
#include <log.h>
#include <GLFW/glfw3.h>

// Required to link with vc2010 GLFW
#if defined(_MSC_VER) && (_MSC_VER >= 1900)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void fraktal_glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Fraktal GLFW error %d: %s\n", error, description);
}

static GLFWwindow *fraktal_context = NULL;
static bool fraktal_gl_symbols_loaded = false;
static const char *fraktal_glsl_version = "#version 150";

bool fraktal_create_context()
{
    fraktal_assert(!fraktal_context && "A context already exists.");
    glfwSetErrorCallback(fraktal_glfw_error_callback);
    if (!glfwInit())
        return false;

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

    fraktal_context = glfwCreateWindow(32, 32, "fraktal", NULL, NULL);
    if (fraktal_context == NULL)
    {
        fprintf(stderr, "Error creating context: failed to create GLFW window.\n");
        return false;
    }
    return true;
}

void fraktal_destroy_context()
{
    if (fraktal_context)
        glfwDestroyWindow(fraktal_context);
    fraktal_context = NULL;
}

void fraktal_push_current_context()
{
    if (fraktal_context)
        glfwMakeContextCurrent(fraktal_context);
}

void fraktal_pop_current_context()
{
    if (fraktal_context)
        glfwMakeContextCurrent(NULL);
}

static void fraktal_ensure_context()
{
    if (fraktal_context)
        glfwMakeContextCurrent(fraktal_context);
    // else: we expect the caller to have made a context current
    // on the thread

    if (!fraktal_gl_symbols_loaded)
    {
        fraktal_gl_symbols_loaded = true;
        if (gl3wInit() != 0)
            fraktal_assert(false && "Failed to load OpenGL symbols.");
    }

    // verify that we have OpenGL symbols loaded by testing one
    // of the function pointers
    fraktal_assert(glCreateShader != NULL && "Failed to load OpenGL symbols.");
}
