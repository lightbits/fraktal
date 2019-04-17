#pragma once
#include <stdlib.h>
#include "log.h"

enum { MAX_LINK_STATE_ITEMS = 1024 };
struct fLinkState
{
    const char *glsl_version;
    GLuint shaders[MAX_LINK_STATE_ITEMS];
    int num_shaders;
};

GLuint compile_shader(const char *name, const char **sources, int num_sources, GLenum type)
{
    assert(name && "Missing name string (e.g. filename or 'built-in')");
    assert(sources && "Missing shader source list");
    assert(num_sources > 0 && "Must have atleast one shader");
    assert((type == GL_VERTEX_SHADER || type == GL_FRAGMENT_SHADER));
    GLint status = 0;
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, num_sources, (const GLchar **)sources, 0);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        GLint length; glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        char *info = (char*)malloc(length);
        glGetShaderInfoLog(shader, length, NULL, info);
        log_err("Failed to compile shader (%s):\n%s", name, info);
        free(info);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool program_link_status(GLuint program)
{
    GLint status; glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status)
    {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        char *info = (char*)malloc(length);
        glGetProgramInfoLog(program, length, NULL, info);
        log_err("Failed to link program:\n%s", info);
        free(info);
        return false;
    }
    return true;
}

// On success, the function returns a handle that should be passed
// to subsequent link_add_file or add_link_data calls. If the call
// is successful, the caller owns the returned fLinkState, which
// should eventually be destroyed with fraktal_destroy_link.
fLinkState *fraktal_create_link()
{
    fLinkState *link = (fLinkState*)calloc(1, sizeof(fLinkState));
    link->num_shaders = 0;
    link->glsl_version = "#version 150";
    return link;
}

void fraktal_destroy_link(fLinkState *link)
{
    fraktal_assert(glGetError() == GL_NO_ERROR);
    if (link)
    {
        for (int i = 0; i < link->num_shaders; i++)
            if (link->shaders[i])
                glDeleteShader(link->shaders[i]);
        free(link);
        fraktal_assert(glGetError() == GL_NO_ERROR);
    }
}

bool add_link_data(fLinkState *link, const void *data, const char *name)
{
    fraktal_assert(glGetError() == GL_NO_ERROR);
    fraktal_assert(link);
    fraktal_assert(link->num_shaders < MAX_LINK_STATE_ITEMS);
    fraktal_assert(link->glsl_version);
    fraktal_assert(data && "'data' must be a non-NULL pointer to a buffer containing kernel source text.");
    const char *sources[] = {
        link->glsl_version,
        "\n#line 0\n",
        (const char*)data,
    };
    int num_sources = sizeof(sources)/sizeof(sources[0]);
    GLuint shader = compile_shader(name, sources, num_sources, GL_FRAGMENT_SHADER);
    if (!shader)
        return false;
    link->shaders[link->num_shaders++] = shader;
    fraktal_assert(glGetError() == GL_NO_ERROR);
    return true;
}

// 'link': Obtained from fraktal_create_link.
// 'data': A pointer to a buffer containing kernel source text. Must be NULL-terminated.
// 'size': Length of input data in bytes (excluding NULL-terminator). 0 can be passed if
//         the input is a NULL-terminated string.
// 'name': An optional name for this input in log messages.
//
// No references are kept to 'data' (it can safely be freed after the function returns).
bool fraktal_add_link_data(fLinkState *link, const void *data, size_t size, const char *name)
{
    return add_link_data(link, data, name);
}

// 'link': Obtained from fraktal_create_link.
// 'path': A NULL-terminated path to a file containing kernel source text.
//
// This method is equivalent to calling add_link_data on the contents of the file.
bool fraktal_add_link_file(fLinkState *link, const char *path)
{
    char *data = read_file(path);
    if (!data)
    {
        log_err("Failed to open file '%s'\n", path);
        return false;
    }
    bool result = add_link_data(link, (const void*)data, path);
    free(data);
    fraktal_assert(glGetError() == GL_NO_ERROR);
    return result;
}

// On success, this function returns a fKernel handle that is required for
// all kernel-specific operations, such as execution, setting parameters,
// or obtaining information on active parameters.
//
// If the call is successful, the caller owns the returned fKernel, which
// should eventually be destroyed with fraktal_destroy_kernel.
fKernel *fraktal_link_kernel(fLinkState *link)
{
    fraktal_assert(glGetError() == GL_NO_ERROR);
    fraktal_assert(link);
    fraktal_assert(link->num_shaders > 0 && "Atleast one kernel must be added to link state.");

    static GLuint vs = 0;
    if (!vs)
    {
        static const char *source =
            "in vec2 iPosition;\n"
            "void main()\n"
            "{\n"
            "    gl_Position = vec4(iPosition, 0.0, 1.0);\n"
            "}\n"
        ;
        const char *sources[] = { link->glsl_version, "\n#line 0\n", source };
        vs = compile_shader("built-in vertex shader", sources, sizeof(sources)/sizeof(char*), GL_VERTEX_SHADER);
    }
    if (!vs)
    {
        log_err("Failed to link kernel\n");
        return NULL;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    for (int i = 0; i < link->num_shaders; i++)
        glAttachShader(program, link->shaders[i]);
    glLinkProgram(program);
    glDetachShader(program, vs);
    for (int i = 0; i < link->num_shaders; i++)
        glDetachShader(program, link->shaders[i]);

    if (!program_link_status(program))
    {
        glDeleteProgram(program);
        log_err("Failed to link kernel\n");
        return NULL;
    }

    fKernel *kernel = (fKernel*)calloc(1, sizeof(fKernel));
    kernel->program = program;
    fraktal_assert(glGetError() == GL_NO_ERROR);
    return kernel;
}

// If NULL is passed the function silently returns.
void fraktal_destroy_kernel(fKernel *f)
{
    fraktal_assert(glGetError() == GL_NO_ERROR);
    if (f)
    {
        if (f->program)
            glDeleteProgram(f->program);
        free(f);
        fraktal_assert(glGetError() == GL_NO_ERROR);
    }
}

// 'path': A NULL-terminated path to a file containing kernel source text.
//
// This method is equivalent to calling link_create -> link_add_file(path) -> link_kernel.
fKernel *fraktal_load_kernel(const char *path)
{
    fraktal_assert(path);
    fLinkState *link = fraktal_create_link();
    fraktal_add_link_file(link, path);
    return fraktal_link_kernel(link);
}
