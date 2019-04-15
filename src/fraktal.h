// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

//
// Interface
//
typedef int fraktal_load_flags_t;
enum fraktal_load_flags_
{
    FRAKTAL_LOAD_ALL          = 0,
    FRAKTAL_LOAD_MODEL        = 1,
    FRAKTAL_LOAD_RENDER       = 2,
    FRAKTAL_LOAD_COMPOSE      = 4
};

struct fraktal_scene_def_t
{
    const char *model_shader_path;
    const char *render_shader_path;
    const char *compose_shader_path;
    const char *glsl_version;
};

//
// Implementation
//
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>
#include <stdio.h>
#include <stdlib.h> // malloc, free
#include <string.h>
#include <GL/gl3w.h>
#include <file.h>
#include "scene_params.h"
#include "scene_parser.h"

struct framebuffer_t
{
    GLuint fbo;
    GLuint color0;
    int width;
    int height;
};

struct fraktal_scene_t
{
    fraktal_scene_def_t def;
    framebuffer_t fb_render;
    framebuffer_t fb_compose;
    GLuint program_render;
    GLuint program_compose;
    bool program_render_is_new;
    bool program_compose_is_new;
    int samples;
    bool should_clear;
    bool initialized;
    bool auto_render;
    GLuint quad;

    scene_params_t params;

    struct key_t
    {
        bool pressed;
        bool released;
        bool down;
    };
    struct keys_t
    {
        key_t Space,Enter;
        key_t Ctrl,Alt,Shift;
        key_t Left,Right,Up,Down;
        key_t W,A,S,D;
    } keys;
};

