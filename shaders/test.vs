#version 150

in vec2 position;
uniform mat4 view;
uniform float aspect;
uniform float tan_fov_h;
out vec3 ray_origin;
out vec3 ray_direction;

void main()
{
    mat4 inv_view = inverse(view);
    vec3 film_coord = vec3(position.x * aspect, position.y, -1.0 / tan_fov_h);
    ray_origin = (inv_view[3]).xyz;
    ray_direction = (inv_view * vec4(film_coord, 1.0)).xyz - ray_origin;
    gl_Position = vec4(position, 0.0, 1.0);
}
