// Preliminary API
typedef int fraktal_enum;
enum fraktal_enum_
{
    FRAKTAL_READ_ONLY_ARRAY,
    FRAKTAL_READ_WRITE_ARRAY,
    FRAKTAL_WRAP_CLAMP_TO_EDGE,
    FRAKTAL_WRAP_REPEAT,
    FRAKTAL_FILTER_LINEAR,
    FRAKTAL_FILTER_NEAREST
}

void *fraktal_load_kernel_file(const char *filename);
void *fraktal_load_kernel_files(const char **filenames, int num_files);
void fraktal_run_kernel(void *output_array, void *kernel, int dim_x, int dim_y);

void *fraktal_create_array2D_uint8(void *data, int dim_x, int dim_y, int components, fraktal_enum access=FRAKTAL_READ_ONLY_ARRAY);
void *fraktal_create_array2D_float(void *data, int dim_x, int dim_y, int components, fraktal_enum access=FRAKTAL_READ_ONLY_ARRAY);
void *fraktal_create_array1D_float(void *data, int dim, int components, fraktal_enum access=FRAKTAL_READ_ONLY_ARRAY);
void fraktal_zero_array(void *handle);

// Transfer a GPU array into CPU memory. The number of bytes to transfer
// is automatically deduced (note: gpu_array must be a valid pointer returned
// from one of the create_array* functions.
void *fraktal_gpu_to_cpu(void *cpu_memory, void *gpu_array);

// A kernel may have multiple named input arguments, declared in the
// kernel source code. For example:
//
//   uniform float aFloatVariable;
//   uniform sampler2D aTexture2DVariable;
//   uniform mat4 aMatrix4fVariable;
//
// are all valid input declarations. Variables are given a value for
// subsequent kernel runs using the functions below. Unset variables
// are initialized to zero (0). Calling the functions on unused variables
// (that is, variables that are not referred to) has no effect.
void fraktal_input_1f(const char *name, float x);
void fraktal_input_2f(const char *name, float x, float y);
void fraktal_input_3f(const char *name, float x, float y, float z);
void fraktal_input_4f(const char *name, float x, float y, float z, float w);
void fraktal_input_1i(const char *name, int x);
void fraktal_input_2i(const char *name, int x, int y);
void fraktal_input_3i(const char *name, int x, int y, int z);
void fraktal_input_4i(const char *name, int x, int y, int z, int w);
void fraktal_input_matrix2fv(const char *name, float *m);
void fraktal_input_matrix3fv(const char *name, float *m);
void fraktal_input_matrix4fv(const char *name, float *m);
void fraktal_input_matrix2fv_transpose(const char *name, float *m);
void fraktal_input_matrix3fv_transpose(const char *name, float *m);
void fraktal_input_matrix4fv_transpose(const char *name, float *m);
void fraktal_input_array2D(const char *name, void *handle, fraktal_enum wrap, fraktal_enum filter);
void fraktal_input_array1D(const char *name, void *handle, fraktal_enum wrap, fraktal_enum filter);

#ifdef FRAKTAL_OPENGL_INTEROP
GLuint fraktal_array_gl_texture(void *handle);
#endif

#include <assert.h>
#define fraktal_assert assert

struct fKernel
{
    GLuint program;

};

struct fArray
{
    GLuint fbo;
    GLuint color0;
    int width;
    int height;
    fEnum format;
    fEnum access;
};

fKernel *fraktal_load_kernel_files(const char **filenames, int num_files)
{

}

fKernel *fraktal_load_kernel_file(const char *filename)
{
    return fraktal_load_kernel_files(&filename, 1);
}

