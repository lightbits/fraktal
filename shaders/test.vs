#version 150

in vec2 position;
uniform mat4 view;
out vec2 texel;
out mat4 inv_view;

void main()
{
    texel = position;
    inv_view = inverse(view);
    gl_Position = vec4(position, 0.0, 1.0);
}
