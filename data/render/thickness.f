// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

uniform vec2      iResolution;
uniform int       iFrame;
uniform vec2      iCameraCenter;
uniform float     iCameraF;
uniform mat4      iView;
out vec4          fragColor;

#define EPSILON 0.0001
#define STEPS 512
#define M_PI 3.1415926535897932384626433832795
#define MAX_DISTANCE 100.0
#define ZERO (min(iFrame,0))
#define MAX_DISTANCE_VISIBILITY_TEST 10.0

float model(vec3 p); // forward declaration

vec3 rayPinhole(vec2 fragOffset)
{
    vec2 uv = vec2(gl_FragCoord.x, iResolution.y - gl_FragCoord.y) + fragOffset - iCameraCenter;
    float d = 1.0/length(vec3(uv, iCameraF));
    return vec3(uv*d, -iCameraF*d);
}

void main()
{
    vec3 rd = rayPinhole(vec2(0.0));
    vec3 ro = (iView * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    rd = normalize((iView * vec4(rd, 0.0)).xyz);

    float t = 0.0;
    float thickness = 0.0;
    for (int i = ZERO; i < STEPS; i++)
    {
        vec3 p = ro + t*rd;
        float d = model(p);
        if (d >= -EPSILON)
        {
            t += max(EPSILON, d);
        }
        else
        {
            t += max(EPSILON, -d);
            thickness += max(EPSILON, -d);
        }
        if (t > MAX_DISTANCE) break;
    }

    fragColor.rgb = vec3(0.0);
    if (thickness > 0.0)
        fragColor.rgb = vec3(thickness/2.0);
    fragColor.a = 1.0;
}
