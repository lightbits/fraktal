// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

uniform vec2      iResolution;
uniform int       iDrawMode;
uniform sampler2D iChannel0;
uniform float     iMinDrawDistance;
uniform float     iMaxDrawDistance;
out vec4 fragColor;
#define DRAW_MODE_NORMALS 0
#define DRAW_MODE_DEPTH   1

void main()
{
    vec2 uv = gl_FragCoord.xy / iResolution.xy;
    vec4 geometry = texture(iChannel0, uv);
    if (iDrawMode == DRAW_MODE_NORMALS)
    {
        vec2 normal_xy = geometry.rg;
        float normal_z = sqrt(1.0 - dot(normal_xy, normal_xy));
        fragColor.rgb = vec3(0.5) + 0.5*vec3(normal_xy, normal_z);
        fragColor.a = geometry.a;
    }
    else if (iDrawMode == DRAW_MODE_DEPTH)
    {
        float rayDistance = geometry.z;
        float normalizedRayDistance = (rayDistance - iMinDrawDistance) / (iMaxDrawDistance - iMinDrawDistance);
        fragColor.rgb = vec3(normalizedRayDistance);
        fragColor.a = geometry.a;
    }
    else
    {
        fragColor = vec4(0.0);
    }
}
