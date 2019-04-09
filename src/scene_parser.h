// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

#pragma once
#include <string.h>
#include <stdio.h>
#include "log.h"
#include "scene_params.h"

void parse_alpha(const char **c)
{
    while (**c)
    {
        if ((**c >= 'a' && **c <= 'z') ||
            (**c >= 'A' && **c <= 'Z') ||
            (**c >= '0' && **c <= '9'))
            (*c)++;
        else
            break;
    }
}

bool parse_is_blank(char c)
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

void parse_blank(const char **c)
{
    while (**c && parse_is_blank(**c))
        (*c)++;
}

void parse_comment(const char **c)
{
    char c0 = (*c)[0];
    char c1 = (*c)[1];
    if (c0 && c1 && c0 == '/' && c1 == '/')
    {
        while (**c && **c != '\n' && **c != '\r')
            (*c)++;
        while (**c && (**c == '\n' || **c == '\r'))
            (*c)++;
    }
    else if (c0 && c1 && c0 == '/' && c1 == '*')
    {
        while ((*c)[0] && (*c)[1] && !((*c)[0] == '*' && (*c)[1] == '/'))
            (*c)++;
        (*c) += 2;
    }
}

bool parse_char(const char **c, char match)
{
    if (**c && **c == match)
    {
        *c = *c + 1;
        return true;
    }
    else
        return false;
}

bool parse_match(const char **c, const char *match)
{
    size_t n = strlen(match);
    if (strncmp(*c, match, n) == 0)
    {
        *c = *c + n;
        return true;
    }
    else
        return false;
}

bool parse_int(const char **c, int *x)
{
    int b;
    int _x;
    if (1 == sscanf(*c, "%d%n", &_x, &b))
    {
        *c = *c + b;
        *x = _x;
        return true;
    }
    return false;
}

bool parse_float(const char **c, float *x)
{
    int b;
    float _x;
    if (1 == sscanf(*c, "%f%n", &_x, &b))
    {
        *c = *c + b;
        *x = _x;
        return true;
    }
    return false;
}

bool parse_angle(const char **c, float *x)
{
    int b;
    float _x;
    if (1 == sscanf(*c, "%f%n", &_x, &b))
    {
        *c = *c + b;
        parse_blank(c);
        if (parse_match(c, "deg"))
        {
            *x = _x;
            return true;
        }
        else if (parse_match(c, "rad"))
        {
            *x = _x*(180.0f/3.1415926535897932384626433832795f);
            return true;
        }
        else log_err("Error parsing angle: must have either 'deg' or 'rad' as suffix.\n");
    }
    return false;
}

bool parse_int2(const char **c, int2 *v)
{
    if (!parse_char(c, '('))  { log_err("Error parsing integer tuple: must begin with parenthesis.\n"); return false; }
    if (!parse_int(c, &v->x)) { log_err("Error parsing integer tuple: first component must be an integer.\n"); return false; }
    if (!parse_char(c, ','))  { log_err("Error parsing integer tuple: components must be seperated by ','.\n"); return false; }
    if (!parse_int(c, &v->y)) { log_err("Error parsing integer tuple: second component must be an integer.\n"); return false; }
    if (!parse_char(c, ')'))  { log_err("Error parsing integer tuple: must end with parenthesis.\n"); return false; }
    return true;
}

bool parse_angle2(const char **c, angle2 *v)
{
    if (!parse_char(c, '('))        { log_err("Error parsing angle tuple: must begin with parenthesis.\n"); return false; }
    if (!parse_angle(c, &v->theta)) { log_err("Error parsing angle tuple: first component must be a number.\n"); return false; }
    if (!parse_char(c, ','))        { log_err("Error parsing angle tuple: components must be seperated by ','.\n"); return false; }
    if (!parse_angle(c, &v->phi))   { log_err("Error parsing angle tuple: second component must be a number.\n"); return false; }
    if (!parse_char(c, ')'))        { log_err("Error parsing angle tuple: must end with parenthesis.\n"); return false; }
    return true;
}

bool parse_float2(const char **c, float2 *v)
{
    if (!parse_char(c, '('))    { log_err("Error parsing tuple: must begin with parenthesis.\n"); return false; }
    if (!parse_float(c, &v->x)) { log_err("Error parsing tuple: first component must be a number.\n"); return false; }
    if (!parse_char(c, ','))    { log_err("Error parsing tuple: components must be seperated by ','.\n"); return false; }
    if (!parse_float(c, &v->y)) { log_err("Error parsing tuple: second component must be a number.\n"); return false; }
    if (!parse_char(c, ')'))    { log_err("Error parsing tuple: must end with parenthesis.\n"); return false; }
    return true;
}

