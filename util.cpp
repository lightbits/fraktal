#include "util.h"

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