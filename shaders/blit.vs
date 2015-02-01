#version 150

in vec2 position;
out vec2 texel;

void main()
{
    texel = vec2(0.5) + 0.5 * position;
    gl_Position = vec4(position, 0.0, 1.0);
}
