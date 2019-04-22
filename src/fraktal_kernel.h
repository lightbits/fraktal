#pragma once
#include <stdlib.h>
#include <log.h>

struct fKernel
{
    GLuint program;
    int loc_iPosition;
    fParams params;
};

static fKernel *fraktal_current_kernel = NULL;

int fraktal_get_param_offset(fKernel *f, const char *name)
{
    fraktal_assert(name);
    fraktal_assert(f);
    fraktal_assert(f->program);
    fraktal_ensure_context();
    fraktal_check_gl_error();
    int location = glGetUniformLocation(f->program, name);
    fraktal_check_gl_error();
    return location;
}

void fraktal_use_kernel(fKernel *f)
{
    fraktal_ensure_context();
    fraktal_check_gl_error();
    static GLint last_program;
    static GLint last_array_buffer;
    static GLint last_vertex_array;
    static GLint last_viewport[4];
    static GLint last_scissor_box[4];
    static GLint last_framebuffer;
    static GLenum last_blend_src_rgb;
    static GLenum last_blend_dst_rgb;
    static GLenum last_blend_src_alpha;
    static GLenum last_blend_dst_alpha;
    static GLenum last_blend_equation_rgb;
    static GLenum last_blend_equation_alpha;
    static GLboolean last_depth_writemask;
    static GLenum last_enable_blend;
    static GLenum last_enable_cull_face;
    static GLenum last_enable_depth_test;
    static GLenum last_enable_scissor_test;
    static GLenum last_enable_color_logic_op;
    static GLuint vao = 0;

    static GLuint quad = 0;
    if (!quad)
    {
        static const float data[] = { -1,-1, +1,-1, +1,+1, +1,+1, -1,+1, -1,-1 };
        glGenBuffers(1, &quad);
        glBindBuffer(GL_ARRAY_BUFFER, quad);
        glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    fraktal_assert(quad && "Failed to create vertex buffer");

    if (f)
    {
        fraktal_assert(glIsProgram(f->program) && "f must be a valid kernel object");
    }

    if (fraktal_current_kernel)
    {
        if (f)
        {
            fraktal_current_kernel = f;
            glUseProgram(f->program);
            if (!f->loc_iPosition)
                f->loc_iPosition = glGetAttribLocation(f->program, "iPosition");
            fraktal_assert(f->loc_iPosition >= 0);
            glEnableVertexAttribArray(f->loc_iPosition);
            glVertexAttribPointer(f->loc_iPosition, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 2, 0);
        }
        else
        {
            glDisableVertexAttribArray(fraktal_current_kernel->loc_iPosition);
            glDeleteVertexArrays(1, &vao);
            fraktal_current_kernel = NULL;

            // Restore GL state
            glUseProgram(last_program);
            glBindVertexArray(last_vertex_array);
            glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
            glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
            glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
            glBindFramebuffer(GL_FRAMEBUFFER, last_framebuffer);
            if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
            if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
            if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
            if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
            if (last_enable_color_logic_op) glEnable(GL_COLOR_LOGIC_OP); else glDisable(GL_COLOR_LOGIC_OP);
            glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
            glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
            glActiveTexture(GL_TEXTURE0);
        }
    }
    else
    {
        if (f)
        {
            // Back-up GL state
            glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
            glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
            glGetIntegerv(GL_VIEWPORT, last_viewport);
            glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &last_framebuffer);
            glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&last_blend_src_rgb);
            glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&last_blend_dst_rgb);
            glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&last_blend_src_alpha);
            glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&last_blend_dst_alpha);
            glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&last_blend_equation_rgb);
            glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&last_blend_equation_alpha);
            glGetBooleanv(GL_DEPTH_WRITEMASK, (GLboolean*)&last_depth_writemask);
            last_enable_blend = glIsEnabled(GL_BLEND);
            last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
            last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
            last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
            last_enable_color_logic_op = glIsEnabled(GL_COLOR_LOGIC_OP);

            fraktal_current_kernel = f;
            glDisable(GL_CULL_FACE);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_SCISSOR_TEST);
            glDisable(GL_COLOR_LOGIC_OP);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            glBlendEquation(GL_FUNC_ADD);
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, quad);

            glUseProgram(f->program);
            if (!f->loc_iPosition)
                f->loc_iPosition = glGetAttribLocation(f->program, "iPosition");
            fraktal_assert(f->loc_iPosition >= 0);
            glEnableVertexAttribArray(f->loc_iPosition);
            glVertexAttribPointer(f->loc_iPosition, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 2, 0);
        }
        else
        {
            // do nothing
        }
    }
    fraktal_check_gl_error();
}

