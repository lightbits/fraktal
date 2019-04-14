// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

void main()
{
    vec2 uv = gl_FragCoord.xy / iResolution.xy;
    fragColor = texture(iChannel0, uv) / float(iSamples);
    fragColor.rgb = sqrt(fragColor.rgb);
}