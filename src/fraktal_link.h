#pragma once
#include <stdlib.h>
#include <log.h>
#include <file.h>

enum { MAX_LINK_STATE_ITEMS = 1024 };
struct fLinkState
{
    const char *glsl_version;
    GLuint shaders[MAX_LINK_STATE_ITEMS];
    int num_shaders;
    fParams params;
};

static GLuint compile_shader(const char *name, const char **sources, int num_sources, GLenum type)
{
    fraktal_ensure_context();
    fraktal_check_gl_error();
    fraktal_assert(sources && "Missing shader source list");
    fraktal_assert(num_sources > 0 && "Must have atleast one shader");
    fraktal_assert((type == GL_VERTEX_SHADER || type == GL_FRAGMENT_SHADER));
    if (!name)
        name = "unnamed";

    GLuint shader = glCreateShader(type);
    if (!shader)
    {
        log_err("Failed to compile shader (%s): glCreateShader returned 0.\n", name);
        return 0;
    }

    glShaderSource(shader, num_sources, (const GLchar **)sources, 0);
    glCompileShader(shader);

    GLint status = 0;
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
    fraktal_check_gl_error();
    return shader;
}

static bool program_link_status(GLuint program)
{
    fraktal_ensure_context();
    fraktal_check_gl_error();
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
    fraktal_check_gl_error();
    return true;
}

static bool add_link_data(fLinkState *link, char *data, const char *name)
{
    fraktal_assert(link);
    fraktal_assert(link->num_shaders < MAX_LINK_STATE_ITEMS);
    fraktal_assert(link->glsl_version);
    fraktal_assert(data && "'data' must be a non-NULL pointer to a buffer containing kernel source text.");
    fraktal_ensure_context();
    fraktal_check_gl_error();
    if (!parse_fraktal_source(data, &link->params, name))
    {
        log_err("Error parsing kernel source\n");
        return false;
    }
    const char *sources[] = {
        link->glsl_version,
        "\nuniform int Dummy;\n"
        "#define ZERO (min(0, Dummy))\n"
        #ifdef FRAKTAL_GUI
        "#define FRAKTAL_GUI\n"
        #endif
        "\n#line 0\n",
        (const char*)data,
    };
    int num_sources = sizeof(sources)/sizeof(sources[0]);
    GLuint shader = compile_shader(name, sources, num_sources, GL_FRAGMENT_SHADER);
    if (!shader)
        return false;
    link->shaders[link->num_shaders++] = shader;
    fraktal_check_gl_error();
    return true;
}

fLinkState *fraktal_create_link()
{
    fraktal_ensure_context();
    fLinkState *link = (fLinkState*)malloc(sizeof(fLinkState));
    link->num_shaders = 0;
    link->glsl_version = "#version 150";
    link->params.count = 0;
    link->params.sampler_count = 0;
    return link;
}

void fraktal_destroy_link(fLinkState *link)
{
    if (link)
    {
        fraktal_ensure_context();
        fraktal_check_gl_error();
        for (int i = 0; i < link->num_shaders; i++)
            if (link->shaders[i])
                glDeleteShader(link->shaders[i]);
        free(link);
        fraktal_check_gl_error();
    }
}

bool fraktal_add_link_data(fLinkState *link, const char *data, unsigned int size, const char *name)
{
    // cannot assume that we are allowed to modify user data, so we make a copy.
    if (size == 0) size = (unsigned int)strlen(data);
    char *copy = (char*)malloc(size + 1);
    strcpy(copy, data);
    fraktal_assert(copy && "Ran out of memory");
    bool result = add_link_data(link, copy, name);
    free(copy);
    return result;
}

bool fraktal_add_link_file(fLinkState *link, const char *path)
{
    char *data = read_file(path);
    if (!data)
    {
        log_err("Failed to open file '%s'\n", path);
        return false;
    }
    bool result = add_link_data(link, data, path);
    free(data);
    return result;
}

fKernel *fraktal_link_kernel(fLinkState *link)
{
    fraktal_assert(link);
    fraktal_ensure_context();
    fraktal_check_gl_error();
    if (link->num_shaders <= 0)
        return NULL;

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

    fKernel *kernel = (fKernel*)malloc(sizeof(fKernel));
    kernel->program = program;
    kernel->params.count = link->params.count;
    kernel->params.sampler_count = link->params.sampler_count;
    kernel->loc_iPosition = 0;
    for (int i = 0; i < link->params.count; i++)
    {
        strcpy(kernel->params.name[i], link->params.name[i]);
        kernel->params.type[i] = link->params.type[i];
        kernel->params.mean[i] = link->params.mean[i];
        kernel->params.scale[i] = link->params.scale[i];
        kernel->params.offset[i] = glGetUniformLocation(program, link->params.name[i]);
        kernel->params.assigned_tex_unit[i] = link->params.assigned_tex_unit[i];
        kernel->params.std140_offset[i] = link->params.std140_offset[i];
        kernel->params.std140_size[i] = link->params.std140_size[i];
    }
    // print kernel information
    #if 0
    {
        for (int i = 0; i < kernel->params.count; i++)
        {
            printf("%s: ", kernel->params.name[i]);
            printf("%d: ", kernel->params.offset[i]);
            printf("%d: ", kernel->params.type[i]);
            printf("%f: ", kernel->params.mean[i].x);
            printf("%f: ", kernel->params.scale[i].x);
            printf("%d: ", kernel->params.assigned_tex_unit[i]);
            printf("%d: ", kernel->params.std140_offset[i]);
            printf("%d: ", kernel->params.std140_size[i]);
            printf("\n");
        }
        printf("num_params: %d\n", kernel->params.count);
        printf("num_samplers: %d\n", kernel->params.sampler_count);
    }
    #endif
    fraktal_check_gl_error();
    return kernel;
}

void fraktal_destroy_kernel(fKernel *f)
{
    if (f)
    {
        fraktal_ensure_context();
        fraktal_check_gl_error();
        if (f->program)
            glDeleteProgram(f->program);
        free(f);
        fraktal_check_gl_error();
    }
}

fKernel *fraktal_load_kernel(const char *path)
{
    fraktal_assert(path);
    fLinkState *link = fraktal_create_link();
    fraktal_add_link_file(link, path);
    return fraktal_link_kernel(link);
}
