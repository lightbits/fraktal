#ifndef _matrix_h_
#define _matrix_h_
#include "math.h"

union vec2
{
    struct
    {
        float x, y;
    };
    float data[2];
};

union vec3
{
    struct
    {
        float x, y, z;
    };
    float data[3];
};

union vec4
{
    struct
    {
        float x, y, z, w;  
    };
    float data[4];
};

union mat4
{
    struct
    {
        vec4 x, y, z, w;
    };
    float data[16];
};

float 
dot(vec2 a, vec2 b)
{
    float result = a.x * b.x + a.y * b.y;
    return result;
}

float 
dot(vec3 a, vec3 b)
{
    float result = a.x * b.x + a.y * b.y + a.z * b.z;
    return result;
}

float 
dot(vec4 a, vec4 b)
{
    float result = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    return result;
}

float 
length(vec2 a)
{
    float result = sqrtf(dot(a, a));
    return result;
}

float 
length(vec3 a)
{
    float result = sqrtf(dot(a, a));
    return result;
}

float 
length(vec4 a)
{
    float result = sqrtf(dot(a, a));
    return result;
}

// vec2 operators

vec2 
operator *(vec2 a, vec2 b)
{ 
    vec2 result = { a.x * b.x, a.y * b.y };  
    return result; 
}

vec2 
operator *(vec2 a, float s)
{ 
    vec2 result = { a.x * s, a.y * s };
    return result; 
}

vec2 
operator +(vec2 a, vec2 b)
{ 
    vec2 result = { a.x + b.x, a.y + b.y };  
    return result; 
}

vec2 
operator -(vec2 a, vec2 b)
{ 
    vec2 result = { a.x - b.x, a.y - b.y };  
    return result; 
}

vec2 
operator /(vec2 a, vec2 b)
{ 
    vec2 result = { a.x / b.x, a.y / b.y };  
    return result; 
}

vec2 
operator /(vec2 a, float s)
{ 
    vec2 result = { a.x / s, a.y / s };
    return result; 
}

vec2 &
operator *=(vec2 &a, vec2 b) 
{ 
    a = a * b; 
    return a; 
}

vec2 &
operator *=(vec2 &a, float s) 
{ 
    a = a * s; 
    return a; 
}

vec2 &
operator +=(vec2 &a, vec2 b) 
{ 
    a = a + b; 
    return a; 
}

vec2 &
operator -=(vec2 &a, vec2 b) 
{ 
    a = a - b; 
    return a; 
}

vec2
normalize(vec2 a)
{
    vec2 result = a / length(a);
    return result;
}

// vec3 operators

vec3 
operator *(vec3 a, vec3 b)
{ 
    vec3 result = { a.x * b.x, a.y * b.y, a.z * b.z }; 
    return result; 
}

vec3 
operator *(vec3 a, float s)
{ 
    vec3 result = { a.x * s, a.y * s, a.z * s };
    return result; 
}

vec3 
operator +(vec3 a, vec3 b)
{ 
    vec3 result = { a.x + b.x, a.y + b.y, a.z + b.z }; 
    return result; 
}

vec3 
operator -(vec3 a, vec3 b)
{ 
    vec3 result = { a.x - b.x, a.y - b.y, a.z - b.z }; 
    return result; 
}

vec3 
operator /(vec3 a, vec3 b)
{ 
    vec3 result = { a.x / b.x, a.y / b.y, a.z / b.z }; 
    return result; 
}

vec3 
operator /(vec3 a, float s)
{ 
    vec3 result = { a.x / s, a.y / s, a.z / s };
    return result; 
}

vec3 &
operator *=(vec3 &a, vec3 b) 
{ 
    a = a * b; 
    return a; 
}

vec3 &
operator *=(vec3 &a, float s) 
{ 
    a = a * s; 
    return a; 
}

vec3 &
operator +=(vec3 &a, vec3 b) 
{ 
    a = a + b; 
    return a; 
}

vec3 &
operator -=(vec3 &a, vec3 b) 
{ 
    a = a - b; 
    return a; 
}

vec3
normalize(vec3 a)
{
    vec3 result = a / length(a);
    return result;
}

// vec4 operators

vec4 
operator *(vec4 a, vec4 b)
{ 
    vec4 result = { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
    return result; 
}

vec4 
operator *(vec4 a, float s)
{ 
    vec4 result = { a.x * s, a.y * s, a.z * s, a.w * s };
    return result; 
}

vec4 
operator +(vec4 a, vec4 b)
{ 
    vec4 result = { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }; 
    return result; 
}

vec4 
operator -(vec4 a, vec4 b)
{ 
    vec4 result = { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w }; 
    return result; 
}

vec4 
operator /(vec4 a, vec4 b)
{ 
    vec4 result = { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
    return result; 
}

vec4 
operator /(vec4 a, float s)
{ 
    vec4 result = { a.x / s, a.y / s, a.z / s, a.w / s };
    return result; 
}

vec4 &
operator *=(vec4 &a, vec4 b) 
{ 
    a = a * b; 
    return a; 
}

vec4 &
operator *=(vec4 &a, float s) 
{ 
    a = a * s; 
    return a; 
}

vec4 &
operator +=(vec4 &a, vec4 b) 
{ 
    a = a + b; 
    return a; 
}

vec4 &
operator -=(vec4 &a, vec4 b) 
{ 
    a = a - b; 
    return a; 
}

vec4
normalize(vec4 a)
{
    vec4 result = a / length(a);
    return result;
}

// mat4 operators

mat4 operator *(mat4 a, mat4 b);
mat4 operator +(mat4 a, mat4 b);
mat4 operator -(mat4 a, mat4 b);

mat4 
mat_identity();

mat4
mat_rotate_x(float angle_in_radians);

mat4
mat_rotate_y(float angle_in_radians);

mat4
mat_rotate_z(float angle_in_radians);

mat4
mat_translate(float x, float y, float z);

#endif