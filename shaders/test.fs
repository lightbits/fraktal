#version 150

in vec2 texel;
out vec4 out_color;

void main()
{
    out_color = vec4(vec2(0.5) + 0.5 * texel, 0.5, 1.0);
}
