// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

#pragma once
#include <math.h>

enum { NUM_MATERIALS = 5 };

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
    params.isolines.enabled = false;
    params.isolines.color.x = 0.3f;
    params.isolines.color.y = 0.3f;
    params.isolines.color.z = 0.3f;
    params.isolines.thickness = 0.25f*0.5f;
    params.isolines.spacing = 0.4f;
    params.isolines.count = 3;
    params.material.albedo.x = 0.6f;
    params.material.albedo.y = 0.1f;
    params.material.albedo.z = 0.1f;
    params.material.glossy = true;
    params.material.specular_albedo.x = 0.3f;
    params.material.specular_albedo.y = 0.3f;
    params.material.specular_albedo.z = 0.3f;
    params.material.specular_exponent = 32.0f;
    params.floor.reflective = false;
    params.floor.height = 0.0f;
    params.floor.specular_exponent = 500.0f;
    params.floor.reflectivity = 0.6f;
    return params;
}