bool parse_float3(const char **c, float3 *v)
{
    if (!parse_char(c, '('))   { log_err("Error parsing tuple: must begin with parenthesis.\n"); return false; }
    if (!parse_float(c, &v->x)) { log_err("Error parsing tuple: first component must be a number.\n"); return false; }
    if (!parse_char(c, ','))   { log_err("Error parsing tuple: components must be seperated by ','.\n"); return false; }
    if (!parse_float(c, &v->y)) { log_err("Error parsing tuple: second component must be a number.\n"); return false; }
    if (!parse_char(c, ','))   { log_err("Error parsing tuple: components must be seperated by ','.\n"); return false; }
    if (!parse_float(c, &v->z)) { log_err("Error parsing tuple: third component must be a number.\n"); return false; }
    if (!parse_char(c, ')'))   { log_err("Error parsing tuple: must end with parenthesis.\n"); return false; }
    return true;
}

static bool parse_inside_list = false;
static bool parse_list_first = false;
static bool parse_list_error = false;

bool parse_begin_list(const char **c)
{
    if (**c == '(')
    {
        *c = *c + 1;
        parse_inside_list = true;
        parse_list_first = true;
        parse_list_error = false;
        return true;
    }
    return false;
}

bool parse_next_in_list(const char **c)
{
    assert(parse_inside_list);
    if (parse_list_error)
        return false;
    parse_blank(c);
    if (parse_char(c, ')'))
        return false;
    if (!parse_list_first)
        if (!parse_char(c, ',')) { log_err("Error parsing list: components must be seperated by ','.\n"); return false; }
    parse_blank(c);
    parse_list_first = false;
    return true;
}

bool parse_end_list(const char **c)
{
    assert(parse_inside_list);
    parse_inside_list = false;
    if (parse_list_error)
        return false;
    return true;
}

void parse_list_unexpected()
{
    log_err("Error parsing list of arguments: unexpected argument.\n");
    parse_list_error = true;
}

