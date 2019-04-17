// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

uniform vec2      iResolution;
uniform sampler2D iChannel0;
uniform int       iSamples;
out vec4          fragColor;

void main()
{
    vec2 uv = gl_FragCoord.xy / iResolution.xy;
    fragColor = texture(iChannel0, uv) / float(iSamples);
    fragColor.rgb = sqrt(fragColor.rgb);
}
