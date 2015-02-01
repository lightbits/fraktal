#ifndef _util_h_
#define _util_h_
#include <stdio.h>
#include "GL/glew.h"

GLuint
load_program(const char *vs_filename, const char *fs_filename);

GLuint
gen_buffer(GLvoid *data, GLsizei size);

#endif