#ifndef _util_h_
#define _util_h_
#include <stdio.h>
#include "GL/glew.h"

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

#endif