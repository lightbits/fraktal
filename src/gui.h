#pragma once

struct Widget;

struct gui_state_t
{
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

    enum { MAX_WIDGETS = 1024 };
    Widget *widgets[MAX_WIDGETS];
};

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
