// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                           FRAKTAL CORE LIBRARY
                            COMPILATION MANUAL
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
This file defines the implementation for the core fraktal library. You can
compile this file to a .dll or .lib or #include "fraktal.cpp" directly in
main if you're a fan of unity builds.

'build.bat' and 'Makefile' in the root show an example of how to build the
fraktal core library and how to link against it.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
§ Compilation flags
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  fraktal can be compiled with several optional flags to enable or disable
certain behaviors. You can provide these to your compiler in your build,
or #define them directly if you do a unity build.

-D FRAKTAL_OMIT_GL_SYMBOLS -> prevents definition of OpenGL symbols
-D FRAKTAL_GUI             -> bakes special symbols into GLSL code
                              used by the GUI.
-D fraktal_assert          -> bring your own assert macro! (BYOA?)


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
§ OpenGL symbols in unity builds
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  If you are compiling fraktal by #including fraktal.cpp directly in your
final translation unit (also known as doing a "unity build"), you can use
    #define FRAKTAL_OMIT_GL_SYMBOLS
to prevent fraktal from defining OpenGL functions and symbols. Naturally,
if you do this you have to make sure GL symbols are defined in your own
code before including this file.
*/

#include "fraktal.h"

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef fraktal_assert
#include <assert.h>
#define fraktal_assert assert
#endif

#ifndef FRAKTAL_OMIT_GL_SYMBOLS
#include <GL/gl3w.h>
#include <GL/gl3w.c>
#endif

#define fraktal_check_gl_error() fraktal_assert(glGetError() == GL_NO_ERROR)

#include "fraktal_context.h"
#include "fraktal_parse.h"
#include "fraktal_array.h"
#include "fraktal_kernel.h"
#include "fraktal_link.h"