// storage: GL_RGBA32F or GL_RGBA8
framebuffer_t create_framebuffer(GLenum storage, int width, int height)
{
    GLuint color0;
    glGenTextures(1, &color0);
    glBindTexture(GL_TEXTURE_2D, color0);
    glTexImage2D(GL_TEXTURE_2D, 0, storage, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color0, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    framebuffer_t result;
    result.color0 = color0;
    result.fbo = fbo;
    result.width = width;
    result.height = height;
    return result;
}

void clear_framebuffer(framebuffer_t &frame)
{
    glBindFramebuffer(GL_FRAMEBUFFER, frame.fbo);
    glViewport(0, 0, frame.width, frame.height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void compute_view_matrix(float dst[4*4], float3 t, float3 r)
{
    float cx = cosf(r.x);
    float cy = cosf(r.y);
    float cz = cosf(r.z);
    float sx = sinf(r.x);
    float sy = sinf(r.y);
    float sz = sinf(r.z);

    float dtx = t.z*(sx*sz + cx*cz*sy) - t.y*(cx*sz - cz*sx*sy) + t.x*cy*cz;
    float dty = t.y*(cx*cz + sx*sy*sz) - t.z*(cz*sx - cx*sy*sz) + t.x*cy*sz;
    float dtz = t.z*cx*cy              - t.x*sy                 + t.y*cy*sx;

    // T(tx,ty,tz)Rz(rz)Ry(ry)Rx(rx)
    dst[ 0] = cy*cz; dst[ 1] = cz*sx*sy - cx*sz; dst[ 2] = sx*sz + cx*cz*sy; dst[ 3] = dtx;
    dst[ 4] = cy*sz; dst[ 5] = cx*cz + sx*sy*sz; dst[ 6] = cx*sy*sz - cz*sx; dst[ 7] = dty;
    dst[ 8] = -sy;   dst[ 9] = cy*sx;            dst[10] = cx*cy;            dst[11] = dtz;
    dst[12] = 0.0f;  dst[13] = 0.0f;             dst[14] = 0.0f;             dst[15] = 0.0f;
}

#if 0
void save_screenshot(const char *filename)
{
    int w = app.fb_width;
    int h = app.fb_height;
    int n = 4;
    int stride = w*n;
    unsigned char *pixels = (unsigned char*)malloc(w*h*n);
    unsigned char *last_row = pixels + stride*(h-1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    // writing from last row with negative stride flips the image vertically
    stbi_write_png(filename, w, h, n, last_row, -stride);
    free(pixels);
}
#endif

GLuint load_shader(const char *source_identifier, const char **sources, int num_sources, GLenum type)
{
    assert(source_identifier && "Missing source identifier (path or 'built-in')");
    assert(sources && "Missing shader source list");
    assert(num_sources > 0 && "Must have atleast one shader");
    assert((type == GL_VERTEX_SHADER || type == GL_FRAGMENT_SHADER) && "Invalid shader type");
    GLint status = 0;
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, num_sources, (const GLchar **)sources, 0);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        GLint length; glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        char *info = (char*)malloc(length);
        glGetShaderInfoLog(shader, length, NULL, info);
        log_err("Failed to compile shader (%s):\n%s", source_identifier, info);
        free(info);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool program_link_status(GLuint program)
{
    GLint status; glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status)
    {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        char *info = (char*)malloc(length);
        glGetProgramInfoLog(program, length, NULL, info);
        log_err("Failed to link program:\n%s", info);
        free(info);
        return false;
    }
    return true;
}

GLuint load_vertex_shader(fraktal_scene_def_t def)
{
    // We only need to compile this one time
    static GLuint vs = 0;
    if (!vs)
    {
        static const char *source =
            "in vec2 iPosition;\n"
            "void main()\n"
            "{\n"
            "    gl_Position = vec4(iPosition, 0.0, 1.0);\n"
            "}\n"
        ;
        const char *sources[] = { def.glsl_version, "\n#line 0\n", source };
        vs = load_shader("built-in", sources, sizeof(sources)/sizeof(char*), GL_VERTEX_SHADER);
        if (!vs)
            return 0;
    }
    return vs;
}

GLuint load_model_shader(fraktal_scene_def_t def, scene_params_t &params)
{
    static const char *header =
        "uniform vec2      iResolution;\n"           // viewport resolution (in pixels)
        "uniform float     iTime;\n"                 // shader playback time (in seconds)
        "uniform int       iFrame;\n"                // shader playback frame
        "uniform vec2      iChannelResolution[4];\n" // channel resolution (in pixels)
        "uniform vec4      iMouse;\n"                // mouse pixel coords. xy: current (if MLB down), zw: click
        "uniform sampler2D iChannel0;\n"             // file or buffer texture
        "uniform sampler2D iChannel1;\n"             // file or buffer texture
        "uniform sampler2D iChannel2;\n"             // file or buffer texture
        "uniform sampler2D iChannel3;\n"             // file or buffer texture
        "#define MATERIAL0 0.0\n"
        "#define MATERIAL1 1.0\n"
        "#define MATERIAL2 2.0\n"
        "#define MATERIAL3 3.0\n"
        "#define MATERIAL4 4.0\n"
        "#define MATERIAL5 5.0\n"
        "#line 0\n"
    ;

    GLuint fs = 0;
    const char *path = def.model_shader_path;
    char *source = read_file(path);
    if (!scene_file_preprocessor(source, &params))
    {
        log_err("Failed to parse source file %s\n", path);
        return 0;
    }
    if (!source)
    {
        log_err("Failed to read source file %s\n", path);
        return 0;
    }
    const char *sources[] = {
        def.glsl_version, "\n",
        header,
        source
    };
    fs = load_shader(path, sources, sizeof(sources)/sizeof(char*), GL_FRAGMENT_SHADER);
    free(source);
    if (!fs)
        return 0;
    return fs;
}

GLuint load_render_shader(fraktal_scene_def_t def)
{
    static const char *header =
        "uniform vec2      iResolution;\n"           // viewport resolution (in pixels)
        "uniform float     iTime;\n"                 // shader playback time (in seconds)
        "uniform int       iFrame;\n"                // shader playback frame
        "uniform vec2      iChannelResolution[4];\n" // channel resolution (in pixels)
        "uniform vec4      iMouse;\n"                // mouse pixel coords. xy: current (if MLB down), zw: click
        "uniform sampler2D iChannel0;\n"             // file or buffer texture
        "uniform sampler2D iChannel1;\n"             // file or buffer texture
        "uniform sampler2D iChannel2;\n"             // file or buffer texture
        "uniform sampler2D iChannel3;\n"             // file or buffer texture
        "uniform vec2      iCameraCenter;\n"
        "uniform float     iCameraF;\n"
        "uniform mat4      iView;\n"
        "uniform int       iSamples;\n"
        "uniform vec3      iToSun;\n"
        "uniform vec3      iSunStrength;\n"
        "uniform float     iCosSunSize;\n"
        "uniform int       iDrawIsolines;\n"
        "uniform vec3      iIsolineColor;\n"
        "uniform float     iIsolineThickness;\n"
        "uniform float     iIsolineSpacing;\n"
        "uniform float     iIsolineMax;\n"
        "uniform int       iMaterialGlossy;\n"
        "uniform float     iMaterialSpecularExponent;\n"
        "uniform vec3      iMaterialSpecularAlbedo;\n"
        "uniform vec3      iMaterialAlbedo;\n"
        "uniform int       iFloorReflective;\n"
        "uniform float     iFloorHeight;\n"
        "uniform float     iFloorSpecularExponent;\n"
        "uniform float     iFloorReflectivity;\n"
        "out vec4          fragColor;\n"
        "#line 0\n"
    ;

    GLuint fs = 0;
    char *source = read_file(def.render_shader_path);
    const char *sources[] = {
        def.glsl_version, "\n",
        header,
        "#line 0\n",
        source
    };
    fs = load_shader(
        def.render_shader_path,
        sources,
        sizeof(sources)/sizeof(char*),
        GL_FRAGMENT_SHADER
    );
    free(source);
    if (!fs)
        return 0;
    return fs;
}

GLuint load_compose_shader(fraktal_scene_def_t def)
{
    static const char *header =
        "uniform vec2      iResolution;\n"
        "uniform sampler2D iChannel0;\n"
        "uniform int       iSamples;\n"
        "out vec4          fragColor;\n"
        "#line 0\n"
    ;

    GLuint fs = 0;
    const char *path = def.compose_shader_path;
    char *source = read_file(path);
    if (!source)
    {
        log_err("Failed to read source file \"%s\"\n", path);
        return 0;
    }
    const char *sources[] = {
        def.glsl_version, "\n",
        header,
        source
    };
    fs = load_shader(
        path,
        sources,
        sizeof(sources)/sizeof(char*),
        GL_FRAGMENT_SHADER
    );
    if (!fs)
        return 0;
    return fs;
}

GLuint load_compose_program(fraktal_scene_def_t def)
{
    assert(def.glsl_version && "GLSL version string is NULL");
    assert(def.compose_shader_path && "Compose shader path is NULL");
    GLuint vs = load_vertex_shader(def); if (!vs) return 0;
    GLuint fs = load_compose_shader(def); if (!fs) return 0;

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDetachShader(program, vs);
    glDetachShader(program, fs);
    glDeleteShader(fs);

    if (!program_link_status(program))
    {
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

GLuint load_render_program(fraktal_scene_def_t def, scene_params_t &params)
{
    assert(def.glsl_version && "GLSL version string is NULL");
    assert(def.model_shader_path && "Model shader path is NULL");
    assert(def.render_shader_path && "Render shader path is NULL");

    GLuint vs = load_vertex_shader(def); if (!vs) return 0;
    GLuint fs1 = load_model_shader(def, params); if (!fs1) return 0;
    GLuint fs2 = load_render_shader(def);
    if (!fs2)
    {
        glDeleteShader(fs1);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs1);
    glAttachShader(program, fs2);
    glLinkProgram(program);
    glDetachShader(program, vs);
    glDetachShader(program, fs1);
    glDetachShader(program, fs2);
    glDeleteShader(fs1);
    glDeleteShader(fs2);

    if (!program_link_status(program))
    {
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

void fraktal_set_resolution(fraktal_scene_t &scene, int x, int y)
{
    assert(x > 0);
    assert(y > 0);

    glDeleteTextures(1, &scene.fb_render.color0);
    glDeleteTextures(1, &scene.fb_compose.color0);
    glDeleteFramebuffers(1, &scene.fb_render.fbo);
    glDeleteFramebuffers(1, &scene.fb_compose.fbo);

    scene.params.resolution.x = x;
    scene.params.resolution.y = y;
    scene.fb_render = create_framebuffer(GL_RGBA32F, x, y);
    scene.fb_compose = create_framebuffer(GL_RGBA8, x, y);
    scene.should_clear = true;
}

bool fraktal_load(fraktal_scene_t &scene,
                  fraktal_scene_def_t def,
                  fraktal_load_flags_t flags)
{
    if (!scene.initialized)
        scene.params = get_default_scene_params();

    if (!scene.quad)
    {
        static const float quad_data[] = { -1,-1, +1,-1, +1,+1, +1,+1, -1,+1, -1,-1 };
        glGenBuffers(1, &scene.quad);
        glBindBuffer(GL_ARRAY_BUFFER, scene.quad);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad_data), quad_data, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    assert(scene.quad && "Failed to create fullscreen-quad buffer");

    if (flags == 0)
        flags = 0xffffffff;

    GLuint compose = scene.program_compose;
    if (flags & FRAKTAL_LOAD_COMPOSE)
        compose = load_compose_program(def);

    GLuint render = scene.program_render;
    scene_params_t params = scene.params;
    if (flags & FRAKTAL_LOAD_RENDER)
        render = load_render_program(def, params);

    if (!compose || !render)
    {
        log_err("Failed to load shader pipeline\n");
        return false;
    }

    if (flags & FRAKTAL_LOAD_COMPOSE)
    {
        glDeleteProgram(scene.program_compose);
        scene.program_compose = compose;
        scene.program_compose_is_new = true;
    }

    if (flags & FRAKTAL_LOAD_RENDER)
    {
        glDeleteProgram(scene.program_render);
        if (!scene.initialized ||
            params.resolution.x != scene.params.resolution.x ||
            params.resolution.y != scene.params.resolution.y)
            fraktal_set_resolution(scene, params.resolution.x, params.resolution.y);
        scene.params = params;
        scene.program_render = render;
        scene.program_render_is_new = true;
        scene.should_clear = true;
    }

    scene.def = def;
    scene.initialized = true;

    return true;
}

#define fetch_attrib(program, name)  static GLint loc_##name; if (scene.program##_is_new) loc_##name = glGetAttribLocation(scene.program, #name);
#define fetch_uniform(program, name) static GLint loc_##name; if (scene.program##_is_new) loc_##name = glGetUniformLocation(scene.program, #name);

void fraktal_render(fraktal_scene_t &scene)
{
    assert(scene.fb_render.fbo);
    assert(scene.fb_render.color0);
    assert(scene.fb_render.width > 0);
    assert(scene.fb_render.height > 0);
    assert(scene.fb_compose.fbo);
    assert(scene.fb_compose.color0);
    assert(scene.fb_compose.width > 0);
    assert(scene.fb_compose.height > 0);
    if (scene.should_clear)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, scene.fb_render.fbo);
        glViewport(0, 0, scene.fb_render.width, scene.fb_render.height);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        scene.samples = 0;
        scene.should_clear = false;
    }

    fetch_attrib (program_render, iPosition);
    fetch_uniform(program_render, iResolution);
    // fetch_uniform(program_render, iChannelResolution);
    // fetch_uniform(program_render, iTime);
    // fetch_uniform(program_render, iTimeDelta);
    // fetch_uniform(program_render, iFrame);
    // fetch_uniform(program_render, iMouse);
    // fetch_uniform(program_render, iChannel0);
    // fetch_uniform(program_render, iChannel1);
    // fetch_uniform(program_render, iChannel2);
    // fetch_uniform(program_render, iChannel3);
    fetch_uniform(program_render, iCameraCenter);
    fetch_uniform(program_render, iCameraF);
    fetch_uniform(program_render, iSamples);
    fetch_uniform(program_render, iToSun);
    fetch_uniform(program_render, iSunStrength);
    fetch_uniform(program_render, iCosSunSize);
    fetch_uniform(program_render, iDrawIsolines);
    fetch_uniform(program_render, iIsolineColor);
    fetch_uniform(program_render, iIsolineThickness);
    fetch_uniform(program_render, iIsolineSpacing);
    fetch_uniform(program_render, iIsolineMax);
    fetch_uniform(program_render, iMaterialGlossy);
    fetch_uniform(program_render, iMaterialSpecularExponent);
    fetch_uniform(program_render, iMaterialSpecularRoughness);
    fetch_uniform(program_render, iMaterialSpecularAlbedo);
    fetch_uniform(program_render, iMaterialAlbedo);
    fetch_uniform(program_render, iFloorReflective);
    fetch_uniform(program_render, iFloorHeight);
    fetch_uniform(program_render, iFloorSpecularExponent);
    fetch_uniform(program_render, iFloorReflectivity);
    fetch_uniform(program_render, iView);
    scene.program_render_is_new = false;

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glBlendEquation(GL_FUNC_ADD);

    framebuffer_t fb = scene.fb_render;
    glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);
    glViewport(0, 0, fb.width, fb.height);

    glUseProgram(scene.program_render);

    float iView[4*4];
    {
        float3 r = {
            deg2rad(scene.params.view.dir.theta),
            deg2rad(scene.params.view.dir.phi),
            0.0f
        };
        compute_view_matrix(iView, scene.params.view.pos, r);
    }

    glUniform2f(loc_iResolution, (float)fb.width, (float)fb.height);
    glUniform2fv(loc_iCameraCenter, 1, &scene.params.camera.center.x);
    glUniform1f(loc_iCameraF, scene.params.camera.f);
    glUniform1i(loc_iSamples, scene.samples);

    {
        auto sun = scene.params.sun;
        float3 iSunStrength = sun.color;
        iSunStrength.x *= sun.intensity;
        iSunStrength.y *= sun.intensity;
        iSunStrength.z *= sun.intensity;
        float3 iToSun = angle2float3(sun.dir);
        float iCosSunSize = cosf(deg2rad(sun.size));
        glUniform3fv(loc_iSunStrength, 1, &iSunStrength.x);
        glUniform3fv(loc_iToSun, 1, &iToSun.x);
        glUniform1f(loc_iCosSunSize, iCosSunSize);
    }

    {
        auto iso = scene.params.isolines;
        glUniform1i(loc_iDrawIsolines, iso.enabled ? 1 : 0);
        glUniform3fv(loc_iIsolineColor, 1, &iso.color.x);
        glUniform1f(loc_iIsolineThickness, iso.thickness);
        glUniform1f(loc_iIsolineSpacing, iso.spacing);
        glUniform1f(loc_iIsolineMax, iso.count*iso.spacing + iso.thickness*0.5f);
    }

    {
        auto material = scene.params.material;
        glUniform1i(loc_iMaterialGlossy, material.glossy ? 1 : 0);
        glUniform1f(loc_iMaterialSpecularExponent, material.specular_exponent);
        glUniform3fv(loc_iMaterialSpecularAlbedo, 1, &material.specular_albedo.x);
        glUniform3fv(loc_iMaterialAlbedo, 1, &material.albedo.x);
    }

    {
        auto floor = scene.params.floor;
        glUniform1i(loc_iFloorReflective, floor.reflective ? 1 : 0);
        glUniform1f(loc_iFloorHeight, floor.height);
        glUniform1f(loc_iFloorSpecularExponent, floor.specular_exponent);
        glUniform1f(loc_iFloorReflectivity, floor.reflectivity);
    }

    glUniformMatrix4fv(loc_iView, 1, GL_TRUE, iView);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, scene.quad);
    glEnableVertexAttribArray(loc_iPosition);
    glVertexAttribPointer(loc_iPosition, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 2, 0);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(loc_iPosition);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDisable(GL_BLEND);
    glDeleteVertexArrays(1, &vao);

    scene.samples++;
}

void fraktal_compose(fraktal_scene_t &scene)
{
    fetch_attrib (program_compose, iPosition);
    fetch_uniform(program_compose, iResolution);
    fetch_uniform(program_compose, iChannel0);
    fetch_uniform(program_compose, iSamples);
    scene.program_compose_is_new = false;

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

    framebuffer_t fb = scene.fb_compose;
    glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);

    glViewport(0, 0, fb.width, fb.height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(scene.program_compose);

    glUniform2f(loc_iResolution, (float)fb.width, (float)fb.height);
    glUniform1i(loc_iSamples, scene.samples);
    glUniform1i(loc_iChannel0, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, scene.fb_render.color0);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, scene.quad);
    glEnableVertexAttribArray(loc_iPosition);
    glVertexAttribPointer(loc_iPosition, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 2, 0);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(loc_iPosition);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDisable(GL_BLEND);
    glDeleteVertexArrays(1, &vao);
}

bool handle_view_change_keys(fraktal_scene_t &scene)
{
    bool moved = false;

    // rotation
    int rotate_step = 5;
    angle2 &dir = scene.params.view.dir;
    if (scene.keys.Left.pressed)  { dir.phi   -= rotate_step; moved = true; }
    if (scene.keys.Right.pressed) { dir.phi   += rotate_step; moved = true; }
    if (scene.keys.Up.pressed)    { dir.theta -= rotate_step; moved = true; }
    if (scene.keys.Down.pressed)  { dir.theta += rotate_step; moved = true; }

    // translation
    // Note: The z_over_f factor ensures that a key press yields the same
    // displacement of the object in image pixels, irregardless of how far
    // away the camera is.
    float3 &pos = scene.params.view.pos;
    float z_over_f = fabsf(pos.z)/scene.params.camera.f;
    float x_move_step = (scene.params.resolution.x*0.05f)*z_over_f;
    float y_move_step = (scene.params.resolution.y*0.05f)*z_over_f;
    float z_move_step = 0.1f*fabsf(scene.params.view.pos.z);
    if (scene.keys.Ctrl.pressed)  { pos.y -= y_move_step; moved = true; }
    if (scene.keys.Space.pressed) { pos.y += y_move_step; moved = true; }
    if (scene.keys.A.pressed)     { pos.x -= x_move_step; moved = true; }
    if (scene.keys.D.pressed)     { pos.x += x_move_step; moved = true; }
    if (scene.keys.W.pressed)     { pos.z -= z_move_step; moved = true; }
    if (scene.keys.S.pressed)     { pos.z += z_move_step; moved = true; }
    return moved;
}

void fraktal_present(fraktal_scene_t &scene)
{
    if (scene.keys.Shift.down && scene.keys.Enter.pressed)
        fraktal_load(scene, scene.def, FRAKTAL_LOAD_RENDER|FRAKTAL_LOAD_COMPOSE);

    if (handle_view_change_keys(scene))
        scene.should_clear = true;

    if (!scene.keys.Shift.down && scene.keys.Enter.pressed)
        scene.auto_render = !scene.auto_render;

    if (scene.auto_render || scene.program_render_is_new || scene.should_clear)
    {
        fraktal_render(scene);
        fraktal_compose(scene);
    }

    float pad = 2.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.325f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 1.0f, 1.0f, 0.078f));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.0f, 1.0f, 1.0f, 0.325f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.325f));
    // blue tabs
    // ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(1.0f, 1.0f, 1.0f, 0.114f));
    // ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.329f, 0.478f, 0.71f, 1.0f));
    // gray tabs
    ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.22f, 0.22f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.28f, 0.28f, 0.28f, 1.0f));

    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1.0f, 1.0f, 1.0f, 0.325f));

    // main menu bar
    float main_menu_bar_height = 0.0f;
    int preview_mode_render = 0;
    int preview_mode_mesh = 1;
    int preview_mode_point_cloud = 2;
    int preview_mode = preview_mode_render;
    {
        ImGui::BeginMainMenuBar();
        {
            main_menu_bar_height = ImGui::GetWindowHeight();
            ImGui::Text("\xce\xb8"); // placeholder for Fraktal icon
            ImGui::MenuItem("File");
            ImGui::MenuItem("Window");
            ImGui::MenuItem("Help");
            if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None))
            {
                if (ImGui::BeginTabItem("Render"))
                {
                    preview_mode = preview_mode_render;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Mesh"))
                {
                    preview_mode = preview_mode_mesh;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Point cloud"))
                {
                    preview_mode = preview_mode_point_cloud;
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::EndMainMenuBar();
    }

    // side panel
    float side_panel_width = 0.0f;
    float side_panel_height = 0.0f;
    {
        ImGuiIO &io = ImGui::GetIO();
        float avail_y = io.DisplaySize.y - main_menu_bar_height - pad;
        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.22f, 0.22f, 0.22f, 1.0f));
        ImGui::SetNextWindowSize(ImVec2(0.3f*io.DisplaySize.x, avail_y), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, avail_y), ImVec2(io.DisplaySize.x, avail_y));
        ImGui::SetNextWindowPos(ImVec2(0.0f, main_menu_bar_height + pad));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f,8.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 12.0f);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.0f,0.0f,0.0f,0.0f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(1.0f,1.0f,1.0f,0.25f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(1.0f,1.0f,1.0f,0.4f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(1.0f,1.0f,1.0f,0.55f));
        ImGui::Begin("Sidepanel", NULL, flags);
        side_panel_width = ImGui::GetWindowWidth();
        side_panel_height = ImGui::GetWindowHeight();

        if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Scene"))
            {
                ImGui::BeginChild("Scene##child");
                if (ImGui::CollapsingHeader("Resolution"))
                {
                    ImGui::InputInt("x##resolution", &scene.params.resolution.x);
                    ImGui::InputInt("y##resolution", &scene.params.resolution.y);
                }
                if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    float3 &pos = scene.params.view.pos;
                    float z_over_f = fabsf(pos.z)/scene.params.camera.f;
                    float3 drag_speeds;
                    drag_speeds.x = (scene.params.resolution.x*0.005f)*z_over_f;
                    drag_speeds.y = (scene.params.resolution.y*0.005f)*z_over_f;
                    drag_speeds.z = 0.01f*fabsf(scene.params.view.pos.z);

                    scene.should_clear |= ImGui::SliderFloat("\xce\xb8##view_dir", &scene.params.view.dir.theta, -90.0f, +90.0f, "%.0f deg");
                    scene.should_clear |= ImGui::SliderFloat("\xcf\x86##view_dir", &scene.params.view.dir.phi, -180.0f, +180.0f, "%.0f deg");
                    scene.should_clear |= ImGui::DragFloat3("pos##view_pos", &pos.x, &drag_speeds.x);
                    scene.should_clear |= ImGui::DragFloat("f##camera_f", &scene.params.camera.f);
                    scene.should_clear |= ImGui::DragFloat2("center##camera_center", &scene.params.camera.center.x);
                }
                if (ImGui::CollapsingHeader("Sun"))
                {
                    scene.should_clear |= ImGui::SliderFloat("size##sun_size", &scene.params.sun.size, 0.0f, 180.0f, "%.0f deg");
                    scene.should_clear |= ImGui::SliderFloat("\xce\xb8##sun_dir", &scene.params.sun.dir.theta, -90.0f, +90.0f, "%.0f deg");
                    scene.should_clear |= ImGui::SliderFloat("\xcf\x86##sun_dir", &scene.params.sun.dir.phi, -180.0f, +180.0f, "%.0f deg");
                    scene.should_clear |= ImGui::SliderFloat3("color##sun_color", &scene.params.sun.color.x, 0.0f, 1.0f);
                    scene.should_clear |= ImGui::DragFloat("intensity##sun_intensity", &scene.params.sun.intensity);
                }
                if (ImGui::CollapsingHeader("Floor"))
                {
                    auto &floor = scene.params.floor;
                    scene.should_clear |= ImGui::DragFloat("Height", &floor.height, 0.01f);

                    if (ImGui::TreeNode("Isolines"))
                    {
                        auto &isolines = scene.params.isolines;
                        scene.should_clear |= ImGui::Checkbox("Enabled##Isolines", &isolines.enabled);
                        scene.should_clear |= ImGui::ColorEdit3("Color", &isolines.color.x);
                        scene.should_clear |= ImGui::DragFloat("Thickness", &isolines.thickness, 0.01f);
                        scene.should_clear |= ImGui::DragFloat("Spacing", &isolines.spacing, 0.01f);
                        scene.should_clear |= ImGui::DragInt("Count", &isolines.count, 0.1f, 0, 100);
                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNode("Reflection"))
                    {
                        scene.should_clear |= ImGui::Checkbox("Enabled##Reflection", &floor.reflective);
                        scene.should_clear |= ImGui::DragFloat("Exponent", &floor.specular_exponent, 1.0f, 0.0f, 10000.0f);
                        scene.should_clear |= ImGui::SliderFloat("Reflectivity", &floor.reflectivity, 0.0f, 1.0f);
                        ImGui::TreePop();
                    }
                }
                if (ImGui::CollapsingHeader("Material"))
                {
                    auto &material = scene.params.material;
                    scene.should_clear |= ImGui::Checkbox("Glossy", &material.glossy);
                    scene.should_clear |= ImGui::ColorEdit3("Albedo", &material.albedo.x);
                    scene.should_clear |= ImGui::ColorEdit3("Specular", &material.specular_albedo.x);
                    scene.should_clear |= ImGui::DragFloat("Exponent", &material.specular_exponent, 1.0f, 0.0f, 1000.0f);
                }

                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Log"))
            {
                ImGui::BeginChild("Log##child");
                ImGui::TextWrapped(log_get_buffer());
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
    }

    // main preview panel
    {
        ImGuiIO &io = ImGui::GetIO();
        float height = side_panel_height;
        float width = io.DisplaySize.x - (side_panel_width + pad);

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_MenuBar;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.14f, 0.14f, 0.14f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f,0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::SetNextWindowSize(ImVec2(width, height));
        ImGui::SetNextWindowPos(ImVec2(side_panel_width + pad, main_menu_bar_height + pad));
        ImGui::Begin("Preview", NULL, flags);

        if (preview_mode == preview_mode_render)
        {
            const int display_mode_1x = 0;
            const int display_mode_2x = 1;
            const int display_mode_fit = 2;
            static int display_mode = display_mode_1x;

            ImGui::BeginMenuBar();
            {
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f,4.0f));
                if (ImGui::BeginMenu("Scale"))
                {
                    if (ImGui::MenuItem("1x", NULL, display_mode==display_mode_1x)) { display_mode = display_mode_1x; }
                    if (ImGui::MenuItem("2x", NULL, display_mode==display_mode_2x)) { display_mode = display_mode_2x; }
                    if (ImGui::MenuItem("Scale to fit", NULL, display_mode==display_mode_fit)) { display_mode = display_mode_fit; }
                    ImGui::EndMenu();
                }
                ImGui::PopStyleVar();
                ImGui::Separator();
                ImGui::Text("Samples: %d", scene.samples);
            }
            ImGui::EndMenuBar();

            ImDrawList *draw = ImGui::GetWindowDrawList();
            {
                ImVec2 image_size = ImVec2(
                    (float)scene.fb_compose.width,
                    (float)scene.fb_compose.height);
                if (io.DisplayFramebufferScale.x > 0.0f &&
                    io.DisplayFramebufferScale.y > 0.0f)
                {
                    image_size.x /= io.DisplayFramebufferScale.x;
                    image_size.y /= io.DisplayFramebufferScale.y;
                }
                ImVec2 avail = ImGui::GetContentRegionAvail();

                ImVec2 top_left = ImGui::GetCursorScreenPos();
                ImVec2 pos0,pos1;
                if (display_mode == display_mode_1x)
                {
                    pos0.x = (float)(int)(top_left.x + avail.x*0.5f - image_size.x*0.5f);
                    pos0.y = (float)(int)(top_left.y + avail.y*0.5f - image_size.y*0.5f);
                    pos1.x = (float)(int)(top_left.x + avail.x*0.5f + image_size.x*0.5f);
                    pos1.y = (float)(int)(top_left.y + avail.y*0.5f + image_size.y*0.5f);
                }
                else if (display_mode == display_mode_2x)
                {
                    pos0.x = (float)(int)(top_left.x + avail.x*0.5f - image_size.x);
                    pos0.y = (float)(int)(top_left.y + avail.y*0.5f - image_size.y);
                    pos1.x = (float)(int)(top_left.x + avail.x*0.5f + image_size.x);
                    pos1.y = (float)(int)(top_left.y + avail.y*0.5f + image_size.y);
                }
                else if (avail.x < avail.y)
                {
                    float h = image_size.y*avail.x/image_size.x;
                    pos0.x = (float)(int)(top_left.x);
                    pos0.y = (float)(int)(top_left.y + avail.y*0.5f - h*0.5f);
                    pos1.x = (float)(int)(top_left.x + avail.x);
                    pos1.y = (float)(int)(top_left.y + avail.y*0.5f + h*0.5f);
                }
                else if (avail.y < avail.x)
                {
                    float w = image_size.x*avail.y/image_size.y;
                    pos0.x = (float)(int)(top_left.x + avail.x*0.5f - w*0.5f);
                    pos0.y = (float)(int)(top_left.y);
                    pos1.x = (float)(int)(top_left.x + avail.x*0.5f + w*0.5f);
                    pos1.y = (float)(int)(top_left.y + avail.y);
                }
                ImU32 tint = 0xFFFFFFFF;
                ImVec2 uv0 = ImVec2(0.0f,0.0f);
                ImVec2 uv1 = ImVec2(1.0f,1.0f);
                draw->AddImage((void*)(intptr_t)scene.fb_compose.color0,
                               pos0, pos1, uv0, uv1, tint);
            }
        }

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
}
