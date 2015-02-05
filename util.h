#ifndef _util_h_
#define _util_h_
#include <stdio.h>
#include "GL/glew.h"
#include "SDL.h"
#include "SDL_opengl.h"
#define assert(x) SDL_assert(x)
#define STBI_ASSERT(x) assert(x)

GLuint
load_program(const char *vs_filename, const char *fs_filename);

GLuint
gen_buffer(GLvoid *data, GLsizei size);

GLuint 
load_texture(const char *filename,
             GLenum min_filter,
             GLenum mag_filter,
             GLenum wrap_s,
             GLenum wrap_t);

void
save_rgb_to_png(const char *filename, int w, int h, const void *data);

#endif