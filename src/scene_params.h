// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

#pragma once
enum { NUM_MATERIALS = 5 };
typedef float angle;
struct int2 { int x,y; };
struct int3 { int x,y,z; };
struct angle2 { angle theta,phi; };
struct float2 { float x,y; };
struct float3 { float x,y,z; };

float deg2rad(float deg) { return (3.14159265358979f/180.0f)*deg; }
float yfov2pinhole_f(float yfov, float resolution_y)
{
    return (resolution_y/2.0f) / tanf(deg2rad(yfov)/2.0f);
}

float3 angle2float3(angle2 dir)
{
    float3 w =
    {
        sinf(deg2rad(dir.theta))*cosf(deg2rad(dir.phi)),
        cosf(deg2rad(dir.theta)),
        sinf(deg2rad(dir.theta))*sinf(deg2rad(dir.phi)),
    };
    return w;
}

struct scene_params_t
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
        bool active;
        float roughness;
        float3 albedo;
    } material[NUM_MATERIALS];
};

scene_params_t get_default_scene_params()
{
    scene_params_t params = {0};
    params.resolution.x = 200;
    params.resolution.y = 200;
    params.view.dir.theta = -20.0f;
    params.view.dir.phi = 30.0f;
    params.view.pos.x = 0.0f;
    params.view.pos.y = 0.0f;
    params.view.pos.z = 24.0f;
    params.camera.f = yfov2pinhole_f(10.0f, (float)params.resolution.y);
    params.camera.center.x = params.resolution.x/2.0f;
    params.camera.center.y = params.resolution.y/2.0f;
    params.sun.size = 3.0f;
    params.sun.dir.theta = 30.0f;
    params.sun.dir.phi = 90.0f;
    params.sun.color.x = 1.0f;
    params.sun.color.y = 1.0f;
    params.sun.color.z = 0.8f;
    params.sun.intensity = 250.0f;
    return params;
}
