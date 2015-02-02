#version 150

in vec2 texel;
uniform sampler2D sampler0;
uniform int iteration;
out vec4 out_color;

void main()
{
    out_color = texture(sampler0, texel) / float(iteration);
}
