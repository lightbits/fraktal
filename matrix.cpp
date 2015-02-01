#include "matrix.h"
#include "math.h"

mat4 
operator *(mat4 a, mat4 b)
{
    mat4 result = {};
    for (int row_in_a = 0; row_in_a < 4; row_in_a++)
    {
        for (int col_in_b = 0; col_in_b < 4; col_in_b++)
        {
            float sum = 0.0f;
            for (int entry = 0; entry < 4; entry++)
                sum += a[entry][row_in_a] * b[col_in_b][entry];
            result[col_in_b][row_in_a] = sum;
        }
    }
    return result;
}

mat4 
operator +(mat4 a, mat4 b)
{
    mat4 result = {};
    for (int i = 0; i < 16; i++)
        result.data[i] = a.data[i] + b.data[i];
    return result;
}

mat4 
operator -(mat4 a, mat4 b)
{
    mat4 result = {};
    for (int i = 0; i < 16; i++)
        result.data[i] = a.data[i] - b.data[i];
    return result;   
}

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
    result.y.z = s;
    result.z.y = -s;
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
    result.x.y = s;
    result.y.x = -s;
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

mat4
mat_translate(vec3 t)
{
    mat4 result = mat_identity();
    result.w.x = t.x;
    result.w.y = t.y;
    result.w.z = t.z;
    return result;
}