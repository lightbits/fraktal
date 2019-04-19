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
};
struct guiSceneParams
{
    int2 resolution;
    struct view_t
    {
        angle2 dir;
        float3 pos;
    } view;
    struct camera_t
    {
        float f;
        float2 center;
    } camera;
    struct sun_t
    {
        float size;
        angle2 dir;
        float3 color;
        float intensity;
    } sun;
    struct material_t
    {
        bool glossy;
        float3 albedo;
        float3 specular_albedo;
        float specular_exponent;
    } material;
    struct isolines_t
    {
        bool enabled;
        float3 color;
        float thickness;
        float spacing;
        int count;
    } isolines;
    struct floor_t
    {
        bool reflective;
        float height;
        float specular_exponent;
        float reflectivity;
    } floor;

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
