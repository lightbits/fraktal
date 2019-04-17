#pragma once
#include <stdlib.h>
#include "log.h"

struct fKernel
{
    GLuint program;
};

int fraktal_get_param_offset(fKernel *f, const char *name)
{
    fraktal_check_gl_error();
    fraktal_assert(name);
    fraktal_assert(f);
    fraktal_assert(f->program);
    int location = glGetUniformLocation(f->program, name);
    fraktal_check_gl_error();
    return location;
}

#if 0
void fraktal_kernel_begin(fKernel *f)
{
    fraktal_assert(f);
    glUseProgram(f->program);
    current_kernel = f;
}

int fraktal_get_param_offset(fKernel *f, const char *name)
{
    fraktal_assert(name);
    fraktal_assert(f);
    return glGetUniformLocation(f->program, name);
}

void fraktal_param_1f(int offset, float x)                            { glUniform1f(offset, x); }
void fraktal_param_2f(int offset, float x, float y)                   { glUniform2f(offset, x, y); }
void fraktal_param_3f(int offset, float x, float y, float z)          { glUniform3f(offset, x, y, z); }
void fraktal_param_4f(int offset, float x, float y, float z, float w) { glUniform4f(offset, x, y, z, w); }
void fraktal_param_1i(int offset, float x)                            { glUniform1i(offset, x); }
void fraktal_param_2i(int offset, float x, float y)                   { glUniform2i(offset, x, y); }
void fraktal_param_3i(int offset, float x, float y, float z)          { glUniform3i(offset, x, y, z); }
void fraktal_param_4i(int offset, float x, float y, float z, float w) { glUniform4i(offset, x, y, z, w); }
void fraktal_param_matrix4f(int offset, float m[4*4])                 { glUniformMatrix4fv(offset, 1, false, m); }
void fraktal_param_transpose_matrix4f(int offset, float m[4*4])       { glUniformMatrix4fv(offset, 1, true, m); }

void fraktal_param_array(int offset, int sampler, fArray *a)
{
    if (offset < 0)
        return;
    fraktal_assert(a);
    fraktal_assert(a->color0);
    fraktal_assert(current_kernel);
    fraktal_assert(sampler >= 0 && sampler < 4 && "At most 4 arrays can be active per kernel invocation.");
    glUniform1i(offset, sampler);
    glActiveTexture(GL_TEXTURE0 + sampler);
    if (a->height == 0)
        glBindTexture(GL_TEXTURE_1D, a->color0);
    else
        glBindTexture(GL_TEXTURE_2D, a->color0);
}

void fraktal_kernel_complete(fArray *out, fKernel *f)
{
    fraktal_assert(out);
    fraktal_assert(out->width > 0);
    fraktal_assert(out->height >= 0);
    fraktal_assert(out->fbo);
    fraktal_assert(out->color0);
    fraktal_assert(f);
    fraktal_assert(f->program);
    if (!f->loc_iPosition)
        f->loc_iPosition = glGetAttribLocation(f->program, "iPosition");
    fraktal_assert(f->loc_iPosition >= 0);

    GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLenum last_blend_src_rgb; glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&last_blend_src_rgb);
    GLenum last_blend_dst_rgb; glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&last_blend_dst_rgb);
    GLenum last_blend_src_alpha; glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&last_blend_src_alpha);
    GLenum last_blend_dst_alpha; glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&last_blend_dst_alpha);
    GLenum last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&last_blend_equation_rgb);
    GLenum last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&last_blend_equation_alpha);
    GLboolean last_depth_writemask; glGetBooleanv(GL_DEPTH_WRITEMASK, (GLboolean*)&last_depth_writemask);
    GLenum last_enable_blend = glIsEnabled(GL_BLEND);
    GLenum last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
    GLenum last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
    GLenum last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
    GLenum last_enable_color_logic_op = glIsEnabled(GL_COLOR_LOGIC_OP);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);

    glUseProgram(f->program);

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glBlendEquation(GL_FUNC_ADD);
    glBindFramebuffer(GL_FRAMEBUFFER, out->fbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, scene.quad);
    glEnableVertexAttribArray(f->loc_iPosition);
    glVertexAttribPointer(f->loc_iPosition, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 2, 0);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(loc_iPosition);
    glDeleteVertexArrays(1, &vao);

    // Restore GL state
    glUseProgram(last_program);
    glBindVertexArray(last_vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
    glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
    if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
    glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glActiveTexture(GL_TEXTURE0);
}
#endif
