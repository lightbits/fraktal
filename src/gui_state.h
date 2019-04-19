#pragma once

enum { NUM_MATERIALS = 5 };
enum { MAX_WIDGETS = 1024 };
struct Widget;
struct guiKey
{
    bool pressed;
    bool released;
    bool down;
};
struct guiKeys
{
    guiKey Space,Enter;
    guiKey Ctrl,Alt,Shift;
    guiKey Left,Right,Up,Down;
    guiKey W,A,S,D,P;
    guiKey PrintScreen;
};
struct guiSceneDef
{
    const char *model_kernel_path;
    const char *color_kernel_path;
    const char *geometry_kernel_path;
    const char *compose_kernel_path;
    const char *glsl_version;
    int resolution_x;
    int resolution_y;
};
struct guiSceneParams
{
    int2 resolution;
    Widget *widgets[MAX_WIDGETS];
    int num_widgets;
};
typedef int guiPreviewMode;
enum guiPreviewMode_ {
    guiPreviewMode_Color=0,
    guiPreviewMode_Thickness,
    guiPreviewMode_Normals,
    guiPreviewMode_Depth,
    guiPreviewMode_GBuffer,
};
struct guiState
{
    guiSceneDef def;
    fArray *render_buffer;
    fArray *compose_buffer;
    fKernel *render_kernel;
    fKernel *compose_kernel;
    bool render_kernel_is_new;
    bool compose_kernel_is_new;
    int samples;
    bool should_clear;
    bool initialized;
    bool auto_render;

    guiKeys keys;

    guiSceneParams params;

    guiPreviewMode mode;
};
