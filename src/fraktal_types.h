#pragma once
#include <math.h>

typedef float angle;
struct int2 { int x,y; };
struct int3 { int x,y,z; };
struct int4 { int x,y,z,w; };
struct angle2 { angle theta,phi; };
struct float2 { float x,y; };
struct float3 { float x,y,z; };
struct float4 { float x,y,z,w; };

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

enum { FRAKTAL_MAX_PARAMS = 1024 };
enum { FRAKTAL_MAX_PARAM_NAME_LEN = 64 };
typedef int fParamType;
enum fParamType_
{
    FRAKTAL_PARAM_FLOAT,
    FRAKTAL_PARAM_FLOAT_VEC2,
    FRAKTAL_PARAM_FLOAT_VEC3,
    FRAKTAL_PARAM_FLOAT_VEC4,
    FRAKTAL_PARAM_FLOAT_MAT2,
    FRAKTAL_PARAM_FLOAT_MAT3,
    FRAKTAL_PARAM_FLOAT_MAT4,
    FRAKTAL_PARAM_INT,
    FRAKTAL_PARAM_INT_VEC2,
    FRAKTAL_PARAM_INT_VEC3,
    FRAKTAL_PARAM_INT_VEC4,
    FRAKTAL_PARAM_SAMPLER1D,
    FRAKTAL_PARAM_SAMPLER2D,
};
struct fParams
{
    float4 mean[FRAKTAL_MAX_PARAMS];
    float4 scale[FRAKTAL_MAX_PARAMS];
    char name[FRAKTAL_MAX_PARAMS][FRAKTAL_MAX_PARAM_NAME_LEN + 1];
    int offset[FRAKTAL_MAX_PARAMS];
    fParamType type[FRAKTAL_MAX_PARAMS];
    int sampler_count;
    int count;
};
