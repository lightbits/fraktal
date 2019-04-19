// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

uniform vec2      iResolution;
uniform int       iFrame;
uniform vec2      iCameraCenter;
uniform float     iCameraF;
uniform mat4      iView;
out vec4          fragColor;

float model(vec3 p); // forward-declaration

#define EPSILON 0.0001
#define STEPS 512
#define M_PI 3.1415926535897932384626433832795
#define MAX_DISTANCE 100.0
#define ZERO (min(iFrame,0))

vec3 rayPinhole(vec2 fragOffset)
{
    vec2 uv = vec2(gl_FragCoord.x, iResolution.y - gl_FragCoord.y) + fragOffset - iCameraCenter;
    float d = 1.0/length(vec3(uv, iCameraF));
    return vec3(uv*d, -iCameraF*d);
}

// Adapted from Inigo Quilez
// Source: http://iquilezles.org/www/articles/normalsSDF/normalsSDF.htm
vec3 normal(vec3 p)
{
    vec3 n = vec3(0.0);
    for (int i = ZERO; i < 4; i++)
    {
        vec3 e = 0.5773*(2.0*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0);
        n += e*model(p + e*0.002);
    }
    return normalize(n);
}

float traceModel(vec3 ro, vec3 rd)
{
    float t = 0.0;
    for (int i = ZERO; i < STEPS; i++)
    {
        vec3 p = ro + t*rd;
        float d = model(p);
        if (d <= EPSILON) return t;
        t += d;
        if (t > MAX_DISTANCE) break;
    }
    return -1.0;
}

void main()
{
    vec3 rd = rayPinhole(vec2(0.0));
    vec3 ro = (iView * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    rd = normalize((iView * vec4(rd, 0.0)).xyz);

    float t = traceModel(ro, rd);
    if (t > 0.0)
    {
        vec3 p = ro + t*rd;
        vec3 n = normal(p);
        fragColor.rg = n.xy;
        fragColor.b = t;
        fragColor.a = 1.0;
    }
    else
    {
        fragColor.rgba = vec4(0.0);
    }
}
