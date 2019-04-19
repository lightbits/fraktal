// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

#define fraktal_assert assert
#define fraktal_check_gl_error() fraktal_assert(glGetError() == GL_NO_ERROR)
#include <GL/gl3w.h>
#include "fraktal.h"
#include "fraktal_parse.h"
#include "fraktal_array.h"
#include "fraktal_kernel.h"
#include "fraktal_link.h"
