#pragma once
#include <math.h>

typedef float angle;
struct int2 { int x,y; };
struct int3 { int x,y,z; };
struct angle2 { angle theta,phi; };
struct float2 { float x,y; };
struct float3 { float x,y,z; };

static float deg2rad(float deg) { return (3.14159265358979f/180.0f)*deg; }
static float yfov2pinhole_f(float yfov, float resolution_y)
{
    return (resolution_y/2.0f) / tanf(deg2rad(yfov)/2.0f);
}

static float3 angle2float3(angle2 dir)
{
    float3 w =
    {
        sinf(deg2rad(dir.theta))*cosf(deg2rad(dir.phi)),
        cosf(deg2rad(dir.theta)),
        sinf(deg2rad(dir.theta))*sinf(deg2rad(dir.phi)),
    };
    return w;
}