bool fraktal_format_to_gl_format(int channels,
                                 fEnum format,
                                 GLenum *internal_format,
                                 GLenum *data_format,
                                 GLenum *data_type)
{
    if (format == FRAKTAL_FORMAT_FLOAT)
    {
        *data_type = GL_FLOAT;
        if      (channels == 1) { *internal_format = GL_R32F; *data_format = GL_RED; return true; }
        else if (channels == 2) { *internal_format = GL_RG32F; *data_format = GL_RG; return true; }
        else if (channels == 4) { *internal_format = GL_RGBA32F; *data_format = GL_RGBA; return true; }
    }
    else if (format == FRAKTAL_FORMAT_UNSIGNED_INT8)
    {
        *data_type = GL_UNSIGNED_BYTE;
        if      (channels == 1) { *internal_format = GL_R8; *data_format = GL_RED; return true; }
        else if (channels == 2) { *internal_format = GL_RG8; *data_format = GL_RG; return true; }
        else if (channels == 4) { *internal_format = GL_RGBA8; *data_format = GL_RGBA; return true; }
    }
    return false;
}

fArray *fraktal_create_array(
    const void *data,
    int width,
    int height,
    int channels,
    fEnum format,
    fEnum access)
{
    fraktal_assert(glGetError() == GL_NO_ERROR);
    fraktal_assert(components > 0 && components <= 4);
    fraktal_assert(width > 0 && height >= 0);
    fraktal_assert(access == FRAKTAL_READ_ONLY || access == FRAKTAL_READ_WRITE);
    fraktal_assert(channels == 1 || channels == 2 || channels == 4);

    GLenum internal_format,data_format,data_type;
    fraktal_assert(fraktal_format_to_gl_format(channels, format, &internal_format, &data_format, &data_type) && "Invalid array format");

    GLenum target = height == 0 ? GL_TEXTURE_1D : GL_TEXTURE_2D;

    GLuint color0 = 0;
    {
        glGenTextures(1, &color0);
        glBindTexture(target, color0);
        if (target == GL_TEXTURE_1D)
        {
            glTexImage1D(target, 0, internal_format, width, 0, data_format, data_type, data);
            glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        }
        else if (target == GL_TEXTURE_2D)
        {
            glTexImage2D(target, 0, internal_format, width, height, 0, data_format, data_type, data);
            glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(target, 0);
        if (glGetError() != GL_NO_ERROR)
        {
            glDeleteTextures(1, &color0);
            log_err("Failed to create OpenGL texture object.\n");
            return 0;
        }
        return color0;
    }

    GLuint fbo = 0;
    if (access == FRAKTAL_READ_WRITE)
    {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        if (target == GL_TEXTURE_1D)
            glFramebufferTexture1D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, color_attachment0, 0);
        else if (target == GL_TEXTURE_2D)
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, color_attachment0, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (glGetError() != GL_NO_ERROR)
        {
            glDeleteFramebuffers(1, &fbo);
            glDeleteTextures(1, &color0);
            log_err("Failed to create framebuffer object.\n");
            return NULL;
        }
    }

    fArray *a = (fArray*)calloc(1, sizeof(fArray));
    a->color0 = color0;
    a->fbo = fbo;
    a->width = width;
    a->height = height;
    a->channels = channels;
    a->format = format;
    a->access = access;
    return a;
}

void fraktal_zero_array(fArray *a)
{
    fraktal_assert(a);
    fraktal_assert(a->access == FRAKTAL_READ_WRITE);
    fraktal_assert(a->fbo);
    fraktal_assert(a->color0);
    glBindFramebuffer(GL_FRAMEBUFFER, a->fbo);
    glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void fraktal_gpu_to_cpu(void *cpu_memory, fArray *a)
{
    fraktal_assert(cpu_memory);
    fraktal_assert(a);
    fraktal_assert(a->color0);
    GLenum target = a->height == 0 ? GL_TEXTURE_1D : GL_TEXTURE_2D;
    GLenum internal_format,data_format,data_type;
    fraktal_assert(fraktal_format_to_gl_format(a->channels, a->format));
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glBindTexture(target, a->color0);
    glGetTexImage(target, 0, data_format, data_type, cpu_memory);
    glBindTexture(target, 0);
}
