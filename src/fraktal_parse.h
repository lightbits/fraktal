// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

#pragma once
#include <string.h>
#include <stdio.h>
#include <log.h>

static void parse_alpha(const char **c)
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

static bool parse_is_blank(char c)
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

static void parse_blank(const char **c)
{
    while (**c && parse_is_blank(**c))
        (*c)++;
}

static void parse_comment(const char **c)
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

static bool parse_char(const char **c, char match)
{
    if (**c && **c == match)
    {
        *c = *c + 1;
        return true;
    }
    else
        return false;
}

static bool parse_match(const char **c, const char *match)
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

static bool parse_bool(const char **c, bool *x)
{
    if (parse_match(c, "true"))       *x = true;
    else if (parse_match(c, "True"))  *x = true;
    else if (parse_match(c, "false")) *x = false;
    else if (parse_match(c, "False")) *x = false;
    else                              return false;
    return true;
}

static bool parse_int(const char **c, int *x)
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

static bool parse_float(const char **c, float *x)
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

static bool parse_angle(const char **c, float *x)
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

// len: does not include zero-terminator
static bool parse_string(const char **c, const char **v, size_t *len)
{
    char delimiter = '\0';
    if (parse_char(c, '\"'))
        delimiter = '\"';
    else if (parse_char(c, '\''))
        delimiter = '\'';
    else
    {
        log_err("Error parsing string: must begin with single or double quotation.\n");
        return false;
    }

    *v = *c;
    while (**c && **c != delimiter)
        c++;
    if (**c == '\0')
    {
        log_err("Error parsing string: missing end quotation.\n");
        return false;
    }
    *len = *c - *v;
}

static bool parse_int2(const char **c, int2 *v)
{
    if (!parse_char(c, '('))  { log_err("Error parsing integer tuple: must begin with parenthesis.\n"); return false; }
    if (!parse_int(c, &v->x)) { log_err("Error parsing integer tuple: first component must be an integer.\n"); return false; }
    if (!parse_char(c, ','))  { log_err("Error parsing integer tuple: components must be seperated by ','.\n"); return false; }
    if (!parse_int(c, &v->y)) { log_err("Error parsing integer tuple: second component must be an integer.\n"); return false; }
    if (!parse_char(c, ')'))  { log_err("Error parsing integer tuple: must end with parenthesis.\n"); return false; }
    return true;
}

static bool parse_int3(const char **c, int3 *v)
{
    if (!parse_char(c, '('))  { log_err("Error parsing integer tuple: must begin with parenthesis.\n"); return false; }
    if (!parse_int(c, &v->x)) { log_err("Error parsing integer tuple: first component must be an integer.\n"); return false; }
    if (!parse_char(c, ','))  { log_err("Error parsing integer tuple: components must be seperated by ','.\n"); return false; }
    if (!parse_int(c, &v->y)) { log_err("Error parsing integer tuple: second component must be an integer.\n"); return false; }
    if (!parse_char(c, ','))  { log_err("Error parsing integer tuple: components must be seperated by ','.\n"); return false; }
    if (!parse_int(c, &v->z)) { log_err("Error parsing integer tuple: second component must be an integer.\n"); return false; }
    if (!parse_char(c, ')'))  { log_err("Error parsing integer tuple: must end with parenthesis.\n"); return false; }
    return true;
}

static bool parse_int4(const char **c, int4 *v)
{
    if (!parse_char(c, '('))  { log_err("Error parsing integer tuple: must begin with parenthesis.\n"); return false; }
    if (!parse_int(c, &v->x)) { log_err("Error parsing integer tuple: first component must be an integer.\n"); return false; }
    if (!parse_char(c, ','))  { log_err("Error parsing integer tuple: components must be seperated by ','.\n"); return false; }
    if (!parse_int(c, &v->y)) { log_err("Error parsing integer tuple: second component must be an integer.\n"); return false; }
    if (!parse_char(c, ','))  { log_err("Error parsing integer tuple: components must be seperated by ','.\n"); return false; }
    if (!parse_int(c, &v->z)) { log_err("Error parsing integer tuple: second component must be an integer.\n"); return false; }
    if (!parse_char(c, ','))  { log_err("Error parsing integer tuple: components must be seperated by ','.\n"); return false; }
    if (!parse_int(c, &v->w)) { log_err("Error parsing integer tuple: second component must be an integer.\n"); return false; }
    if (!parse_char(c, ')'))  { log_err("Error parsing integer tuple: must end with parenthesis.\n"); return false; }
    return true;
}

static bool parse_angle2(const char **c, angle2 *v)
{
    if (!parse_char(c, '('))        { log_err("Error parsing angle tuple: must begin with parenthesis.\n"); return false; }
    if (!parse_angle(c, &v->theta)) { log_err("Error parsing angle tuple: first component must be a number.\n"); return false; }
    if (!parse_char(c, ','))        { log_err("Error parsing angle tuple: components must be seperated by ','.\n"); return false; }
    if (!parse_angle(c, &v->phi))   { log_err("Error parsing angle tuple: second component must be a number.\n"); return false; }
    if (!parse_char(c, ')'))        { log_err("Error parsing angle tuple: must end with parenthesis.\n"); return false; }
    return true;
}

