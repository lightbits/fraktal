#include "util.h"

#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "libs/stb_image_write.h"

void
save_rgb_to_png(const char *filename, int w, int h, const void *data)
{
    stbi_write_png(filename, w, h, 3, data, w * 3);
}

GLuint 
load_texture(const char *filename,
             GLenum min_filter,
             GLenum mag_filter,
             GLenum wrap_s,
             GLenum wrap_t)
{
    int width, height, channels;
    uint8 *pixels = stbi_load(filename, &width, &height, &channels, 4);

    if (pixels == NULL)
    {
        printf("Failed to load texture: %s", stbi_failure_reason());
        return 0;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, 
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(pixels);
    return texture;
}

GLuint
gen_buffer(GLvoid *data, GLsizei size)
{
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return buffer;
}

GLuint 
load_shader(const char *filename, GLenum type)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        printf("Failed open file %s\n", filename);
        return 0;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    char *src = new char[size];
    fseek(file, 0, SEEK_SET);
    fread(src, size, 1, file);
    fclose(file);

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, (const GLchar **)&src, (const GLint *)&size);
    delete[] src;

    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        GLint log_length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        char *info_log = new char[log_length];
        glGetShaderInfoLog(shader, log_length, NULL, info_log);
        printf("Failed to compile %s\n%s", filename, info_log);
        delete[] info_log;
        return 0;
    }
    else
    {
        return shader;
    }
}

GLuint 
make_program(GLuint shaders[], GLenum types[], int count)
{
    GLuint program = glCreateProgram();
    for (int i = 0; i < count; i++)
        glAttachShader(program, shaders[i]);

    glLinkProgram(program);

    for (int i = 0; i < count; i++)
    {
        glDetachShader(program, shaders[i]);
        glDeleteShader(shaders[i]);
    }

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        char *info = new char[length];
        glGetProgramInfoLog(program, length, NULL, info);
        printf("Failed to link program \n%s", info);
        delete[] info;
        return 0;
    }
    else
    {
        return program;
    }
}

GLuint
load_program(const char *vs_filename, const char *fs_filename)
{
    GLenum types[2] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
    GLuint shaders[2];
    shaders[0] = load_shader(vs_filename, GL_VERTEX_SHADER);
    shaders[1] = load_shader(fs_filename, GL_FRAGMENT_SHADER);
    if (!shaders[0] || !shaders[1])
        return 0;
    return make_program(shaders, types, 2);
}