void fraktal_param_1f(int offset, float x)                            { fraktal_assert(fraktal_current_kernel); if (offset < 0) return; glUniform1f(offset, x); }
void fraktal_param_2f(int offset, float x, float y)                   { fraktal_assert(fraktal_current_kernel); if (offset < 0) return; glUniform2f(offset, x, y); }
void fraktal_param_3f(int offset, float x, float y, float z)          { fraktal_assert(fraktal_current_kernel); if (offset < 0) return; glUniform3f(offset, x, y, z); }
void fraktal_param_4f(int offset, float x, float y, float z, float w) { fraktal_assert(fraktal_current_kernel); if (offset < 0) return; glUniform4f(offset, x, y, z, w); }
void fraktal_param_1i(int offset, int x)                              { fraktal_assert(fraktal_current_kernel); if (offset < 0) return; glUniform1i(offset, x); }
void fraktal_param_2i(int offset, int x, int y)                       { fraktal_assert(fraktal_current_kernel); if (offset < 0) return; glUniform2i(offset, x, y); }
void fraktal_param_3i(int offset, int x, int y, int z)                { fraktal_assert(fraktal_current_kernel); if (offset < 0) return; glUniform3i(offset, x, y, z); }
void fraktal_param_4i(int offset, int x, int y, int z, int w)         { fraktal_assert(fraktal_current_kernel); if (offset < 0) return; glUniform4i(offset, x, y, z, w); }
void fraktal_param_matrix4f(int offset, float m[4*4])                 { fraktal_assert(fraktal_current_kernel); if (offset < 0) return; glUniformMatrix4fv(offset, 1, false, m); }
void fraktal_param_transpose_matrix4f(int offset, float m[4*4])       { fraktal_assert(fraktal_current_kernel); if (offset < 0) return; glUniformMatrix4fv(offset, 1, true, m); }

void fraktal_param_array(int offset, int tex_unit, fArray *a)
{
    fraktal_assert(a);
    fraktal_assert(a->color0);
    fraktal_assert(a->width > 0 && a->height > 0 && "Array has invalid dimensions.");
    fraktal_assert(fraktal_current_kernel);
    fraktal_assert(tex_unit >= 0 && tex_unit < 4 && "At most 4 arrays can be active per kernel invocation.");
    if (offset < 0)
        return;
    glUniform1i(offset, tex_unit);
    glActiveTexture(GL_TEXTURE0 + tex_unit);
    if (a->height == 1)
        glBindTexture(GL_TEXTURE_1D, a->color0);
    else
        glBindTexture(GL_TEXTURE_2D, a->color0);
}

void fraktal_run_kernel(fArray *out)
{
    fraktal_assert(fraktal_current_kernel && "Call fraktal_use_kernel first.");
    fraktal_assert(out);
    fraktal_assert(out->width > 0);
    fraktal_assert(out->height > 0);
    fraktal_assert(out->fbo && "The output array's access mode cannot be read-only.");
    fraktal_assert(out->color0);
    fraktal_ensure_context();
    fraktal_check_gl_error();

    glBindFramebuffer(GL_FRAMEBUFFER, out->fbo);
    if (out->height == 0)
        glViewport(0, 0, out->width, 1);
    else
        glViewport(0, 0, out->width, out->height);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    fraktal_check_gl_error();
}