static bool parse_float2(const char **c, float2 *v)
{
    if (!parse_char(c, '('))    { log_err("Error parsing tuple: must begin with parenthesis.\n"); return false; }
    if (!parse_float(c, &v->x)) { log_err("Error parsing tuple: first component must be a number.\n"); return false; }
    if (!parse_char(c, ','))    { log_err("Error parsing tuple: components must be seperated by ','.\n"); return false; }
    if (!parse_float(c, &v->y)) { log_err("Error parsing tuple: second component must be a number.\n"); return false; }
    if (!parse_char(c, ')'))    { log_err("Error parsing tuple: must end with parenthesis.\n"); return false; }
    return true;
}

static bool parse_float3(const char **c, float3 *v)
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

static bool parse_float4(const char **c, float4 *v)
{
    if (!parse_char(c, '('))   { log_err("Error parsing tuple: must begin with parenthesis.\n"); return false; }
    if (!parse_float(c, &v->x)) { log_err("Error parsing tuple: first component must be a number.\n"); return false; }
    if (!parse_char(c, ','))   { log_err("Error parsing tuple: components must be seperated by ','.\n"); return false; }
    if (!parse_float(c, &v->y)) { log_err("Error parsing tuple: second component must be a number.\n"); return false; }
    if (!parse_char(c, ','))   { log_err("Error parsing tuple: components must be seperated by ','.\n"); return false; }
    if (!parse_float(c, &v->z)) { log_err("Error parsing tuple: third component must be a number.\n"); return false; }
    if (!parse_char(c, ','))   { log_err("Error parsing tuple: components must be seperated by ','.\n"); return false; }
    if (!parse_float(c, &v->w)) { log_err("Error parsing tuple: third component must be a number.\n"); return false; }
    if (!parse_char(c, ')'))   { log_err("Error parsing tuple: must end with parenthesis.\n"); return false; }
    return true;
}

static bool parse_inside_list = false;
static bool parse_list_first = false;
static bool parse_list_error = false;

static bool parse_begin_list(const char **c)
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

static bool parse_next_in_list(const char **c)
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

static bool parse_end_list(const char **c)
{
    assert(parse_inside_list);
    parse_inside_list = false;
    if (parse_list_error)
        return false;
    return true;
}

static void parse_list_unexpected()
{
    log_err("Error parsing list of arguments: unexpected argument.\n");
    parse_list_error = true;
}

#define declare_parse_argument_(type) \
    static bool parse_argument_##type(const char **c, const char *name, type *v) \
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

declare_parse_argument_(bool);
declare_parse_argument_(int);
declare_parse_argument_(int2);
declare_parse_argument_(int3);
declare_parse_argument_(int4);
declare_parse_argument_(float);
declare_parse_argument_(angle);
declare_parse_argument_(angle2);
declare_parse_argument_(float2);
declare_parse_argument_(float3);
declare_parse_argument_(float4);

#if 0
static bool parse_argument_string(const char **c, const char *name, const char **v)
{
    if (parse_match(c, name))
    {
        parse_blank(c);
        if (!parse_char(c, '=')) { log_err("Error parsing argument: expected '=' between identifier and value.\n"); return false; }
        if (!parse_string(c, v)) { log_err("Error parsing argument value: unexpected type after '='.\n"); return false; }
    }
    return false;
}
#endif

static const char *get_token(char *c)
{
    char *token = c;
    parse_alpha((const char**)&c);
    *c = '\0';
    return token;
}

static bool parse_param_annotation(const char **cc, fParamType type, fParamAnnotation *anno)
{
    while (parse_next_in_list(cc)) {
        if (type == FRAKTAL_PARAM_FLOAT ||
            type == FRAKTAL_PARAM_FLOAT_VEC2 ||
            type == FRAKTAL_PARAM_FLOAT_VEC3 ||
            type == FRAKTAL_PARAM_FLOAT_VEC4 ||
            type == FRAKTAL_PARAM_INT ||
            type == FRAKTAL_PARAM_INT_VEC2 ||
            type == FRAKTAL_PARAM_INT_VEC3 ||
            type == FRAKTAL_PARAM_INT_VEC4)
        {
            if (parse_argument_float(cc, "mean", (float*)&anno->mean)) continue;
            else if (parse_argument_float(cc, "scale", (float*)&anno->scale)) continue;
        }

        if (type == FRAKTAL_PARAM_FLOAT_VEC2 ||
            type == FRAKTAL_PARAM_INT_VEC2)
        {
            if (parse_argument_float2(cc, "mean", (float2*)&anno->mean)) continue;
            else if (parse_argument_float2(cc, "scale", (float2*)&anno->scale)) continue;
        }

        if (type == FRAKTAL_PARAM_FLOAT_VEC3 ||
            type == FRAKTAL_PARAM_INT_VEC3)
        {
            if (parse_argument_float3(cc, "mean", (float3*)&anno->mean)) continue;
            else if (parse_argument_float3(cc, "scale", (float3*)&anno->scale)) continue;
        }

        if (type == FRAKTAL_PARAM_FLOAT_VEC4 ||
            type == FRAKTAL_PARAM_INT_VEC4)
        {
            if (parse_argument_float4(cc, "mean", (float4*)&anno->mean)) continue;
            else if (parse_argument_float4(cc, "scale", (float4*)&anno->scale)) continue;
        }

        parse_list_unexpected();
    }

    if (!parse_end_list(cc))
    {
        log_err("Failed to parse parameter annotation.\n");
        return false;
    }
    return true;
}

