#version 150

in vec2 position;
uniform mat4 view;
uniform float aspect;
uniform float tan_fov_h;
out vec2 texel;
out vec3 ray_origin;
out vec3 ray_exit;

void main()
{
    texel = position;
    mat4 inv_view = inverse(view);
    vec3 film_coord = vec3(position.x * aspect, position.y, -1.0 / tan_fov_h);
    ray_origin = (inv_view[3]).xyz;
    ray_exit = (inv_view * vec4(film_coord, 1.0)).xyz;
    gl_Position = vec4(position, 0.0, 1.0);
}
