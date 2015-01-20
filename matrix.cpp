#include "matrix.h"
#include "math.h"

mat4 
mat_identity()
{
    mat4 result = {};
    result.x.x = 1;
    result.y.y = 1;
    result.z.z = 1;
    result.w.w = 1;
    return result;
}

mat4
mat_rotate_x(float angle_in_radians)
{
    float c = cosf(angle_in_radians);
    float s = sinf(angle_in_radians);
    mat4 result = mat_identity();
    result.y.y = c;
    result.y.z = -s;
    result.z.x = s;
    result.z.z = c;
    return result;
}

mat4
mat_rotate_y(float angle_in_radians)
{
    float c = cosf(angle_in_radians);
    float s = sinf(angle_in_radians);
    mat4 result = mat_identity();
    result.x.x = c;
    result.x.z = s;
    result.z.x = -s;
    result.z.z = c;
    return result;
}

mat4
mat_rotate_z(float angle_in_radians)
{
    float c = cosf(angle_in_radians);
    float s = sinf(angle_in_radians);
    mat4 result = mat_identity();
    result.x.x = c;
    result.y.x = -s;
    result.x.y = s;
    result.y.y = c;
    return result;
}

mat4
mat_translate(float x, float y, float z)
{
    mat4 result = mat_identity();
    result.w.x = x;
    result.w.y = y;
    result.w.z = z;
    return result;
}