static bool parse_param(const char **cc, fParam *param, fParamAnnotation *anno)
{
    parse_blank(cc);
    const char *name_start = *cc;
    parse_alpha(cc);
    const char *name_end = *cc;
    if (name_start == name_end)
    {
        log_err("Error parsing kernel parameter: missing name\n");
        return false;
    }
    if (**cc == '\0')
    {
        log_err("Error parsing kernel parameter: file ended prematurely.\n");
        return false;
    }
    size_t name_len = name_end - name_start;
    param->name = (char*)malloc(name_len + 1);
    if (!param->name)
    {
        log_err("Error parsing kernel parameter: ran out of memory.\n");
        return false;
    }
    memcpy(param->name, name_start, name_len);
    param->name[name_len] = '\0';
    parse_blank(cc);
    if (parse_begin_list(cc))
    {
        return parse_param_annotation(cc, param->type, anno);
    }
    else if (parse_char(cc, ';'))
    {
        anno->mean.x = 0.0f;
        anno->mean.y = 0.0f;
        anno->mean.z = 0.0f;
        anno->mean.w = 0.0f;
        anno->scale.x = 1.0f;
        anno->scale.y = 1.0f;
        anno->scale.z = 1.0f;
        anno->scale.w = 1.0f;
        return true;
    }
    else
    {
        log_err("Error parsing kernel parameter: unexpected symbol after parameter name.\n");
        return false;
    }
}

static bool fraktal_parse_source(char *fs, fParam *params, fParamAnnotation *annotations, int *num_params)
{
    *num_params = 0;
    char *c = fs;
    while (*c)
    {
        const char **cc = (const char**)&c;
        parse_comment(cc);
        parse_blank(cc);
        if (parse_match(cc, "uniform"))
        {
            if (*num_params >= FRAKTAL_MAX_PARAMS)
            {
                log_err("Exceeded maximum number of parameters in kernel.\n");
                return false;
            }
            fParam *param = params + (*num_params);
            fParamAnnotation *anno = annotations + (*num_params);
            *num_params = *num_params + 1;

            parse_blank(cc);
            if      (parse_match(cc, "float"))     { param->type = FRAKTAL_PARAM_FLOAT; }
            else if (parse_match(cc, "vec2"))      { param->type = FRAKTAL_PARAM_FLOAT_VEC2; }
            else if (parse_match(cc, "vec3"))      { param->type = FRAKTAL_PARAM_FLOAT_VEC3; }
            else if (parse_match(cc, "vec4"))      { param->type = FRAKTAL_PARAM_FLOAT_VEC4; }
            else if (parse_match(cc, "mat2"))      { param->type = FRAKTAL_PARAM_FLOAT_MAT2; }
            else if (parse_match(cc, "mat3"))      { param->type = FRAKTAL_PARAM_FLOAT_MAT3; }
            else if (parse_match(cc, "mat4"))      { param->type = FRAKTAL_PARAM_FLOAT_MAT4; }
            else if (parse_match(cc, "int"))       { param->type = FRAKTAL_PARAM_INT; }
            else if (parse_match(cc, "ivec2"))     { param->type = FRAKTAL_PARAM_INT_VEC2; }
            else if (parse_match(cc, "ivec3"))     { param->type = FRAKTAL_PARAM_INT_VEC3; }
            else if (parse_match(cc, "ivec4"))     { param->type = FRAKTAL_PARAM_INT_VEC4; }
            else if (parse_match(cc, "sampler1D")) { param->type = FRAKTAL_PARAM_TEX1D; }
            else if (parse_match(cc, "sampler2D")) { param->type = FRAKTAL_PARAM_TEX2D; }
            else
            {
                log_err("Invalid parameter type '%s'\n", get_token(c));
                return false;
            }
            if (!parse_param(cc, param, anno))
            {
                log_err("Error parsing source: invalid parameter declaration.\n");
                return false;
            }
        }
        c++;
    }
    return true;
}