#define declare_parse_argument_(type) \
    bool parse_argument_##type(const char **c, const char *name, type *v) \
    { \
        if (parse_match(c, name)) \
        { \
            parse_blank(c); \
            if (!parse_char(c, '=')) { log_err("Error parsing argument: expected '=' between identifier and value.\n"); return false; } \
            if (!parse_##type(c, v)) { log_err("Error parsing argument value: unexpected type after '='.\n"); return false; } \
            return true; \
        } \
        return false; \
    }

declare_parse_argument_(int);
declare_parse_argument_(int2);
declare_parse_argument_(float);
declare_parse_argument_(angle);
declare_parse_argument_(angle2);
declare_parse_argument_(float2);
declare_parse_argument_(float3);

bool parse_view(const char **c, scene_params_t *params)
{
    parse_alpha(c);
    parse_blank(c);
    if (!parse_begin_list(c)) { log_err("Error parsing #view directive: missing ( after '#view'.\n"); return false; }
    while (parse_next_in_list(c)) {
        if (parse_argument_angle2(c, "dir", &params->view.dir)) ;
        else if (parse_argument_float3(c, "pos", &params->view.pos)) ;
        else parse_list_unexpected();
    }
    if (!parse_end_list(c)) { log_err("Error parsing #view directive.\n"); return false; }
    return true;
}

bool parse_camera(const char **c, scene_params_t *params)
{
    parse_alpha(c);
    parse_blank(c);
    if (!parse_begin_list(c)) { log_err("Error parsing #camera directive: missing ( after '#camera'.\n"); return false; }
    while (parse_next_in_list(c)) {
        float yfov;
        if (parse_argument_angle(c, "yfov", &yfov))
        {
            if (params->resolution.y == 0.0f) { log_err("Error parsing #camera directive: #resolution must be set when specifying FOV ('yfov').\n"); return false; }
            params->camera.f = yfov2pinhole_f(yfov, (float)params->resolution.y);
        }
        else if (parse_argument_float(c, "f", &params->camera.f)) ;
        else if (parse_argument_float2(c, "center", &params->camera.center)) ;
        else parse_list_unexpected();
    }
    if (!parse_end_list(c)) { log_err("Error parsing #camera directive.\n"); return false; }
    return true;
}

bool parse_sun(const char **c, scene_params_t *params)
{
    parse_alpha(c);
    parse_blank(c);
    if (!parse_begin_list(c)) { log_err("Error parsing #sun directive: missing ( after '#sun'.\n"); return false; }
    while (parse_next_in_list(c)) {
        if (parse_argument_angle(c, "size", &params->sun.size)) ;
        else if (parse_argument_angle2(c, "dir", &params->sun.dir)) ;
        else if (parse_argument_float3(c, "color", &params->sun.color)) ;
        else if (parse_argument_float(c, "intensity", &params->sun.intensity)) ;
        else parse_list_unexpected();
    }
    if (!parse_end_list(c)) { log_err("Error parsing #sun directive.\n"); return false; }
    return true;
}

bool parse_material(const char **c, int index, scene_params_t *params)
{
    assert(index >= 0 && index < NUM_MATERIALS);
    params->material[index].active = true;
    parse_alpha(c);
    parse_blank(c);
    if (!parse_begin_list(c)) { log_err("Error parsing #material directive: missing ( after '#material'.\n"); return false; }
    while (parse_next_in_list(c)) {
        if (parse_argument_float(c, "roughness", &params->material[index].roughness)) ;
        else if (parse_argument_float3(c, "albedo", &params->material[index].albedo)) ;
        else parse_list_unexpected();
    }
    if (!parse_end_list(c)) { log_err("Error parsing #material directive.\n"); return false; }
    return true;
}

bool parse_resolution(const char **c, scene_params_t *params)
{
    parse_alpha(c);
    parse_blank(c);
    if (!parse_int2(c, &params->resolution)) { log_err("Error parsing #resolution(x,y) directive: expected int2 after token.\n"); return false; }
    return true;
}

void remove_directive_from_source(char *from, char *to)
{
    for (char *c = from; c < to; c++)
        *c = ' ';
}

bool scene_file_preprocessor(char *fs, scene_params_t *params)
{
    char *c = fs;
    while (*c)
    {
        parse_comment((const char **)&c);
        if (*c == '#')
        {
            char *mark = c;
            c++;
            const char **cc = (const char **)&c;
                 if (parse_match(cc, "resolution")) { if (!parse_resolution(cc, params)) return false; }
            else if (parse_match(cc, "view"))       { if (!parse_view(cc, params)) return false; }
            else if (parse_match(cc, "camera"))     { if (!parse_camera(cc, params)) return false; }
            else if (parse_match(cc, "sun"))        { if (!parse_sun(cc, params)) return false; }
            else if (parse_match(cc, "material0"))  { if (!parse_material(cc, 0, params)) return false; }
            else if (parse_match(cc, "material1"))  { if (!parse_material(cc, 1, params)) return false; }
            else if (parse_match(cc, "material2"))  { if (!parse_material(cc, 2, params)) return false; }
            else if (parse_match(cc, "material3"))  { if (!parse_material(cc, 3, params)) return false; }
            else if (parse_match(cc, "material4"))  { if (!parse_material(cc, 4, params)) return false; }
            remove_directive_from_source(mark, c);
        }
        c++;
    }
    return true;
}

bool test_scene_parser()
{
    const char *fs =
        "#resolution(400,a400)\n"
        "#view(dir=(-30 deg, 20deg), pos=(0,0,20))\n"
        "#camera(yfov=20deg, center=(200,200))\n"
        "#material0(roughness=0.1)\n"
        "#material1(albedo=(0.7,0.3,0.2), roughness=0.1)\n"
        "#sun(size=30 deg, dir=(60 deg, 30 deg))\n"
        "float sphere(vec3 p, float r)\n"
        "{\n"
        "    return length(p) - r;\n"
        "}\n"
    ;

    scene_params_t params = {0};
    // params.resolution.x = 400;
    // params.resolution.y = 400;
    // params.view.dir.x = -20.0f;
    // params.view.dir.y = 30.0f;
    // params.view.pos.x = 0.0f;
    // params.view.pos.y = 0.0f;
    // params.view.pos.z = 24.0f;
    // params.camera.yfov = 10.0f*3.14f/180.0f;
    // params.camera.center.x = 200.0f;
    // params.camera.center.y = 200.0f;
    // params.sun.size = 30.0f*3.14f/180.0f;
    // params.sun.dir.x = -60.0f;
    // params.sun.dir.y = 0.0f;
    const char *c = fs;
    while (*c)
    {
        if (*c == '#')
        {
            c++;
                 if (strncmp(c, "resolution", strlen("resolution")) == 0) { if (!parse_resolution(&c, &params)) goto err; }
            else if (strncmp(c, "view",       strlen("view"))       == 0) { if (!parse_view(&c, &params)) goto err; }
            else if (strncmp(c, "camera",     strlen("camera"))     == 0) { if (!parse_camera(&c, &params)) goto err; }
            else if (strncmp(c, "sun",        strlen("sun"))        == 0) { if (!parse_sun(&c, &params)) goto err; }
            else if (strncmp(c, "material0",  strlen("material0"))  == 0) { if (!parse_material(&c, 0, &params)) goto err; }
            else if (strncmp(c, "material1",  strlen("material1"))  == 0) { if (!parse_material(&c, 1, &params)) goto err; }
            else if (strncmp(c, "material2",  strlen("material2"))  == 0) { if (!parse_material(&c, 2, &params)) goto err; }
            else if (strncmp(c, "material3",  strlen("material3"))  == 0) { if (!parse_material(&c, 3, &params)) goto err; }
            else if (strncmp(c, "material4",  strlen("material4"))  == 0) { if (!parse_material(&c, 4, &params)) goto err; }
        }
        c++;
    }

    return true;

    err:
    log_err("Error parsing model definition.\n");
    return false;
}
